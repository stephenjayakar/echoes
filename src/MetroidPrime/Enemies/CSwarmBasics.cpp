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
