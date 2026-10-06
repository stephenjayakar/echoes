#include "MetroidPrime/Enemies/CElitePirate.hpp"

#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CRumbleManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CAnimRes.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "rstl/math.hpp"
#include <float.h>
#include "MetroidPrime/Enemies/CElitePirateGrenadeLauncher.hpp"
#include "MetroidPrime/Enemies/CStateMachine.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/CShockWave.hpp"

#include "REL/REL_Setup.h"

#include <math.h>

const CElitePirate::SJointInfo CElitePirate::skLeftArmJointList[3] = {
    {"L_shoulder", "L_elbow", 1.f, 1.5f},
    {"L_elbow", "L_wrist", 1.1f, 1.3f},
    {"L_knee", "L_ankle", 0.9f, 1.2f},
};
const CElitePirate::SJointInfo CElitePirate::skRightArmJointList[3] = {
    {"R_shoulder", "R_elbow", 1.f, 1.5f},
    {"R_elbow", "R_wrist", 1.1f, 1.3f},
    {"R_knee", "R_ankle", 0.9f, 1.2f},
};
const CElitePirate::SSphereJointInfo CElitePirate::skSphereJointList[8] = {
    {"Head_1", 1.2f},  {"L_Palm_LCTR", 1.5f}, {"R_Palm_LCTR", 1.5f}, {"Spine_1", 1.8f},
    {"Collar", 1.5f},  {"L_ball", 0.8f},      {"R_ball", 0.8f},      {"Grenade_Collision_LCTR", 1.f},
};
const char* const CElitePirate::skpHeadLCTR = "Head_1";
const char* const CElitePirate::skpLauncherLCTR = "grenadeLauncher_LCTR";
const char* const CElitePirate::skpRightClawLCTR = "R_Palm_LCTR";
const char* const CElitePirate::skpLeftClawLCTR = "L_Palm_LCTR";
const char* const CElitePirate::skpGrenadeLauncherLCTR = "lockon_target_LCTR";
const char* const CElitePirate::skpRightBallLCTR = "R_ball";
const CVector3f CElitePirate::skExtendedClawBounds(2.f, 2.f, 2.f);
const CVector3f CElitePirate::skLocalShieldBounds(4.f, 4.f, 2.f);

CElitePirateData::CElitePirateData(
    CAssetId stateMachine, int initialAnim, const CDamageInfo& meleeDamage, CAssetId darkShield,
    ushort darkShieldSound, CAssetId darkShieldPop, CAssetId lightShield, float maxMeleeRange,
    float minShockwaveRange, float maxShockwaveRange, float minRocketRange, float maxRocketRange,
    float meleeChance, float shockwaveChance, float tauntInterval, ushort lightShieldSound,
    CAssetId lightShieldPop, float tauntVariance, float meleeWeight, float shockwaveWeight,
    float rocketWeight, float doubleShockwaveWeight, float repeatedAttackChance,
    float energyAttractionForce, CAssetId energyAbsorbEffect, ushort energyAbsorbSound,
    const CActorParameters& launcherActParams, const CAnimationParameters& launcherAnimParams,
    const CAnimationParameters& possessedLauncherAnimParams, CAssetId rocket,
    const CDamageInfo& rocketDamage, int minRocketCount, int maxRocketCount,
    CAssetId visorElectricEffect, ushort visorElectricSound,
    const SLdrShockWaveInfo& singleShockWave, const SLdrShockWaveInfo& doubleShockWave,
    CAssetId shieldedModel, CAssetId shieldedSkinRules)
: mStateMachine(stateMachine)
, mInitialAnim(initialAnim)
, mMeleeDamage(meleeDamage)
, mTauntInterval(tauntInterval)
, mTauntVariance(tauntVariance)
, mRepeatedAttackChance(repeatedAttackChance)
, mEnergyAttractionForce(energyAttractionForce)
, mEnergyAbsorbEffect(energyAbsorbEffect)
, mEnergyAbsorbSound(energyAbsorbSound)
, mLauncherActParams(launcherActParams)
, mLauncherAnimParams(launcherAnimParams)
, mPossessedLauncherAnimParams(possessedLauncherAnimParams)
, mMinRocketCount(minRocketCount)
, mMaxRocketCount(maxRocketCount)
, mVisorElectricEffect(visorElectricEffect)
, mVisorElectricSound(visorElectricSound)
, mMeleeChance(meleeChance)
, mShockwaveChance(shockwaveChance)
, mDarkShield(darkShield)
, mDarkShieldSound(darkShieldSound)
, mDarkShieldPop(darkShieldPop)
, mLightShield(lightShield)
, mLightShieldSound(lightShieldSound)
, mLightShieldPop(lightShieldPop)
, mSingleShockWave(singleShockWave)
, mDoubleShockWave(doubleShockWave)
, mMeleeWeight(meleeWeight)
, mShockwaveWeight(shockwaveWeight)
, mRocketWeight(rocketWeight)
, mDoubleShockwaveWeight(doubleShockwaveWeight)
, mMaxMeleeRange(maxMeleeRange)
, mMinShockwaveRange(minShockwaveRange)
, mMaxShockwaveRange(maxShockwaveRange)
, mMinRocketRange(minRocketRange)
, mMaxRocketRange(maxRocketRange)
, mRocket(rocket)
, mRocketDamage(rocketDamage)
, mShieldedModel(shieldedModel)
, mShieldedSkinRules(shieldedSkinRules) {}

CElitePirate::CElitePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& modelData,
                           const CPatternedInfo& pInfo, const CActorParameters& actParms,
                           const CElitePirateData& data)
: CPatterned(static_cast< EPatternedAI >(0xc), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, x7c0_(-1)
, mVulnerability(pInfo.GetDamageVulnerability())
, mShieldCollisionMgr(nullptr)
, mLeftClawPos(CVector3f::Zero())
, mRightClawPos(CVector3f::Zero())
, mPathDestination(CVector3f::Zero())
, mAlertPos(CVector3f::Zero())
, mData(data)
, mCollisionActorMgr(nullptr)
, mCollisionAabb(GetBoundingBox(), GetMaterialList())
, mEnergyAbsorbDesc(data.GetEnergyAbsorbEffect() != kInvalidAssetId
                        ? rstl::optional_object< TLockedToken< CGenDescription > >(
                              TLockedToken< CGenDescription >(gpSimplePool->GetObj(
                                  SObjectTag('PART', data.GetEnergyAbsorbEffect()))))
                        : rstl::optional_object< TLockedToken< CGenDescription > >())
, mCollisionHeadId(kInvalidUniqueId)
, mLauncherId(kInvalidUniqueId)
, mShieldCollisionId(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, mInitialSpeed(mSpeed)
, mSteeringSpeed(1.f)
, mHp(0.f)
, mAttackTimer(0.f)
, mShotAtTimer(0.f)
, mTime(0.f)
, mStuckTime(0.f)
, mLastObstacleTime(0.f)
, mClaimedRegion(-1)
, mPathFindSearch(nullptr, pInfo.GetIngPossessionData().isAnEncounter ? 0x201 : 1,
                  pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mTargetDestPos(CVector3f::Zero())
, mPositionHistory(5.f)
, mDamageOn(false)
, mShotAt(false)
, mAlert(false)
, mAlerted(false)
, mReturnedToPatrol(false)
, mLauncherPossessed(false)
, mInvulnAlert(false)
, xc28_(-1)
, mCurrentAction(kA_Invalid)
, mStateMachineToken(TToken< CStateMachine >(
      gpSimplePool->GetObj(SObjectTag('FSM2', data.GetStateMachine()))))
, mAlertTauntType(6)
, mLocomotionType(pas::kLT_Relaxed)
, mPoweredUp(false)
, mMeleeSeverity(1)
, mNextMeleeSeverity(1)
, mLastMeleeTime(-1000.f)
, mMeleeDamageOn(false)
, mMeleeKnockBack(false)
, mAttackType(kAT_Shockwave)
, mLastAttackType(7)
, mTurnDirection(CVector3f::Zero())
, mAttackTarget(CVector3f::Zero())
, mRocketInfo(data.GetRocket(), data.GetRocketDamage())
, mRocketsFired(0)
, mRocketsToFire(0)
, mFireMode(0)
, mNextFireMode(1)
, mBreakProjectileAttack(false)
, mAimBlend(0.f)
, mAimPos(CVector3f::Zero())
, mTauntType(0)
, mAngryCount(0)
, mShockwaveIsNext(false)
, mAngryAttackOver(false)
, mAngryChoice(false)
, mAngryChosen(false)
, mInvulnAlpha(0.f)
, mInvulnerable(false) {
  mRocketInfo.Token().Lock();
  mKnockBackController.EnableAllAnimReactions(false);
  mKnockBackController.EnableBurnDeath(false);
  mKnockBackController.EnableExplodeDeath(false);
  mKnockBackController.EnableBurn(false);
  mKnockBackController.EnableSlow(false);
  mKnockBackController.EnableFreeze(false);
  mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
  SetupPathFindSearch();
  if (IsInitialAnimLocomotion(pas::kLT_Internal10) == true) {
    mLocomotionType = pas::kLT_Internal10;
  } else if (IsInitialAnimLocomotion(pas::kLT_Internal11) == true) {
    mLocomotionType = pas::kLT_Internal11;
  } else if (IsInitialAnimLocomotion(pas::kLT_Internal12) == true) {
    mLocomotionType = pas::kLT_Internal12;
  }
  if (mData.GetShieldedModel() != kInvalidAssetId) {
    mShield.mModel = TLockedToken< CSkinnedModel >(rs_new CSkinnedModel(
        TLockedToken< CModel >(gpSimplePool->GetObj(SObjectTag('CMDL', mData.GetShieldedModel()))),
        TLockedToken< CSkinRules >(
            gpSimplePool->GetObj(SObjectTag('CSKR', mData.GetShieldedSkinRules()))),
        GetAnimationData()->GetModelData()->GetLayoutInfo()));
  }
}

CElitePirate::~CElitePirate() {}

bool CElitePirate::IsInitialAnimLocomotion(int type) const {
  const CPASDatabase& db = GetAnimationData()->GetCharacterInfo().GetPASDatabase();
  const rstl::pair< float, int > best = db.FindBestAnimation(
      CPASAnimParmData(pas::kAS_Locomotion, CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(type),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter()),
      -1);
  return best.second == mData.GetInitialAnim();
}

const CStateMachine* CElitePirate::GetStateMachine() const {
  if (!mStateMachineToken->IsLoaded()) {
    return nullptr;
  }
  return mStateMachineToken->NonConstCopy().GetT();
}


#define TRIGGER(name)                                                                              \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CElitePirate::name) }
#define STATE(name)                                                                                \
  { #name, static_cast< CPatterned::StateMachine::StateFunc >(&CElitePirate::name) }
#define CODE(name)                                                                                 \
  { #name, static_cast< CPatterned::StateMachine::CodeFunc >(&CElitePirate::name) }

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    TRIGGER(Alerted),          TRIGGER(AngryAttackOver),   TRIGGER(AttackPatternOver),
    TRIGGER(BreakProjectileAttack), TRIGGER(CanShockwave), TRIGGER(ClearLineOfSight),
    TRIGGER(DonePursuing),     TRIGGER(DoneTurning),       TRIGGER(HasAttackPattern),
    TRIGGER(InDetectionRange), TRIGGER(InPosition),        TRIGGER(NotReachedTarget),
    TRIGGER(PickedSpreadShot), TRIGGER(PlayerInNoAttack),  TRIGGER(PoweredDown),
    TRIGGER(PoweredUp),        TRIGGER(ReadyToCharge),     TRIGGER(ReturnedToPatrol),
    TRIGGER(ShieldKilled),     TRIGGER(ShockwaveIsNext),   TRIGGER(ShotAt),
    TRIGGER(ShouldAlert),      TRIGGER(ShouldFire),        TRIGGER(ShouldMeleeAttack),
    TRIGGER(ShouldShockwave),  TRIGGER(ShouldTurn),        TRIGGER(SpotPlayer),
    TRIGGER(StillAngry),       TRIGGER(TargetNotOnMesh),   TRIGGER(TargetUnreachable),
    TRIGGER(TooClose),
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    STATE(Alert),        STATE(AngryAttackBegin), STATE(AngryAttackEnd), STATE(Dead),
    STATE(FollowAttackPattern), STATE(InvulnAlert), STATE(MeleeAttack), STATE(Patrol),
    STATE(PowerDown),    STATE(PowerUp),          STATE(ProjectileAttack), STATE(Pursue),
    STATE(Shielding),    STATE(ShieldUp),         STATE(Shockwave),      STATE(SpreadShot),
    STATE(Stunned),      STATE(TargetPatrol),     STATE(Taunt),          STATE(Turn),
    STATE(Wait),
};

static CPatterned::StateMachine::SCodeFunction skCodes[] = {
    CODE(SelectTarget),
    CODE(PickAttackType),
};

void CElitePirate::SetupStateMachineFunctions(CStateManager& mgr) {
  InitializeStateMachine(mgr);
  mStateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  mStateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  mStateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

void CElitePirate::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  if (!mStateMachine->HasState()) {
    SetupStateMachineFunctions(mgr);
    return;
  }
  if (mShield.mPendingRebuild == true) {
    PopShield(mgr);
    mShield.mPendingRebuild = false;
  }
  CPatterned::Think(dt, mgr);
  if (CElitePirateGrenadeLauncher* launcher =
          static_cast< CElitePirateGrenadeLauncher* >(mgr.ObjectById(mLauncherId))) {
    launcher->SetDamageAddColor(mColor);
    launcher->SetSlowedSpeed(BodyController()->GetTimeScale());
    launcher->SetFollowPlayer(!BodyController()->IsFrozen());
  }
  mCollisionActorMgr->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  mShieldCollisionMgr->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  if (IsShieldUp() == true && mShield.mActive == true) {
    mSpeed = 2.f * mInitialSpeed;
  } else {
    mSpeed = mInitialSpeed;
  }
  UpdateAILogicTimers(dt);
  UpdateBreadCrumbTrail();
  UpdateGrenadeLauncher(mgr, mLauncherId, rstl::string_l(skpLauncherLCTR));
  UpdateAimBlend(dt);
  UpdateShieldEffect(mgr, dt);
  mTime += dt;
  if (!mLauncherPossessed && mIngPossessionBlend > 0.15f) {
    DeleteGrenadeLauncher(mgr);
    mLauncherId = mgr.AllocateUniqueId();
    CreateGrenadeLauncher(mgr, mLauncherId);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(GetActive());
    }
  }
  UpdateShieldFade(dt);
  AvoidObstacles(mgr, dt);
}

void CElitePirate::AvoidObstacles(CStateManager& mgr, float dt) {
  switch (mCurrentAction) {
  case kA_PowerUp:
    return;
  default: {
    const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
        CMaterialList(kMT_Floor, kMT_Player),
        CMaterialList(kMT_Unknown59, kMT_AIBlock, kMT_Wall, kMT_AIPassthrough, kMT_CollisionActor));
    const CVector3f offset = 1.8f * GetTransform().GetForward();
    const CVector3f start = mLeftClawPos + offset;
    const CVector3f delta = (mRightClawPos + offset) - start;
    if (delta.CanBeNormalized() == true) {
      if (CGameCollision::RayStaticLineOfSightTest(
              *mgr.World()->Area(GetCurrentAreaId()),
              start, delta.AsNormalized(), delta.Magnitude(), filter) == false) {
        MoveInOneFrameOR(0.15f * (-1.f * CVector3f::Forward()), dt);
        mLastObstacleTime = mTime;
        return;
      }
    }
    if (IsNearNoAttackHint(mgr) == false) {
      if (const CElitePirateGrenadeLauncher* launcher =
              static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId))) {
        const CTransform4f head = GetLctrTransform(rstl::string_l("Head_1"));
        CVector3f forward = GetTransform().GetForward();
        forward.SetZ(0.f);
        if (forward.CanBeNormalized() == true) {
          forward.Normalize();
        }
        const CVector3f from = head.GetTranslation() + 1.5f * forward +
                               2.2f * CVector3f(forward.GetY(), -forward.GetX(), 0.f) +
                               -0.6f * CVector3f::Up();
        const CVector3f to = launcher->GetTurretTransform().GetTranslation() + 2.f * forward +
                             0.1f * CVector3f::Up();
        const CVector3f dir = to - from;
        if (dir.CanBeNormalized() == true) {
          rstl::reserved_vector< TUniqueId, 1024 > nearList;
          CAABox box = CAABox::MakeMaxInvertedBox();
          box.AccumulateBounds(from);
          box.AccumulateBounds(to);
          mgr.BuildNearList(nearList, box, filter, this);
          if (CGameCollision::RayDynamicLineOfSightTest(mgr, from, dir.AsNormalized(),
                                                        dir.Magnitude(), filter, nearList,
                                                        this) == false) {
            MoveInOneFrameOR(0.15f * (-1.f * CVector3f::Forward()), dt);
            mLastObstacleTime = mTime;
          }
        }
      }
    }
  }
  case kA_FollowAttackPattern:
  case kA_PowerDown:
    break;
  }
}

void CElitePirate::UpdateShieldFade(float dt) {
  if (IsShieldUp() == true) {
    mShield.mAlpha = rstl::min_val(1.f, mShield.mAlpha + dt / 0.8f);
  } else {
    mShield.mAlpha = rstl::max_val(0.f, mShield.mAlpha - dt * 0.5f);
  }
  if (mInvulnerable == true) {
    mInvulnAlpha = rstl::min_val(1.f, mInvulnAlpha + dt / 3.f);
  } else {
    mInvulnAlpha = rstl::max_val(0.f, mInvulnAlpha - dt * 0.5f);
  }
}

void CElitePirate::UpdateShieldEffect(CStateManager& mgr, float dt) {
  if (mShield.mElementGen.get() != nullptr) {
    mShield.mElementGen->SetGlobalTranslation(mLeftClawPos);
    mShield.mElementGen->Update(dt);
  }
}

void CElitePirate::UpdateAimBlend(float dt) {
  if (IsShieldUp() == true) {
    mAimBlend += dt / 0.8f;
    if (mAimBlend > 1.f) {
      mAimBlend = 1.f;
    }
  } else {
    mAimBlend -= dt / 0.4f;
    if (mAimBlend < 0.f) {
      mAimBlend = 0.f;
    }
  }
}

void CElitePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const TUniqueId uid = msg.GetSenderId();
  bool shouldPass = true;
  switch (msg.GetMessage()) {
  case kSM_Create: {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    SetupCollisionManager(mgr);
    mLauncherId = mgr.AllocateUniqueId();
    CreateGrenadeLauncher(mgr, mLauncherId);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(GetActive());
    }
    const float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
    if (maxSpeed > 0.f) {
      mSteeringSpeed =
          (0.99f * BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk)) /
          maxSpeed;
    }
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    BodyController()->CommandMgr().SetSteeringSpeedRange(mSteeringSpeed, mSteeringSpeed);
    break;
  }
  case kSM_Activate:
    mCollisionActorMgr->SetActive(mgr, true);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(true);
    }
    break;
  case kSM_Deactivate:
    mCollisionActorMgr->SetActive(mgr, false);
    mShieldCollisionMgr->SetActive(mgr, false);
    break;
  case kSM_Delete:
    ReleaseClaimedRegion(mgr);
    mCollisionActorMgr->Destroy(mgr);
    mShieldCollisionMgr->Destroy(mgr);
    mgr.DeleteObjectRequest(mLauncherId);
    mLauncherId = kInvalidUniqueId;
    break;
  case kSM_Alert:
    mAlert = true;
    mAlerted = true;
    break;
  case kSM_AreaLoaded:
    mPathFindSearch.SetArea(
        mgr.World()->Area(GetCurrentAreaId())->GetPostConstructed()->mPathArea);
    break;
  case kSM_XHIT:
    if (GetAlive()) {
      if (GetHealthInfo()->GetHP() > 0.f) {
        if (mCurrentAction == kA_MeleeAttack) {
          if (TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(uid))) {
            const TUniqueId touched =
                TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(uid))->GetLastTouchedObject();
            if (const CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(touched))) {
              if (mMeleeDamageOn == true) {
                ApplyMeleeDamage(mgr, player->GetUniqueId());
              }
              if (mMeleeKnockBack == true) {
                if (mgr.GetPlayer(0)->GetMorphballTransitionState() != CPlayer::kMS_Morphed) {
                  ApplyPlayerImpulse(mgr, 25.f, 15.f);
                } else {
                  ApplyPlayerImpulse(mgr, 60.f, 15.f);
                }
              }
            }
          }
        }
      }
      if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(uid))) {
        const TUniqueId touched = actor->GetLastTouchedObject();
        if (touched != GetUniqueId()) {
          if (const CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(touched))) {
            if (mCurDamageRemTime <= 0.f && mLastMeleeTime + 1.5f < mTime) {
              mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(),
                              GetContactDamage(),
                              CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59),
                                                                  CMaterialList()),
                              CVector3f::Zero());
              mCurDamageRemTime = mDamageWaitTime;
            }
          }
        }
      }
    }
    break;
  case kSM_XXDG:
    SetShotAt(true);
    break;
  case kSM_Damage:
    shouldPass = false;
    if (IsShieldUp() == true) {
      if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(uid))) {
        const TUniqueId touched = actor->GetLastTouchedObject();
        if (const CGameProjectile* projectile =
                TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(touched))) {
          const float damage = projectile->GetCurrentDamageInfo().GetDamage();
          switch (mShield.mType) {
          case kST_Light: {
            const EWeaponType type = projectile->GetCurrentDamageInfo().GetWeaponMode().GetType();
            if (type == kWT_Light || type == kWT_Annihilator) {
              mShield.mDamage += damage;
              mShield.mLastHitTime = mTime;
            }
            break;
          }
          case kST_Dark: {
            const EWeaponType type = projectile->GetCurrentDamageInfo().GetWeaponMode().GetType();
            if (type == kWT_Dark || type == kWT_Annihilator) {
              mShield.mDamage += damage;
              mShield.mLastHitTime = mTime;
            }
            break;
          }
          }
        }
      }
    } else if (mInvulnerable != true) {
      if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(uid))) {
        const TUniqueId touched = actor->GetLastTouchedObject();
        if (const CGameProjectile* projectile =
                TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(touched))) {
          const TUniqueId projectileId = actor->GetLastTouchedObject();
          CDamageInfo info(projectile->GetCurrentDamageInfo());
          info.SetRadius(0.f);
          mgr.ApplyDamage(projectileId, GetUniqueId(), projectileId, info,
                          CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59),
                                                              CMaterialList()),
                          CVector3f::Zero());
          SetShotAt(true);
        }
      }
      const bool dead = GetHealthInfo()->GetHP() <= 0.f;
      mKnockBackController.EnableFreeze(dead);
      mKnockBackController.EnableSlow(dead);
    }
    break;
  case kSM_ResistedDamage:
    SetShotAt(true);
    break;
  }
  if (shouldPass) {
    CPatterned::AcceptScriptMsg(mgr, msg);
  }
}

void CElitePirate::SetInvulnerable(CStateManager& mgr, EStateMsg msg) {
  switch (msg) {
  case kStateMsg_Activate:
    SetShieldActive(mgr, true);
    mInvulnerable = true;
    break;
  case kStateMsg_Deactivate:
    SetShieldActive(mgr, false);
    mInvulnerable = false;
    break;
  }
}

void CElitePirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

void CElitePirate::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  mLeftClawPos = GetLctrTransform(rstl::string_l(skpLeftClawLCTR)).GetTranslation();
  mRightClawPos = GetLctrTransform(rstl::string_l(skpRightClawLCTR)).GetTranslation();
  if (CActor* launcher = static_cast< CActor* >(mgr.ObjectById(mLauncherId))) {
    launcher->SetModelFlags(GetModelFlags());
  }
  UpdateShieldEffect(mgr, 0.f);
}

void CElitePirate::Render(const CStateManager& mgr) const {
  if (1.f + mShield.mLastHitTime > mTime) {
    gpRender->SetAmbientColor(
        CColor::Lerp(CColor::White(), CColor::Black(), mTime - mShield.mLastHitTime));
  } else {
    gpRender->SetAmbientColor(CColor::Black());
  }
  CPatterned::Render(mgr);
  RenderShield();
  if (GetAlive() == true) {
    if (mShield.mElementGen.get() != nullptr) {
      mShield.mElementGen->Render();
    }
  }
}

void CElitePirate::RenderShield() const {
  if (0.f != mShield.mAlpha || 0.f != mInvulnAlpha) {
    gpRender->SetModelMatrix(GetTransform() * CTransform4f::Scale(GetModelData()->GetScale()));
    CColor color = CColor::White();
    float alpha;
    if (mInvulnAlpha > 0.f) {
      alpha = (1.f + sin(8.f * mTime)) * 0.5f * mInvulnAlpha;
    } else {
      float speed;
      switch (mShield.mType) {
      case kST_Dark:
        color = CColor::Purple();
        speed = 16.f;
        break;
      case kST_Light:
        speed = 10.f;
        break;
      default:
        return;
      }
      alpha = (1.f + sin(speed * mTime)) * 0.5f;
      if (1.f != mShield.mAlpha) {
        alpha *= mShield.mAlpha;
      }
    }
    color.SetAlpha(alpha);
    GetAnimationData()->Render(***mShield.mModel, CModelFlags(CModelFlags::kT_Blend, 0, static_cast< CModelFlags::EFlags >(3), color));
  }
}

void CElitePirate::RenderIngSnatchingTransition(const CStateManager& mgr) const {
  CPatterned::RenderIngSnatchingTransition(mgr);
  GetModelData()->SetupWorldSpacePortalPlane(
      GetTransform(), CPlane(CVector3f::Dot(GetTranslation(), CVector3f::Up()), CVector3f::Up()));
}

CVector3f CElitePirate::GetAimPosition(const CStateManager& mgr, float dt) const {
  const CVector3f aim = CPatterned::GetAimPosition(mgr, dt);
  if (0.f == mAimBlend) {
    return aim;
  }
  if (const CCollisionActor* actor =
          TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(mShieldCollisionId))) {
    mAimPos = actor->GetTranslation();
  }
  const float t = mAimBlend;
  const float u = 1.f - t;
  return CVector3f(aim.GetX() * u + mAimPos.GetX() * t, aim.GetY() * u + mAimPos.GetY() * t,
                   aim.GetZ() * u + mAimPos.GetZ() * t);
}

void CElitePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  bool handled = false;
  switch (type) {
  case kUE_Projectile:
    switch (mCurrentAction) {
    case kA_Shockwave:
      CreateShockWave(mgr, node);
      handled = true;
      break;
    case kA_ProjectileAttack:
    case kA_SpreadShot:
      LaunchRocket(mgr);
      handled = true;
      break;
    }
    break;
  case kUE_BeginAction:
    switch (mCurrentAction) {
    case kA_MeleeAttack:
      mMeleeDamageOn = true;
      mMeleeKnockBack = true;
      ExtendTouchBounds(mgr, mCollisionRJointIds, skExtendedClawBounds);
      ExtendTouchBounds(mgr, mCollisionLJointIds, skExtendedClawBounds);
      break;
    }
    break;
  case kUE_EndAction:
    switch (mCurrentAction) {
    case kA_MeleeAttack:
      mMeleeDamageOn = false;
      mMeleeKnockBack = false;
      ExtendTouchBounds(mgr, mCollisionRJointIds, CVector3f::Zero());
      ExtendTouchBounds(mgr, mCollisionLJointIds, CVector3f::Zero());
      break;
    case kA_ProjectileAttack:
      mBreakProjectileAttack = true;
      break;
    }
    break;
  case kUE_DamageOn:
    mDamageOn = true;
    handled = true;
    break;
  case kUE_DamageOff:
    mDamageOn = false;
    handled = true;
    break;
  case kUE_ScreenShake:
    ApplyScreenShake(mgr, GetTranslation(), FindConnectedObject(mgr, kSS_InternalState01, kSM_Attach));
    ProcessStompGround(mgr);
    handled = true;
    break;
  case kUE_ObjectDrop:
    ApplyScreenShake(mgr, GetTranslation(), FindConnectedObject(mgr, kSS_Footstep, kSM_Attach));
    handled = true;
    break;
  case kUE_BecomeShootThrough:
    for (uint i = 0; i < mCollisionActorMgr->GetNumCollisionActors(); ++i) {
      const TUniqueId id = mCollisionActorMgr->GetCollisionDescFromIndex(i).GetCollisionActorId();
      if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
        actor->AddMaterial(kMT_NoPlatformCollision, mgr);
      }
    }
    handled = true;
    break;
  case kUE_GenerateEnd:
    mPoweredUp = true;
    break;
  case kUE_RemoveCollision:
    mCollisionActorMgr->SetActive(mgr, false);
    RemoveMaterial(kMT_Unknown59, mgr);
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

void CElitePirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    ReleaseClaimedRegion(mgr);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    if (CEntity* launcher = mgr.ObjectById(mLauncherId)) {
      launcher->SetActive(false);
    }
    if (mShield.mSfx) {
      CSfxManager::RemoveEmitter(mShield.mSfx);
    }
    mgr.DeleteObjectRequest(mLauncherId);
    mLauncherId = kInvalidUniqueId;
    mKnockBackController.EnableKnockBackPhysics(false);
  }
  CPatterned::Dead(mgr, msg, dt);
}

void CElitePirate::LaunchRocket(CStateManager& mgr) {
  ++mRocketsFired;
  if (const CElitePirateGrenadeLauncher* launcher =
          static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId))) {
    if (mRocketInfo.Token().IsLoaded()) {
      CTransform4f xf(launcher->GetTurretTransform());
      TUniqueId target = kInvalidUniqueId;
      switch (mFireMode) {
      case 0:
        target = mgr.GetPlayer(0)->GetUniqueId();
        break;
      case 1: {
        CPlayer* player = mgr.Player(0);
        CVector3f dir = player->GetAimPosition(mgr, 0.f) - xf.GetTranslation();
        dir.SetZ(0.f);
        if (dir.CanBeNormalized() == true) {
          dir.Normalize();
          const bool raised =
              GetNearbyHintType(mgr) == CScriptAIHint::kHT_GrenadeLauncherRaisedAim;
          const float scale = raised ? 1.5f : 1.f;
          const CVector3f offsets[3] = {
              CVector3f(scale * (3.2f * -dir.GetY()), scale * (3.2f * dir.GetX()), scale * 0.f),
              CVector3f(scale * (1.2f * -dir.GetY()), scale * (1.2f * dir.GetX()), scale * 0.f),
              CVector3f(scale * (1.2f * dir.GetY()), scale * (1.2f * -dir.GetX()), scale * 0.f),
          };
          CVector3f aim = player->GetAimPosition(mgr, 0.f) + offsets[mRocketsFired - 1];
          if (raised == true) {
            aim.SetZ(aim.GetZ() + mgr.Random()->Range(6.f, 12.f));
            xf = CTransform4f::LookAt(xf.GetTranslation(), aim, CVector3f::Up());
            target = mgr.GetPlayer(0)->GetUniqueId();
          } else {
            xf = CTransform4f::LookAt(xf.GetTranslation(), aim, CVector3f::Up());
            target = kInvalidUniqueId;
          }
        } else {
          target = kInvalidUniqueId;
        }
        break;
      }
      }
      CEnergyProjectile* projectile = rs_new CEnergyProjectile(
          true, mRocketInfo.Token(), kWT_AI, xf, kMT_Character, mRocketInfo.GetDamage(),
          mgr.AllocateUniqueId(), GetCurrentAreaId(), GetUniqueId(), target, 0, false,
          CVector3f::One(), CImpactVisorEffect(), false, true, false, 1.f, 4.f, 4.f);
      if (projectile) {
        mgr.AddObject(projectile);
      }
    }
  }
}

void CElitePirate::CreateShockWave(CStateManager& mgr, const CInt32POINode& node) {
  CTransform4f xf(GetTransform());
  float x;
  float y;
  if (node.GetLocatorName() == rstl::string_l(skpRightBallLCTR)) {
    x = mRightClawPos.GetX();
    y = mRightClawPos.GetY();
  } else {
    x = mLeftClawPos.GetX();
    y = mLeftClawPos.GetY();
  }
  xf.SetTranslation(CVector3f(x, y, GetTranslation().GetZ()));
  CShockWave* wave = rs_new CShockWave(mgr.AllocateUniqueId(), rstl::string_l("Shock Wave"),
                                       CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
                                       xf, GetUniqueId(), GetShockWaveInfo(), 2.f, 0.4f);
  if (wave) {
    mgr.AddObject(wave);
  }
}

void CElitePirate::ApplyMeleeDamage(CStateManager& mgr, const TUniqueId& uid) {
  mgr.ApplyDamage(GetUniqueId(), uid, GetUniqueId(), mData.GetMeleeDamage(),
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59),
                                                      CMaterialList()),
                  CVector3f::Zero());
  mMeleeDamageOn = false;
  mLastMeleeTime = mTime;
}

bool CElitePirate::SpotPlayer(CStateManager& mgr, const CTriggerData& data) const {
  if (mAlert) {
    return true;
  }
  return CPatterned::SpotPlayer(mgr, data);
}

bool CElitePirate::InDetectionRange(CStateManager& mgr, const CTriggerData& data) const {
  if (mAlert) {
    return true;
  }
  return CPatterned::InDetectionRange(mgr, data);
}

bool CElitePirate::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}

bool CElitePirate::AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

void CElitePirate::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_FollowAttackPattern, msg);
  SetInvulnerable(mgr, msg);
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg == kStateMsg_Activate) {
    mAlert = false;
    const TUniqueId id = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mWaypointNavigation.SetDestination(id);
    if (CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id))) {
      if (CVector3f::Dot(GetTransform().GetForward(), wp->GetTranslation() - GetTranslation()) <=
          0.f) {
        mWaypointNavigation.SetInPosition(true);
      }
    }
  }
}

bool CElitePirate::NotReachedTarget(CStateManager& mgr, const CTriggerData& data) const {
  if (mLastObstacleTime > 0.5f * (mShield.mStartTime + mTime)) {
    return true;
  }
  return mLastObstacleTime < mShield.mStartTime;
}

bool CElitePirate::ShieldKilled(CStateManager& mgr, const CTriggerData& data) const {
  if (mShield.mType == kST_Light) {
    return mShield.mDamage >= mData.GetMeleeChance();
  }
  return mShield.mDamage >= mData.GetShockwaveChance();
}

bool CElitePirate::ShouldTurn(CStateManager& mgr, const CTriggerData& data) const {
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const CVector3f dist = target->GetTranslation() - GetTranslation();
    const CVector3f forward = GetTransform().GetForward();
    return CVector2f::GetAngleDiff(CVector2f(forward.GetX(), forward.GetY()),
                                   CVector2f(dist.GetX(), dist.GetY())) > 1.7453293f;
  }
  return false;
}

bool CElitePirate::DoneTurning(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CElitePirate::DonePursuing(CStateManager& mgr, const CTriggerData& data) const {
  if (mStuckTime > 0.5f) {
    return true;
  }
  const int hint = GetNearbyHintType(mgr);
  if (hint == 27) {
    return true;
  }
  if (hint == CScriptAIHint::kHT_GrenadeLauncherRaisedAim &&
      (PathShagged(mgr, CTriggerData(0.f)) == true ||
       mPathFindSearch.GetCurrentWaypoint() >= mPathFindSearch.GetWaypoints().size() - 1)) {
    return true;
  }
  if (mTime > mPursueStartTime) {
    const CPlayer* player = mgr.GetPlayer(0);
    if (GetCurrentAreaId() != player->GetCurrentAreaId()) {
      return true;
    }
    if (hint != CScriptAIHint::kHT_GrenadeLauncherRaisedAim) {
      const CVector3f pos = GetTranslation();
      const CVector3f dist = player->GetAimPosition(mgr, 0.f) - pos;
      if (dist.MagSquared() < mData.GetMaxShockwaveRange() * mData.GetMaxShockwaveRange()) {
        return true;
      }
    }
    if (IsNearNoAttackHint(mgr) == true) {
      return true;
    }
    return ClearLineOfSight(mgr, CTriggerData(0.f));
  }
  const CPlayer* player = mgr.GetPlayer(0);
  const CVector3f pos = GetTranslation();
  const CVector3f dist = player->GetAimPosition(mgr, 0.f) - pos;
  if (dist.MagSquared() < 4.f * (mData.GetMaxMeleeRange() * mData.GetMaxMeleeRange())) {
    return true;
  }
  if (2.5f + mPursueStartTime > mTime &&
      (ClearLineOfSight(mgr, CTriggerData(0.f)) == true || IsNearNoAttackHint(mgr) == true)) {
    return true;
  }
  if (mPredictedLeashTime > 3.f) {
    return true;
  }
  return false;
}

bool CElitePirate::ShouldAlert(CStateManager& mgr, const CTriggerData& data) const {
  if (!mInvulnAlert) {
    if (mAlertPos == CVector3f::Zero() || (mAlertPos - GetTranslation()).MagSquared() > 25.f) {
      return true;
    }
  }
  return false;
}

bool CElitePirate::ShouldFire(CStateManager& mgr, const CTriggerData& data) const {
  return ShouldFireLauncher(mgr, mLauncherId);
}

bool CElitePirate::ShouldFireLauncher(CStateManager& mgr, const TUniqueId& uid) const {
  if (mAttackTimer <= 0.f && uid != kInvalidUniqueId) {
    if (const CActor* launcher = static_cast< const CActor* >(mgr.GetObjectById(uid))) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
        const CVector3f aim = target->GetAimPosition(mgr, 0.f);
        const CVector3f dist = aim - GetTranslation();
        const float magSq = dist.MagSquared();
        if (magSq > mData.GetMinRocketRange() * mData.GetMinRocketRange() &&
            magSq < mData.GetMaxRocketRange() * mData.GetMaxRocketRange() &&
            ShouldTurn(mgr, CTriggerData(0.f)) == false) {
          const CVector3f origin = GetGrenadeLaunchPos(*launcher);
          if (mgr.RayCollideWorld(
                  origin, aim, CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid, kMT_Character)),
                  this)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool CElitePirate::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (GetNearbyHintType(mgr) != -1) {
    return false;
  }
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const CVector3f dist = target->GetTranslation() - GetTranslation();
    if (dist.MagSquared() <= mData.GetMaxMeleeRange() * mData.GetMaxMeleeRange()) {
      const CVector3f forward = GetTransform().GetForward();
      if (CVector2f::GetAngleDiff(CVector2f(forward.GetX(), forward.GetY()),
                                  CVector2f(dist.GetX(), dist.GetY())) < 0.5497787f) {
        return true;
      }
    }
  }
  return false;
}

bool CElitePirate::ShouldShockwave(CStateManager& mgr, const CTriggerData& data) const {
  if (mAttackTimer <= 0.f) {
    for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
      const CPlayer* player = mgr.GetPlayer(i);
      if (GetCurrentAreaId() == player->GetCurrentAreaId()) {
        const CVector3f dist = player->GetAimPosition(mgr, 0.f) - GetTranslation();
        const float magSq = dist.MagSquared();
        if (magSq <= mData.GetMaxShockwaveRange() * mData.GetMaxShockwaveRange()) {
          const CPlayer::EPlayerMorphBallState state =
              player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
                  ? player->GetMorphballTransitionState()
                  : CPlayer::kMS_Unmorphed;
          if (state == CPlayer::kMS_Morphed) {
            return true;
          }
          if (magSq >= mData.GetMinShockwaveRange() * mData.GetMinShockwaveRange()) {
            return dist.GetZ() < 3.f;
          }
        }
      }
    }
  }
  return false;
}

bool CElitePirate::ShotAt(CStateManager& mgr, const CTriggerData& data) const { return mShotAt; }

bool CElitePirate::InPosition(CStateManager& mgr, const CTriggerData& data) const {
  return (mTargetDestPos - GetTranslation()).MagSquared() < 25.f;
}

bool CElitePirate::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    return (GetTranslation() - target->GetTranslation()).MagSquared() <
           mData.GetMaxMeleeRange() * mData.GetMaxMeleeRange();
  }
  return false;
}

bool CElitePirate::TargetNotOnMesh(CStateManager& mgr, const CTriggerData& data) const {
  if (GetNearbyHintType(mgr) == 27) {
    return false;
  }
  if (GetNearbyHintType(mgr) == CScriptAIHint::kHT_GrenadeLauncherRaisedAim) {
    return true;
  }
  return mPathFindSearch.NearlyOnPath(mgr.GetPlayer(0)->GetTranslation(), 4.f) !=
         CPathFindSearch::kR_Success;
}

bool CElitePirate::TargetUnreachable(CStateManager& mgr, const CTriggerData& data) const {
  const int hint = GetNearbyHintType(mgr);
  if (hint == CScriptAIHint::kHT_GrenadeLauncherRaisedAim || hint == 27) {
    return true;
  }
  return mPathFindSearch.GetResult() != CPathFindSearch::kR_Success;
}

bool CElitePirate::PoweredDown(CStateManager& mgr, const CTriggerData& data) const {
  return mLocomotionType != pas::kLT_Relaxed;
}

bool CElitePirate::PoweredUp(CStateManager& mgr, const CTriggerData& data) const {
  return mPoweredUp;
}

bool CElitePirate::ReadyToCharge(CStateManager& mgr, const CTriggerData& data) const {
  if (GetNearbyHintType(mgr) == 27 && ShieldKilled(mgr, CTriggerData(0.f)) == false) {
    return false;
  }
  return GetHealthInfo()->GetHP() < mHp;
}

bool CElitePirate::Alerted(CStateManager& mgr, const CTriggerData& data) const { return mAlerted; }

bool CElitePirate::ReturnedToPatrol(CStateManager& mgr, const CTriggerData& data) const {
  return mReturnedToPatrol;
}

void CElitePirate::Wait(CStateManager& mgr, EStateMsg msg, float dt) {
  SetInvulnerable(mgr, msg);
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().ClearLocomotionCmds();
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
}

void CElitePirate::PowerDown(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_PowerDown, msg);
  SetInvulnerable(mgr, msg);
  BodyController()->SetLocomotionType(static_cast< pas::ELocomotionType >(mLocomotionType));
  mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, false);
}

void CElitePirate::PowerUp(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_PowerUp, msg);
  SetInvulnerable(mgr, msg);
  BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  switch (msg) {
  case kStateMsg_Activate:
    mAlertPos = GetTranslation();
    SendScriptMsgs(kSS_Attack, mgr, kSM_None);
    break;
  case kStateMsg_Deactivate:
    mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
    ActivateGrenadeLauncher(mgr, true);
    break;
  }
}

void CElitePirate::PursueTarget(CStateManager& mgr, EStateMsg msg, const CVector3f& target,
                                float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    UpdatePathDestination(mgr, target, dt);
    mStuckTime = 0.f;
    break;
  case kStateMsg_Update: {
    const float dx = target.GetX() - mPathDestination.GetX();
    const float dy = target.GetY() - mPathDestination.GetY();
    if (0.f + (dx * dx + dy * dy) > 16.f) {
      UpdatePathDestination(mgr, target, dt);
    }
    if (mPathFindSearch.GetWaypoints().size() > 0) {
      CVector3f dist = GetTranslation() - mPathFindSearch.GetWaypoints().back();
      dist.SetZ(0.f);
      if (dist.Magnitude() < 4.f && IsOnPath(mgr) == false) {
        mStuckTime += dt;
        return;
      }
    }
    mStuckTime = 0.f;
    CVector3f move;
    if (PathShagged(mgr, CTriggerData(0.f)) == false &&
        mPathFindSearch.GetCurrentWaypoint() < mPathFindSearch.GetWaypoints().size() - 1) {
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
      move = BodyController()->GetCommandMgr().GetMoveVector();
    } else {
      move = mSteeringBehaviors.Arrival(*this, mPathDestination, 7.f);
    }
    BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(move, CVector3f::Zero(), 1.f));
    break;
  }
  }
}

void CElitePirate::Pursue(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAlert = false;
    mPursueStartTime = mTime;
    mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
    break;
  case kStateMsg_Update: {
    CVector3f target = mPathDestination;
    if (const CActor* actor = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
      target = actor->GetTranslation();
    }
    PursueTarget(mgr, msg, target, dt);
    break;
  }
  }
}

void CElitePirate::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  TryCommand(msg, pas::kAS_Taunt, CBCTauntCmd(static_cast< pas::ETauntType >(mTauntType)));
  switch (msg) {
  case kStateMsg_Activate:
    mInvulnAlert = true;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    break;
  case kStateMsg_Deactivate: {
    const int cycle[4] = {0, 6, 1, 0};
    for (int i = 0; i < 3; ++i) {
      if (mTauntType == cycle[i]) {
        mTauntType = cycle[i + 1];
        break;
      }
    }
    mAngryCount = mgr.Random()->Range(1, 3);
    mAngryAttackOver = false;
    mAngryChosen = false;
    break;
  }
  }
}

void CElitePirate::Stunned(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mShield.mAlpha = 0.f;
  }
  TryCommand(msg, pas::kAS_Taunt, CBCTauntCmd(static_cast< pas::ETauntType >(2)));
}

void CElitePirate::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    if (HasPatrolPath(mgr, CTriggerData(0.f))) {
      SetDestPos(mWaypointNavigation.GetDestinationPosition());
    } else {
      SetDestPos(mLatestLeashPosition);
    }
    mTargetDestPos = mDestPos;
    if (GetSearchPath()) {
      CPatterned::PathFind(mgr, msg, dt);
    }
    SetShotAt(false);
    mReturnedToPatrol = false;
    mCurPlayerLeashTime = 0.f;
    break;
  case kStateMsg_Update:
    if (!PathShagged(mgr, CTriggerData(0.f))) {
      if (mPathFindSearch.GetCurrentWaypoint() >= mPathFindSearch.GetWaypoints().size() - 1) {
        mReturnedToPatrol = true;
      } else {
        CPatterned::PathFind(mgr, msg, dt);
      }
    } else {
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(
          mSteeringBehaviors.Arrival(*this, mTargetDestPos, 25.f), CVector3f::Zero(), 1.f));
      mReturnedToPatrol = true;
    }
    break;
  case kStateMsg_Deactivate:
    mAlert = false;
    break;
  }
}

void CElitePirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mHitByPlayerProjectile = false;
    break;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

void CElitePirate::Turn(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
      const CVector3f dir = target->GetTranslation() - GetTranslation();
      if (dir.CanBeNormalized()) {
        mTurnDirection = dir.AsNormalized();
      }
    }
  }
  if (ShouldAlert(mgr, CTriggerData(0.f)) == true) {
    SetInvulnerable(mgr, msg);
  }
  TryCommand(msg, pas::kAS_Turn,
             CBCLocomotionCmd(CVector3f::Zero(), mTurnDirection.AsNormalized(), 1.f));
}

void CElitePirate::SelectTarget(CStateManager& mgr, float dt) {
  mTargetId = CScriptTeamAiMgr::ChoosePlayer(mgr, *this);
}

void CElitePirate::PickAttackType(CStateManager& mgr, float dt) {
  if (mgr.IsRandomAvailable()) {
    if (GetNearbyHintType(mgr) == CScriptAIHint::kHT_GrenadeLauncherRaisedAim) {
      mFireMode = 1;
    } else if (mgr.Random()->Range(0.f, 1.f) < 0.8f) {
      if (mNextFireMode == 0) {
        mFireMode = 1;
      } else {
        mFireMode = 0;
      }
    } else {
      mFireMode = mNextFireMode;
    }
  }
}

void CElitePirate::Alert(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_Alert, msg);
  SetInvulnerable(mgr, msg);
  TryCommand(msg, pas::kAS_Taunt, CBCTauntCmd(static_cast< pas::ETauntType >(mAlertTauntType)));
  switch (msg) {
  case kStateMsg_Activate:
    mInvulnAlert = true;
    mPathDestination = GetTranslation();
    break;
  case kStateMsg_Deactivate:
    SetShotAt(false);
    ActivateGrenadeLauncher(mgr, true);
    break;
  }
}

void CElitePirate::InvulnAlert(CStateManager& mgr, EStateMsg msg, float dt) {
  SetInvulnerable(mgr, msg);
  Alert(mgr, msg, dt);
}

CProjectileInfo* CElitePirate::ProjectileInfo() { return &mRocketInfo; }

void CElitePirate::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mRocketsFired = 0;
    mRocketsToFire = mgr.Random()->Range(mData.GetMinRocketCount(), mData.GetMaxRocketCount());
    mBreakProjectileAttack = false;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
  SetCurrentAction(kA_ProjectileAttack, msg);
  if (msg == kStateMsg_Deactivate) {
    UpdateAttackTimeLeft(mgr);
  }
  if (mRocketsFired >= mRocketsToFire) {
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
  } else {
    if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
      mAttackTarget = target->GetTranslation();
      BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
    }
    TryCommand(msg, pas::kAS_LoopAttack, CBCLoopAttackCmd(static_cast< pas::ELoopAttackType >(0)));
  }
}

bool CElitePirate::BreakProjectileAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mBreakProjectileAttack;
}

bool CElitePirate::CanShockwave(CStateManager& mgr, const CTriggerData& data) const {
  return GetNearbyHintType(mgr) != CScriptAIHint::kHT_GrenadeLauncherRaisedAim;
}

bool CElitePirate::ClearLineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  if (const CElitePirateGrenadeLauncher* launcher =
          static_cast< const CElitePirateGrenadeLauncher* >(mgr.GetObjectById(mLauncherId))) {
    const CVector3f origin = launcher->GetTurretTransform().GetTranslation();
    const CVector3f dir = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f) - origin;
    if (dir.CanBeNormalized() == true) {
      const CVector3f flat(dir.GetX(), dir.GetY(), 0.f);
      if (flat.MagSquared() > 0.5f * (mData.GetMaxRocketRange() * mData.GetMaxRocketRange())) {
        return false;
      }
      return CGameCollision::RayStaticLineOfSightTest(
          *mgr.World()->Area(GetCurrentAreaId()),
          origin, dir.AsNormalized(), dir.Magnitude(),
          CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid),
                                              CMaterialList(kMT_Player)));
    }
  }
  return false;
}

bool CElitePirate::PickedSpreadShot(CStateManager& mgr, const CTriggerData& data) const {
  return mFireMode == 1;
}

void CElitePirate::SpreadShot(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mRocketsFired = 0;
    mRocketsToFire = 4;
    mBreakProjectileAttack = false;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
  SetCurrentAction(kA_SpreadShot, msg);
  if (msg == kStateMsg_Deactivate) {
    UpdateAttackTimeLeft(mgr);
  }
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    mAttackTarget = target->GetTranslation();
    BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
  }
  TryCommand(msg, pas::kAS_ProjectileAttack,
             CBCProjectileAttackCmd(static_cast< pas::ESeverity >(9), mAttackTarget, false));
}

void CElitePirate::Shockwave(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_Shockwave, msg);
  switch (msg) {
  case kStateMsg_Update:
    break;
  case kStateMsg_Activate: {
    const float roll = mgr.Random()->Range(
        0.f, mData.GetDoubleShockwaveWeight() +
                 (mData.GetRocketWeight() + (mData.GetMeleeWeight() + mData.GetShockwaveWeight())));
    if (roll < mData.GetMeleeWeight()) {
      mAttackType = kAT_Melee;
    } else if (roll < mData.GetMeleeWeight() + mData.GetShockwaveWeight()) {
      mAttackType = kAT_Shockwave;
    } else if (roll < mData.GetRocketWeight() +
                          (mData.GetMeleeWeight() + mData.GetShockwaveWeight())) {
      mAttackType = kAT_Rocket;
    } else {
      mAttackType = kAT_DoubleShockwave;
    }
    break;
  }
  case kStateMsg_Deactivate:
    UpdateAttackTimeLeft(mgr);
    mLastAttackType = mAttackType;
    break;
  }
  const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    mAnimationState.SetState(CAnimationState::kAS_Over);
    return;
  }
  BodyController()->CommandMgr().SetTargetVector(target->GetTranslation() - GetTranslation());
  TryCommand(msg, pas::kAS_ProjectileAttack,
             CBCProjectileAttackCmd(static_cast< pas::ESeverity >(mAttackType),
                                    target->GetTranslation(), false));
}

void CElitePirate::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  SetCurrentAction(kA_MeleeAttack, msg);
  TryCommand(msg, pas::kAS_MeleeAttack,
             CBCMeleeAttackCmd(static_cast< pas::ESeverity >(mMeleeSeverity)));
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
    if (mgr.Random()->Float() < 0.7f) {
      if (mMeleeSeverity == 1) {
        mNextMeleeSeverity = 7;
      } else {
        mNextMeleeSeverity = 1;
      }
    }
    break;
  case kStateMsg_Deactivate:
    mMeleeDamageOn = false;
    mMeleeKnockBack = false;
    mMeleeSeverity = mNextMeleeSeverity;
    mAngryCount = 0;
    ExtendTouchBounds(mgr, mCollisionRJointIds, CVector3f::Zero());
    ExtendTouchBounds(mgr, mCollisionLJointIds, CVector3f::Zero());
    break;
  }
}

void CElitePirate::SetCurrentAction(EAction action, EStateMsg msg) {
  if (msg == kStateMsg_Deactivate) {
    PushAction(mActionHistory, mCurrentAction);
    mCurrentAction = kA_None;
  } else {
    mCurrentAction = action;
  }
}

void CElitePirate::ShieldUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mShieldCollisionMgr->SetActive(mgr, true);
    mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, false);
    BodyController()->CommandMgr().ClearLocomotionCmds();
    mShield.mStartTime = mTime;
    switch (mShield.mType) {
    case kST_Dark:
      mShield.mType = kST_Light;
      break;
    case kST_Light:
      mShield.mType = kST_Dark;
      break;
    case kST_Random:
      if (mgr.Random()->Range(0.f, 1.f) < 0.5f) {
        mShield.mType = kST_Light;
      } else {
        mShield.mType = kST_Dark;
      }
      break;
    }
    CAssetId effect;
    ushort sfx;
    if (mShield.mType == kST_Light) {
      effect = mData.GetLightShield();
      sfx = mData.GetLightShieldSound();
    } else {
      effect = mData.GetDarkShield();
      sfx = mData.GetDarkShieldSound();
    }
    mShield.mElementGen = rstl::auto_ptr< CElementGen >(rs_new CElementGen(
        TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', effect))),
        CElementGen::kMOT_Normal, CElementGen::kOSF_One));
    mShield.mElementGen->SetParticleEmission(true);
    mShield.mDamage = 0.f;
    mShield.mSfx = CSfxManager::AddEmitter(sfx, GetTranslation(), GetCurrentAreaId().Value(),
                                           false, true);
    SetShieldActive(mgr, true);
    break;
  }
  case kStateMsg_Update:
    RotateToPoint(mgr.GetPlayer(0)->GetTranslation(), dt, 1.7453293f);
    break;
  }
}

const char* CElitePirate::GetShieldEffectName() const {
  if (mShield.mType == kST_Dark) {
    return "BodyShield_dark";
  }
  return "BodyShield_light";
}

void CElitePirate::Shielding(CStateManager& mgr, EStateMsg msg, float dt) {
  if (ShieldKilled(mgr, CTriggerData(0.f)) == false) {
    CVector3f target = mPathDestination;
    if (const CActor* actor = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
      target = actor->GetTranslation();
    }
    PursueTarget(mgr, msg, target, dt);
  }
  switch (msg) {
  case kStateMsg_Activate:
    mShield.mActive = true;
    mAngryCount = 0;
    break;
  case kStateMsg_Update:
    if (mShotAt) {
      mShotAtTimer -= dt;
      if (mShotAtTimer <= 0.f) {
        mShotAt = false;
      }
    }
    CSfxManager::UpdateEmitter(mShield.mSfx, mLeftClawPos, CVector3f::Zero(), 0x7f);
    break;
  case kStateMsg_Deactivate:
    mShieldCollisionMgr->SetActive(mgr, false);
    SetShieldActive(mgr, false);
    mShield.mActive = false;
    mShield.mElementGen = rstl::auto_ptr< CElementGen >();
    mShield.mPendingRebuild = true;
    CSfxManager::RemoveEmitter(mShield.mSfx);
    mShield.mSfx = CSfxHandle();
    break;
  }
}

void CElitePirate::PopShield(CStateManager& mgr) {
  const TLockedToken< CGenDescription >* pop = nullptr;
  if (mShield.mType == kST_Light) {
    if (!mShield.mLightPop) {
      if (mData.GetLightShieldPop() != kInvalidAssetId) {
        mShield.mLightPop = TLockedToken< CGenDescription >(
            gpSimplePool->GetObj(SObjectTag('PART', mData.GetLightShieldPop())));
      }
    }
    if (mShield.mLightPop == true) {
      pop = &*mShield.mLightPop;
    }
  } else {
    if (!mShield.mDarkPop) {
      if (mData.GetDarkShieldPop() != kInvalidAssetId) {
        mShield.mDarkPop = TLockedToken< CGenDescription >(
            gpSimplePool->GetObj(SObjectTag('PART', mData.GetDarkShieldPop())));
      }
    }
    if (mShield.mDarkPop == true) {
      pop = &*mShield.mDarkPop;
    }
  }
  if (pop != nullptr) {
    CTransform4f xf(GetTransform());
    xf.SetTranslation(mLeftClawPos);
    CExplosion* explosion =
        rs_new CExplosion(*pop, mgr.AllocateUniqueId(),
                          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
                          rstl::string_l("IngSmasher shield pop"), xf, 0,
                          CVector3f(1.f, 1.f, 1.f), CColor::White(), -1);
    mgr.AddObject(explosion);
  }
}

void CElitePirate::SetupCollisionManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(14);
  AddCollisionList(skLeftArmJointList, 3, joints);
  AddCollisionList(skRightArmJointList, 3, joints);
  AddSphereCollisionList(skSphereJointList, 8, joints);
  mCollisionActorMgr =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, true);
  mCollisionActorMgr->SetActive(mgr, GetActive());
  mCollisionRJointIds.clear();
  mCollisionLJointIds.clear();
  const CSegId segId = GetAnimationData()->GetLocatorSegId(rstl::string_l(skpLeftClawLCTR));
  const CJointCollisionDescription shield = CJointCollisionDescription::OBBCollision(
      segId, skLocalShieldBounds, CVector3f::Zero(), rstl::string_l("Shield"), 5.f);
  joints.clear();
  joints.push_back(shield);
  mShieldCollisionMgr =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  SetupCollisionActorInfo(mgr);
  CMaterialList exclude(kMT_Floor, kMT_AIPassthrough, kMT_Player, kMT_CollisionActor,
                        kMT_Platform);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), exclude));
  AddMaterial(kMT_NoPlatformCollision, mgr);
  mShieldCollisionId = mShieldCollisionMgr->GetCollisionDescFromIndex(0).GetCollisionActorId();
  if (CCollisionActor* actor =
          TCastToPtr< CCollisionActor >(mgr.ObjectById(mShieldCollisionId))) {
    actor->SetWeaponCollisionResponseType(kWCR_None);
  }
  SetShieldActive(mgr, false);
  mShieldCollisionMgr->AddMaterialList(mgr, CMaterialList(kMT_AIJoint, kMT_Unknown54));
}

void CElitePirate::SetupCollisionActorInfo(CStateManager& mgr) {
  for (uint i = 0; i < mCollisionActorMgr->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorMgr->GetCollisionDescFromIndex(i);
    const TUniqueId uid = desc.GetCollisionActorId();
    if (TCastToPtr< CCollisionActor >(mgr.ObjectById(uid))) {
      if (desc.GetName() == rstl::string_l(skpHeadLCTR)) {
        mCollisionHeadId = uid;
      } else if (IsArmClawCollider(desc.GetName(), skpRightClawLCTR, skRightArmJointList, 3)) {
        mCollisionRJointIds.push_back(uid);
      } else if (IsArmClawCollider(desc.GetName(), skpLeftClawLCTR, skLeftArmJointList, 3)) {
        mCollisionLJointIds.push_back(uid);
      }
    }
  }
}

void CElitePirate::AddCollisionList(const SJointInfo* joints, int count,
                                    rstl::vector< CJointCollisionDescription >& list) {
  const CAnimData& animData = *GetAnimationData();
  for (int i = 0; i < count; ++i) {
    const CSegId from = animData.GetLocatorSegId(rstl::string_l(joints[i].mFrom));
    const CSegId to = animData.GetLocatorSegId(rstl::string_l(joints[i].mTo));
    if (from.val() != 0xff && to.val() != 0xff) {
      list.push_back(CJointCollisionDescription::SphereSubdivideCollision(
          from, to, joints[i].mRadius, joints[i].mSeparation,
          CJointCollisionDescription::kOT_BetweenJoints, rstl::string_l(joints[i].mFrom), 5.f));
    }
  }
}

void CElitePirate::AddSphereCollisionList(const SSphereJointInfo* joints, int count,
                                          rstl::vector< CJointCollisionDescription >& list) {
  const CAnimData& animData = *GetAnimationData();
  for (int i = 0; i < count; ++i) {
    const CSegId id = animData.GetLocatorSegId(rstl::string_l(joints[i].mName));
    if (id.val() != 0xff) {
      list.push_back(CJointCollisionDescription::SphereCollision(
          id, CVector3f::Zero(), joints[i].mRadius, rstl::string_l(joints[i].mName), 5.f));
    }
  }
}

bool CElitePirate::IsArmClawCollider(const rstl::string& name, const char* locator,
                                     const SJointInfo* joints, int count) const {
  if (name == rstl::string_l(locator)) {
    return true;
  }
  for (int i = 0; i < count; ++i) {
    if (name == rstl::string_l(joints[i].mFrom)) {
      return true;
    }
  }
  return false;
}

bool CElitePirate::IsArmClawCollider(TUniqueId uid,
                                     const rstl::reserved_vector< TUniqueId, 7 >& ids) const {
  for (rstl::reserved_vector< TUniqueId, 7 >::const_iterator it = ids.begin(); it != ids.end();
       ++it) {
    if (*it == uid) {
      return true;
    }
  }
  return false;
}

void CElitePirate::ExtendTouchBounds(CStateManager& mgr,
                                     const rstl::reserved_vector< TUniqueId, 7 >& ids,
                                     const CVector3f& bounds) const {
  for (rstl::reserved_vector< TUniqueId, 7 >::const_iterator it = ids.begin(); it != ids.end();
       ++it) {
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(*it))) {
      actor->SetExtendedTouchBounds(bounds);
    }
  }
}

void CElitePirate::SetupPathFindSearch() {
  const float scale = 1.5f * GetModelData()->GetScale().GetY();
  const float height = 6.f * GetModelData()->GetScale().GetZ();
  const CAABox box(CVector3f(-scale, -scale, 0.f), CVector3f(scale, scale, height));
  SetBoundingBox(box);
  mCollisionAabb.SetBox(box);
  mPathFindSearch.SetCharacterRadius(scale);
  mPathFindSearch.SetCharacterHeight(height);
}

bool CElitePirate::IsShieldUp() const {
  return GetBodyController()->GetLocomotionType() == pas::kLT_Crouch;
}

void CElitePirate::SetShieldActive(CStateManager& mgr, bool active) {
  const CSegId leftPalm = GetAnimationData()->GetLocatorSegId(rstl::string_l(skpLeftClawLCTR));
  const CHealthInfo* health = GetHealthInfo();
  mHp = health->GetHP();
  for (uint i = 0; i < mCollisionActorMgr->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorMgr->GetCollisionDescFromIndex(i);
    if (CCollisionActor* actor =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()))) {
      *actor->HealthInfo() = *health;
      if (active == true) {
        actor->SetDamageVulnerability(CDamageVulnerability::ReflectVulnerabilty());
      } else {
        actor->SetDamageVulnerability(mVulnerability.MakeIgnoreRadius());
      }
      if (desc.GetPivotId() == leftPalm) {
        if (active == true) {
          actor->AddMaterial(kMT_NoPlatformCollision, mgr);
        } else {
          actor->RemoveMaterial(kMT_NoPlatformCollision, mgr);
        }
      }
    }
  }
  if (CCollisionActor* actor =
          TCastToPtr< CCollisionActor >(mgr.ObjectById(mShieldCollisionId))) {
    if (!active) {
      actor->SetDamageVulnerability(mVulnerability.MakeIgnoreRadius());
      return;
    }
    CDamageVulnerability vuln(CDamageVulnerability::ReflectVulnerabilty());
    const int types[3] = {1, 2, 3};
    for (int i = 0; i < 3; ++i) {
      const int type = types[i];
      if ((mShield.mType != kST_Dark || type != 1) && (mShield.mType != kST_Light || type != 2)) {
        vuln.SetVulnerability(type, CWeaponTypeVulnerability(1.f, CWeaponTypeVulnerability::kE_Normal, false));
        vuln.SetChargedVulnerability(type, CWeaponTypeVulnerability(1.f, CWeaponTypeVulnerability::kE_Normal, false));
        vuln.SetComboVulnerability(type, CWeaponTypeVulnerability(1.f, CWeaponTypeVulnerability::kE_Normal, false));
      }
    }
    actor->SetDamageVulnerability(vuln);
  }
}

void CElitePirate::UpdateAILogicTimers(float dt) {
  if (mAttackTimer > 0.f) {
    mAttackTimer -= dt;
  }
}

void CElitePirate::CreateGrenadeLauncher(CStateManager& mgr, TUniqueId uid) {
  const CAnimationParameters* params = &mData.GetPossessedLauncherAnimParams();
  if (IsIngPossessed() == true) {
  } else {
    params = &mData.GetLauncherAnimParams();
  }
  if (params->GetACSFile() != kInvalidAssetId) {
    CModelData model(CAnimRes(params->GetACSFile(), params->GetCharacter(),
                              GetModelData()->GetScale(), params->GetInitialAnimation(), true));
    CElitePirateGrenadeLauncher* launcher = rs_new CElitePirateGrenadeLauncher(
        uid, rstl::string_l("Rocket Launcher"),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId), GetTransform(), model,
        mData.GetLauncherActParams(), GetUniqueId(), 0.f);
    if (launcher) {
      mgr.AddObject(launcher);
      mLauncherPossessed = IsIngPossessed();
    }
  }
}

void CElitePirate::DeleteGrenadeLauncher(CStateManager& mgr) {
  mgr.DeleteObjectRequest(mLauncherId);
  mLauncherId = kInvalidUniqueId;
}

void CElitePirate::ActivateGrenadeLauncher(CStateManager& mgr, bool active) {
  ActivateGrenadeLauncherById(mgr, active, mLauncherId);
}

void CElitePirate::ActivateGrenadeLauncherById(CStateManager& mgr, bool active,
                                               TUniqueId uid) const {
  if (uid != kInvalidUniqueId) {
    if (CEntity* entity = mgr.ObjectById(uid)) {
      mgr.SendScriptMsg(entity, GetUniqueId(), active ? kSM_Start : kSM_Stop);
    }
  }
}

void CElitePirate::UpdateGrenadeLauncher(CStateManager& mgr, TUniqueId& uid,
                                         const rstl::string& locator) const {
  if (uid != kInvalidUniqueId) {
    if (CActor* actor = static_cast< CActor* >(mgr.ObjectById(uid))) {
      actor->SetTransform(CTransform4f(GetLctrTransform(locator)));
    } else {
      uid = kInvalidUniqueId;
    }
  }
}

CVector3f CElitePirate::GetGrenadeLaunchPos(const CActor& actor) const {
  const CTransform4f locator = actor.GetLocatorTransform(rstl::string_l(skpGrenadeLauncherLCTR));
  const CVector3f position =
      actor.GetTranslation() + actor.GetTransform().Rotate(locator.GetTranslation());
  return position;
}

void CElitePirate::UpdateBreadCrumbTrail() {
  const CVector3f pos = GetTranslation();
  if (mPathFindSearch.OnPath(pos) == CPathFindSearch::kR_Success) {
    mPositionHistory.Clear();
  }
  mPositionHistory.AddValue(pos);
}

void CElitePirate::UpdatePathDestination(CStateManager& mgr, const CVector3f& dest, float dt) {
  CVector3f target = dest;
  mPathFindSearch.SetAvoidanceFilter(4);
  ReleaseClaimedRegion(mgr);
  mPathFindNavigation.SetDestination(target);
  mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
  if (PathShagged(mgr, CTriggerData(0.f)) == true) {
    if (CMath::AbsF(dest.GetZ() - GetTranslation().GetZ()) > 2.f) {
      target.SetZ(GetTranslation().GetZ());
      mPathFindNavigation.SetDestination(target);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
    if (PathShagged(mgr, CTriggerData(0.f)) == true) {
      target = 0.5f * (dest + mPathDestination);
      mPathFindNavigation.SetDestination(target);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
    if (PathShagged(mgr, CTriggerData(0.f)) == true) {
      target = mPathDestination;
      mPathFindNavigation.SetDestination(target);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
  }
  mPathDestination = target;
  ClaimPathRegion(mgr);
}

void CElitePirate::UpdateAttackTimeLeft(CStateManager& mgr) {
  if (mgr.IsRandomAvailable()) {
    if (mgr.Random()->Float() > mData.GetRepeatedAttackChance()) {
      mAttackTimer = mgr.Random()->Float() * mAttackTimeVariation + GetAverageAttackTime();
    }
  }
}

void CElitePirate::ProcessStompGround(CStateManager& mgr) {
  const bool doubleWave = mAttackType == kAT_DoubleShockwave;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    CPlayer* player = mgr.Player(i);
    const float distance = (GetTranslation() - player->GetTranslation()).Magnitude();
    const float scale = doubleWave ? 1.f : 0.25f;
    const CVector3f modelScale = GetModelData()->GetScale();
    if (-(0.05f * distance - scale * modelScale.Magnitude()) > 0.f &&
        player->GetSurfaceRestraint() != CPlayer::kSR_Air && player->GetFluidCount() == 0) {
      const CPlayer::EPlayerMorphBallState state =
          player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
              ? player->GetMorphballTransitionState()
              : CPlayer::kMS_Unmorphed;
      if (state != CPlayer::kMS_Morphed) {
        const TUniqueId cameraId = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetUniqueId();
        if (mgr.GetCameraManager(0)->GetCurrentCameraId(false) == cameraId) {
          mgr.RumbleManager(mgr.MaskUIdNumPlayers(player->GetUniqueId()))
              ->Rumble(mgr, kRFX_CameraShake, 1.f, kRP_Two);
        }
      } else {
        const CVector3f impulse = (doubleWave ? 20.f : 10.f) * CVector3f::Up();
        player->Stop();
        player->ApplyImpulseWR(player->GetMass() * impulse, CAxisAngle::Identity());
        player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
      }
    }
  }
}

void CElitePirate::ApplyPlayerImpulse(CStateManager& mgr, float force, float lift) {
  CPlayer* player = mgr.Player(0);
  CVector3f dir = player->GetTranslation() - GetTranslation();
  dir.SetZ(0.f);
  dir.Normalize();
  dir *= force;
  dir.SetZ(lift);
  player->Stop();
  player->ApplyImpulseWR(player->GetMass() * dir, CAxisAngle::Identity());
  player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
}

void CElitePirate::SetShotAt(bool shotAt) { mShotAt = shotAt; }

CShockWaveInfo CElitePirate::GetShockWaveInfo() const {
  if (mAttackType == kAT_DoubleShockwave) {
    return CShockWaveInfo(mData.GetDoubleShockWave());
  }
  return CShockWaveInfo(mData.GetSingleShockWave());
}

bool CElitePirate::IsOnPath(CStateManager& mgr) const {
  return mPathFindSearch.OnPath(mgr.GetPlayer(0)->GetTranslation()) == CPathFindSearch::kR_Success;
}

bool CElitePirate::CanBeUnPossessed(CStateManager& mgr) const { return false; }

void CElitePirate::AngryAttackBegin(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mAngryAttackOver = false;
    switch (GetNearbyHintType(mgr)) {
    case 22:
      mShockwaveIsNext = true;
      break;
    case CScriptAIHint::kHT_GrenadeLauncherRaisedAim:
      mShockwaveIsNext = false;
      break;
    default:
      if (!mAngryChosen) {
        mShockwaveIsNext = mgr.Random()->Range(0.f, 1.f) < 0.7f;
      } else {
        mShockwaveIsNext = !mAngryChoice;
      }
      break;
    }
    mAngryChosen = true;
    mAngryChoice = mShockwaveIsNext;
  }
}

void CElitePirate::AngryAttackEnd(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    --mAngryCount;
    mAngryAttackOver = true;
  }
}

bool CElitePirate::AngryAttackOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAngryAttackOver;
}

bool CElitePirate::StillAngry(CStateManager& mgr, const CTriggerData& data) const {
  return mAngryCount > 0;
}

bool CElitePirate::ShockwaveIsNext(CStateManager& mgr, const CTriggerData& data) const {
  return mShockwaveIsNext;
}

bool CElitePirate::IsNearNoAttackHint(CStateManager& mgr) const {
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (const CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i])) {
      if (hint->GetHintType() == 26 && hint->GetCurrentAreaId() == GetCurrentAreaId() &&
          hint->GetActive() == true) {
        CVector3f dist = hint->GetTranslation() - GetTranslation();
        dist.SetZ(0.f);
        const float radius = hint->GetRadius();
        if (dist.Magnitude() < radius) {
          return true;
        }
      }
    }
  }
  return false;
}

int CElitePirate::GetNearbyHintType(CStateManager& mgr) const {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  float bestDist = FLT_MAX;
  const CScriptAIHint* best = nullptr;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (const CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i])) {
      if (hint->GetCurrentAreaId() == GetCurrentAreaId() && hint->GetActive() == true) {
        const int type = hint->GetHintType();
        if (type == 22 || type == 27 || type == CScriptAIHint::kHT_GrenadeLauncherRaisedAim) {
          const float dist = (hint->GetTranslation() - playerPos).Magnitude();
          if (dist < hint->GetRadius()) {
            if (hint->GetHintType() == 27) {
              return 27;
            }
            if (dist < bestDist) {
              bestDist = dist;
              best = hint;
            }
          }
        }
      }
    }
  }
  if (best != nullptr) {
    return best->GetHintType();
  }
  return -1;
}

bool CElitePirate::PlayerInNoAttack(CStateManager& mgr, const CTriggerData& data) const {
  return GetNearbyHintType(mgr) == 27;
}

CPFArea* CElitePirate::GetPathArea(CStateManager& mgr) const {
  return mgr.World()->Area(GetCurrentAreaId())->GetPostConstructed()->mPathArea;
}

void CElitePirate::ClaimPathRegion(CStateManager& mgr) {
  const rstl::reserved_vector< CVector3f, 16 >& waypoints = mPathFindSearch.GetWaypoints();
  if (waypoints.size() == 0) {
    return;
  }
  const CVector3f last = waypoints[waypoints.size() - 1];
  const uint mask = GetSearchPath()->GetCreatureMask();
  const uint flags = GetSearchPath()->GetRegionFlags();
  const CPFRegion* lastRegion = GetPathArea(mgr)->FindClosestRegion(last, flags, mask, 2.f);
  for (int i = waypoints.size() - 1; i > 0; --i) {
    const CVector3f mid = 0.5f * (waypoints[i] + waypoints[i - 1]);
    const uint midMask = GetSearchPath()->GetCreatureMask();
    const uint midFlags = GetSearchPath()->GetRegionFlags();
    CPFRegion* region = GetPathArea(mgr)->FindClosestRegion(mid, midFlags, midMask, 2.f);
    if (region != nullptr && region != lastRegion) {
      CPFRegionData* data = region->Data();
      if (data->GetAvoidanceFlags() == 0) {
        data->SetAvoidanceFlags(data->GetAvoidanceFlags() | 4);
        mClaimedRegion = region->GetIndex();
        return;
      }
    }
  }
}

void CElitePirate::ReleaseClaimedRegion(CStateManager& mgr) {
  if (mClaimedRegion != -1) {
    if (GetPathArea(mgr) == nullptr) {
      return;
    }
    CPFRegion* region = GetPathArea(mgr)->GetRegionPtr(mClaimedRegion);
    if (region != nullptr) {
      CPFRegionData* data = region->Data();
      data->SetAvoidanceFlags(data->GetAvoidanceFlags() & ~4);
    }
    mClaimedRegion = -1;
  }
}

CEntity* LoadElitePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrElitePirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CElitePirateData data(
      sldrThis.patterned.stateMachine2, sldrThis.patterned.animationInformation.initial_anim,
      LdrToDamageInfo(sldrThis.meleeDamage), sldrThis.darkShield, sldrThis.darkShieldSound,
      sldrThis.darkShieldPop, sldrThis.lightShield, sldrThis.maxMeleeRange,
      sldrThis.minShockwaveRange, sldrThis.maxShockwaveRange, sldrThis.minRocketRange,
      sldrThis.maxRocketRange, sldrThis.unknown_0x5236c2b6, sldrThis.unknown_0x01eaab17,
      sldrThis.tauntInterval, sldrThis.lightShieldSound, sldrThis.lightShieldPop,
      sldrThis.tauntVariance, sldrThis.unknown_0x28b39197, sldrThis.unknown_0xe27de71b,
      sldrThis.unknown_0x665e7ace, sldrThis.unknown_0xacd4d06d, sldrThis.repeatedAttackChance,
      sldrThis.energyAttractionForce, sldrThis.alwaysFF, sldrThis.alwaysFF_0x23f5e1ee,
      LdrToActorParameters(sldrThis.rocketLauncherActorInfo),
      LdrToAnimationParameters(sldrThis.rocketLauncherAnimInfo),
      LdrToAnimationParameters(sldrThis.unknown_0x7e6e0d38), sldrThis.rocket,
      LdrToDamageInfo(sldrThis.rocketDamage), sldrThis.unknown_0x624222f8,
      sldrThis.unknown_0x31e43a1c, sldrThis.visorElectricEffect, sldrThis.sound_VisorElectric,
      sldrThis.singleShockWaveInfo, sldrThis.doubleShockWaveInfo, sldrThis.shieldedModel,
      sldrThis.shieldedSkinRules);

  return rs_new CElitePirate(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
      LdrToActorParameters(sldrThis.actorInformation), data);
}

SElitePirate_FuncPtrs REL_loader_ElitePirate;

void SetRelLoaderFunctionToLoader() {
  REL_loader_ElitePirate.mLoader = LoadElitePirate;
  SetSElitePirate_FuncPtrs(&REL_loader_ElitePirate);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSElitePirate_FuncPtrs(nullptr); }
