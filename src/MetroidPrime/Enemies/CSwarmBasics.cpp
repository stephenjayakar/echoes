#include "MetroidPrime/Enemies/CSwarmBasics.hpp"

#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/SwarmRenderHelpers.hpp"

#include "WorldFormat/CMetroidAreaCollider.hpp"
#include "Kyoto/Graphics/CGX.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"

#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/Weapons/CIceImpact.hpp"
#include "MetroidPrime/Weapons/CLightComboProjectile.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "Kyoto/Animation/CSoundPOINode.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "rstl/algorithm.hpp"

#include <float.h>
#include <stdlib.h>

#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Animation/CAdvancementDeltas.hpp"
#include "Kyoto/Math/CTri.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"

#include "REL/REL_Setup.h"

// The native record holds a single callback that always returns null; its signature is unknown.
struct SSwarmBasics_FuncPtrs {
  void* (*mFactory)();
};

// Guessed names: qsort comparators over the listener distance cached in each boid.
static int CompareBoidsByListenerDistance(const void* a, const void* b) {
  const CSwarmBasics::CBoid* boidA = *static_cast< CSwarmBasics::CBoid* const* >(a);
  const CSwarmBasics::CBoid* boidB = *static_cast< CSwarmBasics::CBoid* const* >(b);
  if (boidA->GetDistanceSquaredToSoundListener() < boidB->GetDistanceSquaredToSoundListener()) {
    return 1;
  }
  if (boidA->GetDistanceSquaredToSoundListener() > boidB->GetDistanceSquaredToSoundListener()) {
    return -1;
  }
  return 0;
}

static int CompareBoidsByListenerDistanceReverse(const void* a, const void* b) {
  const CSwarmBasics::CBoid* boidA = *static_cast< CSwarmBasics::CBoid* const* >(a);
  const CSwarmBasics::CBoid* boidB = *static_cast< CSwarmBasics::CBoid* const* >(b);
  if (boidA->GetDistanceSquaredToSoundListener() > boidB->GetDistanceSquaredToSoundListener()) {
    return 1;
  }
  if (boidA->GetDistanceSquaredToSoundListener() < boidB->GetDistanceSquaredToSoundListener()) {
    return -1;
  }
  return 0;
}

static int CompareBoidRefsByListenerDistance(const void* a, const void* b) {
  const CSwarmBasics::CBoid* boidA = static_cast< const CSwarmBasics::CBoid* >(a);
  const CSwarmBasics::CBoid* boidB = static_cast< const CSwarmBasics::CBoid* >(b);
  if (boidA->GetDistanceSquaredToSoundListener() > boidB->GetDistanceSquaredToSoundListener()) {
    return 1;
  }
  if (boidA->GetDistanceSquaredToSoundListener() < boidB->GetDistanceSquaredToSoundListener()) {
    return -1;
  }
  return 0;
}

CAABox CSwarmBasics::GetBoundingBox() const {
  CVector3f he = mBoundingBoxExtent * 0.5f;
  return CAABox(-he, he).GetTransformedAABox(GetTransform());
}

rstl::optional_object< CAABox > CSwarmBasics::GetTouchBounds() const { return mAabox; }

void CSwarmBasics::UpdateAllBoidMovement(CStateManager& mgr, float dt) {
  int count = mBoids.size();
  if (x4f0_27_) {
    int mask = mModelDatas.size() - 1;
    for (int i = 0; i < count; ++i) {
      UpdateBoidMovement(mgr, mBoids[i], mAdvancementDeltas[i & mask], dt);
    }
  }
}

void CSwarmBasics::UpdateSwarmAnimations(CStateManager& mgr, float dt) {
  if (x4f0_27_ && x4f0_31_) {
    uint count = mModelDatas.size();
    for (uint i = 0; i < count; ++i) {
      mModelDatas[i].AnimationData()->SetPlaybackRate(mAnimPlaybackSpeed);
      mAdvancementDeltas[i] = mModelDatas[i].AdvanceAnimation(dt, mgr, GetCurrentAreaId(), true);
      UpdateEffects(mgr, *mModelDatas[i].AnimationData());
    }
  }
}

void CSwarmBasics::UpdateBoidMovement(CStateManager& mgr, CBoid& boid,
                                      const CAdvancementDeltas& deltas, float dt) {
  if (boid.GetActive()) {
    if (boid.mFreezeTimer > 0.f) {
      boid.mFreezeTimer -= dt;
      if (boid.mFreezeTimer < 0.7f * mgr.Random()->Float()) {
        KillBoid(boid, mgr, CWeaponMode(kWT_Dark));
      }
    } else {
      float speed = boid.xa4_ / dt;
      boid.mVelocity = speed * boid.GetTransform().Rotate(deltas.GetOffsetDelta());
      boid.mTransform.AddTranslation(dt * boid.mVelocity);
    }
  }
}

void CSwarmBasics::UpdatePartition() {
  mActiveBoidIndices.clear();
  mPartitionedBoidLists.clear();
  for (int i = 0; i < 125; ++i) {
    mPartitionedBoidLists.push_back(nullptr);
  }
  mOutlierBoidList = nullptr;
  const CAABox bounds = GetBoundingBox();
  const CVector3f extent = bounds.GetMaxPoint() - bounds.GetMinPoint();
  const CVector3f size(extent.GetX() / 5.f, extent.GetY() / 5.f, extent.GetZ() / 5.f);
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (!it->GetActive()) {
      if (it->mHasLoopedSound) {
        StopLoopedSound(*it, mLocomotionSounds);
      }
    } else {
      mActiveBoidIndices.push_back_unsafe(it->mIndex);
      const CVector3f pos = it->GetTranslation();
      const CVector3f delta = pos - bounds.GetMinPoint();
      const int x = CCast::ToInt32(delta.GetX() / size.GetX());
      const int y = CCast::ToInt32(delta.GetY() / size.GetY());
      const int z = CCast::ToInt32(delta.GetZ() / size.GetZ());
      const int index = x + 5 * y + 25 * z;
      if (index < 0 || index >= 125 || x < 0 || x >= 5 || y < 0 || y >= 5 || z < 0 || z >= 5) {
        it->mNext = mOutlierBoidList;
        mOutlierBoidList = it.get_pointer();
      } else {
        it->mNext = mPartitionedBoidLists[index];
        mPartitionedBoidLists[index] = it.get_pointer();
      }
    }
  }
}

CSwarmBasics::CBoid* CSwarmBasics::GetListAt(const CVector3f& pos) {
  const CAABox bounds = GetBoundingBox();
  const CVector3f delta = pos - bounds.GetMinPoint();
  const int index = CCast::ToInt32(delta.GetX() / (bounds.GetWidth() / 5.f)) +
                    CCast::ToInt32(delta.GetY() / (bounds.GetHeight() / 5.f)) * 5 +
                    CCast::ToInt32(delta.GetZ() / (bounds.GetDepth() / 5.f)) * 25;
  if (index < 0 || index >= 125) {
    return mOutlierBoidList;
  }
  return mPartitionedBoidLists[index];
}

CAABox CSwarmBasics::BoxForPosition(int x, int y, int z, float margin) const {
  CAABox box = GetBoundingBox();
  CVector3f diff = box.GetMaxPoint() - box.GetMinPoint();
  CVector3f partitionSize = diff / 5.f;
  return CAABox(box.GetMinPoint() + partitionSize * CVector3f(x, y, z) - CVector3f(margin, margin, margin),
                box.GetMinPoint() + partitionSize * CVector3f(x + 1, y + 1, z + 1) +
                    CVector3f(margin, margin, margin));
}

void CSwarmBasics::PreRender(CStateManager& mgr) {
  bool active = false;
  if (x4f0_27_) {
    for (uint i = 0; i < mModelDatas.size(); ++i) {
      mModelDatas[i].AnimationData()->PreRender();
    }
  }
  uint drawMask = -1;
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->mActive) {
      it->mInFrustum = mgr.GetFrustumPlanes().SphereInFrustumPlanes(
          CSphere(it->GetTranslation(), 2.f * mBoidRadius));
      PreRenderBoid(it.get_pointer(), &drawMask);
      active = true;
    } else {
      it->mInFrustum = false;
    }
  }
  SetPreRenderClipped(!active);
}

bool CSwarmBasics::CanRenderUnsorted(const CStateManager& mgr) const { return true; }

void CSwarmBasics::CalculateSkinnedState(CModelData& modelData,
                                         SwarmRenderHelpers::CSwarmSkinnedModelState& state) {
  const CAnimData* animData = modelData.GetAnimationData();
  const CSkinnedModel& model = **mModelData->GetAnimationData()->GetModelData();
  animData->BuildPose();
  model.StoreCalculation(state.State(), &animData->Pose());
  state.StateToArrays();
}

void CSwarmBasics::PreRenderBoid(CBoid* boid, uint* drawMask) {
  if (!(boid->mFreezeTimer > 0.f)) {
    int idx = boid->mIndex & (mModelDatas.size() - 1);
    uint bit = 1 << idx;
    if (*drawMask & bit) {
      *drawMask &= ~bit;
      CalculateSkinnedState(mModelDatas[idx], mSkinnedModelStates[idx]);
    }
  }
}

void CSwarmBasics::RenderBoid(CBoid* boid) const {
  if (x4f0_27_) {
    if (boid->mFreezeTimer > 0.f) {
      RenderBoidModel(boid, *mSkinnedModelState);
    } else {
      RenderBoidModel(boid, mSkinnedModelStates[boid->mIndex & (mModelDatas.size() - 1)]);
    }
  }
}

void CSwarmBasics::RenderBoidModel(CBoid* boid,
                                   const SwarmRenderHelpers::CSwarmSkinnedModelState& state) const {
  CColor color = boid->mAmbientLighting;
  if (boid->mFreezeTimer > 0.f) {
    color = CColor::Lerp(color, CPatterned::skFrozenColor,
                         CMath::Clamp(0.f, boid->mFreezeTimer, 1.f));
  }
  if (x4f0_24_) {
    CGX::SetChanMatColor(CGX::Channel0, color.GetGXColor());
  }
  gpRender->SetModelMatrix(boid->GetTransform());
  mDisplayList->DrawFromState(state);
}

void CSwarmBasics::AddToRenderer(const CStateManager& mgr) const {
  if (GetActive()) {
    RenderParticles();
    if (!GetPreRenderClipped()) {
      if (CanRenderUnsorted(mgr)) {
        Render(mgr);
      } else {
        EnsureRendered(mgr);
      }
    }
  }
}

void CSwarmBasics::BuildBoidNearList(const CBoid& boid, float radius,
                                     rstl::reserved_vector< CBoid*, 50 >& nearList) {
  CBoid* other = GetListAt(boid.GetTranslation());
  while (other != nullptr && nearList.size() < 50) {
    const float distance = (other->GetTranslation() - boid.GetTranslation()).MagSquared();
    if (distance != 0.f && distance < radius) {
      nearList.push_back(other);
    }
    other = other->mNext;
  }
}

void CSwarmBasics::ApplySeparation(CBoid& boid,
                                   const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                   CVector3f& ahead) {
  if (nearList.size() > 0) {
    CVector3f closest(0.f, 0.f, 0.f);
    float minDistance = FLT_MAX;
    for (rstl::reserved_vector< CBoid*, 50 >::const_iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      const CVector3f delta = boid.GetTranslation() - (*it)->GetTranslation();
      const float distance = delta.MagSquared();
      if (distance != 0.f && distance < minDistance) {
        minDistance = distance;
        closest = (*it)->GetTranslation();
      }
    }
    ApplySeparation(boid, closest, mSeparationRadius, mSeparationMagnitude, ahead);
  }
}

void CSwarmBasics::ApplySeparation(CBoid& boid, const CVector3f& pos, float radius,
                                   float magnitude, CVector3f& ahead) {
  const CVector3f delta = boid.GetTranslation() - pos;
  if (delta.CanBeNormalized()) {
    const float distance = delta.MagSquared();
    const float radiusSquared = radius * radius;
    if (distance < radiusSquared) {
      const float factor = 1.f - distance / radiusSquared;
      ahead += factor * delta.AsNormalized() * magnitude;
    }
  }
}

void CSwarmBasics::ApplyCohesion(CBoid& boid, const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                 CVector3f& ahead) {
  if (nearList.size() > 0) {
    CVector3f center(0.f, 0.f, 0.f);
    for (rstl::reserved_vector< CBoid*, 50 >::const_iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      center += (*it)->GetTranslation();
    }
    center = (1.f / nearList.size()) * center;
    ApplyCohesion(boid, center, mSeparationRadius, mCohesionMagnitude, ahead);
  }
}

void CSwarmBasics::ApplyCohesion(CBoid& boid, const CVector3f& pos, float radius, float magnitude,
                                 CVector3f& ahead) {
  const CVector3f delta = pos - boid.GetTranslation();
  if (delta.CanBeNormalized()) {
    const float distance = delta.MagSquared();
    const float radiusSquared = radius * radius;
    const float factor = distance > radiusSquared ? 1.f : distance / radiusSquared;
    ahead += factor * delta.AsNormalized() * magnitude;
  }
}

void CSwarmBasics::ApplyAttraction(CBoid& boid, const CVector3f& pos, float radius,
                                   float magnitude, CVector3f& ahead) {
  const float radiusSquared = radius * radius;
  const CVector3f delta = pos - boid.GetTranslation();
  const float distance = delta.MagSquared();
  if (distance < radiusSquared && delta.CanBeNormalized()) {
    const float factor = 1.f - distance / radiusSquared;
    ahead += factor * delta.AsNormalized() * magnitude;
  }
}

void CSwarmBasics::ApplyAlignment(CBoid& boid, const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                  CVector3f& ahead) {
  if (nearList.size() > 0) {
    CVector3f direction(0.f, 0.f, 0.f);
    for (rstl::reserved_vector< CBoid*, 50 >::const_iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      direction += (*it)->GetTransform().GetForward();
    }
    direction = (1.f / nearList.size()) * direction;
    const float angle =
        CVector3f::GetAngleDiff(boid.GetTransform().GetForward(), direction) / M_PIF;
    ahead += angle * (mAlignmentWeight * direction);
  }
}

static CPlane GetClosestBoxFacePlane(const CAABox& box, const CVector3f& point) {
  float minDistance = FLT_MAX;
  int bestFace = 0;
  for (int i = 0; i < 6; ++i) {
    const CTri tri = box.GetTri(CAABox::EBoxFaceId(i), 0);
    const CPlane plane(tri.GetPointA(), tri.GetPointC(), tri.GetPointB());
    const float distance = plane.GetHeight(point);
    if (distance >= 0.f && distance < minDistance) {
      bestFace = i;
      minDistance = distance;
    }
  }
  const CTri tri = box.GetTri(CAABox::EBoxFaceId(bestFace), 0);
  return CPlane(tri.GetPointA(), tri.GetPointC(), tri.GetPointB());
}

void CSwarmBasics::ApplyBoundsAvoidance(CBoid& boid,
                                        const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                        CVector3f& ahead) {
  const CAABox bounds = GetBoundingBox();
  const CVector3f future = boid.GetTranslation() + 1.5f * boid.mVelocity;
  if (!bounds.PointInside(future)) {
    const CPlane plane = GetClosestBoxFacePlane(bounds, future);
    const float distance = plane.GetHeight(future);
    const float factor = distance > 5.f ? 1.f : 5.f / (0.00001f + distance);
    ahead -= factor * plane.GetNormal();
  }
}

void CSwarmBasics::MoveToWayPoint(CBoid& boid, CStateManager& mgr, CVector3f& ahead) {
  CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(boid.mTargetWaypoint));
  if (wp) {
    if (!wp->GetActive() ||
        boid.mSurfacePlane.GetHeight(boid.GetTranslation()) > -mWaypointGoalRadius) {
      rstl::vector< TUniqueId > nextWaypoints(8);
      for (rstl::vector< SConnection >::const_iterator it = wp->GetConnectionList().begin();
           it != wp->GetConnectionList().end(); ++it) {
        if (it->msg == kSM_Next) {
          TUniqueId uid = mgr.GetIdForScript(it->objId);
          const CScriptWaypoint* next =
              TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(uid));
          if (next && next->GetActive()) {
            nextWaypoints.push_back_unsafe(uid);
          }
        }
      }
      boid.mTargetWaypoint = kInvalidUniqueId;
      if (nextWaypoints.size() != 0) {
        if (nextWaypoints.size() > 1) {
          boid.mTargetWaypoint =
              nextWaypoints[mgr.Random()->Next() % nextWaypoints.size()];
        } else {
          boid.mTargetWaypoint = nextWaypoints[0];
        }
      }
      CScriptWaypoint* next = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(boid.mTargetWaypoint));
      if (!next) {
        boid.mActive = false;
        if (boid.mHasLoopedSound) {
          StopLoopedSound(boid, mLocomotionSounds);
        }
        return;
      }
      CUnitVector3f normal((next->GetTranslation() - wp->GetTranslation()).AsNormalized());
      boid.mSurfacePlane = CPlane(next->GetTranslation(), normal);
      wp = next;
    }
    const float weight = mMoveToWaypointWeight;
    ahead += weight * (wp->GetTranslation() - boid.GetTranslation()).AsNormalized();
  }
}

TUniqueId CSwarmBasics::GetWaypointForState(EScriptObjectState state, CStateManager& mgr) {
  rstl::vector< TUniqueId > waypoints(8);
  for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
       it != GetConnectionList().end(); ++it) {
    if (it->state == state && it->msg == kSM_Follow) {
      TUniqueId uid = mgr.GetIdForScript(it->objId);
      if (TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(uid)) && waypoints.size() < 8) {
        waypoints.push_back_unsafe(uid);
      }
    }
  }
  if (waypoints.size() != 0) {
    if (waypoints.size() > 1) {
      return waypoints[mgr.Random()->Next() % waypoints.size()];
    }
    return waypoints[0];
  }
  return kInvalidUniqueId;
}

void CSwarmBasics::ApplyRadiusDamage(CVector3f pos, const CDamageInfo& info, CStateManager& mgr) {
  const float radiusSquared = info.GetRadius() * info.GetRadius();
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->GetActive()) {
      if ((it->GetTranslation() - pos).MagSquared() < radiusSquared) {
        it->mHealth -= info.GetRadiusDamage(mDamageVulnerability);
        if (it->mHealth <= 0.f) {
          KillBoid(*it, mgr, info.GetWeaponMode());
        }
      }
    }
  }
}

void CSwarmBasics::SetExplodeTimers(const CVector3f& pos, float radius, float minTime,
                                    float maxTime) {
  const float radiusSquared = radius * radius;
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->GetActive() && it->mFreezeTimer <= 0.f) {
      const float distanceSquared = (it->GetTranslation() - pos).MagSquared();
      if (distanceSquared < radiusSquared) {
        const float time = (distanceSquared / radiusSquared) * (maxTime - minTime) + minTime;
        if (it->mTimeToExplode > time || it->mTimeToExplode == 0.f) {
          it->mTimeToExplode = time;
        }
      }
    }
  }
}

bool CSwarmBasics::IsBoidVisibleForLockOn(const CStateManager& mgr, const CBoid& boid,
                                          const CVector3f& cameraPos,
                                          const CVector3f& cameraForward) const {
  const CVector3f delta = boid.GetTranslation() - cameraPos;
  const float distance = delta.Magnitude();
  const CVector3f dir = (1.f / distance) * delta;
  if (CVector3f::Dot(cameraForward, dir) > 0.9238795f) {
    const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Unknown59));
    const CRayCastResult result = mgr.RayStaticIntersection(cameraPos, dir, distance, filter);
    if (!result.IsValid()) {
      return true;
    }
  }
  return false;
}

int CSwarmBasics::GetLockOnIndex(CStateManager& mgr) const {
  if (!x4f0_26_) {
    return -1;
  }
  const CTransform4f cameraXf = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetTransform();
  const CVector3f cameraPos = cameraXf.GetTranslation();
  const CVector3f cameraForward = cameraXf.GetForward();
  const bool playerOrbiting = mgr.GetPlayer(0)->GetOrbitTargetId() == GetUniqueId();
  if (mLockOnIndex != -1) {
    int result = -1;
    if (mBoids[mLockOnIndex].GetActive() &&
        IsBoidVisibleForLockOn(mgr, mBoids[mLockOnIndex], cameraPos, cameraForward)) {
      result = mLockOnIndex;
    }
    if (result != -1 && x54c_24_ && !playerOrbiting) {
      result = FindBestLockOnIndex(mgr);
    }
    return result;
  }
  return FindBestLockOnIndex(mgr);
}

int CSwarmBasics::FindBestLockOnIndex(CStateManager& mgr) const {
  float maxDot = 0.5f;
  int index = 0;
  int result = -1;
  float maxDistanceSq = mgr.GetPlayer(0)->GetOrbitMaxTargetDistance();
  maxDistanceSq *= maxDistanceSq;
  const CTransform4f cameraXf = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetTransform();
  const CVector3f cameraPos = cameraXf.GetTranslation();
  const CVector3f cameraForward = cameraXf.GetForward();
  for (rstl::vector< CBoid >::const_iterator it = mBoids.begin(); it != mBoids.end();
       ++it, ++index) {
    if (it->GetActive()) {
      const CVector3f delta = it->GetTranslation() - cameraPos;
      if (delta.MagSquared() <= maxDistanceSq && delta.CanBeNormalized()) {
        const float dot = CVector3f::Dot(cameraForward, delta.AsNormalized());
        if (dot > maxDot) {
          result = index;
          maxDot = dot;
        }
      }
    }
  }
  return result;
}

// Guessed name: orders seeker candidates by descending view alignment.
struct SSeekerCandidateSorter {
  bool operator()(const rstl::pair< uint, float >& a, const rstl::pair< uint, float >& b) const {
    return a.second > b.second;
  }
};

void CSwarmBasics::AssignSeekerBoids(CStateManager& mgr, const rstl::vector< int >& taken,
                                     uint numNeeded, rstl::vector< int >& out) {
  float maxDistSq = mgr.GetPlayer(0)->GetOrbitMaxTargetDistance();
  maxDistSq *= maxDistSq;
  CTransform4f camXf = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetTransform();
  CVector3f camPos = camXf.GetTranslation();
  CVector3f camFwd = camXf.GetForward();
  rstl::vector< rstl::pair< uint, float > > candidates(mBoids.size());
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->GetActive()) {
      CVector3f delta = it->GetTranslation() - camPos;
      if (delta.MagSquared() <= maxDistSq && delta.CanBeNormalized()) {
        float dot = CVector3f::Dot(camFwd, delta.AsNormalized());
        if (dot > 0.5f) {
          candidates.push_back_unsafe(rstl::pair< uint, float >(uint(it->mIndex), dot));
        }
      }
    }
  }
  rstl::sort(candidates.begin(), candidates.end(), SSeekerCandidateSorter());
  for (uint i = 0; out.size() < numNeeded && i < candidates.size(); ++i) {
    uint idx = candidates[i].first;
    bool found = false;
    for (uint j = 0; j < taken.size(); ++j) {
      if (idx == taken[j]) {
        found = true;
        break;
      }
    }
    if (!found) {
      out.push_back_unsafe(idx);
    }
  }
}

void CSwarmBasics::UpdateLockOnBlend(int prevIndex, int newIndex, float dt) {
  if (x54c_24_) {
    if (newIndex >= 0) {
      if (prevIndex >= 0 && prevIndex != newIndex) {
        if (!x54c_25_) {
          x550_ = mLastOrbitPosition;
        }
        x54c_25_ = true;
        x55c_ = 0.f;
      }
      if (x54c_25_) {
        x55c_ += 3.f * dt;
        if (x55c_ >= 1.f) {
          x54c_25_ = false;
        }
      }
    } else {
      x54c_25_ = false;
    }
  }
}

CVector3f CSwarmBasics::GetAimPosition(const CStateManager& mgr, float dt) const {
  if (mLockOnIndex == -1) {
    return mLastOrbitPosition;
  }
  if (x54c_24_ && x54c_25_ && dt == 0.f) {
    return (1.f - x55c_) * x550_ + x55c_ * mLastOrbitPosition;
  }
  return mLastOrbitPosition + dt * mBoids[mLockOnIndex].mVelocity;
}

CVector3f CSwarmBasics::GetOrbitPosition(const CStateManager& mgr) const {
  return mLastOrbitPosition;
}

void CSwarmBasics::KillBoid(CBoid& boid, CStateManager& mgr, const CWeaponMode& weapon) {
  mHealthInfo.SetCauseOfDeathWeapon(weapon, kInvalidUniqueId, kInvalidUniqueId, false, false);
  mLastKilledOffset = boid.GetTranslation();
  AddParticle(boid.GetTransform());
  boid.mActive = false;
  if (boid.mHasLoopedSound && boid.xb2_4) {
    StopLoopedSound(boid, mAttackSounds);
  } else if (boid.mHasLoopedSound) {
    StopLoopedSound(boid, mLocomotionSounds);
  }
  QueueDeathMessage(mgr);
  x4f1_24_ = true;
  if (boid.xaa_ != kInvalidUniqueId) {
    CLightComboProjectile* proj = TCastToPtr< CLightComboProjectile >(mgr.ObjectById(boid.xa8_));
    if (TCastToConstPtr< CPlasmaProjectile >(mgr.GetObjectById(boid.xaa_)) && proj) {
      proj->RequestRayReset(mgr, boid.xaa_, false);
    }
    boid.xaa_ = kInvalidUniqueId;
    boid.xa8_ = kInvalidUniqueId;
  }
}

void CSwarmBasics::StopLoopedSound(CBoid& boid, rstl::vector< TLoopedSound >& sounds) {
  for (uint i = 0; i < sounds.size(); ++i) {
    if (boid.mIndex == sounds[i].second) {
      CSfxManager::SfxStop(sounds[i].first);
      sounds[i].first = CSfxHandle();
      boid.mHasLoopedSound = false;
      return;
    }
  }
}

void CSwarmBasics::AddParticle(const CTransform4f& xf) {
  if (mParticleGenerator.get()) {
    mParticleGenerator->SetParticleEmission(true);
    mParticleGenerator->SetTranslation(xf.GetTranslation());
    mParticleGenerator->ForceParticleCreation(mNumDeathParticles);
    mParticleGenerator->SetParticleEmission(false);
  }
}

void CSwarmBasics::UpdateParticles(float dt) {
  if (mParticleGenerator.get()) {
    mParticleGenerator->Update(dt);
  }
}

void CSwarmBasics::RenderParticles() const {
  if (mParticleGenerator.get()) {
    gpRender->AddParticleGen(*mParticleGenerator);
  }
}

void CSwarmBasics::FreezeCollision(const CMarkerGrid& grid) {
  const float radius = mTouchRadius * mTouchRadius;
  const float xy = radius + 0.3f;
  const float z = radius + 0.5f;
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->GetActive()) {
      const CVector3f extent(xy, xy, z);
      const CAABox bounds = CAABox(it->GetTranslation() - extent, it->GetTranslation() + extent);
      if (grid.AABoxTouchesData(bounds, 1)) {
        it->mFreezeTimer = 1.f;
      }
    }
  }
}

int CSwarmBasics::CountActiveBoids() const {
  int count = 0;
  for (int i = 0; i < mBoids.size(); ++i) {
    if (mBoids[i].GetActive()) {
      ++count;
    }
  }
  return count;
}

CVector3f CSwarmBasics::FindClosestCell(const CVector3f& pos) const {
  float minDistance = FLT_MAX;
  CVector3f result = CVector3f::Zero();
  for (int x = 0; x < 5; ++x) {
    int rowIndex = x;
    for (int y = 0; y < 5; ++y, rowIndex += 5) {
      for (int z = 0; z < 5; ++z) {
        if (mPartitionedBoidLists[rowIndex + z * 25] != nullptr) {
          const CAABox bounds = BoxForPosition(x, y, z, 0.1f);
          const float distance = (bounds.GetCenterPoint() - pos).MagSquared();
          if (distance < minDistance) {
            result = bounds.GetCenterPoint();
            minDistance = distance;
          }
        }
      }
    }
  }
  return result;
}

CSwarmBasics::CBoid* CSwarmBasics::GetClosestPartitionList(const CVector3f& pos) const {
  float minDistance = FLT_MAX;
  CBoid* result = nullptr;
  for (int x = 0; x < 5; ++x) {
    int rowIndex = x;
    for (int y = 0; y < 5; ++y, rowIndex += 5) {
      for (int z = 0; z < 5; ++z) {
        CBoid* list = mPartitionedBoidLists[rowIndex + z * 25];
        if (list != nullptr) {
          const CAABox bounds = BoxForPosition(x, y, z, 0.1f);
          const float distance = (bounds.GetCenterPoint() - pos).MagSquared();
          if (distance < minDistance) {
            result = list;
            minDistance = distance;
          }
        }
      }
    }
  }
  return result;
}

uint CSwarmBasics::UpdateLoopedSounds(uint maxEmitters, signed char partitionIndex,
                                      rstl::vector< TLoopedSound >& sounds) {
  uint active = 0;
  for (uint i = 0; i < maxEmitters; ++i) {
    if (sounds[i].first) {
      CBoid& boid = mBoids[sounds[i].second];
      if (boid.mPartitionIndex != partitionIndex || partitionIndex == -1) {
        ++boid.xb1_;
        if (boid.xb1_ > x560_) {
          CSfxManager::SfxStop(sounds[i].first);
          sounds[i].first = CSfxHandle();
          boid.mHasLoopedSound = false;
        }
      } else {
        CSfxManager::UpdateEmitter(sounds[i].first, boid.GetTranslation(), CVector3f::Zero(), 127);
        ++active;
        boid.xb1_ = 0;
      }
    }
  }
  return active;
}

void CSwarmBasics::UpdateClosestPartitionLoopedSounds(const CVector3f& listener,
                                                      rstl::vector< TLoopedSound >& sounds,
                                                      uint maxEmitters, ushort sfx,
                                                      ELoopedSoundType type) {
  if (sounds.size() > 0) {
    CBoid* list = GetClosestPartitionList(listener);
    if (list) {
      uint active = UpdateLoopedSounds(maxEmitters, list->mPartitionIndex, sounds);
      if (active < maxEmitters) {
        CBoid* candidates[64];
        uint count = 0;
        for (CBoid* boid = list; boid && count < 64; boid = boid->mNext) {
          if (CanStartLoopedSound(*boid, type)) {
            ++count;
            boid->mDistanceSquaredToSoundListener = (listener - boid->GetTranslation()).MagSquared();
            candidates[count - 1] = boid;
          }
        }
        if (count != 0) {
          if (count > maxEmitters - active) {
            qsort(candidates, count, sizeof(CBoid*), CompareBoidsByListenerDistance);
          }
          CBoid** it = candidates;
          uint used = 0;
          for (uint i = 0; i < maxEmitters && used < count; ++i) {
            if (!sounds[i].first) {
              StartLoopedSound(**it, sounds, sfx, i);
              ++it;
              ++used;
            }
          }
        }
      }
    } else {
      UpdateLoopedSounds(maxEmitters, -1, sounds);
    }
  }
}

bool CSwarmBasics::TryStartLoopedSound(CBoid& boid, rstl::vector< TLoopedSound >& sounds,
                                       ushort sfx) {
  for (uint i = 0; i < sounds.size(); ++i) {
    if (!sounds[i].first) {
      StartLoopedSound(boid, sounds, sfx, i);
      return true;
    }
  }
  return false;
}

void CSwarmBasics::StartLoopedSound(CBoid& boid, rstl::vector< TLoopedSound >& sounds, ushort sfx,
                                    uint slot) {
  sounds[slot].first = AddLoopedEmitter(boid.GetTranslation(), sfx);
  sounds[slot].second = boid.mIndex;
  boid.mHasLoopedSound = true;
}

void CSwarmBasics::UpdateLoopedSoundPositions(const rstl::vector< TLoopedSound >& sounds) {
  uint count = sounds.size();
  const TLoopedSound* data = sounds.data();
  for (uint i = 0; i < count; ++i) {
    if (data[i].first) {
      CSfxManager::UpdateEmitter(data[i].first, mBoids[data[i].second].GetTranslation(),
                                 CVector3f::Zero(), 127);
    }
  }
}

bool CSwarmBasics::CanStartLoopedSound(const CBoid& boid, ELoopedSoundType type) const {
  switch (type) {
  case kLST_Locomotion:
    return !boid.mHasLoopedSound && !boid.xb2_4 && boid.mActive;
  case kLST_Attack:
    return !boid.mHasLoopedSound && boid.xb2_4 && boid.mActive;
  default:
    return false;
  }
}

CSfxHandle CSwarmBasics::AddLoopedEmitter(const CVector3f& pos, ushort sfx) {
  CAudioSys::C3DEmitterParmData parms(mMaxAudibleDistance, mSoundFallOff, 1, mMaxVolume,
                                      mMinVolume);
  parms.mPos = pos;
  parms.mDir = CVector3f::Zero();
  parms.mSfxId = sfx;
  return CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, true);
}

void CSwarmBasics::UpdateEffects(CStateManager& mgr, CAnimData& animData) {
  int count;
  const CSoundPOINode* nodes = animData.GetSoundPOIList(count);
  if (count > 0 && nodes != nullptr) {
    for (int i = 0; i < count; ++i) {
      const CSoundPOINode& node = nodes[i];
      if (mgr.Random()->Float() <= node.GetWeight()) {
        const int character = node.GetCharacterIndex();
        if (node.GetPoiType() == kPT_Sound &&
            (character == -1 || character == animData.GetCharacterIndex())) {
          const uint soundId = node.GetSoundId();
          const int area = GetCurrentAreaId().Value();
          const ushort sfx = soundId;
          if ((soundId & 0x80000000) == 0) {
            const CBoid& boid =
                mBoids[mActiveBoidIndices[mgr.Random()->Next() % mActiveBoidIndices.size()]];
            const CVector3f pos = boid.GetTranslation();
            static float maxDistance = node.GetMaxDistance();
            static float falloff = node.GetFallOff();
            CAudioSys::C3DEmitterParmData params(maxDistance, falloff, 1, mMaxVolume, mMinVolume);
            params.mPos = pos;
            params.mDir = CVector3f::Zero();
            params.mSfxId = sfx;
            CSfxManager::AddEmitter(params, area, true, false);
          }
        }
      }
    }
  }
}

CAreaCollisionCache CSwarmBasics::MakeAreaCollisionCache(int x, int y, int z) const {
  return CAreaCollisionCache(BoxForPosition(x, y, z, mBoidRadius + 0.5f));
}

void CSwarmBasics::BoidCollidedWithPlayerCallback(CStateManager& mgr, CBoid& boid) {
  if (x4f0_29_) {
    KillBoid(boid, mgr, CWeaponMode());
  }
}

bool CSwarmBasics::ShouldBuildAreaCollisionCacheForPartition(int partitionIndex) const {
  return true;
}

void CSwarmBasics::BoidCollidedCallback(CStateManager& mgr, CBoid& boid) {}

void CSwarmBasics::QueueDeathMessage(CStateManager& mgr) {
  if (x530_ < 10) {
    SendScriptMsgs(kSS_DeathRattle, mgr);
    SendScriptMsgs(kSS_Dead, mgr);
    ++x530_;
  } else {
    ++x534_;
  }
}

void CSwarmBasics::FlushDeathMessages(CStateManager& mgr) {
  int count = x534_;
  int avail = 10 - x530_;
  if (avail < count) {
    count = avail;
  }
  for (int i = 0; i < count; ++i) {
    SendScriptMsgs(kSS_DeathRattle, mgr);
    SendScriptMsgs(kSS_Dead, mgr);
  }
  x534_ -= count;
  x530_ = 0;
}

void CSwarmBasics::FreezeBoids(const CVector3f& position, float radius) {
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->GetActive()) {
      CVector3f delta = position - it->GetTranslation();
      if (delta.MagSquared() < radius * radius) {
        it->mFreezeTimer = mFreezeDuration;
      }
    }
  }
}

void CSwarmBasics::UpdateSeekerTargets(CStateManager& mgr) {
  uint numTargets = mSeekerTargets.size();
  const rstl::reserved_vector< rstl::pair< TUniqueId, float >, 5 >& gunTargets =
      mgr.GetPlayer(0)->GetPlayerGun()->GetSeekerTargets();
  rstl::vector< TUniqueId > lostTargets(numTargets);
  rstl::vector< int > keptBoids(numTargets);
  for (uint i = 0; i < numTargets; ++i) {
    bool found = false;
    TUniqueId uid = mSeekerTargets[i];
    for (uint j = 0; j < gunTargets.size(); ++j) {
      if (gunTargets[j].first == uid) {
        found = true;
        keptBoids.push_back_unsafe(mSeekerBoidIndices[i]);
        break;
      }
    }
    if (!found) {
      lostTargets.push_back_unsafe(uid);
    }
  }
  int numLost = lostTargets.size();
  rstl::vector< int > newBoids(numLost);
  AssignSeekerBoids(mgr, keptBoids, numLost, newBoids);
  for (uint i = 0; i < newBoids.size(); ++i) {
    for (uint j = 0; j < numTargets; ++j) {
      if (lostTargets[i] == mSeekerTargets[j]) {
        mSeekerBoidIndices[j] = newBoids[i];
        break;
      }
    }
  }
  for (uint i = newBoids.size(); i < numLost; ++i) {
    for (uint j = 0; j < numTargets; ++j) {
      if (lostTargets[i] == mSeekerTargets[j]) {
        if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(mSeekerTargets[j]))) {
          act->SetActive(false);
        }
        mSeekerBoidIndices[j] = -1;
        break;
      }
    }
  }
  for (uint i = 0; i < numTargets; ++i) {
    if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(mSeekerTargets[i]))) {
      int idx = mSeekerBoidIndices[i];
      if (idx != -1) {
        act->SetActive(true);
        act->SetTransform(mBoids[idx].GetTransform());
      } else {
        act->SetActive(false);
      }
    }
  }
}

TUniqueId CSwarmBasics::GetSeekerTargetLockedOn() const {
  int lockOn = mLockOnIndex;
  if (lockOn == -1) {
    return kInvalidUniqueId;
  }
  for (uint i = 0; i < mSeekerTargets.size(); ++i) {
    if (lockOn == mSeekerBoidIndices[i]) {
      return mSeekerTargets[i];
    }
  }
  return kInvalidUniqueId;
}

const CHealthInfo* CSwarmBasics::GetHealthInfo() const { return &mHealthInfo; }

CHealthInfo* CSwarmBasics::HealthInfo() { return &mHealthInfo; }

static void* NullSwarmBasicsFactory() { return nullptr; }

static void SetFuncPtrs() {
  static SSwarmBasics_FuncPtrs funcPtrs;
  funcPtrs.mFactory = &NullSwarmBasicsFactory;
  SetSSwarmBasics_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSSwarmBasics_FuncPtrs(nullptr); }

CVector3f CSwarmBasics::GetLockOnLocation(int index) const {
  return mBoids[index].GetTranslation();
}

bool CSwarmBasics::GetLockOnLocationValid(int index) const {
  return index > -1 && index < mBoids.size() && mBoids[index].GetActive();
}
