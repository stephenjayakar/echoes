#include "MetroidPrime/Enemies/CMetroid.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

CMetroid::~CMetroid() {}

void CMetroid::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  if (CScriptTeamAiMgr::GetTeamAiRole(mgr, mTeamAiManagerId, GetUniqueId()) == nullptr) {
    SwarmAdd(mgr);
  }
  UpdateAILogicTimers(dt, mgr);
  SuckEnergyFromTarget(dt, mgr);
  PreventWorldCollisions(dt, mgr);
  RestoreSolidCollision(mgr);
  CPatterned::Think(dt, mgr);
}

void CMetroid::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

const CDamageVulnerability* CMetroid::GetDamageVulnerability() const {
  if (IsSuckingEnergy()) {
    if (mIsEnergyDrainVulnerable) {
      return &mMetroidData.mEnergyDrainVulnerability;
    }
    return &mStandingFaceHugVulnerability;
  }
  if (mGrowing && !GetBodyController()->IsFrozen()) {
    return &mMetroidData.mEnergyDrainVulnerability;
  }
  if (GetBodyController()->GetPercentageFrozen() > 0.f) {
    return &mMetroidData.mFrozenVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

const CDamageVulnerability* CMetroid::GetDamageVulnerability(const CVector3f&, const CVector3f&,
                                                             const CDamageInfo&) const {
  return GetDamageVulnerability();
}

EWeaponCollisionResponseTypes CMetroid::GetCollisionResponseType(const CVector3f&, const CVector3f&,
                                                                 const CWeaponMode& mode,
                                                                 int) const {
  EWeaponCollisionResponseTypes response = static_cast< EWeaponCollisionResponseTypes >(33);
  const bool frozen = GetBodyController()->GetPercentageFrozen() > 0.f;
  if (!GetDamageVulnerability()->WeaponHits(mode, 0) && !frozen) {
    response = static_cast< EWeaponCollisionResponseTypes >(63);
  }
  return response;
}

void CMetroid::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                               float dt) {
  bool handled = false;
  switch (type) {
  case kUE_GenerateEnd:
    AddMaterial(kMT_Unknown59, mgr);
    handled = true;
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

void CMetroid::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  CPatterned::Death(mgr, direction, state);
  mVerticalMovement = false;
  SetMuted(true);
  SwarmRemove(mgr);
}

bool CMetroid::CanBeIngPossessed(CStateManager& mgr) const {
  bool ret = false;
  if (CPatterned::CanBeIngPossessed(mgr) && !IsSuckingEnergy() && !xa40_29_) {
    ret = true;
  }
  return ret;
}

bool CMetroid::Attacked(CStateManager& mgr, const CTriggerData&) const {
  if (mGrowthEnergy - mLastGrowthEnergy > 0.f) {
    if (mLastGrowthEnergy < mMetroidData.mStage2GrowthEnergy) {
      return mGrowthEnergy >= mMetroidData.mStage2GrowthEnergy;
    }
    if (mGrowthEnergy >= mMetroidData.mExplosionGrowthEnergy) {
      return true;
    }
  }
  return false;
}

bool CMetroid::ShouldAttack(CStateManager& mgr, const CTriggerData&) const {
  if (CanStartAttack(mgr)) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiManagerId))) {
      return team->StartMeleeAttack(GetUniqueId());
    }
    return true;
  }
  return false;
}

const CCollisionPrimitive* CMetroid::GetCollisionPrimitive() const {
  return &mCollisionPrimitive;
}

bool CMetroid::StateOver(CStateManager&, const CTriggerData&) const {
  return mState == kAiState_Over;
}

bool CMetroid::ShotAt(CStateManager&, const CTriggerData&) const { return mShotAt; }

bool CMetroid::SpotPlayer(CStateManager&, const CTriggerData&) const { return false; }

bool CMetroid::ShouldWallHang(CStateManager&, const CTriggerData&) const {
  return mMetroidData.mStartsInWall;
}

bool CMetroid::ShouldDodge(CStateManager&, const CTriggerData&) const { return false; }

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::StateOver)},
    {"AttackOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::AttackOver)},
    {"LostInterest",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::LostInterest)},
    {"PatternShagged",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::PatternShagged)},
    {"Attacked", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::Attacked)},
    {"ShotAt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::ShotAt)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::ShouldAttack)},
    {"InAttackPosition",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::InAttackPosition)},
    {"InPosition", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::InPosition)},
    {"InRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::InRange)},
    {"InDetectionRange",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::InDetectionRange)},
    {"SpotPlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::SpotPlayer)},
    {"AggressionCheck",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::AggressionCheck)},
    {"ShouldTurn", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::ShouldTurn)},
    {"Leash", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::Leash)},
    {"ShouldWallHang",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::ShouldWallHang)},
    {"ShouldDodge", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMetroid::ShouldDodge)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::Patrol)},
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::Generate)},
    {"SelectTarget", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::SelectTarget)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::PathFind)},
    {"TurnAround", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::TurnAround)},
    {"TelegraphAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::TelegraphAttack)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::Attack)},
    {"WallHang", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::WallHang)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CMetroid::Dodge)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"SetTargetDest", static_cast< CPatterned::StateMachine::CodeFunc >(&CMetroid::SetTargetDest)},
    {"SetPatrolDest", static_cast< CPatterned::StateMachine::CodeFunc >(&CMetroid::SetPatrolDest)},
};

void CMetroid::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

CEntity* REL_LoadMetroid(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

SMetroid_FuncPtrs REL_loader_Metroid;

void SetRelLoaderFunctionToLoader() {
  REL_loader_Metroid.mLoadMetroid = REL_LoadMetroid;
  REL_loader_Metroid.mOnDockTouch = &CMetroid::OnDockTouch;
  SetSMetroid_FuncPtrs(&REL_loader_Metroid);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSMetroid_FuncPtrs(nullptr); }
