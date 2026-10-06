#include "MetroidPrime/Enemies/CSwarmBasics.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"

#include "REL/REL_Setup.h"

// The native record holds a single callback that always returns null; its signature is unknown.
struct SSwarmBasics_FuncPtrs {
  void* (*mFactory)();
};

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

TUniqueId CSwarmBasics::GetSeekerTargetLockedOn() const {
  int lockOn = mLockOnIndex;
  if (lockOn == -1) {
    return kInvalidUniqueId;
  }
  for (int i = 0; i < mActiveBoidIndices.size(); ++i) {
    if (lockOn == mActiveBoidIndices[i]) {
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
