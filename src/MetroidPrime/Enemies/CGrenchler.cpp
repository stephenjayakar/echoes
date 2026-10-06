#include "MetroidPrime/Enemies/CGrenchler.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGrenchler.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

const CVector3f CGrenchler::skAimOffset(0.f, 0.f, 0.5f);

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AbortCharge", static_cast< TriggerFunc >(&CGrenchler::AbortCharge)},
    {"Alerted", static_cast< TriggerFunc >(&CGrenchler::Alerted)},
    {"Attacked", static_cast< TriggerFunc >(&CGrenchler::Attacked)},
    {"AttackPatternOver", static_cast< TriggerFunc >(&CGrenchler::AttackPatternOver)},
    {"BeamBadAngle", static_cast< TriggerFunc >(&CGrenchler::BeamBadAngle)},
    {"BeamHitPlayer", static_cast< TriggerFunc >(&CGrenchler::BeamHitPlayer)},
    {"BeamHitSticky", static_cast< TriggerFunc >(&CGrenchler::BeamHitSticky)},
    {"BeamHitWall", static_cast< TriggerFunc >(&CGrenchler::BeamHitWall)},
    {"Bored", static_cast< TriggerFunc >(&CGrenchler::Bored)},
    {"BreakGrappleLoop", static_cast< TriggerFunc >(&CGrenchler::BreakGrappleLoop)},
    {"CanBeamAttack", static_cast< TriggerFunc >(&CGrenchler::CanBeamAttack)},
    {"CanBite", static_cast< TriggerFunc >(&CGrenchler::CanBite)},
    {"CanBurstAttack", static_cast< TriggerFunc >(&CGrenchler::CanBurstAttack)},
    {"CancelManeuvering", static_cast< TriggerFunc >(&CGrenchler::CancelManeuvering)},
    {"CanCharge", static_cast< TriggerFunc >(&CGrenchler::CanCharge)},
    {"CanGrapple", static_cast< TriggerFunc >(&CGrenchler::CanGrapple)},
    {"ChargeFinished", static_cast< TriggerFunc >(&CGrenchler::ChargeFinished)},
    {"ClearLineOfFire", static_cast< TriggerFunc >(&CGrenchler::ClearLineOfFire)},
    {"ClearPathToPlayer", static_cast< TriggerFunc >(&CGrenchler::ClearPathToPlayer)},
    {"CrystalDamaged", static_cast< TriggerFunc >(&CGrenchler::CrystalDamaged)},
    {"EmergedFromWater", static_cast< TriggerFunc >(&CGrenchler::EmergedFromWater)},
    {"FacingJumpEnd", static_cast< TriggerFunc >(&CGrenchler::FacingJumpEnd)},
    {"FacingPlayer", static_cast< TriggerFunc >(&CGrenchler::FacingPlayer)},
    {"ForceGrapple", static_cast< TriggerFunc >(&CGrenchler::ForceGrapple)},
    {"Frustrated", static_cast< TriggerFunc >(&CGrenchler::Frustrated)},
    {"JumpLanded", static_cast< TriggerFunc >(&CGrenchler::JumpLanded)},
    {"JustBeamAttacked", static_cast< TriggerFunc >(&CGrenchler::JustBeamAttacked)},
    {"JustBiteAttacked", static_cast< TriggerFunc >(&CGrenchler::JustBiteAttacked)},
    {"JustBurstAttacked", static_cast< TriggerFunc >(&CGrenchler::JustBurstAttacked)},
    {"JustHit", static_cast< TriggerFunc >(&CGrenchler::JustHit)},
    {"GrapplingMorphball", static_cast< TriggerFunc >(&CGrenchler::GrapplingMorphball)},
    {"HasAttackPattern", static_cast< TriggerFunc >(&CGrenchler::HasAttackPattern)},
    {"HasValidJumpTarget", static_cast< TriggerFunc >(&CGrenchler::HasValidJumpTarget)},
    {"InBeamRange", static_cast< TriggerFunc >(&CGrenchler::InBeamRange)},
    {"InBiteRange", static_cast< TriggerFunc >(&CGrenchler::InBiteRange)},
    {"InBurstRange", static_cast< TriggerFunc >(&CGrenchler::InBurstRange)},
    {"InChargeRange", static_cast< TriggerFunc >(&CGrenchler::InChargeRange)},
    {"ManeuverDone", static_cast< TriggerFunc >(&CGrenchler::ManeuverDone)},
    {"PauseOver", static_cast< TriggerFunc >(&CGrenchler::PauseOver)},
    {"PlayerBehindMe", static_cast< TriggerFunc >(&CGrenchler::PlayerBehindMe)},
    {"PlayerStuck", static_cast< TriggerFunc >(&CGrenchler::PlayerStuck)},
    {"PlayerSubmerged", static_cast< TriggerFunc >(&CGrenchler::PlayerSubmerged)},
    {"PlayYellowHitReact", static_cast< TriggerFunc >(&CGrenchler::PlayYellowHitReact)},
    {"ReturnToPatrol", static_cast< TriggerFunc >(&CGrenchler::ReturnToPatrol)},
    {"ShouldBackstep", static_cast< TriggerFunc >(&CGrenchler::ShouldBackstep)},
    {"ShouldSlide", static_cast< TriggerFunc >(&CGrenchler::ShouldSlide)},
    {"ShouldTurn", static_cast< TriggerFunc >(&CGrenchler::ShouldTurn)},
    {"SlideOver", static_cast< TriggerFunc >(&CGrenchler::SlideOver)},
    {"SlideStop", static_cast< TriggerFunc >(&CGrenchler::SlideStop)},
    {"StopStruggling", static_cast< TriggerFunc >(&CGrenchler::StopStruggling)},
    {"StruggleOver", static_cast< TriggerFunc >(&CGrenchler::StruggleOver)},
    {"Stuck", static_cast< TriggerFunc >(&CGrenchler::Stuck)},
    {"Submerged", static_cast< TriggerFunc >(&CGrenchler::Submerged)},
    {"TailIntact", static_cast< TriggerFunc >(&CGrenchler::TailIntact)},
    {"TookDamage", static_cast< TriggerFunc >(&CGrenchler::TookDamage)},
    {"TookKnockback", static_cast< TriggerFunc >(&CGrenchler::TookKnockback)},
    {"TooMuchTurning", static_cast< TriggerFunc >(&CGrenchler::TooMuchTurning)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Backstep", static_cast< StateFunc >(&CGrenchler::Backstep)},
    {"BeamAttack", static_cast< StateFunc >(&CGrenchler::BeamAttack)},
    {"BiteAttack", static_cast< StateFunc >(&CGrenchler::BiteAttack)},
    {"BurstAttack", static_cast< StateFunc >(&CGrenchler::BurstAttack)},
    {"Charge", static_cast< StateFunc >(&CGrenchler::Charge)},
    {"ChargeFailed", static_cast< StateFunc >(&CGrenchler::ChargeFailed)},
    {"Dead", static_cast< StateFunc >(&CGrenchler::Dead)},
    {"FollowAttackPattern", static_cast< StateFunc >(&CGrenchler::FollowAttackPattern)},
    {"GrappleAbort", static_cast< StateFunc >(&CGrenchler::GrappleAbort)},
    {"GrappleBite", static_cast< StateFunc >(&CGrenchler::GrappleBite)},
    {"GrappleBreak", static_cast< StateFunc >(&CGrenchler::GrappleBreak)},
    {"GrappleLoop", static_cast< StateFunc >(&CGrenchler::GrappleLoop)},
    {"GrapplePull", static_cast< StateFunc >(&CGrenchler::GrapplePull)},
    {"GrappleSlide", static_cast< StateFunc >(&CGrenchler::GrappleSlide)},
    {"GrappleSlideBonk", static_cast< StateFunc >(&CGrenchler::GrappleSlideBonk)},
    {"GrappleStruggle", static_cast< StateFunc >(&CGrenchler::GrappleStruggle)},
    {"Jump", static_cast< StateFunc >(&CGrenchler::Jump)},
    {"Lurk", static_cast< StateFunc >(&CGrenchler::Lurk)},
    {"Maneuver", static_cast< StateFunc >(&CGrenchler::Maneuver)},
    {"MorphballBite", static_cast< StateFunc >(&CGrenchler::MorphballBite)},
    {"Null", static_cast< StateFunc >(&CGrenchler::Null)},
    {"Patrol", static_cast< StateFunc >(&CGrenchler::Patrol)},
    {"PauseBetweenBeams", static_cast< StateFunc >(&CGrenchler::PauseBetweenBeams)},
    {"PauseBetweenBites", static_cast< StateFunc >(&CGrenchler::PauseBetweenBites)},
    {"Pursue", static_cast< StateFunc >(&CGrenchler::Pursue)},
    {"ShakeOff", static_cast< StateFunc >(&CGrenchler::ShakeOff)},
    {"StopBeamAttack", static_cast< StateFunc >(&CGrenchler::StopBeamAttack)},
    {"Taunt", static_cast< StateFunc >(&CGrenchler::Taunt)},
    {"Turn", static_cast< StateFunc >(&CGrenchler::Turn)},
    {"TurnToJumpEnd", static_cast< StateFunc >(&CGrenchler::TurnToJumpEnd)},
    {"WalkTowardPlayer", static_cast< StateFunc >(&CGrenchler::WalkTowardPlayer)},
    {"YellowHitReact", static_cast< StateFunc >(&CGrenchler::YellowHitReact)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"PickJumpTarget", static_cast< CodeFunc >(&CGrenchler::PickJumpTarget)},
    {"SetLastActionAsBeam", static_cast< CodeFunc >(&CGrenchler::SetLastActionAsBeam)},
};

bool CGrenchler::InChargeRange(CStateManager& mgr, const CTriggerData& data) const {
  return InPlayerRange(mgr, mChargeData.mMinRange, mChargeData.mMaxRange);
}

bool CGrenchler::InBeamRange(CStateManager& mgr, const CTriggerData& data) const {
  float maxRange = mBeamAttack.mMaxRange;
  if (xa88_ > 2.f) { maxRange *= 2.f; }
  return InPlayerRange(mgr, mBeamAttack.mMinRange, maxRange);
}

bool CGrenchler::InBurstRange(CStateManager& mgr, const CTriggerData& data) const {
  return InPlayerRange(mgr, mBurstAttack.mMinRange, mBurstAttack.mMaxRange);
}

bool CGrenchler::Submerged(CStateManager& mgr, const CTriggerData& data) const { return x9fc_; }

bool CGrenchler::PlayYellowHitReact(CStateManager& mgr, const CTriggerData& data) const {
  return xe6c_ == 4;
}

bool CGrenchler::CanBeUnPossessed(CStateManager& mgr) const { return false; }

bool CGrenchler::ForceGrapple(CStateManager& mgr, const CTriggerData& data) const {
  return xe6c_ > 0;
}

bool CGrenchler::FacingPlayer(CStateManager& mgr, const CTriggerData& data) const {
  return IsFacingPlayer(mgr, 0.61086524f);
}

bool CGrenchler::ShouldTurn(CStateManager& mgr, const CTriggerData& data) const {
  return !IsFacingPlayer(mgr, 0.7853982f);
}

bool CGrenchler::SlideOver(CStateManager& mgr, const CTriggerData& data) const {
  return xea8_24_;
}

bool CGrenchler::FacingJumpEnd(CStateManager& mgr, const CTriggerData& data) const {
  return IsFacing(mJumpData.x10_, 0.2617994f);
}

bool CGrenchler::JumpLanded(CStateManager& mgr, const CTriggerData& data) const {
  return mJumpData.x1c_27_;
}

bool CGrenchler::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  return mHitByPlayerProjectile;
}

bool CGrenchler::Alerted(CStateManager& mgr, const CTriggerData& data) const { return x90c_25_; }

bool CGrenchler::ReturnToPatrol(CStateManager& mgr, const CTriggerData& data) const {
  return x90c_28_;
}

bool CGrenchler::HasValidJumpTarget(CStateManager& mgr, const CTriggerData& data) const {
  return mJumpData.x1c_26_;
}

bool CGrenchler::TookDamage(CStateManager& mgr, const CTriggerData& data) const {
  return GetHealthInfo()->GetHP() < mChargeData.x20_;
}

bool CGrenchler::JustBiteAttacked(CStateManager& mgr, const CTriggerData& data) const {
  if (GetLastAction() == 2 && 0.2f + mBiteAttack.x64_ > x8ac_) {
    return true;
  }
  return false;
}

bool CGrenchler::JustBurstAttacked(CStateManager& mgr, const CTriggerData& data) const {
  if (GetLastAction() == 3 && 0.2f + mBurstAttack.x3c_ > x8ac_) {
    return true;
  }
  return false;
}

bool CGrenchler::PauseOver(CStateManager& mgr, const CTriggerData& data) const {
  return x8ac_ > xce0_;
}

bool CGrenchler::AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

bool CGrenchler::TailIntact(CStateManager& mgr, const CTriggerData& data) const {
  return xc6c_ != 1;
}

bool CGrenchler::CanGrapple(CStateManager& mgr, const CTriggerData& data) const { return false; }

bool CGrenchler::BreakGrappleLoop(CStateManager& mgr, const CTriggerData& data) const {
  return false;
}

bool CGrenchler::BeamHitSticky(CStateManager& mgr, const CTriggerData& data) const {
  return mBeamHit.x48_ == 3;
}

bool CGrenchler::BeamHitPlayer(CStateManager& mgr, const CTriggerData& data) const {
  return mBeamHit.x48_ == 1;
}

bool CGrenchler::TookKnockback(CStateManager& mgr, const CTriggerData& data) const {
  return GetBodyController()->GetCurrentStateId() == pas::kAS_KnockBack;
}

bool CGrenchler::Frustrated(CStateManager& mgr, const CTriggerData& data) const {
  return xa7c_ > 1.f;
}

bool CGrenchler::Stuck(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Stuck(mgr, data);
}

CEntity* LoadGrenchler(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

static void SetFuncPtrs() {
  static SGrenchler_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadGrenchler;
  SetSGrenchler_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSGrenchler_FuncPtrs(nullptr); }
