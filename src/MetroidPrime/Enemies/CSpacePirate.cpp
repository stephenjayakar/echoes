#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTrigger.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "rstl/math.hpp"
#include "rstl/string.hpp"

#include <float.h>

const SBurst CSpacePirate::skBurstsQuick[] = {
    {20, {3, 4, 5, -1, 0, 0, 0, 0}, 0.1f, 0.05f}, {20, {2, 3, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {6, 5, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f}, {20, {1, 2, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {7, 6, 5, -1, 0, 0, 0, 0}, 0.1f, 0.05f}, {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsStandard[] = {
    {15, {5, 3, 2, 1, -1, 0, 0, 0}, 0.1f, 0.05f}, {20, {1, 2, 3, 4, -1, 0, 0, 0}, 0.1f, 0.05f},
    {20, {7, 6, 5, 4, -1, 0, 0, 0}, 0.1f, 0.05f}, {15, {3, 4, 5, 6, -1, 0, 0, 0}, 0.1f, 0.05f},
    {15, {6, 5, 4, 3, -1, 0, 0, 0}, 0.1f, 0.05f}, {15, {2, 3, 4, 5, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsFrenzied[] = {
    {40, {1, 2, 3, 4, 5, 6, -1, 0}, 0.1f, 0.05f}, {40, {7, 6, 5, 4, 3, 2, -1, 0}, 0.1f, 0.05f},
    {10, {2, 3, 4, 5, 4, 3, -1, 0}, 0.1f, 0.05f}, {10, {6, 5, 4, 3, 4, 5, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsJumping[] = {
    {20, {16, 4, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {5, 7, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {1, 10, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsInjured[] = {
    {15, {16, 1, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f}, {20, {3, 4, 6, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {25, {7, 5, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},  {25, {2, 6, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {7, 5, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f},  {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsSeated[] = {
    {35, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {35, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsQuickOOV[] = {
    {10, {16, 15, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {13, 12, 10, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {9, 11, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {14, 10, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {10, {9, 11, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsStandardOOV[] = {
    {26, {16, 8, 11, 14, -1, 0, 0, 0}, 0.1f, 0.05f},
    {26, {16, 13, 11, 12, -1, 0, 0, 0}, 0.1f, 0.05f},
    {16, {9, 11, 13, 10, -1, 0, 0, 0}, 0.1f, 0.05f},
    {16, {14, 13, 12, 11, -1, 0, 0, 0}, 0.1f, 0.05f},
    {8, {10, 11, 12, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {8, {6, 8, 11, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsFrenziedOOV[] = {
    {40, {1, 16, 14, 12, 10, 11, -1, 0}, 0.1f, 0.05f},
    {40, {9, 11, 12, 13, 11, 7, -1, 0}, 0.1f, 0.05f},
    {10, {8, 10, 11, 12, 13, 12, -1, 0}, 0.1f, 0.05f},
    {10, {15, 13, 12, 10, 12, 9, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsJumpingOOV[] = {
    {40, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsInjuredOOV[] = {
    {30, {9, 11, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {10, {13, 12, 10, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {9, 11, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {14, 10, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 15, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst CSpacePirate::skBurstsSeatedOOV[] = {
    {35, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {35, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const SBurst* CSpacePirate::skBursts[] = {skBurstsQuick,
                                          skBurstsStandard,
                                          skBurstsFrenzied,
                                          skBurstsJumping,
                                          skBurstsInjured,
                                          skBurstsSeated,
                                          skBurstsQuickOOV,
                                          skBurstsStandardOOV,
                                          skBurstsFrenziedOOV,
                                          skBurstsJumpingOOV,
                                          skBurstsInjuredOOV,
                                          skBurstsSeatedOOV,
                                          nullptr};

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"Stuck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::Stuck)},
    {"PatternShagged",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::PatternShagged)},
    {"HearShot", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::HearShot)},
    {"HearPlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::HearPlayer)},
    {"AggressionCheck",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::AggressionCheck)},
    {"CoverCheck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::CoverCheck)},
    {"CoverFind", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::CoverFind)},
    {"CoverBlown", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::CoverBlown)},
    {"CoverNearlyBlown",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::CoverNearlyBlown)},
    {"CoveringFire",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::CoveringFire)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldAttack)},
    {"LineOfSight", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::LineOfSight)},
    {"PatternOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::PatternOver)},
    {"SpotPlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::SpotPlayer)},
    {"ShouldDodge", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldDodge)},
    {"ShouldRetreat",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldRetreat)},
    {"InRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::InRange)},
    {"ShouldCrouch",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldCrouch)},
    {"ShouldMove", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldMove)},
    {"ShotAt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShotAt)},
    {"Attacked", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::Attacked)},
    {"HasTargetingPoint",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::HasTargetingPoint)},
    {"ShouldWallHang",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldWallHang)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::AnimOver)},
    {"ShouldStrafe",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldStrafe)},
    {"ShouldSpecialAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldSpecialAttack)},
    {"StartAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::StartAttack)},
    {"BreakAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::BreakAttack)},
    {"LostInterest",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::LostInterest)},
    {"BounceFind", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::BounceFind)},
    {"OffLine", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::OffLine)},
    {"Landed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::Landed)},
    {"ShouldJumpBack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldJumpBack)},
    {"Leash", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::Leash)},
    {"HasAttackPattern",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::HasAttackPattern)},
    {"IsAmbushing", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::IsAmbushing)},
    {"ShouldWarpIn",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldWarpIn)},
    {"ShouldLaunchGrenade",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::ShouldLaunchGrenade)},
    {"InProjectileRange",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::InProjectileRange)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Ambushing", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Ambushing)},
    {"WarpIn", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::WarpIn)},
    {"WarpOut", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::WarpOut)},
    {"PostWarpOut", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::PostWarpOut)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Attack)},
    {"Crouch", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Crouch)},
    {"CoverAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::CoverAttack)},
    {"Halt", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Halt)},
    {"Run", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Run)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::PathFind)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Patrol)},
    {"TargetPatrol",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::TargetPatrol)},
    {"Shuffle", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Shuffle)},
    {"TurnAround", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::TurnAround)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Dodge)},
    {"Lurk", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Lurk)},
    {"Taunt", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Taunt)},
    {"Cover", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Cover)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Dead)},
    {"TargetCover", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::TargetCover)},
    {"TargetPlayer",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::TargetPlayer)},
    {"Approach", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Approach)},
    {"WallHang", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::WallHang)},
    {"WallDetach", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::WallDetach)},
    {"GetUp", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::GetUp)},
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Generate)},
    {"Skid", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Skid)},
    {"DoubleSnap", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::DoubleSnap)},
    {"JumpBack", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::JumpBack)},
    {"Bounce", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Bounce)},
    {"PathFindEx", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::PathFindEx)},
    {"Enraged", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Enraged)},
    {"Jump", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Jump)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Deactivate)},
    {"LaunchGrenade",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::LaunchGrenade)},
    {"Captured", static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::Captured)},
    {"RemoveFromWorld",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::RemoveFromWorld)},
};

void CSpacePirate::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

bool CSpacePirate::Listen(CStateManager& mgr, const CVector3f& pos, EListenNoiseType type) {
  bool heard = false;
  if (GetAlive()) {
    CVector3f delta = pos - GetTranslation();
    if (delta.MagSquared() < mPirateData.mHearingRadius * mPirateData.mHearingRadius &&
        (mDetectionHeightRange == 0.f ||
         delta.GetZ() * delta.GetZ() < mDetectionHeightRange * mDetectionHeightRange)) {
      heard = true;
      mHearNoise = true;
    }
    if (type == kLNT_PlayerFire) {
      mHearPlayerFire = true;
      xbb4_ = pos;
    }
  }
  const bool result = heard;
  return result;
}

void CSpacePirate::SetAttackTarget(CStateManager& mgr, TUniqueId id) {
  mTargetId = id;
  SetTeamAiTarget(mgr);
  mBurstFire.SetBurstType(1);
  mAttackRemTime = 0.f;
}

CVector3f CSpacePirate::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                  const CVector3f& aimPos) const {
  return GetTranslation();
}

bool CSpacePirate::PatternShagged(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Stuck(mgr, data);
}

bool CSpacePirate::HearShot(CStateManager& mgr, const CTriggerData& data) const {
  const bool heard = mHearNoise;
  mHearNoise = false;
  return heard;
}

bool CSpacePirate::PatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mDestObj == kInvalidUniqueId;
}

void CSpacePirate::Halt(CStateManager& mgr, EStateMsg msg, float dt) { mSteeringSpeed = 0.f; }

void CSpacePirate::Run(CStateManager& mgr, EStateMsg msg, float dt) { mSteeringSpeed = 1.f; }

bool CSpacePirate::CoverCheck(CStateManager& mgr, const CTriggerData& data) const {
  return mCoverCheck;
}

bool CSpacePirate::StartAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (mMayStartAttack) {
    mMayStartAttack = false;
    return true;
  }
  return false;
}

bool CSpacePirate::BreakAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mBreakAttack;
}

bool CSpacePirate::LostInterest(CStateManager& mgr, const CTriggerData& data) const {
  if (mOnlyAttackInRange && mAttackRemTime < 1.5f) {
    return true;
  }
  return false;
}

bool CSpacePirate::InRange(CStateManager& mgr, const CTriggerData& data) const { return mInRange; }

bool CSpacePirate::LineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return !mNoPlayerLos;
}

bool CSpacePirate::ShotAt(CStateManager& mgr, const CTriggerData& data) const {
  return mLowHealthFrenzyTimer < (data.GetFloat() ? data.GetFloat() : 0.5f);
}

bool CSpacePirate::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  return mTimeSinceHitByPlayer < (data.GetFloat() ? data.GetFloat() : 0.5f);
}

bool CSpacePirate::IsAmbushing(CStateManager& mgr, const CTriggerData& data) const {
  return mPendingAmbush;
}

void CSpacePirate::Deactivate(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mPendingDeath = true;
  }
}

bool CSpacePirate::OffLine(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (GetBodyController()->GetCurrentStateId() != pas::kAS_Jump && !IsOnGround()) {
    ret = true;
  }
  return ret;
}

bool CSpacePirate::Landed(CStateManager& mgr, const CTriggerData& data) const {
  return IsOnGround();
}

bool CSpacePirate::Leash(CStateManager& mgr, const CTriggerData& data) const {
  return mLeashTimer > data.GetFloat();
}

CProjectileInfo* CSpacePirate::ProjectileInfo() {
  if (mProjectileInfo) {
    return &*mProjectileInfo;
  }
  return nullptr;
}

void CSpacePirate::Touch(CActor& actor, CStateManager& mgr) {
  CPatterned::Touch(actor, mgr);
  if (mRagDoll.get() && mRagDoll->IsPrimed()) {
    if (CScriptTrigger* trigger = TCastToPtr< CScriptTrigger >(actor)) {
      if (trigger->GetActive() && (trigger->GetTriggerFlags() & kTFL_DetectAI) &&
          trigger->GetForceMagnitude() > 0.f) {
        CVector3f& impulse = mRagDoll->TorsoImpulse();
        impulse += trigger->GetForceField();
      }
    }
  }
}

bool CSpacePirate::CheckTargetable(CStateManager& mgr) { return GetModelAlphau8(mgr) > 127; }

void CSpacePirate::SetVelocityForJump() {
  if (!mJumpVelSet && !x8fa_31_) {
    CVector3f velocity = CVector3f::Zero();
    const CVector3f& position = GetTranslation();
    CVector3f delta = mPatrolDestPos - position;
    float gravity = GetGravityConstant();
    float jumpZ = mJumpHeight + rstl::max_val(mPatrolDestPos.GetZ(), position.GetZ());
    velocity.SetZ(CMath::SqrtF(2.f * gravity * (jumpZ - position.GetZ())));
    float riseTime = velocity.GetZ() / gravity;
    riseTime += CMath::SqrtF(2.f * (jumpZ - mPatrolDestPos.GetZ()) / gravity);
    float invTime = 1.f / riseTime;
    velocity.SetX(invTime * delta.GetX());
    velocity.SetY(invTime * delta.GetY());
    SetVelocityWR(velocity);
    mJumpVelSet = true;
    x8fa_29_ = true;
  }
}

void CSpacePirate::SquadAdd(CStateManager& mgr) {
  if (mTeamAiMgrId == kInvalidUniqueId) {
    mTeamAiMgrId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      teamMgr->JoinTeam(*this, mMelee ? CTeamAiRole::kTAR_Melee : CTeamAiRole::kTAR_Projectile,
                        CTeamAiRole::kTAR_Unknown, CTeamAiRole::kTAR_Invalid);
    }
  }
}

void CSpacePirate::SquadRemove(CStateManager& mgr) {
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      if (teamMgr->IsPartOfTeam(GetUniqueId())) {
        teamMgr->QuitTeam(GetUniqueId());
        mTeamAiMgrId = kInvalidUniqueId;
      }
    }
  }
}

void CSpacePirate::SquadReset(CStateManager& mgr) {
  CScriptTeamAiMgr::EndAttack(mMelee ? CScriptTeamAiMgr::kAT_Melee
                                     : CScriptTeamAiMgr::kAT_Projectile,
                              mgr, mTeamAiMgrId, GetUniqueId(), true);
}

void CSpacePirate::SetTeamAiTarget(CStateManager& mgr) {
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      teamMgr->SetMemberTargetId(GetUniqueId(), mTargetId);
    }
  }
}

void CSpacePirate::CheckForProjectiles(CStateManager& mgr) {
  if (mHearPlayerFire) {
    CVector3f extent(5.f, 5.f, 5.f);
    CAABox bounds(xbb4_ - extent, xbb4_ + extent);
    mInProjectilePath = false;
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    mgr.BuildNearList(nearList, bounds, CMaterialFilter::MakeInclude(CMaterialList(kMT_Projectile)),
                      nullptr);
    for (int i = 0; i < nearList.size(); ++i) {
      if (const CGameProjectile* projectile =
              TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(nearList[i]))) {
        CVector3f delta = GetBoundingBox().GetCenterPoint() - projectile->GetTranslation();
        if (delta.IsMagnitudeSafe()) {
          if (CVector3f::Dot(GetTransform().GetForward(), delta) < 0.f) {
            delta.Normalize();
            CVector3f projDelta = projectile->GetTranslation() - projectile->GetPreviousPos();
            if (projDelta.IsMagnitudeSafe()) {
              projDelta.Normalize();
              if (CVector3f::Dot(projDelta, delta) > 0.939f) {
                mInProjectilePath = true;
              }
            }
          }
        } else {
          mInProjectilePath = true;
        }
        if (mInProjectilePath) {
          break;
        }
      }
    }
    mHearPlayerFire = false;
  }
}

bool CSpacePirate::LineOfSightTest(CStateManager& mgr, const CVector3f& eyePos,
                                   const CVector3f& targetPos, const CMaterialList& excludeList) {
  CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), excludeList);
  return mgr.RayCollideWorld(eyePos, targetPos, filter, this);
}

void CSpacePirate::UpdateHeldPosition(CStateManager& mgr, float dt) {
  if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId))) {
    CVector2f pos = player->GetTranslation().ToVec2f();
    if ((pos - mHeldPosition).MagSquared() < 3.f) {
      mHoldPositionTime += dt;
    } else {
      mHeldPosition = pos;
      mHoldPositionTime = 0.f;
    }
  } else {
    mHoldPositionTime = 0.f;
  }
}

void CSpacePirate::SetCinematicCollision(CStateManager& mgr) {
  RemoveMaterial(kMT_AIBlock, mgr);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_AIBlock);
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(include, GetMaterialFilter().GetExcludeList()));
}

void CSpacePirate::SetNonCinematicCollision(CStateManager& mgr) {
  AddMaterial(kMT_AIBlock, mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(GetMaterialFilter().GetIncludeList()).Union(CMaterialList(kMT_AIBlock)),
      GetMaterialFilter().GetExcludeList()));
}

void CSpacePirate::Death(CStateManager& mgr, const CVector3f& dir, EScriptObjectState state) {
  if (GetAlive()) {
    CPatterned::Death(mgr, dir, state);
    if (mAttachedActor != kInvalidUniqueId) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockDownCmd(GetTransform().GetForward(), pas::kS_Two));
    }
  }
}

bool CSpacePirate::Stuck(CStateManager& mgr, const CTriggerData& data) const {
  if (mStateMachine->GetTime() > 0.5f) {
    return CPatterned::Stuck(mgr, data) || CPatterned::PathShagged(mgr, data);
  }
  return false;
}

bool CSpacePirate::AttachActor(TUniqueId id) {
  if (mAttachedActor == kInvalidUniqueId) {
    if (mRagDoll.get()) {
      mRagDoll->SetNoAiCollision(true);
    }
    mAttachedActor = id;
    return true;
  }
  return false;
}

void CSpacePirate::DetachActor() {
  mAttachedActor = kInvalidUniqueId;
  if (mRagDoll.get()) {
    mRagDoll->SetNoAiCollision(false);
  }
}

CRagDoll* CSpacePirate::GetRagDoll() const {
  if (mRagDoll.get() && mRagDoll->IsPrimed() && mRagDoll->IsRenderBoundsValid()) {
    return mRagDoll.get();
  }
  return nullptr;
}

rstl::optional_object< CAABox > CSpacePirate::GetTouchBounds() const {
  if (mRagDoll.get() && mRagDoll->IsPrimed() && mRagDoll->IsRenderBoundsValid()) {
    return mRagDoll->GetCachedRenderBounds();
  }
  return CPatterned::GetTouchBounds();
}

bool CSpacePirate::TryToBeCaptured(CStateManager& mgr) {
  mStateMachine->SetState(mgr, *this, rstl::string_l("Captured"));
  return true;
}

void CSpacePirate::PreThink(float dt, CStateManager& mgr) {
  if (!mRagDoll.get() || !mRagDoll->IsPrimed()) {
    mBoneTracking.PreThink(*AnimationData());
  }
  CPatterned::PreThink(dt, mgr);
}

uchar CSpacePirate::GetModelAlphau8(const CStateManager& mgr) const {
  uchar alpha;
  if (mShadowPirate) {
    alpha = static_cast< uchar >(255.f * mShadowPirateAlpha);
  } else {
    alpha = mColor.GetAlphau8();
  }
  const uchar result = alpha;
  return result;
}

bool CSpacePirate::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}

bool CSpacePirate::ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (xc04_ == -1 && mTrooper && x8fa_26_) {
    ret = true;
  }
  return ret;
}

bool CSpacePirate::ShouldLaunchGrenade(CStateManager& mgr, const CTriggerData& data) const {
  if (mPirateData.mWeaponData.mEquippedWeapon == 1) {
    return mAttackRemTime <= 0.f;
  }
  return false;
}

bool CSpacePirate::InProjectileRange(CStateManager& mgr, const CTriggerData& data) const {
  switch (mPirateData.mWeaponData.mEquippedWeapon) {
  case 1:
    if (const CEntity* ent = mgr.GetObjectById(mTargetId)) {
      const CActor* target = static_cast< const CActor* >(ent);
      CVector3f delta = target->GetTranslation() - GetTranslation();
      float distSq = delta.MagSquared();
      float minSq = mPirateData.mWeaponData.x64_ * mPirateData.mWeaponData.x64_;
      float maxSq = mPirateData.mWeaponData.x68_ * mPirateData.mWeaponData.x68_;
      if (distSq >= minSq && distSq <= maxSq) {
        return true;
      }
    }
    break;
  case 0:
    return true;
  }
  return false;
}

bool CSpacePirate::ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const {
  CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint);
  return cp && cp->ShouldWallHang();
}

void CSpacePirate::Ambushing(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    if (mCeilingAmbush) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    }
  }
}

void CSpacePirate::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mDestObj = mgr.GetPlayer(0)->GetUniqueId();
    SetDestPos(mgr.GetPlayer(0)->GetTranslation());
    mReflectedDestPos = GetTranslation();
    mInPosition = false;
    break;
  }
}

void CSpacePirate::TargetCover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      mDestObj = mCoverPoint;
      mDestPos = cp->GetTranslation();
    }
    mReflectedDestPos = GetTranslation();
    mInPosition = false;
    break;
  }
}

bool CSpacePirate::ShouldMove(CStateManager& mgr, const CTriggerData& data) const {
  CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint);
  return cp && !cp->ShouldStay();
}

void CSpacePirate::Approach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    mSteeringSpeed = 1.f;
    break;
  case kStateMsg_Update:
    AvoidActors(mgr);
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

bool CSpacePirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
    result = cp->ShouldCrouch();
  }
  return result;
}

void CSpacePirate::Enraged(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    break;
  }
}

void CSpacePirate::CoverAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_LeanFromCover));
    mInAttackState = true;
    break;
  case kStateMsg_Update:
    UpdateCantSeePlayer(mgr);
    break;
  case kStateMsg_Deactivate:
    mInAttackState = false;
    break;
  }
}

bool CSpacePirate::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  if (mInWallHang) {
    return GetBodyController()->GetCurrentStateId() != pas::kAS_WallHang;
  }
  return CPatterned::AnimOver(mgr, data);
}

bool CSpacePirate::ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const {
  return !mNoShuffleCloseCheck || mHoldPositionTime > 6.f;
}

bool CSpacePirate::ShouldSpecialAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (mOnlyAttackInRange && !mBurstFire.IsBurstSet() && mAttackRemTime > 2.f) {
    return true;
  }
  return false;
}

void CSpacePirate::RemoveFromWorld(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    SetActive(false);
    mgr.DeleteObjectRequest(GetUniqueId());
    break;
  }
}

const CDamageVulnerability* CSpacePirate::GetDamageVulnerability() const {
  if (xc04_ != -1) {
    return &CDamageVulnerability::PassThroughVulnerabilty();
  }
  return CPatterned::GetDamageVulnerability();
}

const CDamageVulnerability* CSpacePirate::GetDamageVulnerability(const CVector3f& position,
                                                                 const CVector3f& direction,
                                                                 const CDamageInfo& damage) const {
  return GetDamageVulnerability();
}

SSpacePirate_FuncPtrs REL_loader_SpacePirate;

CEntity* REL_LoadSpacePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

void SetRelLoaderFunctionToLoader() {
  REL_loader_SpacePirate.mLoadSpacePirate = REL_LoadSpacePirate;
  REL_loader_SpacePirate.mAttachActor = &CSpacePirate::AttachActor;
  REL_loader_SpacePirate.mDetachActor = &CSpacePirate::DetachActor;
  SetSSpacePirate_FuncPtrs(&REL_loader_SpacePirate);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSSpacePirate_FuncPtrs(nullptr); }
