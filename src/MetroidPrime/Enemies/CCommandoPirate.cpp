#include "MetroidPrime/Enemies/CCommandoPirate.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

#include "Kyoto/Particles/CElementGen.hpp"

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::StateOver)},
    {"ShouldWarpIn",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldWarpIn)},
    {"ShouldMeleeAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldMeleeAttack)},
    {"ShouldFireEGrenade",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldFireEGrenade)},
    {"ShouldRetreat",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldRetreat)},
    {"ShouldJumpBack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldJumpBack)},
    {"ShouldAmbush",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldAmbush)},
    {"BreakAmbush",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::BreakAmbush)},
    {"HasLineOfSight",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::HasLineOfSight)},
    {"UnderFire", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::UnderFire)},
    {"HeardShot", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::HeardShot)},
    {"HasTarget", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::HasTarget)},
    {"HasNewTarget",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::HasNewTarget)},
    {"PathShagged",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::PathShagged)},
    {"PathOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::PathOver)},
    {"IsOffPath", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::IsOffPath)},
    {"HasAttackPattern",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::HasAttackPattern)},
    {"IsFacingTarget",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::IsFacingTarget)},
    {"AttackPatternOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::AttackPatternOver)},
    {"TooClose", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::TooClose)},
    {"ShouldDodge",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldDodge)},
    {"ShouldArmShield",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldArmShield)},
    {"ShouldShieldCharge",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldShieldCharge)},
    {"ShouldBoost",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldBoost)},
    {"AbortShieldCharge",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::AbortShieldCharge)},
    {"IsAggressive",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::IsAggressive)},
    {"FoundJumpPoint",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::FoundJumpPoint)},
    {"ShouldCrouch",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldCrouch)},
    {"ShouldWallHang",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldWallHang)},
    {"ShouldCover",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldCover)},
    {"FoundCover", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::FoundCover)},
    {"ShouldCoverAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::ShouldCoverAttack)},
    {"CoverBlown", static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::CoverBlown)},
    {"AbortSeekCover",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CCommandoPirate::AbortSeekCover)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Lurk", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Lurk)},
    {"Ambush", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Ambush)},
    {"Alert", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Alert)},
    {"FaceTarget", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::FaceTarget)},
    {"SelectTarget",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SelectTarget)},
    {"SetTargetDest",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SetTargetDest)},
    {"SetRetreatDest",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SetRetreatDest)},
    {"SetJumpDest", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SetJumpDest)},
    {"SetPathMeshDest",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SetPathMeshDest)},
    {"MeleeAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::MeleeAttack)},
    {"EGrenadeAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::EGrenadeAttack)},
    {"PostEGrenadeAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::PostEGrenadeAttack)},
    {"JumpPointFind",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::JumpPointFind)},
    {"JetBoost", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::JetBoost)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::PathFind)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Patrol)},
    {"FollowAttackPattern",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::FollowAttackPattern)},
    {"WarpIn", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::WarpIn)},
    {"WarpOut", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::WarpOut)},
    {"PostWarpOut", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::PostWarpOut)},
    {"JumpBack", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::JumpBack)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Dodge)},
    {"ArmShield", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::ArmShield)},
    {"ShieldCharge",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::ShieldCharge)},
    {"ScriptedShieldCharge",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::ScriptedShieldCharge)},
    {"RestoreOrientation",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::RestoreOrientation)},
    {"Crouch", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Crouch)},
    {"WallHang", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::WallHang)},
    {"WallDetach", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::WallDetach)},
    {"CoverFind", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::CoverFind)},
    {"SetCoverDest",
     static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::SetCoverDest)},
    {"Cover", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Cover)},
    {"CoverAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::CoverAttack)},
    {"BreakCover", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::BreakCover)},
    {"GetUp", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::GetUp)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CCommandoPirate::Dead)},
};

static const SBurst skBurstsStandard[] = {
    {15, {6, 5, 3, 2, 1, -1, 0, 0}, 0.1f, 0.05f},  {20, {1, 2, 3, 4, 5, -1, 0, 0}, 0.1f, 0.05f},
    {20, {7, 6, 5, 4, 3, -1, 0, 0}, 0.1f, 0.05f},  {15, {3, 4, 5, 6, 7, -1, 0, 0}, 0.1f, 0.05f},
    {15, {6, 5, 4, 3, 2, -1, 0, 0}, 0.1f, 0.05f},  {15, {2, 3, 4, 5, 6, -1, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsJumping[] = {
    {20, {16, 4, 8, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {5, 7, 9, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {1, 5, 10, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsStandardOOV[] = {
    {26, {16, 5, 8, 11, 14, -1, 0, 0}, 0.1f, 0.05f},
    {26, {16, 13, 12, 11, 8, -1, 0, 0}, 0.1f, 0.05f},
    {16, {9, 11, 13, 15, 2, -1, 0, 0}, 0.1f, 0.05f},
    {16, {14, 13, 12, 11, 10, -1, 0, 0}, 0.1f, 0.05f},
    {8, {10, 11, 12, 13, 14, -1, 0, 0}, 0.1f, 0.05f},
    {8, {6, 8, 11, 13, 15, -1, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsJumpingOOV[] = {
    {40, {7, 9, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {9, 5, 1, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {16, 14, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst* skBursts[] = {
    skBurstsStandard, skBurstsJumping, skBurstsStandardOOV, skBurstsJumpingOOV, nullptr,
};

const float CCommandoPirate::skGravityConstant = 50.f;

static CElementGen* CreateElementGen(CAssetId id) {
  if (id != kInvalidAssetId) {
    TToken< CGenDescription > token(gpSimplePool->GetObj(SObjectTag('PART', id)));
    return rs_new CElementGen(token, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
  }
  return nullptr;
}

CCommandoGrenadeInfo::CCommandoGrenadeInfo(const CBouncyGrenadeData& data, float empDuration,
                                           float minAttackInterval, float postAttackPause,
                                           float attackChance, float minAttackDist,
                                           float maxAttackDist, float minLaunchSpeed,
                                           float maxLaunchSpeed)
: mData(data)
, mEMPDuration(empDuration)
, mAttackChance(attackChance)
, mMinAttackInterval(minAttackInterval)
, mPostAttackPause(postAttackPause)
, mMinAttackDist(minAttackDist)
, mMaxAttackDist(maxAttackDist)
, mMinLaunchSpeed(minLaunchSpeed)
, mMaxLaunchSpeed(maxLaunchSpeed) {}

CCommandoShieldInfo::CCommandoShieldInfo(
    const CDamageInfo& chargeDamage, const CDamageVulnerability& vulnerability,
    float chargeMinAttackDist, float chargeMaxAttackDist, float chargeSpeed, CAssetId explodeEffect,
    ushort sfxExplode, float x60, float x64, float armShieldChance, float armShieldTime,
    float armShieldTimeVariation, CAssetId armShieldExplodeEffect, CAssetId chargeEffect,
    CAssetId armShieldEffect, ushort sfxTurnOn, ushort sfxTurnOff)
: mChargeDamage(chargeDamage)
, mVulnerability(vulnerability)
, mChargeMinAttackDist(chargeMinAttackDist)
, mChargeMaxAttackDist(chargeMaxAttackDist)
, mChargeSpeed(chargeSpeed)
, mExplodeEffect(explodeEffect)
, mSfxExplode(sfxExplode)
, x60_(x60)
, x64_(x64)
, mArmShieldChance(armShieldChance)
, mArmShieldTime(armShieldTime)
, mArmShieldTimeVariation(armShieldTimeVariation)
, mArmShieldExplodeEffect(armShieldExplodeEffect)
, mChargeEffect(chargeEffect)
, mArmShieldEffect(armShieldEffect)
, mSfxTurnOn(sfxTurnOn)
, mSfxTurnOff(sfxTurnOff) {}

CCommandoPirateData::CCommandoPirateData(
    uint flags, ushort sfxImpact, ushort sfxHurled, ushort sfxDeath, ushort x6, CAssetId x8,
    const CDamageInfo& bladeDamage, float aggressiveness, float coverCheck, float searchRadius,
    float dodgeCheck, float hearingRadius, float intraBurstShotTime, float intraBurstShotVariation,
    CAssetId projectile, const CDamageInfo& projectileDamage, ushort sfxProjectile,
    const CCommandoGrenadeInfo& grenadeInfo, const CCommandoShieldInfo& shieldInfo)
: mSfxImpact(sfxImpact)
, mSfxHurled(sfxHurled)
, mSfxDeath(sfxDeath)
, x6_(x6)
, x8_(x8)
, mBladeDamage(bladeDamage)
, mProjectile(projectile)
, mProjectileDamage(projectileDamage)
, mSfxProjectile(sfxProjectile)
, mAggressiveness(aggressiveness)
, mCoverCheck(coverCheck)
, mSearchRadius(searchRadius)
, mDodgeCheck(dodgeCheck)
, mHearingRadius(hearingRadius)
, mIntraBurstShotTime(intraBurstShotTime)
, mIntraBurstShotVariation(intraBurstShotVariation)
, mGrenadeInfo(grenadeInfo)
, mShieldInfo(shieldInfo)
, x15c_24_(flags & 1)
, x15c_25_((flags >> 1) & 1)
, x15c_26_((flags >> 2) & 1)
, x15c_27_((flags >> 3) & 1)
, x15c_28_((flags >> 4) & 1)
, x15c_29_((flags >> 5) & 1)
, x15c_30_((flags >> 6) & 1)
, x15c_31_((flags >> 7) & 1)
, x15d_24_((flags >> 8) & 1)
, x15d_25_((flags >> 9) & 1)
, x15d_26_((flags >> 10) & 1)
, x15d_27_((flags >> 11) & 1) {}

CCommandoPirate::CCommandoPirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CTransform4f& xf, const CModelData& modelData,
                                 const CActorParameters& actParms, const CPatternedInfo& pInfo,
                                 const CCommandoPirateData& data)
: CPatterned(static_cast< EPatternedAI >(5), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
, x920_(-1)
, x924_(-1)
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  static_cast< CPFRegion::ERootPosition >(0))
, mProjectileInfo(data.mProjectile, data.mProjectileDamage)
, mShieldVulnerability(data.mShieldInfo.mVulnerability)
, mBoneTracking(*GetAnimationData(), rstl::string_l("Head_1"), 70.f * M_PIF / 180.f,
                CRelAngle::FromDegrees(180.f).AsRadians(), 0)
, mLineOfSightTracker(GetUniqueId(), CSegId(), 0.1f, 0.05f)
, mBurstFire(skBursts, 0)
, xb50_(0)
, xb54_(0)
, xb58_(0)
, xb5c_(0.f)
, xb60_(1.5f)
, xb64_(0.75f)
, xb68_(0.f)
, xb6c_(0.f)
, xb70_(0.f)
, xb74_(pInfo.GetFrozenXDamageThreshold())
, xb78_(2.f)
, xb7c_(1.f)
, xb80_(0.f)
, xb84_(0.f)
, xb88_(0.f)
, xb8c_(0.f)
, xb90_(0.f)
, xb94_(CVector3f::Zero())
, xba0_(CVector3f::Zero())
, xbac_(CVector3f::Zero())
, xbb8_(CVector3f::Zero())
, xbc4_(kInvalidUniqueId)
, xbc6_(kInvalidUniqueId)
, xbc8_(kInvalidUniqueId)
, xbca_(kInvalidUniqueId)
, xbcc_(kInvalidUniqueId)
, xbce_(kInvalidUniqueId)
, xbd0_(kInvalidUniqueId)
, xbd2_(kInvalidUniqueId)
, xbd4_(kInvalidUniqueId)
, xbd6_(kInvalidUniqueId)
, xbd8_(kInvalidUniqueId)
, xbda_(kInvalidUniqueId)
, xbdc_(-1)
, xbe0_(5.f)
, xbe4_(5.f)
, xbe8_(1.f)
, xbec_(0.f)
, xbf0_(CVector3f::Zero())
, xc00_(0.f)
, xc04_(0)
, mShieldExplodeEffect(mData.mShieldInfo.mExplodeEffect != kInvalidAssetId
                           ? rstl::optional_object< TLockedToken< CGenDescription > >(
                                 gpSimplePool->GetObj(
                                     SObjectTag('PART', mData.mShieldInfo.mExplodeEffect)))
                           : rstl::optional_object_null())
, mArmShieldExplodeEffect(
      mData.mShieldInfo.mArmShieldExplodeEffect != kInvalidAssetId
          ? rstl::optional_object< TLockedToken< CGenDescription > >(gpSimplePool->GetObj(
                SObjectTag('PART', mData.mShieldInfo.mArmShieldExplodeEffect)))
          : rstl::optional_object_null())
, xc28_(mData.x8_ != kInvalidAssetId ? rstl::optional_object< TLockedToken< CGenDescription > >(
                                           gpSimplePool->GetObj(SObjectTag('PART', mData.x8_)))
                                     : rstl::optional_object_null())
, xc38_(0)
, mArmShieldEffect(CreateElementGen(mData.mShieldInfo.mArmShieldEffect))
, mShieldChargeEffect(CreateElementGen(mData.mShieldInfo.mChargeEffect))
, xc44_(0.f)
, xc48_(0.f)
, xc4c_(-1)
, xc50_(0.f)
, xc54_(-1)
, xc58_(0.f)
, xc63_24_(false)
, xc63_25_(false)
, xc63_26_(false)
, xc63_27_(false)
, xc63_28_(false)
, xc63_29_(false)
, xc63_30_(false)
, xc63_31_(false)
, xc64_24_(false)
, xc64_25_(false)
, xc64_26_(false)
, xc64_27_(false)
, xc64_28_(false)
, xc64_29_(false)
, xc64_30_(false)
, xc64_31_(false)
, xc65_24_(true)
, xc65_25_(false)
, xc65_26_(false)
, xc65_27_(false)
, xc65_28_(false)
, xc65_29_(false)
, xc65_30_(false)
, xc65_31_(false)
, xc66_24_(false)
, xc66_25_(false)
, xc66_26_(false) {
  mProjectileInfo.Token().Lock();
  KnockBackController().EnableBurn(true);
  KnockBackController().EnableKnockBackPhysics(!mData.x15c_29_);

  CAnimData* animData = ModelData()->AnimationData();
  mHeadSeg = animData->GetLocatorSegId(rstl::string_l("Head_1"));
  mLaunchSeg = animData->GetLocatorSegId(rstl::string_l("Mid_Launch_LCTR"));
  mGunSeg = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
  xc5f_ = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
  mRightWristSeg = animData->GetLocatorSegId(rstl::string_l("R_wrist"));
  mRightElbowSeg = animData->GetLocatorSegId(rstl::string_l("R_elbow"));
  mLeftWristSeg = animData->GetLocatorSegId(rstl::string_l("L_wrist"));
  mLineOfSightTracker.SetSegment(mHeadSeg);
  mBoneTracking.SetDisableTrackingDistance(25.f * GetModelData()->GetScale().GetZ());
  mBurstFire.SetBurstType(0);

  const CPASAnimParmData parms1(static_cast< pas::EAnimationState >(13), CPASAnimParm::FromEnum(0),
                                CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(0));
  xbe8_ = GetModelData()->GetScale().GetY() * GetAnimationDistance(parms1);
  const CPASAnimParmData parms2(static_cast< pas::EAnimationState >(3), CPASAnimParm::FromEnum(3),
                                CPASAnimParm::FromEnum(2));
  xbe4_ = GetModelData()->GetScale().GetX() * GetAnimationDistance(parms2);
  const CPASAnimParmData parms3(static_cast< pas::EAnimationState >(3), CPASAnimParm::FromEnum(1),
                                CPASAnimParm::FromEnum(2));
  xbe0_ = GetModelData()->GetScale().GetY() * GetAnimationDistance(parms3);

  mShieldVulnerability = CPatterned::GetDamageVulnerability()->MakeIgnoreRadius();

  const float runSpeed = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_BackUp);
  if (runSpeed > 0.f) {
    xb90_ = 4.f * (mData.mSearchRadius / runSpeed);
  }
  mPathFindSearch.SetCharacterRadius(GetBoundingBox().GetWidth());
  mPathFindSearch.SetCharacterHeight(GetBoundingBox().GetDepth());
}

const CDamageVulnerability* CCommandoPirate::GetDamageVulnerability() const {
  if (xc65_29_) {
    return &mShieldVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

void CCommandoPirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

void CCommandoPirate::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

CEntity* LoadCommandoPirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrCommandoPirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CBouncyGrenadeData grenadeData(
      sldrThis.unknown_0xfb435257.grenadeMass, sldrThis.unknown_0xfb435257.unknown_0xed086ce0,
      LdrToDamageInfo(sldrThis.unknown_0xfb435257.grenadeDamage),
      sldrThis.unknown_0xfb435257.unknown_0x454f16b1, sldrThis.unknown_0xfb435257.grenadeExplosion,
      sldrThis.unknown_0xfb435257.grenadeExplosion, sldrThis.unknown_0xfb435257.grenadeTrail,
      sldrThis.unknown_0xfb435257.grenadeEffect, sldrThis.unknown_0xfb435257.sound_GrenadeBounce,
      sldrThis.unknown_0xfb435257.sound_GrenadeExplode, 0.1f, 150.f, 0.1f, 150.f, true);
  const CCommandoGrenadeInfo grenadeInfo(
      grenadeData, sldrThis.unknown_0xfb435257.eMPDuration,
      sldrThis.unknown_0xfb435257.grenadeMinAttackInterval,
      sldrThis.unknown_0xfb435257.grenadePostAttackPause,
      sldrThis.unknown_0xfb435257.grenadeAttackChance,
      sldrThis.unknown_0xfb435257.grenadeMinAttackDist,
      sldrThis.unknown_0xfb435257.grenadeMaxAttackDist,
      sldrThis.unknown_0xfb435257.grenadeMinLaunchSpeed,
      sldrThis.unknown_0xfb435257.grenadeMaxLaunchSpeed);

  const CCommandoShieldInfo shieldInfo(
      LdrToDamageInfo(sldrThis.shieldInfo.shieldChargeDamage),
      LdrToDamageVulnerability(sldrThis.shieldInfo.shieldVulnerability), sldrThis.shieldInfo.shieldChargeMinAttackDist,
      sldrThis.shieldInfo.shieldChargeMaxAttackDist, sldrThis.shieldInfo.shieldChargeSpeed, sldrThis.shieldInfo.shieldExplodeEffect,
      sldrThis.shieldInfo.sound_ShieldExplode, sldrThis.shieldInfo.unknown_0x6cb0da5a, sldrThis.shieldInfo.unknown_0xc3938663,
      sldrThis.shieldInfo.armShieldChance, sldrThis.shieldInfo.armShieldTime, sldrThis.shieldInfo.armShieldTimeVariation,
      sldrThis.shieldInfo.armShieldExplodeEffect, sldrThis.shieldInfo.shieldChargeEffect, sldrThis.shieldInfo.armShieldEffect,
      sldrThis.shieldInfo.sound_ShieldTurnOn, sldrThis.shieldInfo.sound_ShieldTurnOff);

  const CCommandoPirateData data(
      sldrThis.sound, sldrThis.sound_Impact, sldrThis.sound_Hurled, sldrThis.sound_Death,
      sldrThis.alwaysFF, sldrThis.alwaysFF_0x467c3d94, LdrToDamageInfo(sldrThis.bladeDamage),
      sldrThis.aggressiveness, sldrThis.coverCheck, sldrThis.searchRadius, sldrThis.dodgeCheck,
      sldrThis.hearingRadius, sldrThis.intraBurstShotTime, sldrThis.intraBurstShotVariation,
      sldrThis.projectile, LdrToDamageInfo(sldrThis.projectileDamage), sldrThis.sound_Projectile,
      grenadeInfo, shieldInfo);

  return rs_new CCommandoPirate(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData), data);
}

static void SetFuncPtrs() {
  static SCommandoPirate_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadCommandoPirate;
  SetSCommandoPirate_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSCommandoPirate_FuncPtrs(nullptr); }
