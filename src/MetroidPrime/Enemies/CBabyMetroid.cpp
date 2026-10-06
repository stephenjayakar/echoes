#include "MetroidPrime/Enemies/CBabyMetroid.hpp"

#include "MetroidPrime/CStateManager.hpp"

void CBabyMetroid::Render(const CStateManager& mgr) const { CMetroid::Render(mgr); }

bool CBabyMetroid::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CBabyMetroid::ShouldSeekEnergySource(CStateManager&, const CTriggerData&) const {
  return mShouldSeekEnergySource;
}

bool CBabyMetroid::AbsorbFinished(CStateManager&, const CTriggerData&) const {
  return xa50_ >= xa4c_;
}

bool CBabyMetroid::InEnergySourcePosition(CStateManager&, const CTriggerData&) const {
  const CVector3f delta = x7c0_ - GetTranslation();
  return delta.MagSquared() < 4.f;
}

void CBabyMetroid::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kAiState_Over;
    break;
  default:
    break;
  }
}
