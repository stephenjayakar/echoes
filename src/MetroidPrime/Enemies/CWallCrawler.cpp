#include "MetroidPrime/Enemies/CWallCrawler.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Tweaks/CTweakBall.hpp"
#include "WorldFormat/CMetroidAreaCollider.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Math/CloseEnough.hpp"

#include "rstl/reserved_vector.hpp"

static EMaterialTypes skSolidMaterial = kMT_Unknown59;

CWallCrawler::CWallCrawler(EPatternedAI character, TUniqueId uid, const rstl::string& name,
                           EFlavorType flavor, CEntityInfo& info, const CTransform4f& xf,
                           const CModelData& mData, const CPatternedInfo& pInfo,
                           EMovementType moveType, EColliderType colType, EBodyType bodyType,
                           const CActorParameters& actParms, float sphereRadius,
                           float collisionCloseMargin, float alignAngVel, float advanceWpRadius,
                           float playerObstructionMinDist, EType type, bool disableMove,
                           float touchBoundsScale, float floorSnapRate, float f3, float f4,
                           float f5, float f6)
: CPatterned(character, uid, name, flavor, info, xf, mData, pInfo, moveType, colType, bodyType,
             actParms)
, mAlignSurface(CVector3f::Zero(), CVector3f::Right(), CVector3f::Forward(), -1)
, mColSphere(CSphere(CVector3f::Zero(), sphereRadius), GetMaterialList())
, mCollisionCloseMargin(collisionCloseMargin)
, mAlignAngVel(alignAngVel)
, mTumbleAngle(0.f)
, mPatrolPauseRemTime(0.f)
, mAdvanceWpRadius(advanceWpRadius)
, mPlayerObstructionMinDist(playerObstructionMinDist)
, mBendingHackWeight(0.f)
, mType(type)
, mThinkCounter(0)
, mConstraint(kC_None)
, mConstraintPlane(0.f, CUnitVector3f(0.f, 1.f, 0.f, CUnitVector3f::kN_Yes))
, mTouchBoundsScale(touchBoundsScale)
, mFloorSnapRate(floorSnapRate)
, x850_(f3)
, x854_(f4)
, x858_(f5)
, x85c_(f6)
, mAlignToFloor(false)
, mHasAlignSurface(false)
, mPlayerObstructed(false)
, mDisableMove(disableMove)
, x860_27_(false)
, x860_28_(false)
, x860_29_(false)
, x860_30_(false)
, x861_24_(true) {}

CWallCrawler::~CWallCrawler() {}

void CWallCrawler::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Increment:
    break;
  case kSM_AreaLoaded:
    break;
  case kSM_Create:
    AddMaterial(kMT_Immovable, mgr);
    RemoveMaterial(kMT_Unknown59, mgr);
    AddMaterial(kMT_NonSolidDamageable, mgr);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

CVector3f CWallCrawler::ProjectPointToPlane(const CVector3f& point, const CPlane& plane) {
  return point - plane.GetHeight(point) * plane.GetNormal();
}

static CVector3f ProjectPointToPlane(const CVector3f& point, const CVector3f& planePoint,
                                     const CVector3f& normal) {
  return point - CVector3f::Dot(point - planePoint, normal) * normal;
}

CVector3f CWallCrawler::ProjectVectorToPlane(const CVector3f& vec, const CVector3f& planeDir) {
  return vec - CVector3f::Dot(vec, planeDir) * planeDir;
}

static bool PointOnSurface(const CCollisionSurface& surface, const CVector3f& point) {
  const CVector3f projected = ProjectPointToPlane(point, surface.GetVert(0), surface.GetNormal());
  const CVector3f normal = surface.GetNormal();
  for (int i = 0; i < 3; ++i) {
    const CVector3f edge = surface.GetVert((i + 2) % 3) - surface.GetVert(i);
    const CVector3f delta = projected - surface.GetVert(i);
    const CVector3f cross = CVector3f::Cross(delta, edge);
    if (CVector3f::Dot(normal, cross) < 0.f) {
      return false;
    }
  }
  return true;
}

void CWallCrawler::AlignToPlane(const CUnitVector3f& normal, float clampAngle) {
  const CVector3f& up = GetTransform().GetUp();
  const float dot = CVector3f::Dot(up, normal);
  if (close_enough(dot, 1.f)) {
    return;
  }
  if (dot < -0.999f) {
    return;
  }
  const CQuaternion rotation = CQuaternion::ShortestRotationArcClamped(
      GetTransform().GetUp(), normal, CRelAngle::FromDegrees(clampAngle));
  const CQuaternion localRotation(rotation.GetScalar(),
                                  GetTransform().TransposeRotate(rotation.GetVector()));
  SetTransform((CQuaternion::FromMatrix(GetTransform()) * localRotation)
                   .BuildNormalized()
                   .BuildTransform4f(GetTranslation()));
}

void CWallCrawler::AlignToFloor(CStateManager& mgr, float radius, const CVector3f& newPos,
                                float dt) {
  bool hasSurface = false;
  const CVector3f extent(radius + mCollisionCloseMargin, radius + mCollisionCloseMargin,
                         radius + mCollisionCloseMargin);
  const CAABox bounds(newPos - extent, newPos + extent);
  CAreaCollisionCache cache(bounds);
  CGameCollision::BuildAreaCollisionCache(mgr, cache);
  float margin = radius + mCollisionCloseMargin;
  if (mHasAlignSurface) {
    mHasAlignSurface = PointOnSurface(mAlignSurface, newPos);
  }
  if (!mHasAlignSurface || !(mThinkCounter & 3)) {
    if (mConstraint == kC_None) {
      for (int i = 0; i < static_cast< int >(cache.GetNumCaches()); ++i) {
        const CMetroidAreaCollider::COctreeLeafCache& leaf = cache.GetOctreeLeafCache(i);
        for (int j = 0; j < leaf.GetNumLeaves(); ++j) {
          const CAreaOctTree::Node& node = leaf.GetLeaf(j);
          const CAreaOctTree::TriListReference triangles = node.GetTriangleArray();
          const CAreaOctTree& tree = node.GetOwner();
          const int triangleCount = triangles.GetSize();
          for (int k = 0; k < triangleCount; ++k) {
            const CCollisionSurface surface(tree.GetTriangle(triangles.GetAt(k)));
            const float dist = CMath::AbsF(surface.GetPlane().GetHeight(newPos));
            if (dist < margin && PointOnSurface(surface, newPos)) {
              margin = dist;
              mAlignSurface = surface;
              hasSurface = true;
            }
          }
        }
      }
    } else {
      const CVector3f constraintNormal = mConstraintPlane.GetNormal();
      for (int i = 0; i < static_cast< int >(cache.GetNumCaches()); ++i) {
        const CMetroidAreaCollider::COctreeLeafCache& leaf = cache.GetOctreeLeafCache(i);
        for (int j = 0; j < leaf.GetNumLeaves(); ++j) {
          const CAreaOctTree::Node& node = leaf.GetLeaf(j);
          const CAreaOctTree::TriListReference triangles = node.GetTriangleArray();
          const CAreaOctTree& tree = node.GetOwner();
          const int triangleCount = triangles.GetSize();
          for (int k = 0; k < triangleCount; ++k) {
            const CCollisionSurface surface(tree.GetTriangle(triangles.GetAt(k)));
            if (!CMaterialList(surface.GetSurfaceFlags()).HasMaterial(kMT_AIPassthrough)) {
              const CPlane plane = surface.GetPlane();
              float dot = CVector3f::Dot(surface.GetNormal(), constraintNormal);
              float weight = 1.f;
              switch (mConstraint) {
              case kC_Negative:
                dot = -dot;
                break;
              case kC_Both:
                dot = CMath::AbsF(dot);
                break;
              }
              if (dot > 0.5f) {
                weight += dot;
              }
              const float dist = weight * CMath::AbsF(plane.GetHeight(newPos));
              if (dist < margin && PointOnSurface(surface, newPos)) {
                margin = dist;
                mAlignSurface = surface;
                hasSurface = true;
              }
            }
          }
        }
      }
    }
    mHasAlignSurface = hasSurface;
  }
  if (mHasAlignSurface) {
    const CUnitVector3f normal(mAlignSurface.GetNormal(), CUnitVector3f::kN_No);
    AlignToPlane(normal, mAlignAngVel * dt);
    mTumbleAngle = 0.f;
    x860_27_ = false;
    x860_28_ = CVector3f::Dot(GetTransform().GetUp(), normal) > 0.86f;
  } else {
    const CVector3f velocity = GetVelocityWR();
    const CVector3f forward = GetTransform().GetForward();
    const float angularVelocity =
        CMath::Rad2Deg(velocity.Magnitude()) / mColSphere.GetSphere().GetRadius();
    if (CVector3f::Dot(velocity, forward) > 0.f) {
      AlignToPlane(CUnitVector3f(forward, CUnitVector3f::kN_No), angularVelocity * dt);
    } else {
      AlignToPlane(-CUnitVector3f(forward, CUnitVector3f::kN_No), angularVelocity * dt);
    }
    x860_27_ = true;
    x860_28_ = false;
    mTumbleAngle += angularVelocity * dt;
  }
}

const CCollisionPrimitive* CWallCrawler::GetCollisionPrimitive() const { return &mColSphere; }

void CWallCrawler::PreThink(float dt, CStateManager& mgr) {
  CPatterned::PreThink(dt, mgr);
  if (GetActive() && !mPlayerObstructed) {
    if (mPatrolPauseRemTime <= 0.f && !mDisableMove &&
        close_enough(BodyController()->GetPercentageFrozen(), 0.f)) {
      if (mAlignToFloor) {
        const CQuaternion oldOrientation = CQuaternion::FromMatrix(GetTransform());
        const CMotionState motion(PredictMotion(dt));
        AddMotionState(motion);
        const CQuaternion newOrientation = CQuaternion::FromMatrix(GetTransform());
        ClearForcesAndTorques();
        if (mHasAlignSurface) {
          const CPlane plane = mAlignSurface.GetPlane();
          const CVector3f position = GetTranslation();
          const CVector3f projected =
              GetTranslation() - (plane.GetHeight(GetTranslation()) -
                                  mColSphere.GetSphere().GetRadius() - 0.01f) *
                                     plane.GetNormal();
          SetTranslation(CVector3f::Lerp(position, projected, 60.f * mFloorSnapRate * dt));
        }
        MoveCollisionPrimitive(CVector3f::Zero());
      }
    } else if (mPatrolPauseRemTime > 0.f) {
      Stop();
    }
  }
}

bool CWallCrawler::UpdateWPDestination(CStateManager& mgr) {
  bool arrived = false;
  if (CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(mDestObj))) {
    const CVector3f position = waypoint->GetTranslation();
    const CVector3f delta = position - GetTranslation();
    if (delta.MagSquared() < mAdvanceWpRadius * mAdvanceWpRadius) {
      mDestObj = GetNextWaypoint(mgr, waypoint, true);
      arrived = true;
      if (const CScriptAIWaypoint* aiWaypoint = TCastToConstPtr< CScriptAIWaypoint >(waypoint)) {
        if (!close_enough(aiWaypoint->GetPause(), 0.f)) {
          mPatrolPauseRemTime = aiWaypoint->GetPause();
          if (mType == kT_Parasite) {
            BodyController()->SetLocomotionType(pas::kLT_Relaxed);
          }
        }
      }
      mgr.SendScriptMsg(waypoint, GetUniqueId(), kSM_Arrived, kInvalidUniqueId);
    }
    SetDestPos(position);
  }
  return arrived;
}

void CWallCrawler::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

void CWallCrawler::Think(float dt, CStateManager& mgr) {
  if (GetMovable()) {
    AddMaterial(kMT_Unknown59, mgr);
  } else {
    RemoveMaterial(kMT_Unknown59, mgr);
  }
  CPatterned::Think(dt, mgr);
  if (mPatrolPauseRemTime > 0.f) {
    mPatrolPauseRemTime = rstl::max_val(0.f, mPatrolPauseRemTime - dt);
  }

  if (BodyController()->HasBodyState(pas::kAS_AdditiveReaction)) {
    x860_27_ = x860_27_ && x861_24_;
    x860_28_ = x860_28_ && x861_24_;
    if ((x860_27_ && !x860_30_) || (x860_28_ && !x860_29_)) {
      if (mBendingHackWeight < 1.f) {
        const float speed = GetVelocityWR().Magnitude();
        if (x860_27_ || x860_29_) {
          mBendingHackWeight += dt * speed / x850_;
        } else {
          mBendingHackWeight += dt * speed / x858_;
        }
        if (mBendingHackWeight >= 1.f) {
          mBendingHackWeight = 1.f;
        }
      }
    } else if (mBendingHackWeight > 0.f) {
      const float speed = GetVelocityWR().Magnitude();
      if (x860_27_ || x860_29_) {
        mBendingHackWeight -= dt * speed / x854_;
      } else {
        mBendingHackWeight -= dt * speed / x85c_;
      }
      if (mBendingHackWeight <= 0.f) {
        mBendingHackWeight = 0.f;
      }
    }

    if (mBendingHackWeight > 0.f || x860_29_ || x860_30_) {
      if (x860_27_ || x860_29_) {
        if (mBendingHackWeight > 0.0001f) {
          if (x860_29_) {
            BodyController()->CommandMgr().DeliverCmd(CBCAdditiveWeightCmd(mBendingHackWeight));
          } else {
            BodyController()->CommandMgr().DeliverCmd(
                CBCAdditiveReactionCmd(pas::kART_Six, mBendingHackWeight, true));
          }
          x860_29_ = true;
        } else {
          BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_StopReaction));
          x860_29_ = false;
        }
      } else if (mBendingHackWeight > 0.0001f) {
        if (x860_30_) {
          BodyController()->CommandMgr().DeliverCmd(CBCAdditiveWeightCmd(mBendingHackWeight));
        } else {
          BodyController()->CommandMgr().DeliverCmd(
              CBCAdditiveReactionCmd(pas::kART_Five, mBendingHackWeight, true));
        }
        x860_30_ = true;
      } else {
        BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_StopReaction));
        x860_30_ = false;
      }
    }
  }

  if (mAlive) {
    CPlayer* player = mgr.GetPlayer(0);
    const rstl::optional_object< CAABox > playerBounds = player->GetTouchBounds();
    const rstl::optional_object< CAABox > bounds = GetTouchBounds();
    if (playerBounds && bounds && playerBounds->DoBoundsOverlap(*bounds)) {
      if (player->GetMorphBall()->IsBoosting()) {
        const CDamageInfo damage = gpTweakBall->GetBoostBallDamage();
        mgr.ApplyDamage(player->GetUniqueId(), GetUniqueId(), player->GetUniqueId(), damage,
                        CMaterialFilter::MakeIncludeExclude(CMaterialList(skSolidMaterial),
                                                            CMaterialList()),
                        CVector3f::Zero());
      } else if (mCurDamageRemTime <= 0.f) {
        CDamageInfo damage = GetContactDamage();
        damage.SetNoImmunity(true);
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), damage,
                        CMaterialFilter::MakeIncludeExclude(CMaterialList(skSolidMaterial),
                                                            CMaterialList()),
                        CVector3f::Zero());
        mCurDamageRemTime = mDamageWaitTime;
      }
    }
  }
}

rstl::optional_object< CAABox > CWallCrawler::GetTouchBounds() const {
  return rstl::optional_object< CAABox >(GetBaseBoundingBox().GetTransformedAABox(
      GetPrimitiveTransform() * CTransform4f::Scale(mTouchBoundsScale)));
}

void CWallCrawler::UpdateConstraintPlane(CStateManager& mgr) {
  const TUniqueId startId =
      mDestObj == kInvalidUniqueId ? GetConnectedObject(mgr, kSS_Patrol, kSM_Follow) : mDestObj;
  CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(startId));
  rstl::reserved_vector< CVector3f, 3 > points;
  for (int i = 0; points.size() < 3 && i < 30; ++i) {
    if (!waypoint) {
      break;
    }
    const CVector3f position = waypoint->GetTranslation();
    bool skip = false;
    if (points.size() == 2) {
      skip = CMath::AbsF(CVector3f::Dot((points[0] - points[1]).AsNormalized(),
                                        (position - points[1]).AsNormalized())) > 0.984f;
    }
    if (!skip) {
      points.push_back(position);
    }
    waypoint =
        TCastToPtr< CScriptWaypoint >(mgr.ObjectById(GetNextWaypoint(mgr, waypoint, false)));
  }
  if (points.size() == 3) {
    mConstraintPlane = CPlane(points[0], points[1], points[2]);
  } else if (points.size() == 2) {
    if (!close_enough(points[0].GetX(), points[1].GetX()) &&
        !close_enough(points[0].GetY(), points[1].GetY())) {
      mConstraintPlane =
          CPlane(points[0], points[1], points[1] + CVector3f(0.f, 0.f, 1.f));
    }
  }
}

CWallCrawler::EConstraint CWallCrawler::GetConstraint() const { return mConstraint; }

void CWallCrawler::SetConstraint(CStateManager& mgr, EConstraint constraint) {
  mConstraint = constraint;
  if (constraint != kC_None) {
    UpdateConstraintPlane(mgr);
  }
}

TUniqueId CWallCrawler::GetNextWaypoint(CStateManager& mgr, const CScriptWaypoint* waypoint,
                                        bool reverse) {
  return waypoint->NextWaypoint(mgr);
}

const CPlane& CWallCrawler::GetConstraintPlane() const { return mConstraintPlane; }

extern "C" void RELExit() {}

extern "C" void RELMain() {}
