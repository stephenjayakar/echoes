#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CActorModelParticles.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTargetingPoint.hpp"
#include "MetroidPrime/BodyState/CBodyState.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWater.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CTimeProvider.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/CPirateEchoEmitter.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CMetroidAlpha.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAiJumpPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTrigger.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "rstl/algorithm.hpp"
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

CSpacePirate::CSpacePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& mData,
                           const CActorParameters& actParms, const CPatternedInfo& pInfo,
                           const CSpacePirateData& data)
: CPatterned(kPAI_SpacePirate, uid, name, kFT_Zero, info, xf, mData, pInfo, kMT_Ground, kCT_One,
             kBT_BiPedal, actParms)
, mPirateData(data)
, mPendingAmbush((mPirateData.mFlags & 0x3) != 0)
, mCeilingAmbush(mPirateData.mFlags & 0x2)
, mNonAggressive(mPirateData.mFlags & 0x4)
, mMelee(mPirateData.mFlags & 0x8)
, mNoShuffleCloseCheck(mPirateData.mFlags & 0x10)
, mOnlyAttackInRange(mPirateData.mFlags & 0x20)
, x8f4_30_(mPirateData.mFlags & 0x40)
, mNoKnockbackImpulseReset(mPirateData.mFlags & 0x80)
, mNoMeleeAttack(mPirateData.mFlags & 0x200)
, mBreakAttack(mPirateData.mFlags & 0x400)
, mSeated(mPirateData.mFlags & 0x1000)
, mShadowPirate(mPirateData.mFlags & 0x2000)
, mAlertBeforeCloak(mPirateData.mFlags & 0x4000)
, mNoBreakDodge(mPirateData.mFlags & 0x8000)
, mFloatingCorpse(mPirateData.mFlags & 0x10000)
, mRagdollNoAiCollision(mPirateData.mFlags & 0x20000)
, mTrooper(mPirateData.mFlags & 0x40000)
, mHearNoise(false)
, mEnableMeleeAttack(false)
, x8f6_27_(false)
, x8f6_28_(false)
, mEnableRetreat(false)
, mShuffleClose(false)
, mEnablePatrol(false)
, mEnableAim(false)
, mHearPlayerFire(false)
, mInProjectilePath(false)
, mNoPlayerLos(false)
, mInWallHang(false)
, mJumpVelSet(false)
, mPrevInCineCam(false)
, mPendingFrenzyChance(false)
, mAppliedBladeDamage(false)
, mAlwaysAggressive(false)
, mCoverCheck(false)
, mEnableDodge(false)
, mNoPlayerDodge(false)
, mAllEnergyDrained(false)
, mMayStartAttack(false)
, x8f9_24_(false)
, mUseJumpBackJump(false)
, mStarted(false)
, mInRange(false)
, mSatUp(false)
, mCloseMelee(false)
, mSentAttackMsg(false)
, mNormalDodge(false)
, x8fa_25_(false)
, x8fa_26_(false)
, x8fa_27_(false)
, x8fa_28_(false)
, x8fa_29_(false)
, x8fa_30_(false)
, x8fa_31_(false)
, mFrenzyFrames(0)
, mCoverPoint(kInvalidUniqueId)
, mPreviousCoverPoint(kInvalidUniqueId)
, mSteeringSpeed(1.f)
, mTargetDelta(CVector3f::Forward())
, mCoverPointRearDir(CVector3f::Zero())
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mSteeringDelayTimer(0.f)
, xa14_(0)
, mInitialHP(pInfo.GetHealthInfo().GetHP())
, mCoverRange(0.f)
, mHeadSeg(CSegId::Invalid())
, xa24_(0)
, mTaunt(pas::kTT_Invalid)
, mBoneTracking(*GetAnimationData(), rstl::string_l("Head_1"), CMath::Deg2Rad(70.f),
                CMath::Deg2Rad(180.f), kBTF_None)
, mCoverDir(pas::kCD_Invalid)
, mIntoJumpDist(1.f)
, mEyeHeight(2.f)
, xa78_(0.f)
, mTimeNoPlayerLos(0.f)
, xa80_(0.f)
, mAttachedActor(kInvalidUniqueId)
, mGunSeg(CSegId::Invalid())
, mElbowSeg(CSegId::Invalid())
, mWristSeg(CSegId::Invalid())
, mSwooshSeg(CSegId::Invalid())
, mLeftHipSeg(CSegId::Invalid())
, mRightHipSeg(CSegId::Invalid())
, mCollarSeg(CSegId::Invalid())
, mAttackRemTime(1.f)
, mTargetId(kInvalidUniqueId)
, mBurstFire(skBursts, mPirateData.mFirstBurstCount)
, mJumpHeight(3.f)
, mPatrolDestPos(CVector3f::Zero())
, mSkidDir(pas::kSD_Invalid)
, mStrafeDelayTimer(0.f)
, mMeleeSeverity(pas::kS_Invalid)
, mJumpPoint(kInvalidUniqueId)
, mDodgeDir(pas::kSD_Invalid)
, mDodgeDist(3.f)
, mBreakDodgeDist(3.f)
, mTimeSinceHitByPlayer(3.4028235e38f)
, mLowHealthFrenzyTimer(3.4028235e38f)
, mRagdollDelayTimer(0.f)
, mRagDoll(nullptr)
, mIkChain()
, mCloakDelayTimer(0.f)
, mElectricParticleTimer(0.f)
, mCloakStepTime(0.f)
, mShadowPirateAlpha(0.5f)
, mMinCloakAlpha(mPirateData.mCloakOpacity)
, mMaxCloakAlpha(mPirateData.mMaxCloakOpacity)
, mDodgeDelayTimer(mPirateData.mDodgeDelayTimeMin)
, mAimDelayTimer(mPirateData.mGunTrackDelay)
, xb9c_(0.f)
, mTeamAiMgrId(kInvalidUniqueId)
, mHeldPosition(CVector2f::Zero())
, mHoldPositionTime(0.f)
, mLeashTimer(0.f)
, xbb4_(CVector3f::Zero())
, xbc0_(-1)
, xbc4_(CVector3f::Zero())
, xbd0_(3.4028235e38f)

, xc04_(-1)
, xc58_(0)
, xc5c_(0.f, CUnitVector3f(static_cast< const CVector3f& >(CVector3f::Forward()))) {
  SetupGrenadeLauncher(data.mWeaponData);
  if (data.mProjectile != kInvalidAssetId) {
    mProjectileInfo = CProjectileInfo(data.mProjectile, data.mProjectileDamage);
    mProjectileInfo->Token().Lock();
  }
  mBurstFire.SetBurstType(1);
  const CVector3f& scale = GetModelData()->GetScale();
  mBoneTracking.SetDisableTrackingDistance(25.f * scale.GetZ());

  CAnimData* animData = AnimationData();
  mHeadSeg = animData->GetLocatorSegId(rstl::string_l("Head_1"));
  mElbowSeg = animData->GetLocatorSegId(rstl::string_l("R_elbow"));
  mWristSeg = animData->GetLocatorSegId(rstl::string_l("R_wrist"));
  mSwooshSeg = animData->GetLocatorSegId(rstl::string_l("Swoosh_LCTR"));
  mGunSeg = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
  mLeftHipSeg = animData->GetLocatorSegId(rstl::string_l("L_hip"));
  mRightHipSeg = animData->GetLocatorSegId(rstl::string_l("R_hip"));
  mCollarSeg = animData->GetLocatorSegId(rstl::string_l("Collar"));

  if (!mOnlyAttackInRange) {
    const CPASAnimParmData jump(pas::kAS_Jump, CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(0),
                                CPASAnimParm::FromEnum(0));
    const CVector3f& jumpScale = GetModelData()->GetScale();
    mIntoJumpDist = jumpScale[kDY] * GetAnimationDistance(jump);
    const CPASAnimParmData dodge(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                 CPASAnimParm::FromEnum(1));
    mDodgeDist = GetModelData()->GetScale().GetX() * GetAnimationDistance(dodge);
    const CPASAnimParmData breakDodge(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                      CPASAnimParm::FromEnum(2));
    mBreakDodgeDist = GetModelData()->GetScale().GetX() * GetAnimationDistance(breakDodge);
  } else {
    BodyController()->BodyStateInfo().SetLocoAnimChangeAtEndOfAnimOnly(true);
  }

  const CAABox& baseAABB = GetBaseBoundingBox();
  mEyeHeight = 0.6f * (baseAABB.GetMaxPoint().GetZ() - baseAABB.GetMinPoint().GetZ());

  if (ActorLights()) {
    ActorLights()->SetAmbienceGenerated(false);
  }

  mKnockBackController.SetLocomotionDuringElectrocution(true);

  if (!BodyController()->HasBodyState(pas::kAS_AdditiveAim)) {
    mMelee = true;
  }

  if (pInfo.GetEchoParameters().mIsEchoEmitter) {
    SetEchoEmitter(
        true, rs_new CPirateEchoEmitter(this, GetTranslation(), pInfo.GetEchoParameters(),
                                        animData->GetLocatorSegId(rstl::string_l("Head_1")),
                                        animData->GetLocatorSegId(rstl::string_l("R_wing_LCTR")),
                                        animData->GetLocatorSegId(rstl::string_l("L_wing_LCTR")),
                                        animData->GetLocatorSegId(rstl::string_l("gun_lctr")),
                                        animData->GetLocatorSegId(rstl::string_l("Swoosh_LCTR")),
                                        animData->GetLocatorSegId(rstl::string_l("R_ankle")),
                                        animData->GetLocatorSegId(rstl::string_l("L_ankle"))));
  }

  if (mShadowPirate) {
    SetDamageHighlight(false);
  }
}

CSpacePirate::~CSpacePirate() {}

void CSpacePirate::SetupGrenadeLauncher(const SSpacePirateWeaponData& data) {
  switch (data.mEquippedWeapon) {
  case 0:
    break;
  case 1:
    if (data.mGrenadeLauncher != kInvalidAssetId) {
      mGrenadeLauncherModel =
          CModelData(CStaticRes(data.mGrenadeLauncher, GetModelData()->GetScale()));
    }
    break;
  }
}

const float CSpacePirate::skGravityConstant = 50.f;

const float CSpacePirate::skRadii[14] = {0.45f, 0.52f, 0.35f, 0.1f,  0.15f, 0.35f, 0.1f,
                                         0.15f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f};

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

void CSpacePirate::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mEnableAim = true;
    BodyController()->AbortScriptedAnimations();
    SquadAdd(mgr);
    if (!mSentAttackMsg) {
      mSentAttackMsg = true;
      SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None);
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    if (mCeilingAmbush) {
      mPatrolDestPos = GetTranslation() + CVector3f::Down();
      mJumpHeight = 0.f;
    } else {
      TUniqueId wpId = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
      if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(wpId))) {
        mPatrolDestPos = actor->GetTranslation();
        mJumpHeight = 3.f;
      } else {
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    break;
  }
  case kStateMsg_Update: {
    int jumpType = 0;
    if (mCeilingAmbush) {
      jumpType = 2;
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
      BodyController()->CommandMgr().DeliverCmd(CBCJumpCmd(
          mPatrolDestPos, pas::EJumpType(jumpType), pas::kJS_IntoJump, 0,
          CBCJumpCmd::kFF_AmbushJump));
    }
    if (mAnimationState.GetState() == CAnimationState::kAS_Repeat) {
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    CVector3f target = GetTargetPos(mgr);
    BodyController()->CommandMgr().SetTargetVector(
        CVector3f(target.GetX() - GetTranslation().GetX(),
                  target.GetY() - GetTranslation().GetY(), 0.f));
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    mJumpHeight = 3.f;
    mCeilingAmbush = false;
    mBoneTracking.SetActive(true);
    mBoneTracking.SetTarget(mTargetId);
    break;
  }
}

void CSpacePirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mSteeringSpeed = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk) /
                     BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Run);
    break;
  case kStateMsg_Update:
    UpdateCantSeePlayer(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    if (!mSentAttackMsg) {
      mSentAttackMsg = true;
      SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None);
    }
    break;
  }
  if (mEnablePatrol) {
    CPatterned::Patrol(mgr, msg, dt);
    switch (msg) {
    case kStateMsg_Activate:
      BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
      BodyController()->SetTurnSpeed(BodyController()->GetTurnSpeed() / 1.25f);
      break;
    case kStateMsg_Update:
      AvoidActors(mgr);
      mPatrolDestPos = mWaypointNavigation.GetDestinationPosition();
      break;
    case kStateMsg_Deactivate:
      BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
      BodyController()->SetTurnSpeed(1.25f * BodyController()->GetTurnSpeed());
      break;
    }
  }
}

void CSpacePirate::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::Patrol(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Activate: {
    mSteeringSpeed = 1.f;
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    TUniqueId wpId = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    if (CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(wpId))) {
      mWaypointNavigation.SetDestination(wpId);
      CVector3f delta = wp->GetTranslation() - GetTranslation();
      if (CVector3f::Dot(GetTransform().GetForward(), delta) <= 0.f) {
        mWaypointNavigation.SetInPosition(true);
      }
    }
    break;
  }
  case kStateMsg_Update: {
    CScriptAIWaypoint* wp =
        TCastToPtr< CScriptAIWaypoint >(mgr.ObjectById(mWaypointNavigation.GetDestination()));
    if (wp) {
      const uint jump = (wp->GetFlags() >> 1) & 1;
      const uint drop = (wp->GetFlags() >> 2) & 1;
      if (jump || drop) {
        float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const CVector3f& scale = GetModelData()->GetScale();
        float distance = maxSpeed * ((1.5f * dt + 0.1f) * scale.GetY()) + mIntoJumpDist;
        if ((GetTranslation() - wp->GetTranslation()).MagSquared() < distance * distance) {
          mWaypointNavigation.SetInPosition(true);
          mJumpHeight = jump ? 3.f : 0.f;
        }
      }
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_Jump) {
      bool targetPlayer = true;
      if (wp && wp->CheckConnectedObject(mgr, kSS_Arrived, kSM_Next) != kInvalidUniqueId) {
        targetPlayer = false;
      }
      if (targetPlayer) {
        BodyController()->CommandMgr().SetTargetVector(GetTargetPos(mgr) - GetTranslation());
      }
    }
    mPatrolDestPos = mWaypointNavigation.GetDestinationPosition();
    break;
  }
  case kStateMsg_Deactivate:
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    break;
  }
}

bool CSpacePirate::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (mEnableRetreat && !mTrooper) {
    TUniqueId wpId = GetConnectedObject(mgr, kSS_Patrol, kSM_Follow);
    const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(wpId));
    if (!wp) {
      for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
           it != GetConnectionList().end(); ++it) {
        if (it->state == kSS_Retreat && it->msg == kSM_Follow) {
          TUniqueId id = mgr.GetIdForScript(it->objId);
          wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id));
          if (wp) {
            break;
          }
        }
      }
    }
    if (wp) {
      const_cast< CSpacePirate* >(this)->mDestObj = wpId;
      const_cast< CSpacePirate* >(this)->SetDestPos(wp->GetTranslation());
    } else {
      const_cast< CSpacePirate* >(this)->mDestObj = kInvalidUniqueId;
      const_cast< CSpacePirate* >(this)->SetDestPos(GetTranslation());
    }
    const_cast< CSpacePirate* >(this)->mEnableRetreat = false;
    result = true;
    const_cast< CSpacePirate* >(this)->mReflectedDestPos = GetTranslation();
    const_cast< CSpacePirate* >(this)->mInPosition = false;
    const_cast< CSpacePirate* >(this)->ReleaseCoverPoint(mgr, const_cast< CSpacePirate* >(this)->mCoverPoint, true);
    mHearNoise = false;
    const_cast< CSpacePirate* >(this)->mEnableAim = false;
    const_cast< CSpacePirate* >(this)->mHitByPlayerProjectile = false;
  }
  return result;
}

bool CSpacePirate::HasTargetingPoint(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  const CPlayer* player = TCastToConstPtr< CPlayer >(actor);
  if (player || !actor || !actor->GetActive()) {
    result = false;
    if (!player) {
      const_cast< CSpacePirate* >(this)->mTargetId = const_cast< CSpacePirate* >(this)->ChooseAttackTarget(mgr);
      const_cast< CSpacePirate* >(this)->SetTeamAiTarget(mgr);
      const_cast< CSpacePirate* >(this)->mBoneTracking.SetTarget(mTargetId);
    }
    float scale = 1.f;
    float margin = mPirateData.mSearchRadius * scale;
    CVector3f extent(margin, margin, margin);
    CAABox bounds(GetTranslation() - extent, GetTranslation() + extent);
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    mgr.BuildNearList(nearList, bounds,
                      CMaterialFilter::MakeExclude(CMaterialList(kMT_Unknown59)), nullptr);
    for (int i = 0; i < nearList.size(); ++i) {
      const CScriptTargetingPoint* const point =
          TCastToConstPtr< CScriptTargetingPoint >(mgr.GetObjectById(nearList[i]));
      if (point && point->GetActive() && point->GetCurrentAreaId() == GetCurrentAreaId() &&
          !point->GetLocked()) {
        result = true;
        const_cast< CSpacePirate* >(this)->mTargetId = point->GetUniqueId();
        const_cast< CSpacePirate* >(this)->SetTeamAiTarget(mgr);
        const_cast< CSpacePirate* >(this)->mBoneTracking.SetTarget(mTargetId);
        break;
      }
    }
  }
  return result;
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
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
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

void CSpacePirate::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mJumpPoint = kInvalidUniqueId;
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      mReflectedDestPos = GetTranslation();
      mInPosition = false;
      mDestObj = cp->GetUniqueId();
      mDestPos = cp->GetTranslation();
    }
    if (GetSearchPath()->Search(GetTranslation(), mDestPos) == CPathFindSearch::kR_Success) {
      mReflectedDestPos = GetTranslation();
      mDestPos = GetSearchPath()->GetPoint();
      mInPosition = false;
      BodyController()->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(mDestPos - GetTranslation(), CVector3f::Zero(), 1.f));
    } else {
      CScriptAiJumpPoint* best = nullptr;
      float minDistSq = 3.4028235e38f;
      CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
      for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
        if (CScriptAiJumpPoint* jp = TCastToPtr< CScriptAiJumpPoint >(list[i])) {
          if (jp->GetActive() && jp->GetType() == 0 && !jp->GetInUse(GetUniqueId()) &&
              jp->GetJumpTarget() == kInvalidUniqueId &&
              jp->GetCurrentAreaId() == GetCurrentAreaId()) {
            CVector3f toJump = jp->GetTranslation() - GetTranslation();
            float distSq = toJump.MagSquared();
            if (distSq > 25.f && CVector3f::Dot(jp->GetTransform().GetForward(), toJump) > 0.f) {
              if (const CScriptWaypoint* wp =
                      TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(jp->GetJumpPoint()))) {
                if ((mDestPos[kDZ] - GetTranslation().GetZ()) *
                        (wp->GetTranslation().GetZ() - jp->GetTranslation().GetZ()) >
                    0.f) {
                  distSq += 4.f * toJump.GetZ() * toJump.GetZ();
                  CVector3f toDest = mDestPos - wp->GetTranslation();
                  distSq += toDest.MagSquared() + 9.f * toDest.GetZ() * toDest.GetZ();
                  if (distSq < minDistSq &&
                      GetSearchPath()->PathExists(GetTranslation(), jp->GetTranslation()) ==
                          CPathFindSearch::kR_Success) {
                    bool good = false;
                    bool noPath = GetSearchPath()->PathExists(wp->GetTranslation(), mDestPos) !=
                                  CPathFindSearch::kR_Success;
                    if (noPath) {
                      distSq += 1000.f;
                    }
                    if (!noPath) {
                      good = true;
                    }
                    if (distSq < minDistSq) {
                      minDistSq = distSq;
                      best = jp;
                      if (good) {
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      if (best) {
        mDestPos = best->GetTranslation();
        if (GetSearchPath()->Search(GetTranslation(), mDestPos) == CPathFindSearch::kR_Success) {
          mReflectedDestPos = GetTranslation();
          mDestPos = GetSearchPath()->GetPoint();
          mInPosition = false;
          mJumpPoint = best->GetUniqueId();
          mJumpHeight = best->GetJumpApex();
          if (const CScriptWaypoint* wp =
                  TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(best->GetJumpPoint()))) {
            mPatrolDestPos = wp->GetTranslation();
            BodyController()->CommandMgr().DeliverCmd(
                CBCLocomotionCmd(mDestPos, CVector3f::Zero(), 1.f));
          }
        }
      }
    }
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    if (mEnableAim) {
      mSteeringSpeed = 1.f;
    }
    mInRange = false;
    mNormalDodge = true;
    break;
  case kStateMsg_Update:
    CPatterned::PathFind(mgr, msg, dt);
    BodyController()->CommandMgr().SetTargetVector(mgr.GetPlayer(0)->GetTranslation() -
                                                   GetTranslation());
    if (mJumpPoint != kInvalidUniqueId) {
      if (CScriptAiJumpPoint* jp = TCastToPtr< CScriptAiJumpPoint >(mgr.ObjectById(mJumpPoint))) {
        float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const CVector3f& scale = GetModelData()->GetScale();
        float jumpDistance = maxSpeed * ((1.5f * dt + 0.1f) * scale.GetY()) + mIntoJumpDist;
        if ((GetTranslation() - jp->GetTranslation()).MagSquared() <
            jumpDistance * jumpDistance) {
          mAnimationState.SetState(CAnimationState::kAS_Ready);
          if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
            BodyController()->CommandMgr().DeliverCmd(CBCJumpCmd(
                mDestPos, pas::kJT_Normal, pas::kJS_IntoJump, 0, CBCJumpCmd::kFF_AmbushJump));
          }
          x8fa_29_ = true;
        }
      }
    }
    AvoidActors(mgr);
    if (!mInRange) {
      if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
        float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const CVector3f& scale = GetModelData()->GetScale();
        mCoverRange = maxSpeed * ((1.5f * dt + 0.1f) * scale.GetY());
        if (cp->ShouldWallHang()) {
          mCoverRange += mIntoJumpDist;
        }
        mInRange =
            (GetTranslation() - cp->GetTranslation()).MagSquared() < mCoverRange * mCoverRange;
      }
    }
    UpdateCantSeePlayer(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    CPatterned::PathFind(mgr, msg, dt);
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mJumpPoint = kInvalidUniqueId;
    mInRange = false;
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    break;
  }
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

void CSpacePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  const TUniqueId sender = msg.GetSenderId();
  if (mInWallHang || mCeilingAmbush) {
    switch (message) {
    case kSM_Falling:
      if ((mInWallHang && BodyController()->GetCurrentStateId() == pas::kAS_WallHang &&
           !BodyController()->GetBodyStateInfo().GetCurrentState()->ApplyGravity()) ||
          (mCeilingAmbush &&
           (BodyController()->GetCurrentStateId() == pas::kAS_Locomotion ||
            (BodyController()->GetCurrentStateId() == pas::kAS_Jump &&
             !BodyController()->GetBodyStateInfo().GetCurrentState()->IsInAir(*BodyController()))))) {
        CPhysicsActor::Stop();
        SetMomentumWR(CVector3f::Zero());
        return;
      }
      break;
    case kSM_Landed:
      mTimeSinceHitByPlayer = 3.4028235e38f;
      break;
    default:
      break;
    }
  }
  switch (message) {
  case kSM_Alert:
  case kSM_Activate:
    if (GetActive()) {
      if (mOnlyAttackInRange) {
        mMayStartAttack = true;
      } else {
        mHitByPlayerProjectile = true;
      }
    } else if (mCeilingAmbush) {
      RemoveMaterial(kMT_GroundCollider, mgr);
      mOnGround = false;
    }
    if (message == kSM_Activate && mTrooper) {
      if (!GetActive() && xc04_ == -1) {
        mColor.SetAlpha(0.f);
      }
      x8fa_26_ = true;
    }
    break;
  default:
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_AreaLoaded: {
    for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
         it != GetConnectionList().end(); ++it) {
      if (it->state == kSS_Retreat && it->msg == kSM_Next) {
        TUniqueId id = mgr.GetIdForScript(it->objId);
        if (CScriptCoverPoint* cp = TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(id))) {
          cp->Reserve(GetUniqueId());
        }
      } else if (it->state == kSS_Patrol && it->msg == kSM_Follow) {
        mEnablePatrol = true;
      }
    }
    const TAreaId areaId = GetCurrentAreaId();
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(areaId).GetPostConstructed()->mPathArea);
    if (mFloatingCorpse) {
      mRagdollDelayTimer = 0.01f;
      RemoveMaterial(kMT_Character, kMT_Unknown59, kMT_Target, kMT_Orbit, mgr);
      mAlive = false;
      HealthInfo()->SetHP(-1.f);
    } else {
      SetEyeParticleActive(mgr, true);
    }
    break;
  }
  case kSM_Decrement:
    if (mRagDoll.get()) {
      mRagDoll->SetNoOverTimer(false);
      mRagDoll->SetContinueSmallMovements(false);
    }
    break;
  case kSM_Create: {
    if (mCeilingAmbush && mShadowPirate) {
      mColor.SetAlpha(mPirateData.mCloakOpacity);
      mAlphaDelta = -1.f;
    }
    xa24_ = mgr.Random()->Next() % 6;
    CMaterialList include = GetMaterialFilter().GetIncludeList();
    CMaterialList exclude = GetMaterialFilter().GetExcludeList();
    CMaterialList passthrough(kMT_AIPassthrough);
    include.Remove(passthrough);
    exclude.Add(passthrough);
    SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(include, exclude));
    break;
  }
  case kSM_SetToZero:
    if (GetActive()) {
      mEnableRetreat = true;
      StartWarpOut(mgr, false);
    }
    break;
  case kSM_Falling:
    if (BodyController()->GetPercentageFrozen() != 1.f) {
      float momentum = GetGravityConstant() * GetMass();
      if (mCeilingAmbush) {
        momentum *= 3.f;
      }
      SetMomentumWR(CVector3f(0.f, 0.f, -momentum));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {
      SetVelocityWR(CVector3f(0.f, 0.f, GetVelocityWR().GetZ()));
    }
    mBurstFire.SetBurstType(3);
    break;
  case kSM_Launching:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Hurled) {
      CPatterned::AcceptScriptMsg(mgr, CScriptMsg(sender, GetUniqueId(), kSM_Falling));
      SetMomentumWR(CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass()));
      SetVelocityForJump();
    }
    break;
  case kSM_Landed:
    if (!mOnlyAttackInRange) {
      mBurstFire.SetBurstType(1);
    } else {
      mBurstFire.SetBurstType(4);
    }
    mJumpVelSet = false;
    x8fa_29_ = false;
    if (mShadowPirate && GetVelocityWR().GetZ() < -1.f) {
      mAlphaDelta = 1.f;
      mCloakDelayTimer += -0.05f * GetVelocityWR().GetZ();
      mCloakDelayTimer = CMath::Clamp(0.f, mCloakDelayTimer, 1.f);
      mMaxCloakAlpha = 0.5f;
      if (mAlive) {
        mgr.ActorModelParticles()->StartElectric(*this);
        mElectricParticleTimer = 1.f + mCloakDelayTimer;
      }
    }
    break;
  case kSM_Action:
    if (CScriptTargetingPoint* point =
            TCastToPtr< CScriptTargetingPoint >(mgr.ObjectById(sender))) {
      if (point->GetActive()) {
        mBoneTracking.SetTarget(sender);
        mTargetId = sender;
        SetTeamAiTarget(mgr);
        mHitByPlayerProjectile = true;
      } else {
        mTargetId = ChooseAttackTarget(mgr);
        SetTeamAiTarget(mgr);
        mBoneTracking.SetTarget(mTargetId);
      }
      mAttackRemTime = 0.f;
    }
    break;
  case kSM_Deactivate:
  case kSM_Delete:
    SquadRemove(mgr);
    mChargePlayerList.remove(GetUniqueId());
    break;
  case kSM_Start:
    mStarted = false;
    break;
  case kSM_Stop:
    mStarted = true;
    break;
  case kSM_Escape:
    if (GetActive()) {
      StartWarpOut(mgr, true);
    }
    break;
  default:
    break;
  }
}

void CSpacePirate::SetEyeParticleActive(CStateManager& mgr, bool active) {}

TUniqueId CSpacePirate::ChooseAttackTarget(CStateManager& mgr) {
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      if (teamMgr->IsTeamMemberInRange(mgr, *this, 20.f)) {
        return teamMgr->FindBestIndividualAttackTarget(mgr, *this);
      }
    }
  }
  TUniqueId player = CScriptTeamAiMgr::ChoosePlayer(mgr, *this);
  return player;
}

void CSpacePirate::StartWarpOut(CStateManager& mgr, bool flag) {
  if (GetAlive() && !x8fa_25_ && xc04_ == -1 && mTrooper) {
    mStateMachine->SetState(mgr, *this, rstl::string_l("WarpOut"));
    x8fa_27_ = flag;
  }
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

void CSpacePirate::CheckBlade(CStateManager& mgr) {
  if (!mAppliedBladeDamage && mSwooshSeg != CSegId::Invalid()) {
    if (CPhysicsActor* actor = TCastToPtr< CPhysicsActor >(mgr.ObjectById(mTargetId))) {
      CTransform4f swoosh = GetLctrTransform(mSwooshSeg);
      const CVector3f& scale = GetModelData()->GetScale();
      CVector3f extent(scale.GetX() * 0.5f, scale.GetY() * 0.5f, scale.GetZ() * 0.5f);
      CAABox bounds(swoosh.GetTranslation() - extent, swoosh.GetTranslation() + extent);
      if (bounds.DoBoundsOverlap(actor->GetBoundingBox())) {
        mgr.ApplyDamage(
            GetUniqueId(), actor->GetUniqueId(), GetUniqueId(), mPirateData.mBladeDamage,
            CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), CMaterialList()),
            CVector3f::Zero());
        mAppliedBladeDamage = true;
      }
    }
  }
}

pas::EStepDirection CSpacePirate::GetStrafeDir(CStateManager& mgr, float dist) {
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  bool left = true;
  bool right = true;
  pas::EStepDirection result = pas::kSD_Invalid;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CSpacePirate* pirate = TCastToPtr< CSpacePirate >(const_cast< CEntity* >(list[i]));
    if (pirate && pirate != this && pirate->GetCurrentAreaId() == GetCurrentAreaId()) {
      CVector3f delta = pirate->GetTranslation() - GetTranslation();
      float deltaSq = delta.MagSquared();
      if (deltaSq < dist * dist) {
        float dot = CVector3f::Dot(delta, GetTransform().GetRight());
        if (dot > 0.866f * deltaSq || (dot > 0.f && deltaSq < 3.f)) {
          right = false;
        } else if (dot < 0.866f * -deltaSq || (dot < 0.f && deltaSq < 3.f)) {
          left = false;
        }
      }
    }
  }
  CVector3f center = GetBoundingBox().GetCenterPoint();
  CVector3f rightDir = GetTransform().GetRight();
  if (right) {
    right = mPathFindSearch.OnPath(center + dist * rightDir) == CPathFindSearch::kR_Success;
  }
  if (left) {
    left = mPathFindSearch.OnPath(center - dist * rightDir) == CPathFindSearch::kR_Success;
  }
  if (left || right) {
    CVector3f start = left ? center : center - dist * rightDir;
    bool both = false;
    if (left && right) {
      both = true;
    }
    float length = both ? 2.f * dist : dist;
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Unknown59));
    mgr.BuildNearList(nearList, start, rightDir, length, filter, this);
    if (left) {
      left = CGameCollision::RayDynamicLineOfSightTest(mgr, start, rightDir, dist, filter, nearList,
                                                        nullptr);
    }
    if (right) {
      right = CGameCollision::RayDynamicLineOfSightTest(mgr, center, rightDir, dist, filter,
                                                         nearList, nullptr);
    }
    if (left && right) {
      if ((mgr.Random()->Next() & 0x4000) != 0) {
        left = false;
      } else {
        right = false;
      }
    }
    if (left) {
      result = pas::kSD_Left;
    } else if (right) {
      result = pas::kSD_Right;
    }
  }
  return result;
}

bool CSpacePirate::CantJumpBack(CStateManager& mgr, const CVector3f& dir, float dist) {
  CVector3f center = GetBoundingBox().GetCenterPoint();
  bool result = false;
  if (mPathFindSearch.OnPath(center + dist * dir) != CPathFindSearch::kR_Success) {
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Unknown59));
    mgr.BuildNearList(nearList, center, dir, dist, filter, this);
    if (CGameCollision::RayDynamicLineOfSightTest(mgr, center, dir, dist, filter, nearList,
                                                   nullptr)) {
      result = true;
    }
  }
  const bool clear = result;
  return clear;
}

void CSpacePirate::AvoidActors(CStateManager& mgr) {
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (const CPatterned* ai = TCastToConstPtr< CPatterned >(list[i])) {
      if (ai != this && ai->GetCurrentAreaId() == GetCurrentAreaId()) {
        CVector3f separation =
            mSteeringBehaviors.Separation(*this, ai->GetTranslation(), mPirateData.mAvoidDistance);
        if (separation.IsMagnitudeSafe()) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(separation.AsNormalized(), CVector3f::Zero(), 0.5f));
          if (!mSteeringDelayTimer) {
            if (CSpacePirate* pirate =
                    TCastToPtr< CSpacePirate >(const_cast< CPatterned* >(ai))) {
              if (!pirate->mSteeringDelayTimer) {
                CVector3f delta = pirate->GetTranslation() - GetTranslation();
                if (CVector3f::Dot(GetTransform().GetForward(), delta) > 0.f &&
                    CVector3f::Dot(pirate->GetVelocityWR(), pirate->GetTransform().GetForward()) >
                        0.f) {
                  mSteeringDelayTimer = 1.f;
                }
              }
            }
          }
        }
      }
    }
  }
}

static CVector3f Random2f(CStateManager& mgr, float min, float max) {
  CVector3f result(mgr.Random()->Float() - 0.5f, mgr.Random()->Float() - 0.5f, 0.f);
  if (CMath::AbsF(result.GetX()) < 0.001f) {
    result.SetX(0.001f);
  }
  result.Normalize();
  result *= (max - min) * mgr.Random()->Float() + min;
  return result;
}

static CVector3f SpacePirateCross(const CVector3f& lhs, const CVector3f& rhs) {
  const float lX = lhs.GetX();
  const float lY = lhs.GetY();
  const float lZ = lhs.GetZ();
  const float rX = rhs.GetX();
  const float rY = rhs.GetY();
  const float rZ = rhs.GetZ();
  return CVector3f(lY * rZ - rY * lZ, lZ * rX - rZ * lX, lX * rY - rX * lY);
}

void CSpacePirate::UpdateCantSeePlayer(CStateManager& mgr, float dt) {
  xa80_ += dt;
  xa78_ += dt;
  if (xa80_ > 0.1f) {
    xa80_ = 0.f;
    CVector3f eyePos = GetTranslation() + CVector3f(0.f, 0.f, mEyeHeight);
    const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
    if (!player && !mgr.IsMultiplayer()) {
      player = mgr.GetPlayer(0);
    }
    if (player) {
      CVector3f aimPos = player->GetAimPosition(mgr, 0.f);
      if (GetCoverPoint(mgr, mCoverPoint)) {
        switch (mCoverDir) {
        case pas::kCD_Left:
          eyePos -= 2.f * GetTransform().GetRight();
          break;
        case pas::kCD_Right:
          eyePos += 2.f * GetTransform().GetRight();
          break;
        default:
          break;
        }
      } else {
        CVector3f toPlayer = (aimPos - eyePos).AsNormalized();
        eyePos += 1.1f * SpacePirateCross(toPlayer, CVector3f::Up());
      }
      if (!LineOfSightTest(mgr, eyePos, aimPos,
                           CMaterialList(kMT_Player, kMT_NoPlatformCollision))) {
        xa78_ = 0.f;
      }
    }
  }
  mNoPlayerLos = xa78_ < mPirateData.xc4_;
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

void CSpacePirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  if (mCeilingAmbush && mAlive) {
    return;
  }
  if (mRagDoll.get() || mAttachedActor != kInvalidUniqueId) {
    return;
  }
  mKnockBackController.EnableKnockBackPhysics(!mNoKnockbackImpulseReset);
  mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, IsOnGround());
  bool enableFreeze = true;
  if (IsIngPossessed() ||
      (mShadowPirate && !info.GetDamageInfo().GetWeaponMode().IsCharged() &&
       !info.GetDamageInfo().GetWeaponMode().IsComboed())) {
    enableFreeze = false;
  }
  mKnockBackController.SetEnableFreeze(enableFreeze);
  CPatterned::KnockBack(mgr, info);
  if (mShadowPirate) {
    if (mAlive) {
      if (info.GetDamageInfo().GetKnockBackPower(*GetDamageVulnerability(), 0.f) >= 4.f &&
          BodyController()->GetPercentageFrozen() != 1.f) {
        mAlphaDelta = 1.f;
        mCloakDelayTimer +=
            0.1f * info.GetDamageInfo().GetKnockBackPower(*GetDamageVulnerability(), 0.f);
        mCloakDelayTimer = CMath::Clamp(0.f, mCloakDelayTimer, 1.f);
        mMaxCloakAlpha = 0.5f;
        mgr.ActorModelParticles()->StartElectric(*this);
        mElectricParticleTimer = mCloakDelayTimer + 1.f;
      }
    } else {
      mMaxCloakAlpha = mAlphaDelta = 1.f;
      mMinCloakAlpha = 0.f;
      mgr.ActorModelParticles()->StartElectric(*this);
      mElectricParticleTimer = 2.f;
    }
  }
  if (mAlive) {
    switch (mKnockBackController.GetReaction()) {
    case CKnockBackMgr::kAR_Hurled:
      mStateMachine->SetState(mgr, *this, rstl::string_l("GetUpNow"));
      xc00_ = CSfxManager::AddEmitter(mPirateData.mSound_Hurled, GetTranslation(), 127,
                                      GetCurrentAreaId().Value(), false, false,
                                      CSfxManager::kMedPriority);
      break;
    }
  } else if (!mFloatingCorpse) {
    switch (mKnockBackController.GetReaction()) {
    case CKnockBackMgr::kAR_Hurled:
      if (mKnockBackController.GetFollowUp() != CKnockBackMgr::kFU_LaggedBurnDeath &&
          mKnockBackController.GetFollowUp() != CKnockBackMgr::kFU_BurnDeath &&
          mKnockBackController.GetFollowUp() != CKnockBackMgr::kFU_ExplodeDeath &&
          mKnockBackController.GetFollowUp() != CKnockBackMgr::kFU_IceDeath) {
        xc00_ = CSfxManager::AddEmitter(mPirateData.mSound_Death, GetTranslation(), 127,
                                        GetCurrentAreaId().Value(), false, false,
                                        CSfxManager::kMedPriority);
      }
      break;
    }
  }
}

CVector3f CSpacePirate::GetTargetPos(CStateManager& mgr) {
  const CActor* actor = static_cast< const CActor* >(mgr.GetObjectById(mTargetId));
  const CPlayer* player = TCastToConstPtr< CPlayer >(actor);
  if (!player) {
    if (actor && actor->GetActive()) {
      return actor->GetTranslation();
    }
    mTargetId = ChooseAttackTarget(mgr);
    SetTeamAiTarget(mgr);
    mBoneTracking.SetTarget(mTargetId);
    player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  } else {
    return player->GetTranslation();
  }
  return GetTranslation() + 10.f * GetTransform().GetForward();
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
      GetMaterialFilter().GetIncludeList().Union(CMaterialList(kMT_AIBlock)),
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

void CSpacePirate::UpdateLeashTimer(float dt) {
  if (BodyController()->GetPercentageFrozen() != 1.f && !BodyController()->IsElectrocuting()) {
    mLeashTimer += dt;
  }
}

void CSpacePirate::WarpIn(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mColor.SetAlpha(0.f);
    mAlphaDelta = 0.f;
    xc04_ = 0;
    x8fa_28_ = false;
    RemoveMaterial(kMT_Character, kMT_Unknown59, kMT_Target, kMT_Orbit, mgr);
    break;
  case kStateMsg_Update:
    switch (xc04_) {
    case 0: {
      rstl::vector< CScriptWaypoint* > waypoints;
      waypoints.reserve(GetConnectionList().size());
      for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
           it != GetConnectionList().end(); ++it) {
        if (it->state == kSS_GRNT && it->msg == kSM_Follow) {
          TUniqueId id = mgr.GetIdForScript(it->objId);
          if (CScriptWaypoint* wp =
                  TCastToPtr< CScriptWaypoint >(const_cast< CEntity* >(mgr.GetObjectById(id)))) {
            waypoints.push_back_unsafe(wp);
          }
        }
      }
      if (!waypoints.empty()) {
        int idx = mgr.Random()->Range(0, waypoints.size() - 1);
        SetTranslation(waypoints[idx]->GetTranslation());
        SetTransform(CQuaternion::FromMatrix(waypoints[idx]->GetTransform())
                         .BuildTransform4f(GetTranslation()));
      }
      rstl::reserved_vector< TUniqueId, 1024 > nearList;
      CMaterialFilter filter =
          CMaterialFilter::MakeInclude(CMaterialList(kMT_Unknown59, kMT_Player, kMT_Character));
      CVector3f extent(10.f, 10.f, 10.f);
      CAABox bounds(GetTranslation() - extent, GetTranslation() + extent);
      mgr.BuildNearList(nearList, bounds, filter, this);
      if (!CGameCollision::DetectDynamicCollisionBoolean(*GetCollisionPrimitive(), GetTransform(),
                                                         nearList, mgr)) {
        xc04_ = 1;
        AddMaterial(kMT_Character, kMT_Unknown59, kMT_Target, mgr);
      }
      break;
    }
    case 1:
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
        BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(0), -1));
      } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
        if (!x8fa_28_) {
          x8fa_28_ = true;
          xbd0_ = BodyController()->GetAnimTimeRemaining();
        } else if (xbd0_ > FLT_EPSILON) {
          mColor.SetAlpha(
              rstl::max_val(0.f, 1.f - BodyController()->GetAnimTimeRemaining() / xbd0_));
        }
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    AddMaterial(kMT_Character, kMT_Unknown59, kMT_Target, kMT_Orbit, mgr);
    mColor.SetAlpha(1.f);
    mAlphaDelta = 0.f;
    xc04_ = -1;
    x8fa_26_ = false;
    break;
  }
}

void CSpacePirate::WarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    xc04_ = 1;
    x8fa_28_ = false;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(1), -1));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      if (!x8fa_28_) {
        x8fa_28_ = true;
        xbd0_ = BodyController()->GetAnimTimeRemaining();
      } else if (xbd0_ > FLT_EPSILON) {
        mColor.SetAlpha(rstl::min_val(BodyController()->GetAnimTimeRemaining() / xbd0_, 1.f));
      }
    }
    break;
  case kStateMsg_Deactivate:
    mColor.SetAlpha(0.f);
    mAlphaDelta = 0.f;
    break;
  }
}

void CSpacePirate::PostWarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (!x8fa_26_) {
      mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), GetUniqueId(), kSM_Deactivate));
    }
    xc04_ = -1;
    if (x8fa_27_) {
      mgr.DeleteObjectRequest(GetUniqueId());
    }
    break;
  }
}

void CSpacePirate::LaunchGrenade(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mTeamAiMgrId == kInvalidUniqueId ||
        CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                      GetUniqueId())) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
      xc58_ = mgr.Random()->Range(1, mPirateData.mWeaponData.x58_);
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero, true));
    } else if (xc58_ <= 0) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mAttackRemTime = GetAverageAttackTime();
    if (mgr.IsRandomAvailable() == true) {
      float variation = mAttackTimeVariation;
      mAttackRemTime += variation * mgr.Random()->Float();
    }
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                GetUniqueId(), false);
    break;
  }
}

void CSpacePirate::Captured(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mDisabledAnimationDeltas = kADF_Translation | kADF_Rotation;
    Stop();
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(7), -1));
    RemoveMaterial(kMT_Orbit, kMT_Target, kMT_Unknown59, mgr);
    SetDrawShadow(false);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(7), -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::PathFindEx(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::PathFind(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Activate:
    mInRange = false;
    break;
  case kStateMsg_Update:
    AvoidActors(mgr);
    if (!mInRange) {
      if (const CScriptAiJumpPoint* jp =
              TCastToConstPtr< CScriptAiJumpPoint >(mgr.GetObjectById(mJumpPoint))) {
        float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const CVector3f& scale = GetModelData()->GetScale();
        mCoverRange = maxSpeed * ((1.5f * dt + 0.1f) * scale.GetY()) + mIntoJumpDist;
        mInRange =
            (GetTranslation() - jp->GetTranslation()).MagSquared() < mCoverRange * mCoverRange;
      }
    }
    break;
  case kStateMsg_Deactivate:
    mInRange = false;
    break;
  }
}

bool CSpacePirate::BounceFind(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  float minDistSq = 3.4028235e38f;
  CScriptAiJumpPoint* best = nullptr;
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (CScriptAiJumpPoint* jp = TCastToPtr< CScriptAiJumpPoint >(list[i])) {
      if (jp->GetActive() && !jp->GetInUse(GetUniqueId()) && jp->GetType() == 0 &&
          jp->GetJumpTarget() != kInvalidUniqueId && jp->GetCurrentAreaId() == GetCurrentAreaId()) {
        CVector3f toJump = jp->GetTranslation() - GetTranslation();
        float distSq = toJump.MagSquared();
        if (distSq < minDistSq && CVector3f::Dot(jp->GetTransform().GetForward(), toJump) > 0.f) {
          if (const CScriptWaypoint* wp =
                  TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(jp->GetJumpTarget()))) {
            CVector3f toDest = mDestPos - wp->GetTranslation();
            distSq += toDest.MagSquared() + 9.f * toDest.GetZ() * toDest.GetZ();
            if (distSq < minDistSq &&
                CVector3f::Dot(wp->GetTransform().GetForward(), toDest) > 0.f &&
                GetSearchPath()->PathExists(GetTranslation(), jp->GetTranslation()) ==
                    CPathFindSearch::kR_Success) {
              bool good = false;
              bool noPath = GetSearchPath()->PathExists(wp->GetTranslation(), mDestPos) !=
                            CPathFindSearch::kR_Success;
              if (noPath) {
                distSq += 1000.f;
              }
              if (!noPath) {
                good = true;
              }
              if (distSq < minDistSq) {
                minDistSq = distSq;
                best = jp;
                if (good) {
                  break;
                }
              }
            }
          }
        }
      }
    }
  }
  if (best) {
    if (const CScriptWaypoint* wp =
            TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(best->GetJumpPoint()))) {
      CSpacePirate* self = const_cast< CSpacePirate* >(this);
      self->SetDestPos(best->GetTranslation());
      result = true;
      self->mJumpPoint = best->GetUniqueId();
      self->mJumpHeight = best->GetJumpApex();
      self->mPatrolDestPos = wp->GetTranslation();
    }
  }
  return result;
}

bool CSpacePirate::AggressionCheck(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!mNonAggressive) {
    if (mAlwaysAggressive) {
      result = true;
    } else if (mChargePlayerList.empty() && mTimeNoPlayerLos > 10.f) {
      result = true;
    }
    if (result) {
      if (rstl::find< rstl::list< TUniqueId >::const_iterator, TUniqueId >(
              mChargePlayerList.begin(), mChargePlayerList.end(), GetUniqueId()) ==
          mChargePlayerList.end()) {
        mChargePlayerList.push_back(GetUniqueId());
      }
    }
  }
  return result;
}

bool CSpacePirate::CoverFind(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  float minDistSq = mPirateData.mSearchRadius * mPirateData.mSearchRadius;
  const CScriptCoverPoint* closest = nullptr;
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (const CScriptCoverPoint* cp = TCastToConstPtr< CScriptCoverPoint >(list[i])) {
      if (cp->GetActive() && !cp->ShouldLandHere() && !cp->GetInUse(GetUniqueId()) &&
          cp->GetCurrentAreaId() == GetCurrentAreaId() &&
          cp->GetUniqueId() != mPreviousCoverPoint) {
        float distSq = (GetTranslation() - cp->GetTranslation()).MagSquared();
        if (distSq < minDistSq &&
            !cp->Blown(const_cast< CSpacePirate* >(this)->GetTargetPos(mgr))) {
          minDistSq = distSq;
          closest = cp;
        }
      }
    }
  }
  if (closest) {
    CSpacePirate* self = const_cast< CSpacePirate* >(this);
    self->ReleaseCoverPoint(mgr, self->mCoverPoint, true);
    if (CScriptCoverPoint* cp =
            TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(closest->GetUniqueId()))) {
      self->SetCoverPoint(cp, self->mCoverPoint);
      result = true;
      self->mPreviousCoverPoint = mCoverPoint;
      self->mCoverPointRearDir = -closest->GetTransform().GetForward();
    }
  }
  return result;
}

bool CSpacePirate::CoverBlown(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  CVector3f target = const_cast< CSpacePirate* >(this)->GetTargetPos(mgr);
  CVector3f toTarget = target - GetTranslation();
  if (toTarget.MagSquared() > mMinAttackRange * mMinAttackRange) {
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      result = cp->Blown(target);
      if (!result && mSteeringSpeed == 0.f &&
          GetBodyController()->GetCurrentStateId() != pas::kAS_Step) {
        CVector3f toCover = cp->GetTranslation() - GetTranslation();
        if (toCover.MagSquared() > 3.f * GetModelData()->GetScale().GetY()) {
          result = true;
        }
      }
    }
  }
  return result;
}

bool CSpacePirate::CoverNearlyBlown(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
    result = false;
    if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId))) {
      CVector3f pos = player->GetTranslation() + 1.f * player->GetVelocityWR();
      result = cp->Blown(pos);
    }
  }
  return result;
}

bool CSpacePirate::CoveringFire(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CSpacePirate* pirate = TCastToPtr< CSpacePirate >(const_cast< CEntity* >(list[i]));
    if (pirate && pirate != this && pirate->mInAttackState &&
        pirate->GetCurrentAreaId() == GetCurrentAreaId()) {
      result = true;
    }
  }
  return result;
}

bool CSpacePirate::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  if (TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId))) {
    CVector3f target = const_cast< CSpacePirate* >(this)->GetTargetPos(mgr);
    int numCloserPirates = 0;
    float distSq = (GetTranslation() - target).MagSquared();
    const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
    for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
      CSpacePirate* pirate = TCastToPtr< CSpacePirate >(const_cast< CEntity* >(list[i]));
      if (pirate && pirate != this && pirate->mInAttackState && pirate->mAlive &&
          pirate->GetCurrentAreaId() == GetCurrentAreaId()) {
        if ((pirate->GetTranslation() - target).MagSquared() < distSq) {
          ++numCloserPirates;
          if (numCloserPirates > 3) {
            result = false;
          }
        }
      }
    }
  }
  return result;
}

void CSpacePirate::Jump(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mLeashTimer = 0.f;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
      mJumpVelSet = true;
      BodyController()->CommandMgr().DeliverCmd(CBCJumpCmd(
          mDestPos, pas::kJT_Normal, pas::kJS_Loop, 0, CBCJumpCmd::kFF_AmbushJump));
    }
    UpdateLeashTimer(dt);
    break;
  case kStateMsg_Deactivate:
    mJumpVelSet = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::Dead(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Activate:
    mBoneTracking.SetActive(false);
    SetEyeParticleActive(mgr, false);
    SquadReset(mgr);
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) {
      RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
      RemoveMaterial(kMT_GroundCollider, kMT_Unknown59, kMT_AIBlock, mgr);
      AddMaterial(kMT_NoPlatformCollision, mgr);
      SetMomentumWR(CVector3f::Zero());
      CPhysicsActor::Stop();
    }
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CSpacePirate::Bounce(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (const CScriptAiJumpPoint* jp =
            TCastToConstPtr< CScriptAiJumpPoint >(mgr.GetObjectById(mJumpPoint))) {
      TUniqueId target = jp->GetJumpTarget();
      if (const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(target))) {
        CBodyStateCmdMgr& cmdMgr = BodyController()->CommandMgr();
        cmdMgr.DeliverCmd(CBCJumpCmd(mPatrolDestPos, wp->GetTranslation()));
      }
    }
    break;
  case kStateMsg_Update:
    if (mStateMachine->GetTime() > 0.1f &&
        BodyController()->GetCurrentStateId() != pas::kAS_Jump) {
      static_cast< TStateMachineState< CPatterned >& >(*mStateMachine).SetCodeTrigger();
    }
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

class CValidWaypointPredicate : public CValidEntityPredicate {
public:
  ~CValidWaypointPredicate() {}
  bool IsValid(const CStateManager& mgr, TUniqueId id) const {
    return TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id)) != nullptr;
  }
};

void CSpacePirate::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mInWallHang = true;
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      if (const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(
              cp->CheckConnectedObject_if(mgr, kSS_Arrived, kSM_Next, CValidWaypointPredicate())))) {
        mDestObj = wp->GetUniqueId();
        mDestPos = wp->GetTranslation();
        mReflectedDestPos = GetTranslation();
        mInPosition = false;
      }
      mTargetDelta = cp->GetTransform().GetForward();
    }
    mInAttackState = true;
    mBoneTracking.SetActive(false);
    x8fa_30_ = true;
    break;
  case kStateMsg_Update: {
    bool canHang = true;
    if (mAnimationState.GetState() == CAnimationState::kAS_Ready) {
      if (CVector3f::GetAngleDiff(GetTransform().GetForward(), mTargetDelta) > 0.2617994f) {
        canHang = false;
        BodyController()->CommandMgr().DeliverCmd(
            CBCLocomotionCmd(CVector3f::Zero(), mTargetDelta, 1.f));
      }
    }
    if (canHang && mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_WallHang)) {
      BodyController()->CommandMgr().DeliverCmd(CBCWallHangCmd(mDestObj));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_WallHang) {
      x8fa_30_ = !BodyController()->GetBodyStateInfo().GetCurrentState()->CanShoot();
    }
    mBurstFire.SetBurstType(1);
    break;
  }
  case kStateMsg_Deactivate:
    mInWallHang = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mInAttackState = false;
    mBoneTracking.SetActive(true);
    x8fa_30_ = false;
    break;
  }
}

void CSpacePirate::WallDetach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mInWallHang = true;
    x8fa_30_ = true;
    x8fa_31_ = true;
    break;
  case kStateMsg_Update:
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    break;
  case kStateMsg_Deactivate:
    mInWallHang = false;
    x8fa_30_ = false;
    x8fa_31_ = false;
    break;
  }
}

bool CSpacePirate::ShouldStrafe(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  bool noPlayerStrafe = false;
  const_cast< CSpacePirate* >(this)->mSkidDir = pas::kSD_Invalid;
  if (!mNonAggressive) {
    CVector3f toTarget = const_cast< CSpacePirate* >(this)->GetTargetPos(mgr) - GetTranslation();
    if (CVector3f::Dot(toTarget, GetTransform().GetForward()) > 0.f) {
      if ((mLowHealthFrenzyTimer < 0.66f || mTimeSinceHitByPlayer < 0.66f) &&
          mStrafeDelayTimer == 0.f) {
        CVector3f center = GetBoundingBox().GetCenterPoint();
        const CVector3f& delta =
            (const_cast< CSpacePirate* >(this)->GetTargetPos(mgr) - center).AsNormalized();
        if (CVector3f::Dot(delta, GetTransform().GetForward()) > 0.707f) {
          const_cast< CSpacePirate* >(this)->mSkidDir =
              const_cast< CSpacePirate* >(this)->GetStrafeDir(mgr, 10.f);
          if (mSkidDir != pas::kSD_Invalid) {
            result = true;
          } else {
            noPlayerStrafe = true;
          }
        }
      }
      if (!noPlayerStrafe && !result && mTimeNoPlayerLos > 1.f) {
        if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId))) {
          if ((player->GetTranslation() - GetTranslation()).Magnitude() < 15.f &&
              mSkidDir == pas::kSD_Invalid) {
            const_cast< CSpacePirate* >(this)->mSkidDir =
                const_cast< CSpacePirate* >(this)->GetStrafeDir(mgr, 5.f);
            if (mSkidDir != pas::kSD_Invalid) {
              result = true;
            }
          }
        }
      }
    }
  }
  return result;
}

void CSpacePirate::Crouch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      mTargetDelta = cp->GetTransform().GetForward();
    }
    mSteeringSpeed = 0.f;
    mCoverDir = pas::kCD_Invalid;
    break;
  case kStateMsg_Update:
    BodyController()->CommandMgr().SetTargetVector(mTargetDelta);
    UpdateCantSeePlayer(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CSpacePirate::Skid(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mStrafeDelayTimer = 4.f;
    mInAttackState = true;
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Step) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mSkidDir, pas::kStep_Dodge));
    }
    break;
  case kStateMsg_Deactivate:
    mInAttackState = false;
    break;
  }
}

bool CSpacePirate::SpotPlayer(CStateManager& mgr, const CTriggerData& data) const {
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    CVector3f toPlayer = mgr.GetPlayer(i)->GetTranslation() - GetTranslation();
    float distance = toPlayer.Magnitude();
    if (CVector3f::Dot(toPlayer, GetTransform().GetForward()) > distance * mDetectionAngle) {
      return true;
    }
  }
  return false;
}

void CSpacePirate::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mEnableBreakDodge = false;
    if (!mNormalDodge && !mNoBreakDodge && mDodgeDelayTimer <= 0.f) {
      float chance = 0.15f * (1.f + (4.f * (mInitialHP - HealthInfo()->GetHP())) / mInitialHP);
      if (mgr.Random()->Float() < chance) {
        mEnableBreakDodge = true;
      }
      mDodgeDelayTimer = mgr.Random()->Range(mPirateData.mDodgeDelayTimeMin,
                                             mPirateData.mDodgeDelayTimeMax);
    }
    mDodgeDir = GetStrafeDir(mgr, mEnableBreakDodge ? mBreakDodgeDist : mDodgeDist);
    if (mDodgeDir != pas::kSD_Invalid) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    }
    break;
  case kStateMsg_Update:
    if (!mEnableBreakDodge) {
      if (mNormalDodge || mgr.Random()->Float() < 0.5f) {
        if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
          BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mDodgeDir, pas::kStep_Dodge));
        }
      } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
        BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mDodgeDir, pas::kStep_RollDodge));
      }
    } else {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
        BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mDodgeDir, pas::kStep_BreakDodge));
      }
      if (GetMaterialList().HasMaterial(kMT_Orbit) && mStateMachine->GetTime() > 0.5f) {
        RemoveMaterial(kMT_Orbit, mgr);
        mgr.GetPlayer(0)->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource,
                                                   mgr);
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mNoPlayerDodge = true;
    if (!GetMaterialList().HasMaterial(kMT_Orbit)) {
      AddMaterial(kMT_Orbit, mgr);
    }
    break;
  }
}

bool CSpacePirate::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (mEnableDodge) {
    if (!mNonAggressive && !mNoPlayerDodge) {
      CVector3f toTarget = const_cast< CSpacePirate* >(this)->GetTargetPos(mgr) - GetTranslation();
      if (CVector3f::Dot(toTarget, GetTransform().GetForward()) > 0.f &&
          (mTimeSinceHitByPlayer < 0.33f || mLowHealthFrenzyTimer < 0.33f) &&
          mTimeNoPlayerLos < 0.5f) {
        result = true;
      }
    }
    if (!result) {
      if (const CMetroidAlpha* metroid =
              TCastToConstPtr< CMetroidAlpha >(mgr.GetObjectById(mTargetId))) {
        if (metroid->IsAttacking()) {
          CVector3f delta = GetTranslation() - metroid->GetTranslation();
          if (CVector3f::Dot(delta, metroid->GetTransform().GetForward()) > 0.f) {
            result = true;
          }
        }
      }
    }
  }
  return result;
}

void CSpacePirate::Shuffle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    CVector3f target = GetTargetPos(mgr);
    if (!mNoShuffleCloseCheck && TooClose(mgr, CTriggerData(0.f))) {
      SetDestPos(GetTranslation() + mMinAttackRange * (GetTranslation() - target).AsNormalized() +
                 Random2f(mgr, 0.f, 5.f));
      mDestObj = kInvalidUniqueId;
      mShuffleClose = true;
    } else {
      CVector3f fromTarget = GetTranslation() - target;
      CVector3f side = SpacePirateCross(CVector3f::Up(), fromTarget);
      float range = mMaxAttackRange;
      float distance = range * mgr.Random()->Float() + range;
      range = mMaxAttackRange;
      float sideDistance = 2.f * range * (mgr.Random()->Float() - 0.5f);
      SetDestPos(target + distance * fromTarget.AsNormalized() + sideDistance * side.AsNormalized());
      mDestObj = kInvalidUniqueId;
      mShuffleClose = false;
    }
    mSteeringSpeed = 1.f;
    break;
  }
  }
  CPatterned::PathFind(mgr, msg, dt);
  BodyController()->CommandMgr().SetTargetVector(mgr.GetPlayer(0)->GetTranslation() -
                                                 GetTranslation());
  switch (msg) {
  case kStateMsg_Update:
    AvoidActors(mgr);
    break;
  case kStateMsg_Deactivate:
    mShuffleClose = false;
    break;
  }
}

void CSpacePirate::TurnAround(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xbc4_ = GetTargetPos(mgr);
    CVector3f delta = xbc4_ - GetTranslation();
    delta.SetZ(0.f);
    if (CVector3f::Dot(GetTransform().GetForward(), delta.AsNormalized()) < 0.8f) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    }
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
      CVector3f delta = xbc4_ - GetTranslation();
      if (delta.IsMagnitudeSafe()) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCLocomotionCmd(CVector3f::Zero(), delta.AsNormalized(), 1.f));
      } else {
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
    }
    UpdateCantSeePlayer(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mEnableAim = true;
    BodyController()->AbortScriptedAnimations();
    SquadAdd(mgr);
    if (mTargetId == kInvalidUniqueId) {
      mTargetId = ChooseAttackTarget(mgr);
      SetTeamAiTarget(mgr);
    }
    mBoneTracking.SetActive(true);
    mBoneTracking.SetTarget(mTargetId);
    if (BodyController()->HasBodyState(pas::kAS_Taunt)) {
      if (!mShadowPirate) {
        bool findOtherPirate = true;
        if (mMelee) {
          const CPASAnimParmData parms(pas::kAS_Taunt, CPASAnimParm::FromEnum(2));
          const CPASDatabase& db = BodyController()->GetPASDatabase();
          const rstl::pair< float, int > anim = db.FindBestAnimation(parms, *mgr.Random(), -1);
          if (anim.first > 0.f) {
            findOtherPirate = false;
            mTaunt = pas::kTT_Two;
          }
        }
        if (findOtherPirate) {
          bool withOtherPirate = false;
          const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
          for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
            CSpacePirate* pirate = TCastToPtr< CSpacePirate >(const_cast< CEntity* >(list[i]));
            if (pirate && pirate != this && !pirate->mEnableAim && pirate->mAlive &&
                pirate->GetCurrentAreaId() == GetCurrentAreaId()) {
              if ((pirate->GetTranslation() - GetTranslation()).MagSquared() <
                  mPirateData.mHearingRadius * mPirateData.mHearingRadius) {
                withOtherPirate = true;
              }
            }
          }
          mTaunt = withOtherPirate ? pas::kTT_Zero : pas::kTT_One;
        }
      } else {
        mTaunt = mAlertBeforeCloak ? pas::kTT_One : pas::kTT_Zero;
      }
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    } else {
      CSfxManager::AddEmitter(mPirateData.mSound_Alert, GetTranslation(),
                              GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(mTaunt));
    }
    break;
  case kStateMsg_Deactivate:
    if (mTaunt == pas::kTT_Zero) {
      mgr.InformListeners(GetTranslation(), kLNT_PlayerFire);
    }
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    ReleaseCoverPoint(mgr, mCoverPoint, true);
    mSteeringSpeed = 0.f;
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    mNoPlayerLos = true;
    mTimeNoPlayerLos = 0.f;
    float aggression = mPirateData.mAggressionCheck;
    mAlwaysAggressive = mgr.Random()->Range(0.f, 100.f) < aggression;
    float cover = mPirateData.mCoverCheck;
    mCoverCheck = mgr.Random()->Range(0.f, 100.f) < cover;
    float dodge = mPirateData.mDodgeCheck;
    mEnableDodge = mgr.Random()->Range(0.f, 100.f) < dodge;
    mEnableAim = true;
    BodyController()->AbortScriptedAnimations();
    if (mTargetId == kInvalidUniqueId) {
      mTargetId = ChooseAttackTarget(mgr);
      SetTeamAiTarget(mgr);
      mBoneTracking.SetActive(true);
      mBoneTracking.SetTarget(mTargetId);
    }
    if (mOnlyAttackInRange) {
      mBurstFire.SetBurstType(4);
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    mNormalDodge = false;
    break;
  }
  case kStateMsg_Update:
    if (BodyController()->HasBodyState(pas::kAS_Turn)) {
      if (mAnimationState.GetState() != CAnimationState::kAS_NotReady &&
          mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
        CVector3f delta = xbc4_ - GetTranslation();
        if (delta.IsMagnitudeSafe()) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(CVector3f::Zero(), delta.AsNormalized(), 1.f));
        }
      }
      if (mAnimationState.GetState() != CAnimationState::kAS_Repeat) {
        xbc4_ = GetTargetPos(mgr);
        CVector3f delta = xbc4_ - GetTranslation();
        delta.SetZ(0.f);
        if (CVector3f::Dot(GetTransform().GetForward(), delta.AsNormalized()) < 0.9f) {
          mAnimationState.SetState(CAnimationState::kAS_Ready);
        }
      }
    }
    if (mSeated && mSatUp) {
      if (mAttackRemTime > GetAverageAttackTime() &&
          BodyController()->GetLocomotionType() == pas::kLT_Combat) {
        BodyController()->SetLocomotionType(pas::kLT_Internal5);
      } else if (mAttackRemTime < 0.5f * GetAverageAttackTime() &&
                 BodyController()->GetLocomotionType() == pas::kLT_Internal5) {
        BodyController()->SetLocomotionType(pas::kLT_Combat);
      }
    }
    UpdateCantSeePlayer(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mAlwaysAggressive = false;
    mNoPlayerDodge = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::GetUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    SquadReset(mgr);
    mLeashTimer = 0.f;
    x8fa_25_ = true;
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() == pas::kAS_LieOnGround &&
        mPathFindSearch.Search(GetTranslation(), GetTranslation()) ==
            CPathFindSearch::kR_NoSourcePoint) {
      mPendingDeath = true;
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Getup)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGetupCmd(pas::kGetup_Zero));
    }
    UpdateLeashTimer(dt);
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x8fa_25_ = false;
    break;
  }
}

bool CSpacePirate::HearPlayer(CStateManager& mgr, const CTriggerData& data) const {
  bool heard = false;
  xbc0_ = (xbc0_ + 1) % mgr.GetNumPlayers();
  if (xbc0_ != -1) {
    const CPlayer* player = mgr.GetPlayer(xbc0_);
    if (player->GetVelocityWR().MagSquared() > 0.1f) {
      CVector3f delta = player->GetTranslation() - GetTranslation();
      if (delta.MagSquared() < mPirateData.mHearingRadius * mPirateData.mHearingRadius) {
        heard = true;
      }
    }
  }
  return heard;
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

void CSpacePirate::DoubleSnap(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (!mNoMeleeAttack) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    }
    xbc4_ = GetTargetPos(mgr);
    mTargetDelta = xbc4_ - GetTranslation();
    mSteeringSpeed = 0.f;
    mEnableMeleeAttack = true;
    mMeleeSeverity = pas::kS_One;
    mAppliedBladeDamage = false;
    mInAttackState = true;
    mCloseMelee = false;
    mChargePlayerList.remove(GetUniqueId());
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(mMeleeSeverity));
    }
    if (mMeleeSeverity == pas::kS_One &&
        mAnimationState.GetState() == CAnimationState::kAS_Over) {
      CVector3f delta = GetTargetPos(mgr) - GetTranslation();
      if (delta.MagSquared() < mMinAttackRange * mMinAttackRange &&
          CVector3f::Dot(delta.AsNormalized(), GetTransform().GetForward()) > -0.123f) {
        mAnimationState.SetState(CAnimationState::kAS_Ready);
        mMeleeSeverity = pas::kS_Two;
        mAppliedBladeDamage = false;
        mTargetDelta = delta;
        mCloseMelee = true;
      }
    }
    if (mCloseMelee) {
      mTargetDelta = GetTargetPos(mgr) - GetTranslation();
    }
    BodyController()->CommandMgr().SetTargetVector(mTargetDelta);
    if (mShadowPirate) {
      if (mAnimationState.GetState() == CAnimationState::kAS_Over) {
        mAlphaDelta = -0.4f;
      } else {
        mAlphaDelta = 1.f;
        mMaxCloakAlpha = 0.75f;
      }
    }
    UpdateCantSeePlayer(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    CheckBlade(mgr);
    break;
  case kStateMsg_Deactivate:
    mEnableMeleeAttack = false;
    mInAttackState = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
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

void CSpacePirate::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    xbc4_ = GetTargetPos(mgr);
    mTargetDelta = xbc4_ - GetBoundingBox().GetCenterPoint();
    mSteeringSpeed = 0.f;
    mEnableMeleeAttack = false;
    if (!mNoMeleeAttack && TooClose(mgr, CTriggerData(0.f))) {
      mEnableMeleeAttack = true;
      mAppliedBladeDamage = false;
    }
    if (CVector3f::Dot(GetTransform().GetForward(), mTargetDelta.AsNormalized()) < 0.8f) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(CVector3f::Zero(), mTargetDelta, 1.f));
    }
    mInAttackState = true;
    mMaxCloakAlpha = 0.75f;
    break;
  case kStateMsg_Update:
    if (mEnableMeleeAttack) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
        BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_One));
      }
      BodyController()->CommandMgr().SetTargetVector(mTargetDelta);
      CheckBlade(mgr);
      if (mShadowPirate) {
        if (mAnimationState.GetState() == CAnimationState::kAS_Over) {
          mAlphaDelta = -0.4f;
        } else {
          mAlphaDelta = 1.f;
          mMaxCloakAlpha = 0.75f;
        }
      }
    }
    UpdateCantSeePlayer(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mEnableMeleeAttack = false;
    mInAttackState = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::Cover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Cover) {
      if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
        mCoverDir = static_cast< pas::ECoverDirection >(
            (static_cast< uint >(cp->GetAttackDirection()) >> 1) & 1);
        mAnimationState.SetState(CAnimationState::kAS_Ready);
        mDestPos = cp->GetTranslation();
        if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Cover)) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCCoverCmd(mCoverDir, cp->GetTranslation(), -cp->GetTransform().GetForward()));
        }
      }
    }
    break;
  case kStateMsg_Update:
    if (CScriptCoverPoint* cp = GetCoverPoint(mgr, mCoverPoint)) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Cover)) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCCoverCmd(mCoverDir, cp->GetTranslation(), -cp->GetTransform().GetForward()));
      }
      BodyController()->CommandMgr().SetTargetVector(-cp->GetTransform().GetForward());
    }
    UpdateCantSeePlayer(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
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
    UpdateCantSeePlayer(mgr, dt);
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

void CSpacePirate::JumpBack(CStateManager& mgr, EStateMsg msg, float dt) {
  if (!ShouldJumpBack(mgr, CTriggerData(0.f))) {
    return;
  }
  switch (msg) {
  case kStateMsg_Activate:
    if (!mOnlyAttackInRange && !CantJumpBack(mgr, -GetTransform().GetForward(), 5.f)) {
      float height = GetSearchPath()->GetCharacterHeight();
      mPathFindSearch.SetCharacterHeight(5.f + height);
      CVector3f dest = GetTranslation() + 10.f * GetTransform().GetForward();
      if (GetSearchPath()->Search(GetTranslation(), dest) == CPathFindSearch::kR_Success &&
          (GetSearchPath()->GetWaypoints().back() - dest).MagSquared() < 3.f &&
          CMath::AbsF(GetSearchPath()->RemainingPathDistance(GetTranslation()) - 10.f) < 4.f) {
        mPatrolDestPos = GetSearchPath()->GetWaypoints().back();
        mJumpHeight = 5.f;
        mUseJumpBackJump = true;
        mAnimationState.SetState(CAnimationState::kAS_Ready);
      }
      GetSearchPath()->SetCharacterHeight(height);
    }
    break;
  case kStateMsg_Update:
    if (!mUseJumpBackJump) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(pas::kSD_Backward, pas::kStep_Normal));
      BodyController()->CommandMgr().SetTargetVector(GetTargetPos(mgr) - GetTranslation());
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCJumpCmd(mDestPos, pas::kJT_Normal, pas::kJS_IntoJump, 0, CBCJumpCmd::kFF_AmbushJump));
    }
    break;
  case kStateMsg_Deactivate:
    if (mUseJumpBackJump) {
      mAnimationState.SetState(CAnimationState::kAS_NotReady);
      mUseJumpBackJump = false;
    }
    mHoldPositionTime = 0.f;
    break;
  }
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

void CSpacePirate::LaunchGrenadeProjectile(CStateManager& mgr) {
  --xc58_;
  const CTransform4f locator = GetLctrTransform(mGunSeg);
  const CVector3f origin = locator.GetTranslation();
  float angle = 0.f;
  float speed = mPirateData.mWeaponData.x5c_;
  if (const CEntity* target = mgr.GetObjectById(mTargetId)) {
    const CVector3f targetPos =
        GetGrenadeTargetPosition(mgr, const_cast< CActor* >(static_cast< const CActor* >(target)));
    ComputeLaunchSpeedAndAngle(targetPos, origin, angle, speed);
    CVector3f dist = targetPos - origin;
    dist.SetZ(0.f);
    const CVector3f forward = locator.GetForward();
    CVector3f direction = dist.CanBeNormalized() ? dist.AsNormalized() : forward;
    float maxAngle = M_PIF / 4.f;
    if (CVector3f::GetAngleDiff(forward, direction) > maxAngle) {
      direction = CVector3f::Slerp(forward, direction, CRelAngle::FromRadians(maxAngle));
    }
    const CVector3f look = CVector3f::Slerp(direction, CVector3f::Up(), CRelAngle::FromRadians(angle));
    const CTransform4f xf = CTransform4f::LookAt(origin, origin + look, CVector3f::Up());
    CEntity* grenade = rs_new CBouncyGrenade(
        mgr.AllocateUniqueId(), rstl::string_l("Bouncy Grenade"),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId), xf,
        CModelData::CModelDataNull(), CActorParameters::None(), GetUniqueId(), speed,
        mPirateData.mWeaponData.mGrenadeData, 5.f, CAABox::MakeMaxInvertedBox(), kInvalidUniqueId,
        0.f, 0, nullptr, nullptr);
    if (grenade) {
      mgr.AddObject(grenade);
    }
  }
}

bool CSpacePirate::FireProjectile(float dt, CStateManager& mgr) {
  bool ret = false;
  const CTransform4f gunXf = GetLctrTransform(mGunSeg);
  if (!GetAlive()) {
    LaunchProjectile(gunXf, mgr, 6, 0, false, CImpactVisorEffect(), CVector3f(1.f, 1.f, 1.f));
    ret = true;
  } else if (const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId))) {
    bool inTurret = false;
    CVector3f targetPos = target->GetTranslation();
    if (const CPlayer* player = TCastToConstPtr< CPlayer >(target)) {
      if (player->GetTurretState() == CPlayer::kTS_Active) {
        inTurret = true;
        targetPos = player->GetTranslation();
      } else {
        targetPos = ProjectileInfo()->PredictInterceptPos(
            gunXf.GetTranslation(), player->GetAimPosition(mgr, 0.f), *player, true, dt);
      }
    }
    CVector3f dir = targetPos - gunXf.GetTranslation();
    const float distance = dir.Magnitude();
    dir *= 1.f / distance;
    const float dot = CVector3f::Dot((GetLctrTransform(mWristSeg).GetTranslation() -
                                      GetLctrTransform(mElbowSeg).GetTranslation())
                                         .AsNormalized(),
                                     dir);
    if ((dot > 0.707f || (distance < 6.f && dot > 0.5f)) &&
        (inTurret || LineOfSightTest(mgr, gunXf.GetTranslation(), targetPos,
                                     CMaterialList(kMT_Player, kMT_NoPlatformCollision)))) {
      targetPos += GetTransform().Rotate(mBurstFire.GetDistanceCompensatedError(distance, 6.f));
      const CTransform4f xf =
          CTransform4f::LookAt(gunXf.GetTranslation(), targetPos, CVector3f::Up());
      LaunchProjectile(xf, mgr, 6, 0, false, CImpactVisorEffect(), CVector3f(1.f, 1.f, 1.f));
      ret = true;
    }
  }
  if (ret) {
    CSfxManager::AddEmitter(mPirateData.mSound_Projectile, GetTranslation(),
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  const bool result = ret;
  return result;
}

void CSpacePirate::ComputeLaunchSpeedAndAngle(const CVector3f& target, const CVector3f& origin,
                                              float& angleOut, float& speedOut) const {
  const float height = target.GetZ() - origin.GetZ();
  float bestError = 3.4028235e38f;
  float angle = 0.f;
  float speed = mPirateData.mWeaponData.x5c_;
  CVector2f distXY = CVector2f(target.GetX() - origin.GetX(), target.GetY() - origin.GetY());
  float distance = distXY.Magnitude();
  float minSpeedSq = mPirateData.mWeaponData.x5c_ * mPirateData.mWeaponData.x5c_;
  float startAngle = 0.f;
  float angleStep = M_PIF / 40.f;
  float maxSpeedSq = mPirateData.mWeaponData.x60_ * mPirateData.mWeaponData.x60_;
  if (target.GetZ() > origin.GetZ()) {
    angleStep = -angleStep;
    startAngle = M_PIF / 4.f;
  }
  const float halfGravity = distance * (0.5f * kDefaultGravityAccel * distance);
  for (float i = 0.f; i < 10.f; i += 1.f) {
    float candidateAngle = angleStep * i + startAngle;
    float cosine = CMath::FastCosR(candidateAngle);
    float sine = CMath::FastSinR(candidateAngle);
    float divisor = distance * (cosine * sine) - height * (cosine * cosine);
    if (divisor > FLT_EPSILON) {
      float speedSq = halfGravity / divisor;
      if (speedSq >= minSpeedSq && speedSq <= maxSpeedSq) {
        angle = candidateAngle;
        speed = CMath::SqrtF(speedSq);
        break;
      }
      float error = speedSq > maxSpeedSq ? speedSq - maxSpeedSq : minSpeedSq - speedSq;
      if (error < bestError) {
        angle = candidateAngle;
        speed = CMath::SqrtF(speedSq);
        bestError = error;
      }
    }
  }
  angleOut = angle;
  speedOut = speed;
}

CVector3f CSpacePirate::GetGrenadeTargetPosition(const CStateManager& mgr, CActor* target) const {
  CVector3f pos = target->GetAimPosition(mgr, 0.5f);
  if (CPlayer* player = TCastToPtr< CPlayer >(target)) {
    if (player->GetMorphballTransitionState() == CPlayer::kMS_Unmorphed) {
      pos -= CVector3f(0.f, 0.f, 0.5f * player->GetEyeHeight());
    }
  }
  if (mPirateData.mWeaponData.mGrenadeData.GetNumBounces() != 0) {
    if (pos.GetZ() <= GetTranslation().GetZ() + 2.f) {
      pos = GetTranslation() + (pos - GetTranslation()) * 0.7f;
    }
  }
  return pos;
}

void CSpacePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  bool handled = false;
  switch (type) {
  case kUE_BeginAction:
    RemoveMaterial(kMT_Unknown59, mgr);
    mAllEnergyDrained = true;
    handled = true;
    break;
  case kUE_EndAction:
    mCloseMelee = false;
    handled = true;
    break;
  case kUE_DeGenerate:
  case kUE_BecomeRagDoll:
    if (mOnlyAttackInRange || GetHealthInfo()->GetHP() <= 0.f) {
      mRagdollDelayTimer = mgr.Random()->Float() * 0.05f + 0.001f;
    }
    handled = true;
    break;
  case kUE_IkLock:
    if (!mIkChain.GetActive()) {
      const CSegId& bone =
          GetModelData()->GetAnimationData()->GetLocatorSegId(node.GetLocatorName());
      if (bone.val() != 0) {
        CTransform4f xf = GetLctrTransform(bone);
        mIkChain.Activate(*GetModelData()->GetAnimationData(), bone, xf);
        mSatUp = true;
      }
    }
    handled = true;
    break;
  case kUE_IkRelease:
    mIkChain.Deactivate();
    handled = true;
    break;
  case kUE_ScreenShake:
    SendScriptMsgs(kSS_Play, mgr, kInvalidUniqueId, kSM_None);
    handled = true;
    break;
  case kUE_FadeOut:
    if (mShadowPirate) {
      mAlphaDelta = -0.8f;
      mgr.ActorModelParticles()->StartElectric(*this);
      mElectricParticleTimer = 1.f;
    }
    handled = true;
    break;
  case kUE_Projectile:
    LaunchGrenadeProjectile(mgr);
    handled = true;
    break;
  case kUE_BreakLockOn:
    if (GetAlive()) {
      handled = true;
    }
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

void CSpacePirate::PreRenderAllViewports(CStateManager& mgr) {
  if (mRagDoll.get() && mRagDoll->IsPrimed()) {
    mRagDoll->PreRenderAllViewports(*this, 0.2f);
    UpdatePortalSystemState(mgr);
  } else {
    CPatterned::PreRenderAllViewports(mgr);
  }
}

void CSpacePirate::PreRender(CStateManager& mgr) {
  if (mRagDoll.get() && mRagDoll->IsPrimed()) {
    mRagDoll->PreRender(GetTranslation(), *ModelData());
  }
  CPatterned::PreRender(mgr);
  if (!mRagDoll.get() || !mRagDoll->IsPrimed()) {
    mBoneTracking.PreRender(mgr, *ModelData()->AnimationData(), GetTransform(),
                            ModelData()->GetScale(), *BodyController());
    mIkChain.PreRender(*ModelData()->AnimationData(), GetTransform(), ModelData()->GetScale());
  }
  if (CMath::AbsF(xc5c_.GetConstant()) > 0.f) {
    SetModelFlags(CModelFlags(GetModelFlags(), GetModelFlags().GetOtherFlags() | 0x80));
  }
}

void CSpacePirate::Render(const CStateManager& mgr) const {
  float time = GetAlive() ? CGraphics::GetSecondsMod900() : 0.f;
  if (xc04_ == 1) {
    mgr.DrawSpaceWarp(GetBoundingBox().GetCenterPoint(),
                      CMath::FastSinR(M_PIF * mColor.GetAlpha()));
  }
  CTimeProvider provider(time);
  if (CMath::AbsF(xc5c_.GetConstant()) > 0.f) {
    GetModelData()->SetupWorldSpacePortalPlane(GetTransform(), xc5c_);
  }
  CPatterned::Render(mgr);
  RenderGrenadeLauncher(mgr, GetTransform(), GetModelFlags());
}

void CSpacePirate::RenderGrenadeLauncher(const CStateManager& mgr, const CTransform4f& xf,
                                         const CModelFlags& flags) const {
  int alpha = GetRenderAlphaBufferAlpha(mgr);
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
  if (GetModelAlphau8(mgr) && mGrenadeLauncherModel) {
    CTransform4f launcherXf = xf * GetScaledLocatorTransform(mWristSeg);
    mGrenadeLauncherModel->Render(mgr, launcherXf, GetActorLights(), flags);
  }
  if (alpha != -1) {
    gpRender->DisableDestinationAlpha();
  }
}

CAABox CSpacePirate::GetSortingBounds(const CStateManager& mgr) const {
  CAABox bounds = GetModelData()->GetBounds(GetTransform());
  CVector3f center = bounds.GetCenterPoint();
  CVector3f radius = (bounds.GetMaxPoint() - bounds.GetMinPoint()) * 0.25f;
  return CAABox(center - radius, center + radius);
}

CAABox CSpacePirate::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox bounds = CAABox::MakeMaxInvertedBox();
  bounds = GetModelData()->GetAnimationData()->CalcBoundingBoxFromModelVerts();
  bounds = bounds.GetTransformedAABox(CTransform4f::Translate(-GetTranslation()) * GetTransform() *
                                      CTransform4f::Scale(GetModelData()->GetScale()));
  return bounds;
}

void CSpacePirate::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                   const CModelFlags& flags) const {
  if (!GetModelData()->IsNull()) {
    GetModelData()->Render(CModelData::kWM_Normal, xf, nullptr, flags);
  }
  RenderGrenadeLauncher(mgr, xf, flags);
}

bool CSpacePirate::ShouldFrenzy(CStateManager& mgr) {
  bool reset = false;
  if (mPendingFrenzyChance) {
    mPendingFrenzyChance = false;
    if (mgr.Random()->Next() % 100 < 25) {
      reset = true;
    }
  }
  if (!mChargePlayerList.empty()) {
    reset = true;
  }
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  if (player && player->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    reset = true;
  }
  if (GetHealthInfo()->GetHP() < 0.3f * mInitialHP && mgr.Random()->Next() % 100 < 60 &&
      mLowHealthFrenzyTimer < 0.5f) {
    reset = true;
  }
  if (reset) {
    mFrenzyFrames = mgr.Random()->Range(2, 4);
  }
  return --mFrenzyFrames >= 0;
}

void CSpacePirate::UpdateCloak(float dt, CStateManager& mgr) {
  if (mShadowPirate) {
    if (mAlive) {
      if (mCloakDelayTimer > 0.f) {
        mCloakDelayTimer -= dt;
        if (mCloakDelayTimer <= 0.f) {
          mAlphaDelta = -0.4f;
        }
      }
    } else {
      mMinCloakAlpha = 0.f;
      mMaxCloakAlpha = 1.f;
    }
    if (mElectricParticleTimer > 0.f) {
      mElectricParticleTimer -= dt;
      if (mElectricParticleTimer <= 0.f && !BodyController()->IsElectrocuting()) {
        mgr.ActorModelParticles()->StopElectric(*this);
      }
    }
    if (BodyController()->GetPercentageFrozen() != 1.f) {
      mAlphaDelta = 2.f;
    }
    if (mAlphaDelta < 0.f && mColor.GetAlpha() < mMinCloakAlpha) {
      mColor.SetAlpha(mMinCloakAlpha);
      mAlphaDelta = 0.f;
      RemoveMaterial(kMT_Target, mgr);
    }
    if (mAlphaDelta > 0.f && mColor.GetAlpha() > mMaxCloakAlpha) {
      mColor.SetAlpha(mMaxCloakAlpha);
      AddMaterial(kMT_Target, mgr);
    }
    mCloakStepTime -= dt;
    if (mCloakStepTime < 0.f) {
      float random = mgr.Random()->Float();
      mCloakStepTime = (1.f - random) * 0.08f;
      if (mAlphaDelta < 0.f) {
        mShadowPirateAlpha = mColor.GetAlpha();
        if (mAlive) {
          mShadowPirateAlpha -= random * (mColor.GetAlpha() - mMinCloakAlpha);
        }
      } else if (mAlphaDelta > 0.f) {
        mShadowPirateAlpha = mColor.GetAlpha() + random * (mMaxCloakAlpha - mColor.GetAlpha());
      } else {
        mShadowPirateAlpha = mColor.GetAlpha();
      }
    }
  }
}

void CSpacePirate::UpdateSfxEmitter() {
  if (xc00_) {
    if (CSfxManager::IsPlaying(xc00_) || CSfxManager::IsQueued(xc00_)) {
      CSfxManager::UpdateEmitter(xc00_, GetTranslation(), GetTransform().GetForward(), 127);
    } else {
      xc00_ = CSfxHandle();
    }
  }
}

void CSpacePirate::UpdateAttacks(float dt, CStateManager& mgr) {
  switch (mPirateData.mWeaponData.mEquippedWeapon) {
  case 0: {
    bool reset = true;
    if ((!mAlive || (BodyController()->GetBodyStateInfo().GetCurrentState()->CanShoot() &&
                     mEnableAim && !BodyController()->IsFrozen() && !mMelee &&
                     !mCeilingAmbush && !mStarted && !BodyController()->IsElectrocuting())) &&
        mBurstFire.GetBurstType() != -1) {
      const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
      if (mAlive && (!mOnlyAttackInRange ||
                     (player && (player->GetTranslation() - GetTranslation()).MagSquared() <
                                    mLeashRadius * mLeashRadius))) {
        reset = false;
        mAttackRemTime -= dt;
        if (mAttackRemTime < 0.f) {
          const CTeamAiRole* role =
              CScriptTeamAiMgr::GetTeamAiRole(mgr, mTeamAiMgrId, GetUniqueId());
          if ((!role || role->GetTeamAiRole() == CTeamAiRole::kTAR_Projectile) &&
              (mTeamAiMgrId == kInvalidUniqueId ||
               CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                             GetUniqueId()))) {
            if (ShouldFrenzy(mgr)) {
              mBurstFire.SetBurstType(2);
            }
            if (mSeated) {
              mBurstFire.SetBurstType(5);
            }
            if (player) {
              CVector3f toPirate = GetTranslation() - player->GetTranslation();
              if (CVector3f::Dot(player->GetTransform().GetForward(), toPirate) < 0.f &&
                  mBurstFire.GetBurstType() < 6) {
                mBurstFire.SetBurstType(mBurstFire.GetBurstType() + 6);
              }
            }
            mBurstFire.Start(mgr);
            mAttackRemTime = mgr.Random()->Float() * mAttackTimeVariation + GetAverageAttackTime();
            if (player) {
              const CVector3f& fromPlayer =
                  (GetGunEyePos() - player->GetAimPosition(mgr, 0.f)).AsNormalized();
              const CVector3f& forward = player->GetTransform().GetForward();
              if (CVector3f::Dot(fromPlayer, forward) < 0.9f) {
                const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
                for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
                  CSpacePirate* pirate = TCastToPtr< CSpacePirate >(const_cast< CEntity* >(list[i]));
                  if (pirate && pirate != this && pirate->mEnableAim &&
                      pirate->GetCurrentAreaId() == GetCurrentAreaId()) {
                    mAttackRemTime += 0.2f;
                  }
                }
              }
            }
          }
        }
      }
      mBurstFire.Update(mgr, dt);
      if (mBurstFire.ShouldFire()) {
        if (player && player->IsSidewaysDashing() && mgr.Random()->Float() < 0.5f) {
          mBurstFire.SetAvoidAccuracy(true);
        }
        FireProjectile(dt, mgr);
        mBurstFire.SetAvoidAccuracy(false);
        if (IsIngPossessed()) {
          float variation = mPirateData.x98_;
          float average = mPirateData.x94_;
          mBurstFire.SetTimeToNextShot(variation * (mgr.Random()->Float() - 0.5f) + average);
        } else {
          float variation = mPirateData.mNextShotTimeVariation;
          float average = mPirateData.mAverageNextShotTime;
          mBurstFire.SetTimeToNextShot(variation * (mgr.Random()->Float() - 0.5f) + average);
        }
      } else if (!mBurstFire.IsBurstSet()) {
        reset = true;
      }
    }
    if (reset) {
      SquadReset(mgr);
    }
    for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
      SetValidTarget(i, CheckTargetable(mgr));
    }
    break;
  }
  case 1:
    mAttackRemTime -= dt;
    break;
  }
}

void CSpacePirate::UpdateAimBodyState(float dt, CStateManager& mgr) {
  if (mAlive && mEnableAim && !BodyController()->IsFrozen() &&
      !BodyController()->IsElectrocuting() && !mMelee && !mRagDoll.get() &&
      (!mSeated || mSatUp) && !x8fa_30_) {
    mAimDelayTimer = rstl::max_val(mAimDelayTimer - dt, 0.f);
    if (mAimDelayTimer <= 0.f) {
      BodyController()->CommandMgr().DeliverCmd(CBCAdditiveAimCmd(mInWallHang));
      CTransform4f gunXf = GetLctrTransform(mGunSeg);
      CVector3f offset = mInWallHang ? CVector3f::Zero() : GetTranslation() - gunXf.GetTranslation();
      CVector3f targetPos = GetTargetPos(mgr);
      CVector3f dir = GetTransform().TransposeRotate(
          CVector3f(targetPos.GetX() + offset.GetX(), targetPos.GetY() + offset.GetY(),
                    targetPos.GetZ() + 0.f) -
          GetTranslation());
      if (mInWallHang) {
        dir.SetX(-dir.GetX());
        dir.SetY(-dir.GetY());
      }
      BodyController()->CommandMgr().SetAdditiveTargetVector(dir);
      xb9c_ = 0.5f;
    }
  } else if (xb9c_ > 0.f) {
    xb9c_ -= dt;
    BodyController()->CommandMgr().SetAdditiveTargetVector(GetTransform().GetForward());
    if (xb9c_ <= 0.f) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AdditiveIdle));
    }
  }
}

TUniqueId CSpacePirate::UpdateTarget(CStateManager& mgr) {
  float closestDist = 10.f;
  float targetDist = 0.f;
  TUniqueId closest = kInvalidUniqueId;
  TUniqueId result = mTargetId;
  bool chose = false;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    CPlayer* player = mgr.GetPlayer(i);
    if (player->GetUniqueId() == mTargetId) {
      if (!mgr.GetPlayerState(i)->IsPlayerAlive()) {
        chose = true;
        result = ChooseAttackTarget(mgr);
        break;
      }
      targetDist = (player->GetTranslation() - GetTranslation()).Magnitude();
    } else {
      float dist = (player->GetTranslation() - GetTranslation()).Magnitude();
      if (dist < closestDist) {
        closestDist = dist;
        closest = mgr.GetPlayer(i)->GetUniqueId();
      }
    }
  }
  if (targetDist > 30.f && closestDist < 10.f) {
    result = closest;
  }
  if (!chose && mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      if (teamMgr->IsTeamMemberInRange(mgr, *this, 50.f)) {
        return mTargetId;
      }
    }
  }
  return result;
}

void CSpacePirate::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EchoEmitter()->SetBounds(CAABox(GetTranslation(), GetTranslation()));
  if (!BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
  bool inCineCam = false;
  if (!mgr.IsMultiplayer() && mgr.GetCameraManager(0)->IsInCinematicCamera()) {
    inCineCam = true;
  }
  if (inCineCam && !mPrevInCineCam) {
    SetCinematicCollision(mgr);
  } else if (!inCineCam && mPrevInCineCam && !mRagdollNoAiCollision) {
    SetNonCinematicCollision(mgr);
  }
  mPrevInCineCam = inCineCam;
  float steeringSpeed = mSteeringDelayTimer ? 0.f : mSteeringSpeed;
  BodyController()->CommandMgr().SetSteeringSpeedRange(steeringSpeed, steeringSpeed);
  mUnkTimer = rstl::max_val(mUnkTimer - dt, 0.f);
  if (mAlive) {
    mTimeSinceHitByPlayer += dt;
    mLowHealthFrenzyTimer += dt;
    if (mInProjectilePath) {
      mLowHealthFrenzyTimer = 0.f;
      mInProjectilePath = false;
    }
    if (mHitByPlayerProjectile) {
      mTimeSinceHitByPlayer = 0.f;
      mHitByPlayerProjectile = false;
    }
  }
  UpdateCloak(dt, mgr);
  UpdateSfxEmitter();
  if (BodyController()->GetPercentageFrozen() < 1.f) {
    if (mAlive) {
      mSteeringDelayTimer = rstl::max_val(mSteeringDelayTimer - dt, 0.f);
      if (mNoPlayerLos) {
        mTimeNoPlayerLos += dt;
      } else {
        mTimeNoPlayerLos = 0.f;
      }
      mStrafeDelayTimer = rstl::max_val(mStrafeDelayTimer - dt, 0.f);
      mDodgeDelayTimer = rstl::max_val(mDodgeDelayTimer - dt, 0.f);
      CheckForProjectiles(mgr);
      if (mEnableAim) {
        mTargetId = UpdateTarget(mgr);
        SetTeamAiTarget(mgr);
      }
    }
    UpdateAttacks(dt, mgr);
    UpdateAimBodyState(dt, mgr);
    mIkChain.Update(dt);
  }
  bool noRagDoll = mRagDoll.null();
  if (noRagDoll || (!mFloatingCorpse && !mRagDoll->IsPrimed())) {
    CPatterned::Think(dt, mgr);
    if (BodyController()->GetPercentageFrozen() != 1.f) {
      mBoneTracking.Think(dt);
    }
  } else {
    CActor::Think(dt, mgr);
    UpdateAlphaDelta(mgr, dt);
    UpdateHitDamageTime(dt);
    UpdateIngPossession(dt);
    if (BodyController()->IsFrozen()) {
      BodyController()->UnFreeze();
    }
  }
  if (!noRagDoll) {
    if (!mRagDoll->IsPrimed()) {
      mRagDoll->Prime(mgr, GetTransform(), *ModelData());
      CVector3f translation = GetTranslation();
      SetTransform(CTransform4f::Identity());
      SetTranslation(translation);
      BodyController()->SetPlaybackRate(0.f);
    } else {
      float waterTop = -1.7014117e38f;
      if (InFluidId() != kInvalidUniqueId) {
        if (const CScriptWater* water =
                TCastToConstPtr< CScriptWater >(mgr.GetObjectById(InFluidId()))) {
          if (water->GetActive()) {
            waterTop = water->GetTriggerBoundsWR().GetMaxPoint().GetZ();
          }
        }
      }
      mRagDoll->Update(mgr, dt * GetDeathTimeScale(), waterTop);
      ModelData()->AdvanceParticles(GetTransform(), dt, mgr);
    }
    if (mRagDoll->IsOver() && !mRagDoll->WillContinueSmallMovements()) {
      SetMomentumWR(CVector3f::Zero());
      CPhysicsActor::Stop();
      if (!mFadeToDeath) {
        mFadeToDeath = true;
        mAlphaDelta = -1.f / 3.f;
        mAllEnergyDrained = true;
      }
    }
  }
  if (mRagdollDelayTimer > 0.f) {
    mRagdollDelayTimer -= dt;
    if (mRagdollDelayTimer <= 0.f) {
      if (mRagDoll.null()) {
        rstl::reserved_vector< float, 14 > radii;
        for (int i = 0; i < ARRAY_SIZE(skRadii); ++i) {
          radii.push_back(skRadii[i]);
        }
        mRagDoll = rs_new CPirateRagDoll(
            mgr, this, mPirateData.mSound_Impact,
            (mFloatingCorpse ? 3 : 0) | (mRagdollNoAiCollision ? 4 : 0), skGravityConstant, -3.f,
            radii);
        RemoveMaterial(kMT_Orbit, kMT_Target, mgr);
      }
      mRagdollDelayTimer = 0.f;
    }
  }
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
