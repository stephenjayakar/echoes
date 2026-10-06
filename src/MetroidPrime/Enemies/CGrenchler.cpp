#include "MetroidPrime/Enemies/CGrenchler.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGrenchler.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

// Guessed names for the file-scope constants below.
static const char* const skBubblesLocator = "BodyBubbles";
static const char* const skGrappleGuardianName = "BossGrappleGuardian";
static const char* const skElectricLocators[5] = {
    "Electric", "ElectricBody", "ElectricHead", "ElectricLLeg", "ElectricRLeg",
};
static const char* const skHeadBoneName = "head";
static const char* const skEyeLocator = "eye";
static const char* const skHornLocator = "horn_LCTR";
static const char* const skRootLocator = "Skeleton_Root";
static const char* const skJawLocator = "jaw";
static const float skGrappleLoopTime = 1.5f;
static const char* const skAttachLocator = "attach_LCTR_SDK";

static EMaterialTypes skSolidMaterial = kMT_Unknown59;

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

struct SJointSphereInfo {
  const char* mName;
  float mRadius;
  float x8_;
  int xc_;
  int x10_;
  bool x14_;
};

static const SJointSphereInfo skJointSpheres[] = {
    {"Skeleton_Root", 1.3f, 1.6f, 0, 75, true}, {"R_hip", 0.9f, 0.9f, 0, 75, true},
    {"L_hip", 0.9f, 0.9f, 0, 75, true},         {"tailbone_1", 0.6f, 0.9f, 0, 75, true},
    {"tailbone_2", 0.45f, 0.55f, 0, 75, false}, {"horn_LCTR", 0.8f, 0.7f, 1, 105, true},
    {"eye", 0.8f, 0.9f, 1, 105, true},          {"jaw", 1.2f, 1.1f, 2, 105, true},
    {"R_knee", 0.9f, 0.9f, 2, 105, true},       {"L_knee", 0.9f, 0.9f, 2, 105, true},
};

static EMaterialTypes skAlignExcludeMaterial1 = kMT_Ceiling;
static EMaterialTypes skAlignExcludeMaterial2 = kMT_Wall;

CGrenchler::CGrenchler(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CModelData& mData, const CPatternedInfo& pInfo, CAssetId stateMachine,
    float tailDestroyedHealth, float minTimeBetweenCharges, float f3, float chargeAttackMinRange,
    float chargeAttackMaxRange, float biteAttackMinRange, float biteAttackMaxRange,
    float biteAttackMinPause, bool isGrappleGuardian, bool hasHealthBar,
    const CDamageVulnerability& vulnerability, int tail0, int tail1, int tail2, int tail3,
    CAssetId taillessModel, CAssetId taillessSkinRules, int tailDark0, int tailDark1,
    int tailDark2, int tailDark3, CAssetId taillessModelDark, CAssetId taillessSkinRulesDark,
    ushort tailHitSound, ushort tailDestroyedSound, float biteAttackMaxPause,
    float biteAttackDamageRadius, const CDamageInfo& biteDamage, const CDamageInfo& beamDamage,
    float beamAttackMinRange, float beamAttackMaxRange, float beamAttackMinPause,
    float beamAttackMaxPause, float beamAttackMaxAngle, const SLdrAudioPlaybackParms& beamAttackSound,
    const CDamageInfo& burstDamage, CAssetId burstProjectile, float burstAttackMinRange,
    float burstAttackMaxRange, float burstAttackMinPause, float burstAttackMaxPause,
    float burstAttackDamageRadius, CAssetId surfaceRingsEffect, CAssetId shallowWaterRing,
    CAssetId shallowWaterSplash, CAssetId part, CAssetId grappleSwoosh, CAssetId grappleBeamPart,
    CAssetId grappleHitFx, const CDamageInfo& grappleDamage,
    const SLdrAudioPlaybackParms& grappleBeamSound, CAssetId beamEffect, int unknown,
    float unknown1, float unknown2, float unknown3, CAssetId grappleVisorEffect,
    const CDamageInfo& damageInfo, CAssetId part2, const SLdrAudioPlaybackParms& audioPlaybackParms,
    CAssetId grappleGuardianEyeGlow, CAssetId alternateScannableInfo,
    const CActorParameters& actParms)
: CPatterned(kPAI_Grenchler, uid, name, kFT_Zero, info, xf, mData, pInfo, kMT_Ground, kCT_One,
             kBT_BiPedal, actParms)
, mPathFindSearch(nullptr, (pInfo.GetIngPossessionData().isAnEncounter ? 0x200 : 0) + 0x11,
                  pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, x8ac_(0.f)
, x8b0_(CVector3f::Zero())
, x8bc_(-1000.f)
, mBoneTracking(*GetModelData()->GetAnimationData(), rstl::string_l(skHeadBoneName), 0.5235988f,
                0.9424779f, kBTF_NoParent)
, x8fc_(-1000.f)
, x900_(-1000.f)
, x904_(-1000.f)
, x908_(-1)
, x90c_24_(false)
, x90c_25_(false)
, mIsGrappleGuardian(isGrappleGuardian)
, mHasHealthBar(hasHealthBar)
, x90c_28_(false)
, x90c_29_(false)
, x90c_30_(false)
, mSurfaceAlignment(CMaterialFilter::MakeExclude(
      CMaterialList(skAlignExcludeMaterial1, skAlignExcludeMaterial2)))
, mAlternateScannableInfo(nullptr)
, mVulnerability(vulnerability)
, mStateMachine2(stateMachine)
, mTailModel(taillessModel, taillessSkinRules)
, mTailModelDark(taillessModelDark, taillessSkinRulesDark)
, x9fc_(false)
, xa00_(-1000.f)
, xa04_(kInvalidUniqueId)
, xa06_(false)
, mCollisionActorManager(nullptr)
, mBiteAttack(biteDamage, biteAttackMinRange, biteAttackMaxRange, biteAttackMinPause,
              biteAttackMaxPause, biteAttackDamageRadius)
, mBeamAttack(beamDamage, beamAttackSound, beamAttackMinRange, beamAttackMaxRange,
              beamAttackMinPause, beamAttackMaxPause, beamAttackMaxAngle)
, mBurstAttack(burstProjectile, burstDamage, burstAttackMinRange, burstAttackMaxRange,
               burstAttackMinPause, burstAttackMaxPause, burstAttackDamageRadius)
, xc3c_(CTransform4f::Identity())
, xc6c_(0)
, xc70_(-1000.f)
, mTailHealth(tailDestroyedHealth)
, xc78_(-1000.f)
, mTailHitSound(tailHitSound)
, mTailDestroyedSound(tailDestroyedSound)
, xc80_(false)
, mChargeData(minTimeBetweenCharges, f3, chargeAttackMinRange, chargeAttackMaxRange)
, xcb8_(0.f)
, xcbc_(false)
, xcc0_(0.f)
, xcc4_(0.f)
, xcc8_(0)
, xccc_(kInvalidUniqueId)
, xcd0_(CVector3f::Zero())
, xcdc_(0.f)
, xce0_(0.f)
, xce4_(false)
, xce8_(-1000.f)
, xcec_(160.f)
, xcf0_(-1000.f)
, xcf4_(false)
, xcf8_(part)
, mGrappleBeam(surfaceRingsEffect, shallowWaterSplash, beamEffect, grappleHitFx,
               grappleGuardianEyeGlow)
, mStruggle(unknown, grappleVisorEffect, unknown1)
, mBeamHit(grappleSwoosh, grappleBeamPart, grappleDamage, grappleBeamSound)
, xe24_(0.f)
, xe28_(0.f)
, xe2c_(CVector3f::Zero())
, xe38_(tail0)
, xe3c_(tail1)
, xe40_(tail2)
, xe44_(tail3)
, xe48_(tailDark0)
, xe4c_(tailDark1)
, xe50_(tailDark2)
, xe54_(tailDark3)
, xe58_(0.f)
, xe5c_(0.f)
, xe60_(0.f)
, xe64_(unknown2)
, xe68_(unknown3)
, xe6c_(0)
, xe70_(CTransform4f::Identity())
, xea0_24_(false)
, xea0_25_(false)
, xea0_26_(false)
, xea4_(-1000.f)
, xea8_24_(false)
, xea8_25_(false)
, xeac_(0)
, xeb0_(audioPlaybackParms)
, xec8_(0)
, xecc_(damageInfo, part2)
, xf18_(5)
, xf1c_(1.f)
, xf20_(kInvalidUniqueId)
, xf24_(shallowWaterRing) {
  if (!mIsGrappleGuardian) {
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_NearbySurface);
    mSurfaceAlignment.SetAngularRate(16.f);
  } else {
    KnockBackController().EnableFreeze(false);
    KnockBackController().EnableSlow(false);
    KnockBackController().EnableKnockBackPhysics(false);
    KnockBackController().EnableLaggedBurnDeath(false);
    KnockBackController().EnableBurnDeath(false);
    KnockBackController().EnableExplodeDeath(false);
  }
  if (mTailHealth < 0.f || mTailHealth > GetHealthInfo()->GetHP()) {
    mTailHealth = CMath::Clamp(0.f, mTailHealth, GetHealthInfo()->GetHP());
  }
  if (alternateScannableInfo != kInvalidAssetId) {
    mAlternateScannableInfo = rs_new TCachedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', alternateScannableInfo)), true);
  }
}

bool CGrenchler::InChargeRange(CStateManager& mgr, const CTriggerData& data) const {
  return InPlayerRange(mgr, mChargeData.mMinRange, mChargeData.mMaxRange);
}

bool CGrenchler::InBeamRange(CStateManager& mgr, const CTriggerData& data) const {
  float maxRange = mBeamAttack.mMaxRange;
  if (mAttackHistory.x28_ > 2.f) { maxRange *= 2.f; }
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
  return mAttackHistory.x1c_ > 1.f;
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
