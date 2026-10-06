#include "MetroidPrime/Enemies/CParasite.hpp"

#include "Collision/CCollidableAABox.hpp"
#include "Collision/CCollisionInfoList.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrParasite.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptDynamicLight.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "REL/REL_Setup.h"

#include <float.h>

float CParasite::skAttackTime = 2.f * CMath::SqrtF(2.5f / CPhysicsActor::GravityConstant());
float CParasite::skAttackVelocity = 15.f / skAttackTime;
float CParasite::skRetreatTime = 2.f * CMath::SqrtF(2.5f / CPhysicsActor::GravityConstant());
float CParasite::skRetreatVelocity = 3.f / skRetreatTime;

struct SSphereJointInfo {
  const char* name;
  float radius;
};
static const SSphereJointInfo skIceJoints[] = {{"Skeleton_Root", 0.f}};

CParasite::CParasite(TUniqueId uid, const rstl::string& name, EFlavorType flavor,
                     const CEntityInfo& info, const CTransform4f& xf, const CModelData& mData,
                     const CPatternedInfo& pInfo, EBodyType bodyType, float maxTelegraphReactDist,
                     float advanceWpRadius, float f3, float alignAngVel, float f5,
                     float stuckTimeThreshold, float collisionCloseMargin,
                     float parasiteSearchRadius, float parasiteSeparationDist,
                     float parasiteSeparationWeight, float parasiteAlignmentWeight,
                     float parasiteCohesionWeight, float destinationSeekWeight,
                     float forwardMoveWeight, float playerSeparationDist,
                     float playerSeparationWeight, float playerObstructionMinDist, float haltDelay,
                     bool disableMove, EType wType, const CDamageVulnerability& dVuln,
                     const CDamageInfo& dInfo, ushort haltSfx, ushort getUpSfx, ushort crouchSfx,
                     CAssetId modelRes, CAssetId skinRes, float iceZoomerJointHP,
                     float wallWalkerF6, const CDamageInfo& dInfo2, const CActorParameters& aParams)
: CWallWalker(static_cast< EPatternedAI >(39), uid, name, flavor, info, xf, mData, pInfo, kMT_Flyer,
              kCT_Zero, bodyType, aParams, pInfo.GetHalfExtent(), collisionCloseMargin,
              alignAngVel, advanceWpRadius, playerObstructionMinDist, wallWalkerF6, 0.167f, 0.6f,
              wType, disableMove, 1.5f, 0.6f, 1.5f)
, mStateProgress(-1)
, x87c_(CVector3f::Zero())
, mTargetPos(CVector3f::Zero())
, mActiveSpeed(1.f)
, mTelegraphRemTime(0.f)
, mStuckTime(0.f)
, mLastStuckPos(CVector3f::Zero())
, mCollisionActorManager(nullptr)
, mExtraModel(nullptr)
, mParasiteSeparationMove(CVector3f::Zero())
, mParasiteCohesionMove(CVector3f::Zero())
, mParasiteAlignmentMove(CVector3f::Zero())
, mOculusHaltDVuln(dVuln)
, mOculusHaltDInfo(dInfo)
, x928_(dInfo2)
, x944_(0.f)
, mMaxTelegraphReactDist(maxTelegraphReactDist)
, x94c_(f3)
, x954_(f5)
, mStuckTimeThreshold(stuckTimeThreshold)
, mParasiteSearchRadius(parasiteSearchRadius)
, mParasiteSeparationDist(parasiteSeparationDist)
, mParasiteSeparationWeight(parasiteSeparationWeight)
, mParasiteAlignmentWeight(parasiteAlignmentWeight)
, mParasiteCohesionWeight(parasiteCohesionWeight)
, mDestinationSeekWeight(destinationSeekWeight)
, mForwardMoveWeight(forwardMoveWeight)
, mPlayerSeparationDist(playerSeparationDist)
, mPlayerSeparationWeight(playerSeparationWeight)
, mUnmorphedRadius(pInfo.GetHeight() * 0.5f)
, mHaltDelay(haltDelay)
, mIceZoomerJointHP(iceZoomerJointHP)
, x990_(CVector3f::Zero())
, x99c_(CVector3f::Zero())
, x9a8_(CVector3f::Zero())
, mHaltSfx(haltSfx)
, mGetUpSfx(getUpSfx)
, mCrouchSfx(crouchSfx)
, x9ba_(kInvalidUniqueId)
, x9bc_(kInvalidUniqueId)
, x9c0_(0)
, x9c4_(0)
, mReceivedTelegraph(false)
, mJumpVelDirty(false)
, x9c8_26_(false)
, mLanded(false)
, mOnGround(true)
, x9c8_29_(false)
, mAttackOver(true)
, x9c8_31_(false)
, mHalted(false)
, mVulnerable(false)
, mOculusShotAt(false)
, mInJump(false)
, mLineOfSight(GetUniqueId(), CSegId(0xff), 0.2f, 0.05f) {
  SetCallTouch(false);
  switch (mWalkerType) {
  case kWT_Geemer:
    mKnockBackController.SetEnableFreeze(false);
  case kWT_Oculus:
    mKnockBackController.EnableKnockBackPhysics(false);
    break;
  case kWT_IceZoomer:
    mExtraModel = rs_new TLockedToken< CSkinnedModel >(
        rs_new CSkinnedModel(gpSimplePool->GetObj(SObjectTag('CMDL', modelRes)),
                             gpSimplePool->GetObj(SObjectTag('CSKR', skinRes)),
                             GetModelData()->GetAnimationData()->GetModelData()->GetLayoutInfo()));
    break;
  default:
    break;
  }
  if (mWalkerType == kWT_Oculus) {
    mKnockBackController.EnableShock(false);
    mKnockBackController.SetEnableBurn(false);
    mKnockBackController.SetEnableBurnDeath(false);
    mKnockBackController.EnableExplodeDeath(false);
  }
}

CParasite::~CParasite() {}

static TUniqueId lastParasite = kInvalidUniqueId;

void CParasite::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const TUniqueId uid = msg.GetSenderId();
  const EScriptObjectMessage message = msg.GetMessage();
  CWallWalker::AcceptScriptMsg(mgr, msg);

  switch (message) {
  case kSM_Create: {
    mBodyController->Activate(mgr, pas::kAS_Invalid);
    SetDrawShadow(false);
    mActiveSpeed = mSpeed;
    const float radius = mColSphere.GetSphere().GetRadius();
    const CVector3f extent(radius, radius, radius);
    SetBoundingBox(CAABox(-extent, extent));
    lastParasite = GetUniqueId();
    AddDoorRepulsors(mgr);
    if (mWalkerType == kWT_IceZoomer) {
      SetupIceZoomerCollision(mgr);
      SetupIceZoomerVulnerability(
          mgr, mOculusHaltDVuln,
          CHealthInfo(mIceZoomerJointHP, GetHealthInfo()->GetKnockBackResistance()));
    }
    break;
  }
  case kSM_Delete:
    switch (mWalkerType) {
    case kWT_IceZoomer:
      DestroyActorManager(mgr);
      break;
    case 10:
      mgr.DeleteObjectRequest(x9ba_);
      mgr.DeleteObjectRequest(x9bc_);
      break;
    }
    break;
  case kSM_AreaLoaded:
    if (mWalkerType == 10) {
      for (const SConnection* it = GetConnectionList().data();
           it != GetConnectionList().data() + GetConnectionList().size(); ++it) {
        if (it->state == kSS_GRNT && it->msg == kSM_Activate) {
          const CScriptObjectLoaderHelper::SGeneratedObject generated =
              mgr.ScriptObjectLoaderHelper().GenerateScriptObject(it->objId, mgr);
          const TUniqueId id = generated.mUniqueId;
          CEntity* entity = generated.mEntity;
          if (TCastToPtr< CScriptSafeZone >(entity)) {
            x9ba_ = id;
          } else if (TCastToPtr< CScriptDynamicLight >(entity)) {
            x9bc_ = id;
          } else {
            mgr.DeleteObjectRequest(id);
          }
        }
      }
    }
    mLineOfSight.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    break;
  case kSM_Launching:
    if (mJumpVelDirty) {
      UpdateJumpVelocity();
      mJumpVelDirty = false;
    }
    break;
  case kSM_Activate:
    mDisableMove = false;
    switch (mWalkerType) {
    case kWT_Parasite:
      mBodyController->SetLocomotionType(pas::kLT_Lurk);
      break;
    case 10:
      UpdateShell(mgr, 0);
      break;
    }
    break;
  case kSM_Deactivate:
    if (mWalkerType == 10) {
      UpdateShell(mgr, 1);
    }
    break;
  case kSM_ResistedDamage:
    switch (mWalkerType) {
    case kWT_Oculus:
      if (const CActor* act = TCastToConstPtr< CActor >(mgr.GetObjectById(uid))) {
        const float distSq = (act->GetTranslation() - GetTranslation()).MagSquared();
        const float maxComp = rstl::max_val(
            GetTouchBounds()->GetWidth(),
            rstl::max_val(GetTouchBounds()->GetDepth(), GetTouchBounds()->GetHeight()));
        const float maxCompSq = maxComp * maxComp + 1.f;
        if (distSq < maxCompSq * maxCompSq) {
          mOculusShotAt = true;
        }
      }
      break;
    case 10: {
      int type = -1;
      if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(uid))) {
        type = weapon->GetType();
      }
      switch (type) {
      case 2:
        UpdateShell(mgr, 2);
        break;
      case 1:
        UpdateShell(mgr, 1);
        break;
      }
      break;
    }
    }
  case kSM_XXDG:
    if (mWalkerType == kWT_IceZoomer) {
      mBodyController->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(1.f));
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionActorManager.get()) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  default:
    break;
  }
}

void CParasite::AddDoorRepulsors(CStateManager& mgr) {
  const rstl::list< CEntity* >& doors = mgr.GetDoorList();
  int count = 0;
  for (rstl::list< CEntity* >::const_iterator it = doors.begin(); it != doors.end(); ++it) {
    if (*it && (*it)->GetCurrentAreaId() == GetCurrentAreaId()) {
      ++count;
    }
  }
  mDoorRepulsors.reserve(count);
  for (rstl::list< CEntity* >::const_iterator it = doors.begin(); it != doors.end(); ++it) {
    const CActor* door = static_cast< const CActor* >(*it);
    if (door && door->GetCurrentAreaId() == GetCurrentAreaId()) {
      rstl::optional_object< CAABox > bounds = door->GetTouchBounds();
      if (bounds.valid()) {
        float diagonal = (bounds->GetMinPoint() - bounds->GetMaxPoint()).Magnitude();
        mDoorRepulsors.push_back_unsafe(CRepulsor(bounds->GetCenterPoint(), 0.75f * diagonal));
      }
    }
  }
}

void CParasite::PreThink(float dt, CStateManager& mgr) { CWallWalker::PreThink(dt, mgr); }

static EMaterialTypes skContactMaterial = kMT_Unknown59;

void CParasite::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  ++mThinkCounter;
  switch (mWalkerType) {
  case kWT_IceZoomer:
    UpdateCollisionActors(dt, mgr);
    break;
  case 10:
    if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x9ba_))) {
      act->SetTransform(GetTransform());
    }
    if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x9bc_))) {
      act->SetTransform(GetTransform());
    }
    break;
  }

  mPlayerObstructed = false;
  const CGameArea& area = mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId());
  if (area.GetOcclusionState() != CGameArea::kOS_Visible) {
    mPlayerObstructed = true;
  }

  if (!mPlayerObstructed) {
    const CPlayer* player = mgr.GetPlayer(0);
    if ((player->GetTranslation() - GetTranslation()).Magnitude() > mPlayerObstructionMinDist) {
      mLineOfSight.Update(dt, mgr);
      if (!mLineOfSight.HasLineOfSight()) {
        mPlayerObstructed = true;
      }
    }
  }

  if (mPlayerObstructed) {
    SetMovable(false);
    return;
  }

  SetMovable(!mAlignToFloor);

  if (!mDisableMove) {
    if (close_enough(mBodyController->GetPercentageFrozen(), 0.f)) {
      static const float ksMinMoveDistance = 0.3f * dt * mSpeed;
      const CVector3f stuckDelta = GetTranslation() - mLastStuckPos;
      if (stuckDelta.MagSquared() < ksMinMoveDistance * dt) {
        mStuckTime += dt;
      } else {
        mStuckTime = 0.f;
      }

      mLastStuckPos = GetTranslation();
      if (mTelegraphRemTime > 0.f) {
        mTelegraphRemTime -= dt;
      } else {
        mTelegraphRemTime = 0.f;
      }
    }
  }

  if (mAlive) {
    CPlayer* player = mgr.Player(0);
    bool useCollisionRadius =
        player->GetMorphballTransitionState() == CPlayer::kMS_Morphed || !mAttackOver;
    float radius = useCollisionRadius ? mColSphere.GetSphere().GetRadius() : mUnmorphedRadius;

    const CVector3f extent(radius, radius, radius);
    CAABox aabox(GetTranslation() - extent, GetTranslation() + extent);
    rstl::optional_object< CAABox > plBox = player->GetTouchBounds();

    if (plBox.valid() && plBox->DoBoundsOverlap(aabox)) {
      if (!mAttackOver) {
        mAttackOver = true;
        mLanded = false;
      }

      if (mCurDamageRemTime <= 0.f) {
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
                        CMaterialFilter::MakeIncludeExclude(CMaterialList(skContactMaterial),
                                                            CMaterialList()),
                        CVector3f::Zero());
        if (mWalkerType == kWT_IceZoomer && mVulnerable) {
          CSfxManager::AddEmitter(mCrouchSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                                  false);
          CSfxManager::SfxStart(mGetUpSfx, 100, 64);
          x944_ = 2.f;
          mgr.CameraBlurPass(0, 0).SetBlur(CCameraBlurPass::kBT_LoBlur, 4.f, 0.f, false);
          mgr.CameraBlurPass(0, 0).DisableBlur(2.f);
          mgr.CameraFilterPass(0, 0).SetFilter(CCameraFilterPass::kFT_Blend,
                                                CCameraFilterPass::kFS_Fullscreen, 0.f,
                                                CColor(140, 255, 109, 63.75f), -1);
          mgr.CameraFilterPass(0, 0).DisableFilter(2.f);
        }
        mCurDamageRemTime = mDamageWaitTime;
      }
    }

    if (x944_ > 0.f) {
      mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), CDamageInfo(x928_, dt),
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(skContactMaterial),
                                                          CMaterialList()),
                      CVector3f::Zero());
      x944_ -= dt;
    }
  }

  CWallWalker::Think(dt, mgr);

  if (mDisableMove) {
    return;
  }

  if (!close_enough(mBodyController->GetPercentageFrozen(), 0.f)) {
    return;
  }

  mSpeed = mActiveSpeed;
  if (mAlignToFloor) {
    AlignToFloor(mgr, mColSphere.GetSphere().GetRadius(),
                 GetTranslation() + 2.f * (dt * GetVelocityWR()), dt);
  }

  mLanded = false;
}

CAdvancementDeltas CParasite::UpdateWalkerAnimation(CStateManager& mgr, float dt) {
  return UpdateAnimation(dt, mgr, true);
}

void CParasite::UpdateJumpVelocity() {
  SetMomentumWR(CVector3f(0.f, 0.f, -GetWeight()));
  CVector3f velocity = CVector3f::Zero();
  if (!mAttackOver) {
    float speed = skAttackVelocity;
    velocity = speed * GetTransform().GetForward();
    velocity.SetZ(0.5f * skAttackVelocity);
  } else {
    float speed = skRetreatVelocity;
    velocity = speed * GetTransform().GetForward();
    velocity.SetZ(0.5f * skRetreatVelocity);
  }
  const CVector3f& position = GetTranslation();
  float height = mTargetPos.GetZ() - position.GetZ();
  float acceleration = GetMomentumWR().GetZ() / GetMass();
  CVector3f delta(mTargetPos.GetX() - position.GetX(), mTargetPos.GetY() - position.GetY(), 0.f);
  float distance = delta.Magnitude();
  if (distance > FLT_EPSILON) {
    delta *= 1.f / distance;
    float speed = CVector3f::Dot(delta, velocity);
    if (speed > FLT_EPSILON) {
      float time = 0.f;
      bool below = height < 0.f;
      float positiveRoot, negativeRoot;
      if (CMath::SolveQuadratic(acceleration, velocity.GetZ(), -height, positiveRoot,
                                negativeRoot)) {
        time = below ? negativeRoot : positiveRoot;
      }
      if (!below) {
        time += distance / speed;
      }
      if (time < 10.f) {
        velocity = distance / time * delta;
        velocity.SetZ(-(0.5f * acceleration * time - height / time));
      }
    }
  }
  SetVelocityWR(velocity);
}

CVector3f CParasite::GetAimPosition(const CStateManager&, float) const { return GetTranslation(); }

void CParasite::ThinkAboutMove(float dt) {
  if (!GetMaterialList().HasMaterial(kMT_GroundCollider)) {
    CPatterned::ThinkAboutMove(dt);
  }
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::AnimOver)},
    {"Landed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::Landed)},
    {"HitSomething",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::HitSomething)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::ShouldAttack)},
    {"Stuck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::Stuck)},
    {"AttackOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::AttackOver)},
    {"ShotAt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::ShotAt)},
    {"PatrolPathOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::PatrolPathOver)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Generate)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Deactivate)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::PathFind)},
    {"TargetPatrol",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TargetPatrol)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Patrol)},
    {"Run", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Run)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Attack)},
    {"TelegraphAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TelegraphAttack)},
    {"Jump", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Jump)},
    {"TargetPlayer",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TargetPlayer)},
    {"Retreat", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Retreat)},
    {"Halt", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Halt)},
    {"Crouch", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Crouch)},
    {"GetUp", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::GetUp)},
};

bool CParasite::Stuck(CStateManager&, const CTriggerData&) const {
  return mStuckTime > mStuckTimeThreshold;
}

bool CParasite::AttackOver(CStateManager&, const CTriggerData&) const { return mAttackOver; }

bool CParasite::ShotAt(CStateManager&, const CTriggerData&) const {
  switch (mWalkerType) {
  case 10:
  case kWT_Oculus:
    return mOculusShotAt;
  default:
    return mHitByPlayerProjectile;
  }
}

bool CParasite::PatrolPathOver(CStateManager&, const CTriggerData&) const {
  return mDestObj == kInvalidUniqueId;
}

static EMaterialTypes skWallMaterial = kMT_Unknown59;

bool CParasite::CloseToWall(CStateManager& mgr) const {
  static const CMaterialFilter kSolidFilter =
      CMaterialFilter::MakeInclude(CMaterialList(skWallMaterial));
  const CAABox& bounds = GetBoundingBox();
  float radius = mColSphere.GetSphere().GetRadius();
  float margin = radius + mCollisionCloseMargin;
  CAABox expanded(bounds.GetMinPoint() - CVector3f(margin, margin, margin),
                  bounds.GetMaxPoint() + CVector3f(margin, margin, margin));
  CCollidableAABox collisionBox(expanded, GetMaterialList());
  return CGameCollision::DetectStaticCollisionBoolean(mgr, collisionBox, CTransform4f::Identity(),
                                                      kSolidFilter);
}

static EMaterialTypes skPlayerMaterial = kMT_Player;
static EMaterialTypes skCharacterMaterial = kMT_Character;

void CParasite::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                             CStateManager& mgr) {
  static const CMaterialList testList = CMaterialList(skPlayerMaterial, skCharacterMaterial);
  if (mInJump) {
    for (const CCollisionInfo* it = list.Begin(); it < list.End(); ++it) {
      const CCollisionInfo& info = *it;
      if (!mAlignToFloor && !testList.SharesMaterials(info.GetMaterialLeft())) {
        const CVector3f normal = info.GetNormalLeft();
        OrientToSurfaceNormal(normal, 360.f);
        CPhysicsActor::Stop();
        SetVelocityWR(CVector3f::Zero());
        mLanded = true;
        mOnGround = true;
      }
    }
  }
}

bool CParasite::IsOnGround() const { return mOnGround; }

TUniqueId CParasite::RecursiveFindClosestWayPoint(CStateManager& mgr, TUniqueId id,
                                                  float& dist) const {
  TUniqueId ret = id;
  CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id));
  if (!wp) {
    return ret;
  }
  wp->SetActive(false);
  dist = (wp->GetTranslation() - GetTranslation()).MagSquared();
  wp->SetActive(true);
  return ret;
}

TUniqueId CParasite::GetClosestWaypointForState(EScriptObjectState state,
                                                CStateManager& mgr) const {
  float minDist = FLT_MAX;
  TUniqueId ret = kInvalidUniqueId;
  for (const SConnection* it = GetConnectionList().data();
       it != GetConnectionList().data() + GetConnectionList().size(); ++it) {
    const SConnection& conn = *it;
    if (state == conn.state && conn.msg == kSM_Follow) {
      TUniqueId id = mgr.GetIdForScript(conn.objId);
      float dist;
      TUniqueId closestWp = RecursiveFindClosestWayPoint(mgr, id, dist);
      if (dist < minDist) {
        minDist = dist;
        ret = closestWp;
      }
    }
  }
  return ret;
}

bool CParasite::AnimOver(CStateManager&, const CTriggerData&) const {
  return mStateProgress == 2;
}

bool CParasite::Landed(CStateManager&, const CTriggerData&) const { return mLanded; }

bool CParasite::HitSomething(CStateManager& mgr, const CTriggerData&) const {
  if (mThinkCounter & 0x1) {
    return true;
  }
  return mTumbleAngle < 270.f && CloseToWall(mgr);
}

bool CParasite::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool shouldAttack = false;
  if (mReceivedTelegraph && mTelegraphRemTime > 0.1f) {
    shouldAttack = true;
  }
  return !TooClose(mgr, data) && InMaxRange(mgr, data) &&
         (shouldAttack || InDetectionRange(mgr, CTriggerData(0.f)));
}

void CParasite::Generate(CStateManager&, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    mStateProgress = 0;
    break;
  case kStateMsg_Update:
    switch (mStateProgress) {
    case 0:
      if (mBodyController->GetCurrentStateId() == pas::kAS_Generate) {
        mStateProgress = 1;
      } else {
        mBodyController->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
      }
      break;
    case 1:
      if (mBodyController->GetCurrentStateId() != pas::kAS_Generate) {
        mStateProgress = 2;
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CParasite::Deactivate(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    mStateProgress = 0;
    SendScriptMsgs(kSS_DGNR, mgr, kInvalidUniqueId, kSM_None);
    mgr.DeleteObjectRequest(GetUniqueId());
    break;
  case kStateMsg_Update:
    switch (mStateProgress) {
    case 0:
      if (mBodyController->GetCurrentStateId() == pas::kAS_Generate) {
        mStateProgress = 1;
      } else {
        mBodyController->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CParasite::Run(CStateManager&, EStateMsg, float) {}

void CParasite::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x9c8_26_ = true;
    mAlignToFloor = true;
    if (mWalkerType == kWT_Parasite) {
      mBodyController->SetLocomotionType(pas::kLT_Lurk);
    }
    SetMomentumWR(CVector3f::Zero());
    SetMovable(false);
    break;
  case kStateMsg_Update:
    UpdatePFDestination(mgr);
    DoFlockingBehavior(mgr);
    break;
  case kStateMsg_Deactivate:
    SetMovable(true);
    mAlignToFloor = false;
    x9c8_26_ = false;
    break;
  }
}

void CParasite::Jump(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    AddMaterial(kMT_GroundCollider, mgr);
    SetMomentumWR(CVector3f(0.f, 0.f, -GetWeight()));
    mOnGround = false;
    mAlignToFloor = false;
    mLanded = false;
    mInJump = true;
    break;
  case kStateMsg_Update:
    SetMomentumWR(CVector3f(0.f, 0.f, -GetWeight()));
    break;
  case kStateMsg_Deactivate:
    RemoveMaterial(kMT_GroundCollider, mgr);
    SetMomentumWR(CVector3f::Zero());
    mOnGround = true;
    mLanded = false;
    mInJump = false;
    break;
  }
}

void CParasite::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    SetMomentumWR(CVector3f::Zero());
    TUniqueId wpId = GetClosestWaypointForState(kSS_Patrol, mgr);
    if (wpId != kInvalidUniqueId) {
      mDestObj = wpId;
    }
    break;
  }
  case kStateMsg_Deactivate:
    break;
  case kStateMsg_Update:
    break;
  }
}

void CParasite::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x9c8_26_ = true;
    mAlignToFloor = true;
    if (!mDisableMove && mWalkerType == kWT_Parasite) {
      mBodyController->SetLocomotionType(pas::kLT_Lurk);
    }
    SetMomentumWR(CVector3f::Zero());
    mHasAlignSurface = false;
    SetMovable(false);
    break;
  case kStateMsg_Update: {
    if (mPatrolPauseRemTime > 0.f) {
      mPatrolPauseRemTime -= dt;
      if (mPatrolPauseRemTime <= 0.f) {
        if (mWalkerType == kWT_Parasite) {
          mBodyController->SetLocomotionType(pas::kLT_Lurk);
        }
        mPatrolPauseRemTime = 0.f;
      }
    }
    GotoNextWaypoint(mgr);
    if (mPatrolPauseRemTime <= 0.f && !mDisableMove) {
      DoFlockingBehavior(mgr);
    }
    break;
  }
  case kStateMsg_Deactivate:
    mAlignToFloor = false;
    SetMovable(true);
    break;
  }
}

void CParasite::FaceTarget(CVector3f target) {
  CVector3f delta = target - GetTranslation();
  delta.SetZ(0.f);
  CQuaternion rotation = CQuaternion::LookAt(CVector3f::Forward(), CUnitVector3f(delta),
                                             CRelAngle::FromDegrees(360.f));
  SetTransform(rotation.BuildTransform4f(GetTranslation()));
}

void CParasite::Attack(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate: {
    mTelegraphRemTime = 0.f;
    CRandom16& random = *mgr.Random();
    if (mgr.GetPlayer(0)->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
      mTargetPos =
          mgr.GetPlayer(0)->GetTranslation() +
          0.5f * CVector3f(random.Float() - 0.5f, random.Float() - 0.5f, random.Float() - 0.5f);
    } else {
      mTargetPos =
          GetTranslation() +
          15.f * (mgr.GetPlayer(0)->GetTranslation() +
                  CVector3f(random.Float() - 0.5f, random.Float() - 0.5f, random.Float() + 0.5f) -
                  GetTranslation())
                     .AsNormalized();
    }
    FaceTarget(mTargetPos);
    mStateProgress = 0;
    mAttackOver = false;
    mReceivedTelegraph = false;
    mOnGround = false;
    break;
  }
  case kStateMsg_Update:
    switch (mStateProgress) {
    case 0:
      if (mBodyController->GetCurrentStateId() == pas::kAS_Jump) {
        mStateProgress = 1;
      } else {
        mJumpVelDirty = true;
        FaceTarget(mTargetPos);
        mBodyController->CommandMgr().DeliverCmd(
            CBCJumpCmd(mTargetPos, pas::kJT_Normal, pas::kJS_IntoJump, 0, 2));
      }
      break;
    case 1:
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mOnGround = true;
    mAttackOver = true;
    break;
  }
}

void CParasite::Retreat(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate: {
    CVector3f dir = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
    dir.SetZ(0.f);
    if (dir.CanBeNormalized()) {
      dir = dir.AsNormalized();
    } else {
      dir = mgr.GetPlayer(0)->GetTransform().GetForward();
    }
    mTargetPos = GetTranslation() - 3.f * dir;
    FaceTarget(mTargetPos);
    mStateProgress = 0;
    mLanded = false;
    mOnGround = false;
    mJumpVelDirty = true;
    mBodyController->CommandMgr().DeliverCmd(
        CBCJumpCmd(mTargetPos, static_cast< pas::EJumpType >(1), pas::kJS_IntoJump, 0, 2));
    break;
  }
  case kStateMsg_Update:
    mSpeed = 1.f;
    break;
  case kStateMsg_Deactivate:
    mOnGround = true;
    break;
  }
}

void CParasite::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mTargetPos = mgr.GetPlayer(0)->GetTranslation() + CVector3f(0.f, 0.f, 1.5f);
    break;
  case kStateMsg_Update:
    mBodyController->FaceDirectionOnSurface(
        ProjectVectorToPlane(mTargetPos - GetTranslation(), GetTransform().GetUp()),
        GetTransform().GetForward(), 2.f);
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CParasite::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  CPhysicsActor::Stop();
  TelegraphAttack(mgr, kStateMsg_Activate, 0.f);
  SetMomentumWR(CVector3f(0.f, 0.f, -GetWeight()));
  CPatterned::Death(mgr, direction, state);
}

void CParasite::TelegraphAttack(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate: {
    const rstl::list< CEntity* >& parasites = mgr.GetParasiteList();
    for (rstl::list< CEntity* >::const_iterator it = parasites.begin(); it != parasites.end();
         ++it) {
      CParasite* other = TCastToPtr< CParasite >(*it);
      if (other && other != this && other->GetAlive() &&
          (other->GetTranslation() - GetTranslation()).MagSquared() <
              mMaxTelegraphReactDist * mMaxTelegraphReactDist) {
        other->mReceivedTelegraph = true;
        other->mTelegraphRemTime = mgr.Random()->Float() * 0.5f + 0.5f;
        other->mTargetPos = GetTranslation();
      }
    }
    mHitByPlayerProjectile = false;
    break;
  }
  case kStateMsg_Deactivate:
    break;
  case kStateMsg_Update:
    break;
  }
}

void CParasite::Halt(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    mStateMachine->SetDelay(mHaltDelay);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mHalted = true;
    mAlignToFloor = true;
    if (mWalkerType == kWT_Geemer) {
      CSfxManager::AddEmitter(mHaltSfx, GetTranslation(), GetCurrentAreaId().Value(), true, false);
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*mBodyController, pas::kAS_LoopReaction)) {
      mBodyController->CommandMgr().DeliverCmd(
          CBCLoopReactionCmd(static_cast< pas::EReactionType >(1)));
    }
    mHitByPlayerProjectile = false;
    break;
  case kStateMsg_Deactivate:
    mBodyController->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mHalted = false;
    mAlignToFloor = false;
    break;
  }
}

void CParasite::Crouch(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    switch (mWalkerType) {
    case 10:
      mBodyController->SetLocomotionType(x9c0_ == 2 ? pas::kLT_Crouch : pas::kLT_Internal9);
      break;
    default:
      mBodyController->SetLocomotionType(pas::kLT_Crouch);
      break;
    }
    switch (mWalkerType) {
    case kWT_Geemer:
      CSfxManager::AddEmitter(mCrouchSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                              false);
      break;
    case 10:
      mStateMachine->SetDelay(mHaltDelay);
      mOculusShotAt = false;
      break;
    }
    break;
  }
}

void CParasite::GetUp(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    mBodyController->SetLocomotionType(pas::kLT_Relaxed);
    switch (mWalkerType) {
    case kWT_Geemer:
      CSfxManager::AddEmitter(mGetUpSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                              false);
      break;
    case 10:
      UpdateShell(mgr, 0);
      break;
    }
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CParasite::Touch(CActor& actor, CStateManager& mgr) { CPatterned::Touch(actor, mgr); }

void CParasite::UpdatePFDestination(CStateManager&) {}

static EMaterialTypes skFlockMaterial = kMT_Character;

void CParasite::DoFlockingBehavior(CStateManager& mgr) {
  CVector3f upVec = GetTransform().GetUp();
  rstl::reserved_vector< TUniqueId, 1024 > parasiteList;
  float radius = mParasiteSearchRadius;
  const CVector3f position = GetTranslation();
  CAABox aabb(position - CVector3f(radius, radius, radius),
              position + CVector3f(radius, radius, radius));
  if ((mThinkCounter % 6) == 0) {
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    nearList.clear();
    static const CMaterialFilter filter =
        CMaterialFilter::MakeInclude(CMaterialList(skFlockMaterial));
    CParasite* closestParasite = nullptr;
    float minDistSq = 2.f + mParasiteSeparationDist * mParasiteSeparationDist;
    mgr.BuildNearList(nearList, aabb, filter, nullptr);
    for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      if (CParasite* parasite = TCastToPtr< CParasite >(mgr.ObjectById(*it))) {
        if (parasite->GetUniqueId() != GetUniqueId() && parasite->GetAlive()) {
          parasiteList.push_back(parasite->GetUniqueId());
          float distSq = (parasite->GetTranslation() - GetTranslation()).MagSquared();
          if (distSq < minDistSq) {
            minDistSq = distSq;
            closestParasite = parasite;
          }
        }
      }
    }
    if (closestParasite && mParasiteSeparationWeight > 0.f && mParasiteSeparationDist > 0.f) {
      mParasiteSeparationMove =
          mSteeringBehaviors.Separation(*this, closestParasite->GetTranslation(),
                                        mParasiteSeparationDist) *
          mActiveSpeed;
    } else {
      mParasiteSeparationMove = CVector3f::Zero();
    }
    mParasiteCohesionMove =
        mSteeringBehaviors.Cohesion(*this, parasiteList, 0.6f, mgr) * mActiveSpeed;
    mParasiteAlignmentMove = mSteeringBehaviors.Alignment(*this, parasiteList, mgr) * mActiveSpeed;
  }

  if ((mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).MagSquared() <
      mPlayerSeparationDist * mPlayerSeparationDist) {
    const CVector3f playerSeparation =
        ProjectVectorToPlane(mSteeringBehaviors.Separation(*this,
                                                           mgr.GetPlayer(0)->GetTranslation(),
                                                           mPlayerSeparationDist),
                             upVec) *
        mActiveSpeed;
    mBodyController->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(playerSeparation, CVector3f::Zero(), mPlayerSeparationWeight));
  }

  if (!(mParasiteSeparationMove == CVector3f::Zero())) {
    mBodyController->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(ProjectVectorToPlane(mParasiteSeparationMove, upVec), CVector3f::Zero(),
                         mParasiteSeparationWeight));
  }

  for (rstl::vector< CRepulsor >::iterator it = mDoorRepulsors.begin();
       it != mDoorRepulsors.end(); ++it) {
    const CRepulsor& r = *it;
    if ((r.GetPos() - GetTranslation()).MagSquared() < r.GetRadius() * r.GetRadius()) {
      const CVector3f doorSeparation =
          mSteeringBehaviors.Separation(*this, r.GetPos(), r.GetRadius()) * mActiveSpeed;
      mBodyController->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(ProjectVectorToPlane(doorSeparation, upVec), CVector3f::Zero(), 1.f));
    }
  }

  if (mTelegraphRemTime <= 0.f) {
    mBodyController->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(ProjectVectorToPlane(mParasiteCohesionMove, upVec), CVector3f::Zero(),
                         mParasiteCohesionWeight));
    mBodyController->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(ProjectVectorToPlane(mParasiteAlignmentMove, upVec), CVector3f::Zero(),
                         mParasiteAlignmentWeight));
    const CVector3f seek =
        ProjectVectorToPlane(mSteeringBehaviors.Seek(*this, mDestPos), upVec) * mActiveSpeed;
    mBodyController->CommandMgr().DeliverCmd(CBCLocomotionCmd(
        ProjectVectorToPlane(seek, upVec), CVector3f::Zero(), mDestinationSeekWeight));
    mBodyController->CommandMgr().DeliverCmd(CBCLocomotionCmd(
        GetTransform().GetForward() * mActiveSpeed, CVector3f::Zero(), mForwardMoveWeight));
  }
}

void CParasite::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  if (mWalkerType == 10) {
    SetModelFlags(GetModelFlags().UseShaderSet(x9c4_));
  }
}

void CParasite::Render(const CStateManager& mgr) const { CWallWalker::Render(mgr); }

const CDamageVulnerability* CParasite::GetDamageVulnerability() const {
  switch (mWalkerType) {
  case kWT_Oculus:
    if (mHalted) {
      return &mOculusHaltDVuln;
    }
    break;
  case kWT_IceZoomer:
    if (!mVulnerable) {
      return &CDamageVulnerability::ImmuneVulnerabilty();
    }
    break;
  case 10:
    if (x9c0_ == 1) {
      return &mOculusHaltDVuln;
    }
    break;
  }
  return CPatterned::GetDamageVulnerability();
}

EWeaponCollisionResponseTypes CParasite::GetCollisionResponseType(const CVector3f& position,
                                                                  const CVector3f& direction,
                                                                  const CWeaponMode& mode,
                                                                  int attributes) const {
  switch (mWalkerType) {
  case 10:
    if (GetDamageVulnerability()->GetEffect(mode) == 1) {
      return static_cast< EWeaponCollisionResponseTypes >(15);
    }
    break;
  }
  return CPatterned::GetCollisionResponseType(position, direction, mode, attributes);
}

CDamageInfo CParasite::GetContactDamage() const {
  switch (mWalkerType) {
  case kWT_Oculus:
    if (mHalted) {
      return mOculusHaltDInfo;
    }
    return CPatterned::GetContactDamage();
  case kWT_IceZoomer:
    if (!mVulnerable) {
      return mOculusHaltDInfo;
    }
    return CPatterned::GetContactDamage();
  default:
    return CPatterned::GetContactDamage();
  }
}

void CParasite::SetupIceZoomerCollision(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > descs;
  descs.reserve(2);
  CAnimData& animData = *ModelData()->AnimationData();
  for (int i = 0; i < ARRAY_SIZE(skIceJoints); ++i) {
    const SSphereJointInfo& joint = skIceJoints[i];
    const CJointCollisionDescription desc = CJointCollisionDescription::SphereCollision(
        animData.GetLocatorSegId(rstl::string_l(joint.name)), CVector3f::Zero(),
        0.01f + mColSphere.GetSphere().GetRadius(), rstl::string_l(joint.name), 0.001f);
    descs.push_back_unsafe(desc);
  }
  RemoveMaterial(kMT_Unknown59, mgr);
  AddMaterial(kMT_NoPlatformCollision, mgr);
  mCollisionActorManager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), descs, GetActive());
}

void CParasite::DestroyActorManager(CStateManager& mgr) { mCollisionActorManager->Destroy(mgr); }

void CParasite::SetupIceZoomerVulnerability(CStateManager& mgr, const CDamageVulnerability& dVuln,
                                            const CHealthInfo& hInfo) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& cDesc = mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = cDesc.GetCollisionActorId();
    if (CCollisionActor* const act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      act->SetDamageVulnerability(dVuln);
      *act->HealthInfo() = hInfo;
    }
  }
}

void CParasite::UpdateCollisionActors(float dt, CStateManager& mgr) {
  mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  if (!mVulnerable) {
    float totalHP = 0.f;
    for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
      const CJointCollisionDescription& cDesc = mCollisionActorManager->GetCollisionDescFromIndex(i);
      const TUniqueId id = cDesc.GetCollisionActorId();
      if (CCollisionActor* cact = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
        totalHP += cact->HealthInfo()->GetHP();
      }
    }
    if (totalHP <= 0.f) {
      mVulnerable = true;
      AddMaterial(kMT_Unknown59, mgr);
      RemoveMaterial(kMT_NoPlatformCollision, mgr);
      DestroyActorManager(mgr);
      ModelData()->AnimationData()->SetSkinnedModel(*mExtraModel);
      if (mWalkerType == kWT_IceZoomer) {
        CSfxManager::AddEmitter(mHaltSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                                false);
        mBodyController->SetLocomotionType(pas::kLT_Internal8);
        mBodyController->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_Zero));
        SendScriptMsgs(kSS_AboutToMassivelyDie, mgr, kInvalidUniqueId, kSM_None);
      }
    }
  }
}

void CParasite::MassiveDeath(CStateManager& mgr) { CPatterned::MassiveDeath(mgr); }

void CParasite::MassiveFrozenDeath(CStateManager& mgr) { CPatterned::MassiveFrozenDeath(mgr); }

void CParasite::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

void CParasite::UpdateShell(CStateManager& mgr, int state) {}

CEntity* LoadParasite(CStateManager& mgr, CInputStream& input, CEntityInfo& info) { return nullptr; }

CEntity* LoadBrizgee(CStateManager& mgr, CInputStream& input, CEntityInfo& info) { return nullptr; }

CEntity* LoadCrystallite(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  return nullptr;
}

static void SetFuncPtrs() {
  static SParasite_FuncPtrs funcPtrs;
  funcPtrs.mLoadParasite = &LoadParasite;
  funcPtrs.mLoadBrizgee = &LoadBrizgee;
  funcPtrs.mLoadCrystallite = &LoadCrystallite;
  SetSParasite_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSParasite_FuncPtrs(nullptr); }

