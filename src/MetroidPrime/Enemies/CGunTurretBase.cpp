#include "MetroidPrime/Enemies/CGunTurretBase.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Cameras/CGameCamera.hpp"
#include "MetroidPrime/Enemies/CGunTurretTop.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGunTurretBase.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "REL/REL_Setup.h"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Animation/CJointData_LinearStorage.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"

#include <stdio.h>

// Guessed name; an empty debug-draw stub in the DOL.
void DebugDrawAABox(const CAABox& box, float r, float g, float b, float a);

const char* const CGunTurretBase::skConnectLocator = "connect_LCTR";

static CVector3f skPanLeftVector = CMatrix3f::RotateZ(CRelAngle::FromDegrees(85.f)) * CVector3f::Forward();
static CVector3f skPanRightVector =
    CMatrix3f::RotateZ(CRelAngle::FromDegrees(-85.f)) * CVector3f::Forward();

CGunTurretBase::CGunTurretBase(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CModelData& modelData, const CPatternedInfo& patternedInfo,
    const CDamageInfo& attackDamage, float hurtSleepDelay, float gunAimTurnSpeed,
    float gunLockOnTurnSpeed, float minTimeBetweenAttacks, float maxTimeBetweenAttacks,
    float minTimeBetweenShots, float maxTimeBetweenShots, float maxPitchAngleUp,
    const CActorParameters& actorParameters, bool gunRespawns, uchar minShotsInABurst,
    uchar maxShotsInABurst, bool isPirateTurret, CAssetId crscId, CAssetId pirateProjectile,
    CAssetId pirateProjectileEffect, bool unknown5cf1, bool unknown479d, float maxPitchAngleDown,
    float unknownFc03, float unknown8a35, float unknownD49b, float attackDelay, float patrolDelay,
    float withdrawDelay, float detectionHeightUp, float detectionHeightDown,
    float shotAngleVariance, float attackLeashTime, ushort gfFireShotSfx, ushort pirateFireShotSfx,
    ushort lockOnSfx, ushort gunPanSfx, ushort gfGunChargeSfx, ushort pirateGunChargeSfx,
    ushort gunLowerLoopedSfx, ushort gunLowerOffSfx, ushort gunRaiseLoopedSfx,
    ushort gunRaiseOffSfx, ushort pirateGunDeathLowerLoopedSfx, ushort gfGunDeathLowerLoopedSfx,
    ushort poleSparksSfx, float unknown80ce, float sfxFallOff, float sfxMaxDistance)
: CPatterned(static_cast< EPatternedAI >(0x43), uid, name, kFT_Zero, info, xf, modelData,
             patternedInfo, kMT_Flyer, kCT_One, static_cast< EBodyType >(5), actorParameters)
, mDetectionRange(patternedInfo.GetDetectionRange())
, mMaxAttackRange(patternedInfo.GetMaxAttackRange())
, mMinAttackRange(patternedInfo.GetMinAttackRange())
, mAttackDamage(attackDamage)
, mHurtSleepDelay(hurtSleepDelay)
, mTopId(kInvalidUniqueId)
, mGunDestroyed(false)
, mGunRespawns(gunRespawns)
, mTargetPos(CVector3f::Zero())
, mOriginalFront(xf.GetColumn(kDY))
, x80c_(-1)
, mGunAimTurnSpeed(CRelAngle::FromDegrees(gunAimTurnSpeed).AsRadians())
, mGunLockOnTurnSpeed(CRelAngle::FromDegrees(gunLockOnTurnSpeed).AsRadians())
, mGunRotation(CQuaternion::NoRotation())
, mTargetGunRotation(CQuaternion::NoRotation())
, mMinTimeBetweenAttacks(minTimeBetweenAttacks)
, mMaxTimeBetweenAttacks(maxTimeBetweenAttacks)
, mTimeBetweenAttacks(minTimeBetweenAttacks)
, mMinTimeBetweenShots(minTimeBetweenShots)
, mMaxTimeBetweenShots(maxTimeBetweenShots)
, mTimeBetweenShots(minTimeBetweenShots)
, mMinShotsInABurst(minShotsInABurst)
, mMaxShotsInABurst(maxShotsInABurst)
, mShotsInBurst(minShotsInABurst)
, mAttackTimer(mTimeBetweenAttacks)
, mShotTimer(mTimeBetweenShots)
, mShotCount(0)
, mIsPirateTurret(isPirateTurret)
, mCrsc(gpSimplePool->GetObj(SObjectTag('CRSC', crscId)), true)
, mPirateProjectile(pirateProjectile)
, mPirateProjectileEffect(
      pirateProjectileEffect == kInvalidAssetId
          ? rstl::optional_object< TLockedToken< CGenDescription > >()
          : rstl::optional_object< TLockedToken< CGenDescription > >(TLockedToken< CGenDescription >(
                gpSimplePool->GetObj(SObjectTag('PART', pirateProjectileEffect)))))
, mProjectileInfo(mPirateProjectile, attackDamage)
, x8ac_(0.f)
, x8b0_(0.f)
, mFiring(false)
, mEffectIndex(0)
, mInBurst(false)
, mHitTarget(kInvalidUniqueId)
, mHitTargetValid(false)
, mLastHitTarget(kInvalidUniqueId)
, x8be_(unknown5cf1)
, x8bf_(unknown479d)
, mMaxPitchAngleUp(CRelAngle::FromDegrees(maxPitchAngleUp).AsRadians())
, mMaxPitchAngleDown(CRelAngle::FromDegrees(maxPitchAngleDown).AsRadians())
, xfc03_(CRelAngle::FromDegrees(unknownFc03).AsRadians())
, mRaiseSpeed(unknown8a35 / 10.f)
, mDestroyedLowerSpeed(unknownD49b / 10.f)
, mAttackDelay(attackDelay)
, mPatrolDelay(patrolDelay)
, mWithdrawDelay(withdrawDelay)
, mShotAngleVariance(shotAngleVariance)
, mGunHit(false)
, mDelayTimer(0.f)
, mLowerDuration(0.f)
, mRaiseDuration(0.f)
, mPanDuration(0.f)
, mGunVulnerability(CDamageVulnerability::ImmuneVulnerabilty())
, mGFFireShotSfx(gfFireShotSfx)
, mPirateFireShotSfx(pirateFireShotSfx)
, mLockOnSfx(lockOnSfx)
, mGunPanSfx(gunPanSfx)
, mGFGunChargeSfx(gfGunChargeSfx)
, mPirateGunChargeSfx(pirateGunChargeSfx)
, mGunRaiseLoopedSfx(gunRaiseLoopedSfx)
, mGunRaiseOffSfx(gunRaiseOffSfx)
, mGunLowerLoopedSfx(gunLowerLoopedSfx)
, mGunLowerOffSfx(gunLowerOffSfx)
, mGFGunDeathLowerLoopedSfx(gfGunDeathLowerLoopedSfx)
, mPirateGunDeathLowerLoopedSfx(pirateGunDeathLowerLoopedSfx)
, mPoleSparksSfx(poleSparksSfx)
, mDetectionHeightUp(detectionHeightUp)
, mDetectionHeightDown(detectionHeightDown)
, mCanCharge(true)
, mCharging(false)
, mChargeTime(0.f)
, mFirstShot(true)
, mLeashTimer(mPlayerLeashTime)
, mAttackLeashTime(attackLeashTime)
, mAttackLeashTimer(attackLeashTime)
, mTargetIsNonPlayer(false)
, mChargeSfx(0)
, mShellWaypointId(kInvalidUniqueId)
, mCollisionPrimitive(GetAnimationData()->GetBoundingBox(), GetMaterialList())
, mAdditiveAnim(0)
, mMaxRaise(CMath::Min(unknown80ce / 10.f, 1.f))
, mRaise(0.f)
, mSfxFallOff(sfxFallOff)
, mSfxMaxDistance(sfxMaxDistance)
, x9b4_(CAABox::MakeNullBox()) {
  mAlert = true;
  mOccluded = false;
  mKnockBackController.EnableKnockBackPhysics(false);
  SetDrawShadow(false);
  mProjectileInfo.Token().Lock();

  CPASAnimParmData lowerParms(pas::kAS_Generate, CPASAnimParm::FromEnum(3),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter());
  CPASAnimParmData raiseParms(pas::kAS_Generate, CPASAnimParm::FromEnum(4),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                              CPASAnimParm::NoParameter());
  CPASAnimParmData panParms(pas::kAS_Generate, CPASAnimParm::FromEnum(2),
                            CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                            CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                            CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                            CPASAnimParm::NoParameter());
  mLowerDuration = GetAnimationDuration(lowerParms);
  mRaiseDuration = GetAnimationDuration(raiseParms);
  mPanDuration = GetAnimationDuration(panParms);

  AnimationData()->SetKeepJSPose(true);

  rstl::pair< float, int > best = AnimationData()->GetPASDatabase().FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(5),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter()),
      -1);
  if (best.first > FLT_EPSILON) {
    mAdditiveAnim = best.second;
  }
}

void CGunTurretBase::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Delete:
  case kSM_XHIT:
  case kSM_AIUpdateDisabled:
  case kSM_Decrement:
  case kSM_Activate:
    break;
  case kSM_AreaLoaded: {
    rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
    for (; it != GetConnectionList().end(); ++it) {
      const EScriptObjectState state = it->state;
      if (state == kSS_APRC) {
        TUniqueId id = mgr.GetIdForScript(it->objId);
        if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(id))) {
          mTopId = id;
          mGunVulnerability = *top->GetDamageVulnerability();
          *top->DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
        }
      }
      if (state == kSS_Attack) {
        TUniqueId id = mgr.GetIdForScript(it->objId);
        if (TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id))) {
          mShellWaypointId = id;
        }
      }
    }
    break;
  }
  case kSM_Create:
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Internal7);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    AddMaterial(kMT_ExcludeFromRadar, mgr);
    *DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
    if (CActorLights* lights = ActorLights()) {
      lights->SetNeedsRelight(true);
    }
    break;
  case kSM_Alert:
    mLeashTimer = 0.f;
    mAlert = true;
    break;
  case kSM_Reset:
    mAlert = false;
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::ShouldPatrol)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::ShouldAttack)},
    {"ShouldPan", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::ShouldPan)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::AnimOver)},
    {"SpawnOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::SpawnOver)},
    {"WithdrawOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::WithdrawOver)},
    {"Attacked", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::Attacked)},
    {"Delay", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::Delay)},
    {"PatrolDelay",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::PatrolDelay)},
    {"WithdrawDelay",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::WithdrawDelay)},
    {"GunDestroyed",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::GunDestroyed)},
    {"AttackExitDone",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretBase::AttackExitDone)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Sleep)},
    {"Spawn", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Spawn)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Patrol)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Attack)},
    {"Withdraw", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Withdraw)},
    {"Flinch", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::Flinch)},
    {"IntoPan", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::IntoPan)},
    {"PanLeft", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::PanLeft)},
    {"PanRight", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::PanRight)},
    {"AttackExit",
     static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::AttackExit)},
    {"OpenDoor", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::OpenDoor)},
    {"CloseDoor", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretBase::CloseDoor)},
};

void CGunTurretBase::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

bool CGunTurretBase::ShouldPatrol(CStateManager&, const CTriggerData&) const {
  return mLeashTimer < mPlayerLeashTime;
}

bool CGunTurretBase::ShouldAttack(CStateManager& mgr, const CTriggerData&) const {
  bool ret = false;
  if (TCastToConstPtr< CPhysicsActor >(mgr.GetObjectById(mHitTarget))) {
    if (mAlert && !mOccluded) {
      ret = true;
    }
  }
  return ret;
}

bool CGunTurretBase::AttackExitDone(CStateManager&, const CTriggerData&) const {
  if (mGunDestroyed) {
    return true;
  }
  return CVector3f::GetAngleDiff(mGunRotation.BuildTransform() * CVector3f::Forward(),
                                 CVector3f::Forward()) < 0.1f;
}

bool CGunTurretBase::GunDestroyed(CStateManager&, const CTriggerData&) const {
  return mGunDestroyed;
}

bool CGunTurretBase::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Attacked(mgr, data);
}

bool CGunTurretBase::Delay(CStateManager&, const CTriggerData&) const {
  if (mGunDestroyed) {
    if (mStateMachine->GetTime() > mHurtSleepDelay && mGunRespawns) {
      const_cast< CGunTurretBase* >(this)->mGunDestroyed = false;
      return true;
    }
    return false;
  }
  return true;
}

bool CGunTurretBase::PatrolDelay(CStateManager&, const CTriggerData&) const {
  const bool over = mStateMachine->GetTime() > mPatrolDelay;
  return over;
}

bool CGunTurretBase::WithdrawDelay(CStateManager&, const CTriggerData&) const {
  return mDelayTimer > mWithdrawDelay;
}

bool CGunTurretBase::ShouldPan(CStateManager&, const CTriggerData&) const {
  const bool over = mStateMachine->GetTime() > mPanDuration;
  return over;
}

bool CGunTurretBase::PlayerInRange(CStateManager& mgr, float range) const {
  const CVector3f pos = GetTranslation();
  const float rangeSq = range * range;
  if (TCastToConstPtr< CGunTurretTop >(mgr.GetObjectById(mTopId))) {
    for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
      if (x80c_ == -1 || x80c_ != mgr.GetPlayer(i)->GetPlayerIndex()) {
        const CPlayer* player = mgr.GetPlayer(i);
        const CVector3f delta = player->GetTranslation() - pos;
        if (delta.MagSquared() < rangeSq &&
            InDetectionHeight(*player, mDetectionHeightUp, -mDetectionHeightDown)) {
          return true;
        }
      }
    }
    if (x8bf_) {
      static CMaterialFilter filter =
          CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Character),
                                              CMaterialList(kMT_NoPlatformCollision, kMT_Player));
      rstl::reserved_vector< TUniqueId, 1024 > nearList;
      const float r = mDetectionRange;
      mgr.BuildNearList(nearList, CAABox(pos - CVector3f(r, r, r), pos + CVector3f(r, r, r)),
                        filter, this);
      for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
           it != nearList.end(); ++it) {
        const CPatterned* patterned = TCastToConstPtr< CPatterned >(mgr.GetObjectById(*it));
        if (patterned && patterned->GetUniqueId() != mTopId && patterned->GetAlive()) {
          if (!TCastToConstPtr< CGunTurretBase >(patterned) &&
              !TCastToConstPtr< CGunTurretTop >(patterned)) {
            const CVector3f delta = patterned->GetTranslation() - pos;
            if (delta.MagSquared() < rangeSq &&
                InDetectionHeight(*patterned, mDetectionHeightUp, -mDetectionHeightDown)) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

bool CGunTurretBase::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CGunTurretBase::SpawnOver(CStateManager&, const CTriggerData&) const {
  return mRaise == mMaxRaise;
}

bool CGunTurretBase::WithdrawOver(CStateManager&, const CTriggerData&) const {
  return mRaise == 0.f;
}

void CGunTurretBase::OpenDoor(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    mState = kS_OpenDoor;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CGunTurretBase::CloseDoor(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    mState = kS_CloseDoor;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CGunTurretBase::IntoPan(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Two, -1));
    mState = kS_IntoPan;
    if (!mGunDestroyed) {
      PlaySfx(mGunPanSfx | 0x80000000, mgr);
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Two, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CGunTurretBase::PanLeft(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    mState = kS_PanLeft;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CGunTurretBase::PanRight(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    mState = kS_PanRight;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CGunTurretBase::Withdraw(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_NextState));
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      top->SetShouldPatrol(false);
      top->RemoveMaterial(kMT_RadarObject, mgr);
    }
    mGunHit = false;
    StopLoopedSounds();
    mState = kS_Withdraw;
    ushort sfx;
    if (mGunDestroyed) {
      sfx = mIsPirateTurret ? mPirateGunDeathLowerLoopedSfx : mGFGunDeathLowerLoopedSfx;
    } else {
      sfx = mGunLowerLoopedSfx;
    }
    PlaySfx(sfx | 0x80000000, mgr);
    break;
  }
  case kStateMsg_Update:
    LowerGun(dt);
    break;
  case kStateMsg_Deactivate:
    if (mGunDestroyed) {
      if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
        top->Revive(mgr);
      }
    }
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      mGunVulnerability = *top->GetDamageVulnerability();
      *top->DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
      top->RemoveMaterial(kMT_Unknown59, mgr);
    }
    StopLoopedSounds();
    if (!mGunDestroyed) {
      PlaySfx(mGunLowerOffSfx, mgr);
    }
    if (mGunDestroyed) {
      AnimationData()->SetEffectState(rstl::string_l("sparks"), false, mgr);
    }
    break;
  }
}

void CGunTurretBase::Spawn(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mState = kS_Spawn;
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      *top->DamageVulnerability() = mGunVulnerability;
      top->AddMaterial(kMT_Unknown59, mgr);
      top->AddMaterial(kMT_RadarObject, mgr);
    }
    PlaySfx(mGunRaiseLoopedSfx | 0x80000000, mgr);
    break;
  case kStateMsg_Update:
    RaiseGun(dt);
    break;
  case kStateMsg_Deactivate:
    StopLoopedSounds();
    PlaySfx(mGunRaiseOffSfx, mgr);
    break;
  }
}

void CGunTurretBase::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      top->RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    }
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    mState = kS_Sleep;
    mAlert = true;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretBase::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      top->SetShouldPatrol(true);
      top->AddMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    }
    mState = kS_Patrol;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretBase::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_NextState));
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      top->SetShouldAttack(true);
    }
    StopLoopedSounds();
    mState = kS_Attack;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      top->SetShouldAttack(false);
    }
    break;
  }
}

void CGunTurretBase::AttackExit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_AttackExit;
    ResetAttack(mgr);
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretBase::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {}

void CGunTurretBase::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  UpdateGunPose(mgr, 1.f / 60.f);
}

void CGunTurretBase::Render(const CStateManager& mgr) const {
  CPatterned::Render(mgr);
  DebugDrawAABox(x9b4_.GetTransformedAABox(CTransform4f::Translate(GetTranslation())), 1.f, 1.f,
                 1.f, 1.f);
}

void CGunTurretBase::Think(float dt, CStateManager& mgr) {
  if (GetActive()) {
    if (mgr.GetWorld()->GetArea(GetCurrentAreaId())->GetOcclusionState() == CGameArea::kOS_Occluded) {
      mOccluded = true;
    } else {
      mOccluded = false;
    }
    if (PlayerInRange(mgr, mDetectionRange)) {
      mLeashTimer = 0.f;
    } else if (mLeashTimer < mPlayerLeashTime && mAttackLeashTimer >= mAttackLeashTime) {
      mLeashTimer += dt;
    }

    if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
      if (mCanCharge && mFiring && !mCharging && !mGunDestroyed &&
          mAttackTimer >= mTimeBetweenAttacks && !top->GetBodyController()->IsFrozen()) {
        mCharging = true;
        mCanCharge = false;
        top->SetChargeEffect(mgr, true, mIsPirateTurret);
        CAudioSys::C3DEmitterParmData parms(mSfxMaxDistance, mSfxFallOff, 1, 127, 20);
        parms.mPos = GetTranslation();
        parms.mDir = CVector3f::Zero();
        parms.mSfxId = mIsPirateTurret ? mPirateGunChargeSfx : mGFGunChargeSfx;
        mChargeSfx = CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, false,
                                             CSfxManager::kMedPriority);
      }
      if (mCharging &&
          (mAttackTimer >= mTimeBetweenAttacks || top->GetBodyController()->IsFrozen())) {
        if (mChargeTime >= mAttackDelay || top->GetBodyController()->IsFrozen()) {
          mCharging = false;
          mChargeTime = 0.f;
          if (CGunTurretTop* top2 = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
            top2->SetChargeEffect(mgr, false, mIsPirateTurret);
            CSfxManager::RemoveEmitter(mChargeSfx);
          }
        } else {
          mChargeTime += dt;
        }
      }
      if (mFiring && !mGunDestroyed) {
        mFirstShot = false;
      }
      if (!top->GetBodyController()->IsFrozen()) {
        UpdateAttack(mgr, dt);
      } else {
        ResetAttack(mgr);
      }
    }
  }

  if (mHitTargetValid) {
    mgr.ApplyDamage(GetUniqueId(), mLastHitTarget, GetUniqueId(), mAttackDamage,
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), CMaterialList()), CVector3f::Zero());
    const CPatterned* patterned = TCastToConstPtr< CPatterned >(mgr.GetObjectById(mHitTarget));
    if (patterned && !patterned->GetAlive()) {
      mHitTarget = kInvalidUniqueId;
    }
    mHitTargetValid = false;
    mLastHitTarget = kInvalidUniqueId;
  }

  if (GetActive()) {
    if (mGunDestroyed) {
      mDelayTimer += dt;
    } else {
      mDelayTimer = 0.f;
    }
    CPatterned::Think(dt, mgr);
    SetBoundingBox(GetAnimationData()->GetBoundingBox());
    mCollisionPrimitive = CCollidableAABox(GetAnimationData()->GetBoundingBox(), GetMaterialList());

    CActor* target = FindTarget(mgr);
    const CPhysicsActor* current = TCastToConstPtr< CPhysicsActor >(mgr.GetObjectById(mHitTarget));
    if (target && target != current && (mState == kS_Patrol || mState == kS_Attack)) {
      PlaySfx(mLockOnSfx, mgr);
    }
    if (target) {
      mHitTarget = target->GetUniqueId();
      mAttackLeashTimer = 0.f;
    } else if (!mTargetIsNonPlayer && mAttackLeashTimer < mAttackLeashTime) {
      mAttackLeashTimer += dt;
    } else {
      mHitTarget = kInvalidUniqueId;
    }

    if (CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(mShellWaypointId))) {
      if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
        CTransform4f shellXf = top->GetScaledLocatorTransform("shell_LCTR");
        wp->SetTransform(top->GetTransform() * shellXf * CTransform4f::RotateY(CRelAngle::FromDegrees(90.f)));
      }
    }
    AnimationData()->AddAdditiveAnimation(mAdditiveAnim, mRaise, false, false);
  }
}

void CGunTurretBase::Touch(CActor& actor, CStateManager& mgr) { CPatterned::Touch(actor, mgr); }

void CGunTurretBase::DestroyGun(CStateManager& mgr) {
  mGunDestroyed = true;
  mGunHit = false;
  AnimationData()->SetEffectState(rstl::string_l("sparks"), true, mgr);
  PlaySfx(mPoleSparksSfx | 0x80000000, mgr);
  ResetAttack(mgr);
  CSfxManager::RemoveEmitter(mChargeSfx);
}

void CGunTurretBase::UpdateGunPose(CStateManager& mgr, float dt) {
  CSegId seg = GetAnimationData()->GetLocatorSegId(skConnectLocator);
  if (seg == CSegId(0xFF)) {
    return;
  }
  if (mState >= kS_Patrol && mState < kS_Spawn) {
    if (CPhysicsActor* target = TCastToPtr< CPhysicsActor >(mgr.ObjectById(mHitTarget))) {
      if (mTopId != kInvalidUniqueId) {
        if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
          CVector3f prevTarget = mTargetPos;
          CVector3f gunPos = GetGunFirePosition(mgr);
          mTargetPos = target->GetAimPosition(mgr, 0.f);
          if (CPlayer* player = TCastToPtr< CPlayer >(target)) {
            prevTarget = mProjectileInfo.PredictInterceptPos(gunPos, mTargetPos, *player, true, dt);
          }
          top->SetTarget(mTargetPos + (prevTarget - mTargetPos));
        }
      }
      UpdateGunOrientation(mgr, seg, mState == kS_Attack, dt);
    } else {
      UpdateGunOrientation(mgr, seg, false, dt);
    }
  } else {
    mTargetGunRotation = GetAnimationData()->JointData().Rotation(seg.val());
    CTransform4f xf = mTargetGunRotation.BuildTransform4f();
    x8b0_ = -atan2(xf.Get01(), xf.Get11());
    mFiring = false;
  }
  mGunRotation = GetAnimationData()->JointData().Rotation(seg.val());
  if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
    top->SetTransform(GetTransform() * GetScaledLocatorTransform(seg));
  }
}

void CGunTurretBase::ResetAttack(CStateManager& mgr) {
  mTimeBetweenAttacks = mgr.Random()->Range(mMinTimeBetweenAttacks, mMaxTimeBetweenAttacks);
  mTimeBetweenShots = mgr.Random()->Range(mMinTimeBetweenShots, mMaxTimeBetweenShots);
  mShotsInBurst = mMinShotsInABurst + mgr.Random()->Range(0, mMaxShotsInABurst - mMinShotsInABurst);
  mAttackTimer = mTimeBetweenAttacks;
  mShotTimer = mTimeBetweenShots;
  mShotCount = 0;
  mInBurst = false;
  mFiring = false;
  if (CGunTurretTop* top = TCastToPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
    top->SetChargeEffect(mgr, false, mIsPirateTurret);
  }
  mCharging = false;
  mChargeTime = 0.f;
  mCanCharge = true;
  mFirstShot = true;
  mAttackLeashTimer = mAttackLeashTime;
}

CVector3f CGunTurretBase::GetGunFirePosition(CStateManager& mgr) const {
  if (const CGunTurretTop* top = TCastToConstPtr< CGunTurretTop >(mgr.ObjectById(mTopId))) {
    const CVector3f topPos = top->GetTranslation();
    const CTransform4f gunXf = top->GetScaledLocatorTransform("gun_LCTR");
    return topPos + top->GetTransform().Rotate(gunXf.GetTranslation());
  }
  return GetTranslation();
}

bool CGunTurretBase::InRange(const CActor& actor, float range) const {
  return (actor.GetTranslation() - GetTranslation()).MagSquared() < range * range;
}

void CGunTurretBase::LaunchProjectile(CStateManager& mgr) {
  const CGunTurretTop* top = TCastToConstPtr< CGunTurretTop >(mgr.GetObjectById(mTopId));
  if (!top) {
    return;
  }
  CTransform4f topXf = top->GetTransform();
  if (!mProjectileInfo.Token().IsLoaded()) {
    return;
  }
  if (!mgr.CanCreateProjectile(GetUniqueId(), kWT_AI, 8)) {
    return;
  }
  CVector3f gunPos = GetGunFirePosition(mgr);
  CVector3f target = mTargetPos;
  float signX = (mgr.Random()->Next() % 2) == 0 ? 1.f : -1.f;
  float signZ = (mgr.Random()->Next() % 2) == 0 ? 1.f : -1.f;
  float angleX = signX * (mShotAngleVariance * mgr.Random()->Float());
  float angleZ = signZ * (mShotAngleVariance * mgr.Random()->Float());
  CTransform4f xf = CTransform4f::LookAt(gunPos, target, CVector3f::Up()) *
                    (CTransform4f::RotateX(CRelAngle::FromDegrees(angleX)) *
                     CTransform4f::RotateZ(CRelAngle::FromDegrees(angleZ)));
  CEnergyProjectile* projectile = rs_new CEnergyProjectile(
      true, mProjectileInfo.Token(), static_cast< EWeaponType >(mAttackDamage.GetWeaponMode1()),
      xf, kMT_NoPlatformCollision, mProjectileInfo.GetDamage(), mgr.AllocateUniqueId(), GetCurrentAreaId(),
      GetUniqueId(), kInvalidUniqueId, 0, false, CVector3f(1.f, 1.f, 1.f),
      CImpactVisorEffect::ParticleEffect(mPirateProjectileEffect, CSfxManager::kInternalInvalidSfxId, false),
      false, true, false, 1.f, 4.f, 4.f);
  mgr.AddObject(projectile);
}

void CGunTurretBase::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                     EUserEventType type, float dt) {
  switch (type) {
  case kUE_EffectOff:
    if (mGunDestroyed) {
      AnimationData()->SetEffectState(rstl::string_l("sparks"), false, mgr);
    }
  case kUE_EffectOn:
    break;
  }
}

float CGunTurretBase::GetClosestCameraDistanceSq(CStateManager& mgr) const {
  float distanceSquared = 3.4028235e38f;
  const CVector3f position = GetTranslation();
  for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CGameCamera* camera = mgr.GetCameraManager(i)->GetCurrentCamera(mgr, true);
    const CVector3f delta = camera->GetTranslation() - position;
    const float cameraDistanceSquared = delta.MagSquared();
    if (cameraDistanceSquared < distanceSquared) {
      distanceSquared = cameraDistanceSquared;
    }
  }
  return distanceSquared;
}

bool CGunTurretBase::InDetectionHeight(const CActor& actor, float up, float down) const {
  const float dz = actor.GetTranslation().GetZ() - GetTranslation().GetZ();
  if ((up == 0.f || dz < up) && (down == 0.f || dz > down)) {
    return true;
  }
  return false;
}

rstl::optional_object< CAABox > CGunTurretBase::GetTouchBounds() const {
  return mCollisionPrimitive.GetBox().GetTransformedAABox(GetTransform());
}

void CGunTurretBase::RaiseGun(float dt) {
  float raise = mRaise + mRaiseSpeed * dt;
  mRaise = raise < mMaxRaise ? raise : mMaxRaise;
}

void CGunTurretBase::LowerGun(float dt) {
  float speed;
  if (mGunDestroyed) {
    speed = mDestroyedLowerSpeed * dt;
  } else {
    speed = mRaiseSpeed * dt;
  }
  float raise = mRaise - speed;
  mRaise = 0.f < raise ? raise : 0.f;
}

CAABox CGunTurretBase::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox bounds = GetModelBounds();
  if (mgr.GetPlayer(0)->GetOrbitTargetId() == GetUniqueId() && !mGunDestroyed &&
      (mState == kS_Attack || mState == kS_AttackExit || mState == kS_Patrol ||
       (mState >= kS_IntoPan && mState <= kS_PanRight) ||
       (mState == kS_Spawn && mRaise > 0.1f) || (mState == kS_Withdraw && mRaise > 0.1f))) {
    CTransform4f locXf = GetScaledLocatorTransform(skConnectLocator);
    CTransform4f xf = GetTransform() * CTransform4f::Translate(-GetTranslation()) * locXf;
    if (const CGunTurretTop* top = TCastToConstPtr< CGunTurretTop >(mgr.GetObjectById(mTopId))) {
      CAABox topBounds = top->GetModelBounds();
      CAABox box = topBounds.GetTransformedAABox(CTransform4f::Translate(xf.GetTranslation()));
      bounds.AccumulateBounds(box.GetMinPoint());
      bounds.AccumulateBounds(box.GetMaxPoint());
    }
  }
  return bounds;
}

void CGunTurretBase::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                     const CModelFlags& flags) const {
  const CModelData* modelData = GetModelData();
  if (!modelData->IsNull()) {
    modelData->Render(CModelData::kWM_Normal, xf, nullptr, flags);
  }
  if (mgr.GetPlayer(0)->GetOrbitTargetId() == GetUniqueId() && !mGunDestroyed &&
      (mState == kS_Attack || mState == kS_AttackExit || mState == kS_Patrol ||
       (mState >= kS_IntoPan && mState <= kS_PanRight) ||
       (mState == kS_Spawn && mRaise > 0.1f) || (mState == kS_Withdraw && mRaise > 0.1f))) {
    CTransform4f topXf = xf * GetScaledLocatorTransform(skConnectLocator);
    if (const CGunTurretTop* top = TCastToConstPtr< CGunTurretTop >(mgr.GetObjectById(mTopId))) {
      top->ScanVisorRender(mgr, topXf, flags);
    }
  }
}

CAABox CGunTurretBase::GetModelBounds() const {
  CAABox box = CAABox::MakeMaxInvertedBox();
  box = GetAnimationData()->CalcBoundingBoxFromModelVerts();
  return box.GetTransformedAABox(GetTransform() * CTransform4f::Translate(-GetTranslation()) *
                                 CTransform4f::Scale(GetModelData()->GetScale()));
}

// Guessed name; unreferenced in the REL.
int CGunTurretBase::GetRenderAlpha(const CStateManager& mgr) const {
  return GetRenderAlphaBufferAlpha(mgr);
}

CEntity* LoadGunTurretBase(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrGunTurretBase sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrGunTurretBase.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CGunTurretBase(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, nullptr),
      LdrToDamageInfo(sldrThis.attackDamage), sldrThis.hurtSleepDelay, sldrThis.gunAimTurnSpeed,
      sldrThis.gunLockOnTurnSpeed, sldrThis.minTimeBetweenAttacks, sldrThis.maxTimeBetweenAttacks,
      sldrThis.minTimeBetweenShots, sldrThis.maxTimeBetweenShots, sldrThis.maxPitchAngleUp,
      LdrToActorParameters(sldrThis.actorInformation), sldrThis.gunRespawns,
      sldrThis.minShotsInABurst, sldrThis.maxShotsInABurst, sldrThis.isPirateTurret,
      sldrThis.cRSC, sldrThis.pirateProjectileEffect, sldrThis.alwaysFF,
      sldrThis.unknown_0x5cf12e9a, sldrThis.unknown_0x479d8dc4, sldrThis.maxPitchAngleDown,
      sldrThis.unknown_0xfc036e93, sldrThis.unknown_0x8a35b1ea, sldrThis.unknown_0xd49bec5a,
      sldrThis.attackDelay, sldrThis.patrolDelay, sldrThis.withdrawDelay,
      sldrThis.detectionHeightUp, sldrThis.detectionHeightDown, sldrThis.shotAngleVariance,
      sldrThis.attackLeashTime, sldrThis.gFFireShotSound, sldrThis.pirateFireShotSound,
      sldrThis.lockOnSound, sldrThis.gunPanSound, sldrThis.gFGunChargeSound,
      sldrThis.pirateGunChargeSound, sldrThis.gunLowerLoopedSound, sldrThis.gunLowerOffSound,
      sldrThis.gunRaiseLoopedSound, sldrThis.gunRaiseOffSound,
      sldrThis.pirateGunDeathLowerLoopedSound, sldrThis.gFGunDeathLowerLoopedSound,
      sldrThis.poleSparksSound, sldrThis.unknown_0x80ce481a, sldrThis.soundFallOff,
      sldrThis.maxAudibleDistance);
}

SGunTurretBase_FuncPtrs REL_loader_GunTurret;

void SetRelLoaderFunctionToLoader() {
  REL_loader_GunTurret.mLoadBase = LoadGunTurretBase;
  REL_loader_GunTurret.mLoadTop = LoadGunTurretTop;
  SetSGunTurretBase_FuncPtrs(&REL_loader_GunTurret);
}

void RELMain() { SetRelLoaderFunctionToLoader(); }

void RELExit() { SetSGunTurretBase_FuncPtrs(nullptr); }

CGunTurretBase::~CGunTurretBase() {}
