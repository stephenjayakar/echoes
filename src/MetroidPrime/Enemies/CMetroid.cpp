#include "MetroidPrime/Enemies/CMetroid.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSpacePirate.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWater.hpp"
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

void CMetroid::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  const TUniqueId sender = msg.GetSenderId();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_Create:
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    break;
  case kSM_Delete:
  case kSM_Deactivate:
    SwarmRemove(mgr);
    DetachFromTarget(mgr, false);
    break;
  case kSM_Damage:
  case kSM_ResistedDamage:
    ApplyDamageGrowth(mgr, sender);
    mShotAt = true;
    mAlert = true;
    break;
  case kSM_Alert:
    mAlert = true;
    break;
  case kSM_AreaLoaded:
    if (mTeamAiManagerId == kInvalidUniqueId) {
      mTeamAiManagerId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
    }
    {
      const TAreaId areaId = GetCurrentAreaId();
      mPathFindSearch.SetArea(
          mgr.GetWorld()->GetAreaAlways(areaId).GetPostConstructed()->mPathArea);
    }
    break;
  default:
    break;
  }
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

void CMetroid::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  const CWeaponMode& mode = info.GetDamageInfo().GetWeaponMode();
  const CDamageVulnerability* vulnerability = GetDamageVulnerability();
  const bool frozen = BodyController()->GetPercentageFrozen() > 0.f;
  if (mAttackState == 2) {
    if (vulnerability->WeaponHurts(mode)) {
      const float maxDrain = mMetroidData.mMaxEnergyDrainAllowed;
      mEnergyDrained = maxDrain * GetDamageMultiplier();
    }
  } else if (vulnerability->WeaponHits(mode, 0)) {
    const float variation = mAttackTimeVariation;
    mAttackChance = mgr.Random()->Float() * variation + GetAverageAttackTime();
    if (frozen) {
      BodyController()->UnFreeze();
    }
    CPatterned::KnockBack(mgr, info);
  } else if (!frozen && vulnerability->WeaponHurts(mode) &&
             (mode.IsCharged() || mode.IsComboed() || mode.GetType() == kWT_Missile)) {
    CPatterned::KnockBack(mgr, info);
    mSeekTime = mMaxSeekTime;
  }
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

bool CMetroid::IsSuckingEnergy() const {
  return mAttackState == 2 && !GetBodyController()->IsFrozen();
}

bool CMetroid::IsPirateValidTarget(const CSpacePirate& pirate) const {
  if (pirate.GetAttachedActor() == kInvalidUniqueId) {
    const CHealthInfo* healthInfo = pirate.GetHealthInfo();
    return healthInfo != nullptr && healthInfo->GetHP() > 0.f;
  }
  return false;
}

bool CMetroid::IsPlayerInFluid(const CPlayer& player, const CStateManager& mgr) const {
  if (player.GetFluidCount() != 0) {
    if (player.InFluidId() != kInvalidUniqueId) {
      const CVector3f aimPos = player.GetAimPosition(mgr, 0.f);
      if (const CScriptWater* water =
              TCastToConstPtr< CScriptWater >(mgr.GetObjectById(player.InFluidId()))) {
        return aimPos.GetZ() < water->GetTriggerBoundsWR().GetMaxPoint().GetZ();
      }
    }
    return true;
  }
  return false;
}

bool CMetroid::IsTargetGettingSucked(const CStateManager& mgr) const {
  if (const CEntity* target = mgr.GetObjectById(GetAttackTargetId())) {
    if (const CPlayer* player = TCastToConstPtr< CPlayer >(target)) {
      const TUniqueId attached = player->GetAttachedActorId();
      if (attached != kInvalidUniqueId && attached != GetUniqueId()) {
        return true;
      }
    } else if (const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(target)) {
      const TUniqueId attached = pirate->GetAttachedActor();
      if (attached != kInvalidUniqueId && attached != GetUniqueId()) {
        return true;
      }
    }
  }
  return false;
}

void CMetroid::UpdateAILogicTimers(float dt, CStateManager& mgr) {
  if (IsTargetGettingSucked(mgr)) {
    const float variation = mAttackTimeVariation;
    mAttackChance = mgr.Random()->Float() * variation + GetAverageAttackTime();
  } else if (mAttackChance > 0.f) {
    mAttackChance -= dt;
  }
}

bool CMetroid::CanStartAttack(CStateManager& mgr) const {
  if (mAttackChance <= 0.f) {
    const CEntity* target = mgr.GetObjectById(mAttackTarget);
    if (const CPlayer* player = TCastToConstPtr< CPlayer >(target)) {
      if (IsPlayerInFluid(*player, mgr) ||
          mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*player, mgr) ||
          player->GetMorphBall()->InScrewAttackMode() ||
          ((player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
                ? player->GetMorphballTransitionState()
                : CPlayer::kMS_Unmorphed) == CPlayer::kMS_Morphed &&
           player->GetMorphBall()->GetBallState() == CMorphBall::kBS_Spider)) {
        return false;
      }
    }
    if (target != nullptr && target->GetCurrentAreaId() == GetCurrentAreaId()) {
      return !IsTargetGettingSucked(mgr);
    }
  }
  return false;
}

void CMetroid::SwarmRemove(CStateManager& mgr) {
  if (mTeamAiManagerId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiManagerId))) {
      if (team->IsPartOfTeam(GetUniqueId())) {
        team->QuitTeam(GetUniqueId());
      }
    }
  }
}

void CMetroid::SwarmAdd(CStateManager& mgr) {
  if (mTeamAiManagerId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiManagerId))) {
      if (!team->IsPartOfTeam(GetUniqueId())) {
        team->JoinTeam(*this, CTeamAiRole::kTAR_Melee, CTeamAiRole::kTAR_Invalid,
                       CTeamAiRole::kTAR_Invalid);
      }
    }
  }
}

bool CMetroid::Leash(CStateManager& mgr, const CTriggerData&) const {
  if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget))) {
    if (IsPlayerInFluid(*player, mgr)) {
      return true;
    }
  }
  const CVector3f leashDelta = mLatestLeashPosition - GetTranslation();
  if (leashDelta.MagSquared() > mLeashRadius * mLeashRadius) {
    if (mAttackTarget != kInvalidUniqueId) {
      if (const CEntity* target = mgr.GetObjectById(mAttackTarget)) {
        const CActor* actor = static_cast< const CActor* >(target);
        const CVector3f targetDelta = actor->GetTranslation() - GetTranslation();
        return targetDelta.MagSquared() > mPlayerLeashRadius * mPlayerLeashRadius &&
               mCurPlayerLeashTime > mPlayerLeashTime;
      }
    }
    return true;
  }
  return false;
}

bool CMetroid::LostInterest(CStateManager& mgr, const CTriggerData&) const {
  if (mAttackTarget != kInvalidUniqueId) {
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
      if (const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(actor)) {
        if (pirate->GetAttachedActor() != kInvalidUniqueId) {
          return true;
        }
      } else if (const CPlayer* player =
                     TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget))) {
        if (IsPlayerInFluid(*player, mgr) || player->GetCurrentAreaId() != GetCurrentAreaId() ||
            mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*player, mgr)) {
          return true;
        }
      }
      return false;
    }
  }
  return true;
}

bool CMetroid::PatternShagged(CStateManager& mgr, const CTriggerData&) const {
  if (mAttackTarget != kInvalidUniqueId) {
    if (const CSpacePirate* pirate =
            TCastToConstPtr< CSpacePirate >(mgr.GetObjectById(mAttackTarget))) {
      if (!pirate->GetAlive()) {
        return true;
      }
    }
    if (!CanStartAttack(mgr)) {
      return true;
    }
    if (mState == kAiState_Two) {
      return mSeekTime >= mMaxSeekTime;
    }
    return false;
  }
  return true;
}

bool CMetroid::InPosition(CStateManager& mgr, const CTriggerData&) const {
  if (mPathFindSearch.GetCurrentWaypoint() < mPathFindSearch.GetWaypoints().size() - 1) {
    const CVector3f delta = x7c0_ - GetTranslation();
    return delta.MagSquared() < 4.f;
  }
  return true;
}

bool CMetroid::InRange(CStateManager& mgr, const CTriggerData&) const {
  if (mAttackTarget != kInvalidUniqueId) {
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
      if (const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(actor)) {
        if (!IsPirateValidTarget(*pirate)) {
          return false;
        }
      }
      return (actor->GetTranslation() - GetTranslation()).MagSquared() <
             mMaxAttackRange * mMaxAttackRange;
    }
  }
  return false;
}

bool CMetroid::AggressionCheck(CStateManager& mgr, const CTriggerData&) const {
  if (mAttackTarget != kInvalidUniqueId) {
    const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget));
    if (const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(actor)) {
      if (!IsPirateValidTarget(*pirate)) {
        return false;
      }
    }
    if (actor != nullptr) {
      const CVector3f delta = actor->GetTranslation() - GetTranslation();
      if (delta.MagSquared() < mDetectionRange * mDetectionRange) {
        if (mDetectionHeightRange > 0.f) {
          return delta.GetZ() * delta.GetZ() < mDetectionHeightRange * mDetectionHeightRange;
        }
        return true;
      }
    }
  }
  return false;
}

bool CMetroid::ShouldTurn(CStateManager& mgr, const CTriggerData&) const {
  if (mAttackTarget != kInvalidUniqueId) {
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
      const CVector2f direction = (actor->GetTranslation() - GetTranslation()).ToVec2f();
      const CVector2f forward = GetTransform().GetForward().ToVec2f();
      return CVector2f::GetAngleDiff(forward, direction) > CRelAngle::FromDegrees(15.f).AsRadians();
    }
  }
  return false;
}

void CMetroid::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAttackTarget = kInvalidUniqueId;
    mShotAt = false;
    break;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

void CMetroid::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  switch (msg) {
  case kStateMsg_Update:
    ApplySeparationBehavior(mgr);
    break;
  }
}

void CMetroid::TurnAround(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Update:
    if (mAttackTarget != kInvalidUniqueId) {
      if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
        UpdateAttackTarget(mgr);
        const CVector3f direction = actor->GetTranslation() - GetTranslation();
        if (ShouldTurn(mgr, CTriggerData(0.f)) && direction.CanBeNormalized()) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(CVector3f::Zero(), direction.AsNormalized(), 1.f));
        }
      }
    }
    break;
  }
}

void CMetroid::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mState = kAiState_Zero;
    RemoveMaterial(kMT_Unknown59, mgr);
    mRestoreSolidCollision = false;
    break;
  case kStateMsg_Update:
    switch (mState) {
    case kAiState_Zero:
      if (mAlert) {
        mState = kAiState_One;
        mRestoreSolidCollision = true;
        mDetachPos = CVector3f::Zero();
      }
      break;
    case kAiState_One:
      if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
        mState = kAiState_Two;
      } else {
        BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
      }
      break;
    case kAiState_Two:
      if (BodyController()->GetCurrentStateId() != pas::kAS_Generate) {
        mState = kAiState_Over;
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    xa40_27_ = true;
    break;
  }
}

void CMetroid::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mDodgeDirection != pas::kSD_Invalid) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mDodgeDirection, pas::kStep_Dodge));
      mState = kAiState_Two;
    }
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Step) {
      mState = kAiState_Over;
    } else if (mAttackTarget != kInvalidUniqueId) {
      if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
        BodyController()->CommandMgr().SetTargetVector(actor->GetTranslation() - GetTranslation());
      }
    }
    break;
  case kStateMsg_Deactivate:
    mDodgeDirection = pas::kSD_Invalid;
    break;
  }
}

float CMetroid::GetGrowthStage() const {
  if (mGrowthEnergy < mMetroidData.mStage2GrowthEnergy) {
    return 1.f + mGrowthEnergy / mMetroidData.mStage2GrowthEnergy;
  }
  if (mGrowthEnergy < mMetroidData.mExplosionGrowthEnergy) {
    return 2.f + (mGrowthEnergy - mMetroidData.mStage2GrowthEnergy) /
                     (mMetroidData.mExplosionGrowthEnergy - mMetroidData.mStage2GrowthEnergy);
  }
  return 3.f;
}

float CMetroid::GetDamageMultiplier() const {
  float result = 0.5f * (GetGrowthStage() - 1.f) + 1.f;
  return result;
}

bool CMetroid::AttachToTarget(CStateManager& mgr) {
  if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mAttackTarget))) {
    if (player->AttachActorToPlayer(GetUniqueId(), false)) {
      player->GetEnergyDrain().AddEnergyDrainSource(GetUniqueId(), 1.f);
      return true;
    }
  } else {
    return PreDamageSpacePirate(mgr);
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

void CMetroid::OnDockTouch(CStateManager& mgr) {
  DetachFromTarget(mgr, true);
  mPendingDeath = true;
}

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
