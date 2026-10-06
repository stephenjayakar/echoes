#include "MetroidPrime/Enemies/CSporbBase.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSporbPowerBomb.hpp"
#include "MetroidPrime/Enemies/CSporbProjectile.hpp"
#include "MetroidPrime/Enemies/CSporbTop.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSporbBase.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"


CEntity* REL_LoadSporbTop(CStateManager& mgr, CInputStream& input, CEntityInfo& info);
CEntity* REL_LoadSporbNeedle(CStateManager& mgr, CInputStream& input, CEntityInfo& info);
CEntity* REL_LoadSporbProjectile(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

const char* const CSporbBase::skTopAttachLocator = "Skeleton_Root_2_SDK";

TUniqueId gSporbGrabbedBallOwner = kInvalidUniqueId; // Guessed name; base holding the ball.

rstl::optional_object< CAABox > CSporbBase::GetTouchBounds() const { return GetBoundingBox(); }

CSporbBase::~CSporbBase() {}

CSporbBase::CSporbBase(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CModelData& mData, const CPatternedInfo& pInfo, const CActorParameters& aParms,
    float minTimeBetweenAttacks, float maxTimeBetweenAttacks, float minTimeBetweenShots,
    float maxTimeBetweenShots, uchar minShotsInABurst, uchar maxShotsInABurst,
    float shotAngleVariance, const CVector3f& attackAimOffset, float grabberOutAcceleration,
    float grabberInAcceleration, float initialGrabberOutSpeed, float initialGrabberInSpeed,
    float grabberAttachTime, float minGrabberGrabTime, float maxGrabberGrabTime, float spitForce,
    CAssetId tendrilParticleEffect, ushort grabberFireSfx, ushort grabberFlightSfx,
    ushort grabberHitPlayerSfx, ushort grabberHitWorldSfx, ushort grabberRetractSfx,
    ushort grabberRetractMissedPlayerSfx, ushort morphballSpitSfx, ushort grabberExplosionSfx,
    ushort ballEscapeSfx, ushort needleTelegraphSfx, ushort grabberTelegraphSfx, float spitDamage,
    float grabDamage, float ballEscapeDamage, float maxGrabberGrabRange, float minGrabberGrabRange,
    bool isPowerBombGuardian, CAssetId powerBombProjectileParticle,
    const CDamageInfo& powerBombProjectileDamage, float maxPowerBombProjectileHeight,
    float powerBombProjectileFuseTime, ushort powerBombProjectileSfx, float x9f4, float x9f8,
    float powerBombProjectileStartDamageTime, float powerBombProjectileEndDamageTime,
    const StageList& stages, float xa08, float xa5c, float xa60,
    float powerBombProjectileDamageWaitTime)
: CPatterned(static_cast< EPatternedAI >(0x47), uid, name, kFT_Zero, info, xf, mData, pInfo,
             kMT_Flyer, kCT_One, static_cast< EBodyType >(5), aParms)
, mTopId(kInvalidUniqueId)
, mProjectileId(kInvalidUniqueId)
, mDetectionRange(pInfo.GetDetectionRange())
, mMaxAttackRange(pInfo.GetMaxAttackRange())
, mMinAttackRange(pInfo.GetMinAttackRange())
, x7e8_(1.f)
, x7ec_(0.f)
, x7f0_(0.f)
, x7f4_(0.f)
, x7f8_(0.f)
, x7fc_(0.1f)
, x800_(kInvalidUniqueId)
, mSpitDamage(CDamageInfo(CWeaponMode(kWT_AI), spitDamage, 0.f, 0.f, false, false))
, mGrabDamage(CDamageInfo(CWeaponMode(kWT_AI), grabDamage, 0.f, 0.f, false, false))
, mBallEscapeDamage(CDamageInfo(CWeaponMode(kWT_AI), 0.f, 0.f, 0.f, true, false))
, mMaxTimeBetweenShots2(minTimeBetweenShots)
, x85c_(0.f)
, x860_(0.f)
, mMinTimeBetweenAttacks2(minTimeBetweenAttacks)
, mMinTimeBetweenShots2(minTimeBetweenShots)
, mMinShotsInABurst2(minShotsInABurst)
, mMinTimeBetweenAttacks(minTimeBetweenAttacks)
, mMaxTimeBetweenAttacks(maxTimeBetweenAttacks)
, mMinTimeBetweenShots(minTimeBetweenShots)
, mMaxTimeBetweenShots(maxTimeBetweenShots)
, mMinShotsInABurst(minShotsInABurst)
, mMaxShotsInABurst(maxShotsInABurst)
, x886_(0)
, x887_(0)
, x888_(true)
, x889_(false)
, x88a_(false)
, x88b_(false)
, x88c_(false)
, x88d_(false)
, x88e_(false)
, x890_(CSfxManager::kInternalInvalidSfxId)
, x892_(CSfxManager::kInternalInvalidSfxId)
, x894_(mPlayerLeashTime)
, x898_(kInvalidEditorId)
, mShotAngleVariance(shotAngleVariance)
, mAttackAimOffset(attackAimOffset)
, x8ac_(0)
, mInitialGrabberOutSpeed(initialGrabberOutSpeed)
, mInitialGrabberInSpeed(initialGrabberInSpeed)
, mGrabberOutAcceleration(grabberOutAcceleration)
, mGrabberInAcceleration(grabberInAcceleration)
, x8c0_(CVector3f::Zero())
, x8cc_(0.f)
, x8d0_(0.05f)
, x8d4_(0.f)
, mGrabberAttachTime(grabberAttachTime)
, x8dc_(initialGrabberOutSpeed)
, x8e0_(initialGrabberInSpeed)
, mMinGrabberGrabTime(minGrabberGrabTime)
, mMaxGrabberGrabTime(maxGrabberGrabTime)
, x8ec_(minGrabberGrabTime)
, x8f0_(0.f)
, mSpitForce(spitForce)
, x8f8_(false)
, x8fc_(0.f)
, mMaxGrabberGrabRange(maxGrabberGrabRange)
, mMinGrabberGrabRange(minGrabberGrabRange)
, x908_(false)
, x909_(true)
, x90c_(-1)
, x910_(4)
, x914_(CVector3f::Zero())
, x920_(CQuaternion::NoRotation())
, mTendrilEffect(tendrilParticleEffect != kInvalidAssetId
                     ? rs_new TLockedToken< CGenDescription >(
                           gpSimplePool->GetObj(SObjectTag('PART', tendrilParticleEffect)))
                     : nullptr)
, mTendrilGen(mTendrilEffect.null() ? nullptr : rs_new CSporbTendrilGen(*mTendrilEffect))
, mGrabberFireSfx(grabberFireSfx)
, mGrabberFlightSfx(grabberFlightSfx)
, mGrabberHitPlayerSfx(grabberHitPlayerSfx)
, mGrabberHitWorldSfx(grabberHitWorldSfx)
, mGrabberRetractSfx(grabberRetractSfx)
, mGrabberRetractMissedPlayerSfx(grabberRetractMissedPlayerSfx)
, mMorphballSpitSfx(morphballSpitSfx)
, mGrabberExplosionSfx(grabberExplosionSfx)
, mBallEscapeSfx(ballEscapeSfx)
, mNeedleTelegraphSfx(needleTelegraphSfx)
, mGrabberTelegraphSfx(grabberTelegraphSfx)
, x94e_(false)
, x94f_(false)
, x950_(CVector3f::Zero())
, x95c_(1.f)
, x960_(CVector3f::Zero())
, x96c_(0.f)
, x970_(0.f)
, x974_(0.f)
, x978_(0.f)
, x97c_(CDamageVulnerability::ImmuneVulnerabilty())
, mIsPowerBombGuardian(isPowerBombGuardian)
, mProjectileInfo(powerBombProjectileParticle, powerBombProjectileDamage)
, mMaxPowerBombProjectileHeight(maxPowerBombProjectileHeight)
, mPowerBombProjectileFuseTime(powerBombProjectileFuseTime)
, mPowerBombProjectileSfx(powerBombProjectileSfx)
, x9f4_(x9f4)
, x9f8_(x9f8)
, mPowerBombProjectileStartDamageTime(powerBombProjectileStartDamageTime)
, mPowerBombProjectileEndDamageTime(powerBombProjectileEndDamageTime)
, mStage(0)
, xa08_(isPowerBombGuardian ? xa08 : 1.f)
, xa0c_(1.f)
, xa10_(false)
, xa11_(false)
, mPowerBombProjectileDamageWaitTime(powerBombProjectileDamageWaitTime)
, xa18_(0)
, mStages(stages)
, mShots(SShot())
, xa50_(CVector3f::Zero())
, xa5c_(xa5c)
, xa60_(xa60)
, xa64_(2)
, xa68_(CVector3f::Zero())
, xa74_24_(true)
, xa74_25_(false)
, xa74_26_(false) {
  mProjectileInfo.Token().Lock();
  const CPASDatabase& pasDatabase = GetModelData()->GetAnimationData()->GetPASDatabase();
  rstl::pair< float, int > best = pasDatabase.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(5)), -1);
  if (best.first > FLT_EPSILON) {
    mGenerateAnims[0] = best.second;
  }
  best = pasDatabase.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(6)), -1);
  if (best.first > FLT_EPSILON) {
    mGenerateAnims[3] = best.second;
  }
  best = pasDatabase.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(4)), -1);
  if (best.first > FLT_EPSILON) {
    mGenerateAnims[1] = best.second;
  }
  best = pasDatabase.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(7)), -1);
  if (best.first > FLT_EPSILON) {
    mGenerateAnims[2] = best.second;
  }
  best = pasDatabase.FindBestAnimation(CPASAnimParmData(pas::kAS_AdditiveFlinch),
                                       -1);
  if (best.first > FLT_EPSILON) {
    mGenerateAnims[4] = best.second;
  }
  SetDrawShadow(false);
  if (mMinGrabberGrabRange < mMinAttackRange) {
    mMinGrabberGrabRange = mMinAttackRange;
  }
  if (mMaxGrabberGrabRange > mMaxAttackRange) {
    mMaxGrabberGrabRange = mMaxAttackRange;
  }
  if (mMaxGrabberGrabRange > 30.f) {
    mMaxGrabberGrabRange = 30.f;
  }
  x9e0_.reserve(0x20);
  SetStage(mStage);
  const int numEvents = GetNumUserEventsForAnimation(
      CPASAnimParmData(pas::kAS_Generate, CPASAnimParm::FromEnum(7)), kUE_DamageOn);
  if (numEvents != 0) {
    mBallEscapeDamage.SetDamage(ballEscapeDamage / static_cast< float >(numEvents));
  }
}

void CSporbBase::SetStage(uchar stage) {
  if (mIsPowerBombGuardian && mStages.size() > stage) {
    const CPowerBombGuardianStageData& data = *mStages[stage];
    mMinTimeBetweenAttacks = data.mMinTimeBetweenAttacks;
    mMaxTimeBetweenAttacks = data.mMaxTimeBetweenAttacks;
    mMinTimeBetweenShots = data.mMinTimeBetweenShots;
    mMaxTimeBetweenShots = data.mMaxTimeBetweenShots;
    mMinShotsInABurst = data.mMinShotsInABurst;
    mMaxShotsInABurst = data.mMaxShotsInABurst;
  }
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldPatrol", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::ShouldPatrol)},
    {"ShouldAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::ShouldAttack)},
    {"ShouldFire", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::ShouldFire)},
    {"ShouldSpit", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::ShouldSpit)},
    {"ShouldFlail", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::ShouldFlail)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::AnimOver)},
    {"AttackOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::AttackOver)},
    {"SpitOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::SpitOver)},
    {"AttackExitOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbBase::AttackExitOver)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Sleep)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Patrol)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Attack)},
    {"WakeUp", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::WakeUp)},
    {"GoToSleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::GoToSleep)},
    {"Fire", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Fire)},
    {"ContinueFire", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::ContinueFire)},
    {"AttackExit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::AttackExit)},
    {"FakeDeath", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::FakeDeath)},
    {"FakeDead", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::FakeDead)},
    {"Flail", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Flail)},
    {"ContinueFlail",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::ContinueFlail)},
    {"Spit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Spit)},
    {"ContinueSpit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::ContinueSpit)},
    {"SpitExit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::SpitExit)},
    {"Flinch", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbBase::Flinch)},
};

void CSporbBase::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}


void CSporbBase::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_AreaLoaded:
    for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
         it != GetConnectionList().end(); ++it) {
      const EScriptObjectState state = it->state;
      const TUniqueId id = mgr.GetIdForScript(it->objId);
      if (state == kSS_Approach) {
        if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(id))) {
          mTopId = id;
          x97c_ = *top->GetDamageVulnerability();
          *top->DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
          top->x800_24_ = true;
          top->mBaseId = GetUniqueId();
        } else if (TCastToPtr< CSporbProjectile >(mgr.ObjectById(id)) != nullptr) {
          mProjectileId = id;
        } else if (TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id)) != nullptr) {
          x9e0_.push_back_unsafe(id);
        }
      } else if (state == kSS_GRNT) {
        x898_ = it->objId;
      }
    }
    gSporbGrabbedBallOwner = kInvalidUniqueId;
    if (mProjectileId != kInvalidUniqueId) {
      if (CSporbProjectile* projectile =
              TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
        projectile->mTopId = mTopId;
      }
    }
    if (mTopId != kInvalidUniqueId) {
      if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
        top->mProjectileId = mProjectileId;
      }
    }
    break;
  case kSM_Create:
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    *DamageVulnerability() = mIsPowerBombGuardian ? CDamageVulnerability::ReflectVulnerabilty()
                                                  : CDamageVulnerability::ImmuneVulnerabilty();
    RemoveMaterial(kMT_Target, mgr);
    AddMaterial(kMT_Unknown54, mgr);
    if (mIsPowerBombGuardian && mStages.size() != 0 && mStage < mStages.size()) {
      const CPowerBombGuardianStageData& data = *mStages[mStage];
      int extra = 0;
      const int range = data.x21_ - data.x20_;
      if (range != 0) {
        extra = mgr.Random()->Next() % (range + 1);
      }
      xa11_ = extra + data.x20_;
    }
    if (!mTendrilGen.null()) {
      mTendrilGen->SetGlobalTranslation(GetTranslation());
      mTendrilGen->ForceParticleCreation(static_cast< int >(mMaxGrabberGrabRange / 0.15f) + 2);
    }
    break;
  case kSM_Increment:
    if (mIsPowerBombGuardian) {
      mStage = rstl::max_val(mStage - 1, 0);
      SetStage(mStage);
    }
    break;
  case kSM_Decrement:
    if (mIsPowerBombGuardian) {
      ++mStage;
      SetStage(mStage);
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
      mStateMachine->SetState(mgr, *this, rstl::string_l("Flinch"));
      const int stageCount = mStages.size();
      const int remaining = stageCount - mStage;
      const float hp = HealthInfo()->GetHP();
      HealthInfo()->SetHP((static_cast< float >(remaining) / static_cast< float >(stageCount)) * hp);
    }
    break;
  case kSM_Alert:
    x894_ = 0.f;
    x909_ = true;
    break;
  case kSM_Reset:
    x909_ = false;
    x894_ = mPlayerLeashTime;
    x94f_ = false;
    break;
  case kSM_Start:
    xa74_24_ = true;
    break;
  case kSM_Stop:
    xa74_24_ = false;
    break;
  case kSM_SetToMax:
    xa0c_ = xa08_;
    xa74_26_ = true;
    break;
  case kSM_SetToZero:
    xa0c_ = 1.f;
    xa74_26_ = false;
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

void CSporbBase::DecayAimBlend() {
  x7ec_ = rstl::max_val(x7ec_ - 0.025f, 0.f);
  x7f0_ = rstl::max_val(x7f0_ - 0.025f, 0.f);
  x7f4_ = rstl::max_val(x7f4_ - 0.025f, 0.f);
  x7f8_ = rstl::max_val(x7f8_ - 0.025f, 0.f);
}

void CSporbBase::Render(const CStateManager& mgr) const {
  CPatterned::Render(mgr);
  if (!mTendrilGen.null()) {
    mTendrilGen->Render();
  }
}

void CSporbBase::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

void CSporbBase::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);
}

void CSporbBase::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  if (!mTendrilGen.null()) {
    const rstl::optional_object< CAABox > bounds = mTendrilGen->GetBounds();
    if (bounds && !bounds->Invalid()) {
      gpRender->AddParticleGen(*mTendrilGen);
    }
  }
}

void CSporbBase::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 0;
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    x94e_ = false;
    x909_ = true;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbBase::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 1;
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 3;
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 6;
    }
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

void CSporbBase::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mState = 2;
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 4;
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 7;
    }
    x888_ = true;
    x860_ = 0.f;
    xa74_25_ = false;
    CPlayer* player = mgr.GetPlayer(0);
    bool inRange = false;
    if (IsInRange(*player, mMaxGrabberGrabRange) && !IsInRange(*player, mMinGrabberGrabRange)) {
      inRange = true;
    }
    const CPlayer::EPlayerMorphBallState morphState =
        player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
            ? player->GetMorphballTransitionState()
            : CPlayer::kMS_Unmorphed;
    if (morphState == CPlayer::kMS_Morphed && inRange &&
        gSporbGrabbedBallOwner == kInvalidUniqueId && !mIsPowerBombGuardian) {
      x90c_ = 1;
      x910_ = 4;
    } else {
      x90c_ = 0;
    }
    if (mIsPowerBombGuardian && mStages.size() != 0 && mStage < mStages.size() &&
        mMinShotsInABurst2 != 2 && mStages[mStage]->mDoubleShotChance > 0.f) {
      xa10_ = rstl::min_val< int >(xa10_ + 1, xa11_);
    }
    xa18_ = 0;
    break;
  }
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbBase::Fire(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    if (mMinShotsInABurst2 > 1 && mIsPowerBombGuardian) {
      xa64_ = 5;
    } else {
      xa64_ = 2;
    }
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 5;
      if (!mIsPowerBombGuardian) {
        top->AnimationData()->SetEffectState(rstl::string_l("telegraph"), true, mgr);
      }
      if (xa64_ == 2) {
        top->mGenerateType = 3;
      } else {
        top->mGenerateType = 5;
      }
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 8;
    }
    mState = 3;
    x887_ = 1;
    if (x90c_ == 1) {
      x910_ = 0;
    }
    x94e_ = true;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(static_cast< pas::EGenerateType >(xa64_), -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::Spit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Two, -1));
    mState = 9;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Two, -1));
    }
    if (mStateMachine->GetTime() > 0.5f) {
      if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
        top->mState = 1;
      }
      if (CSporbProjectile* projectile =
              TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
        projectile->mState = 4;
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::Flail(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Seven, -1));
    mState = 8;
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      xa68_ = projectile->GetTranslation();
    }
    SendScriptMsgs(kSS_MaxReached, mgr, mgr.GetPlayer(0)->GetUniqueId(), kSM_None);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Seven, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x908_ = true;
    break;
  }
}

void CSporbBase::ContinueFlail(CStateManager&, EStateMsg, float) {}

void CSporbBase::ContinueSpit(CStateManager&, EStateMsg, float) {}

void CSporbBase::AttackExit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 5;
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 6;
      if (!mIsPowerBombGuardian) {
        top->AnimationData()->SetEffectState(rstl::string_l("telegraph"), false, mgr);
      }
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 9;
    }
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    ResetAttackTimers(mgr);
    break;
  }
}

void CSporbBase::SpitExit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 10;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbBase::ContinueFire(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal6);
    mState = 4;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbBase::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->StartFlinch(mgr);
    }
    mState = 11;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::WakeUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 3;
      *top->DamageVulnerability() = x97c_;
      top->x800_24_ = false;
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 6;
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::GoToSleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    if (CSporbTop* top = TCastToPtr< CSporbTop >(mgr.ObjectById(mTopId))) {
      top->mState = 2;
      x97c_ = *top->GetDamageVulnerability();
      *top->DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
      top->x800_24_ = true;
    }
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 5;
    }
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::FakeDeath(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 6;
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Six, -1));
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mHoldingBall = false;
      mgr.DeleteObjectRequest(projectile->GetUniqueId());
    }
    ResetTendril();
    SendScriptMsgs(kSS_Exited, mgr, mgr.GetPlayer(0)->GetUniqueId(), kSM_None);
    if (gSporbGrabbedBallOwner == GetUniqueId()) {
      CPlayer* player = mgr.GetPlayer(0);
      player->GetMorphBall()->SetBallBoostState(CMorphBall::kBBS_BoostAvailable);
      gSporbGrabbedBallOwner = kInvalidUniqueId;
    }
    BodyController()->UnFreeze();
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Six, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbBase::FakeDead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = 7;
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

bool CSporbBase::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CSporbBase::ShouldAttack(CStateManager& mgr, const CTriggerData&) const {
  if (!x909_) {
    return false;
  }
  if (gSporbGrabbedBallOwner == kInvalidUniqueId) {
    const CPlayer* player = mgr.GetPlayer(0);
    bool inRange = false;
    if (IsInRange(*player, mMaxAttackRange) && !IsInRange(*player, mMinAttackRange)) {
      inRange = true;
    }
    if (TCastToConstPtr< CSporbTop >(mgr.GetObjectById(mTopId)) != nullptr) {
      if (inRange || x94f_) {
        return true;
      }
      return false;
    }
    return inRange;
  }
  return false;
}

bool CSporbBase::ShouldPatrol(CStateManager&, const CTriggerData&) const {
  bool result = false;
  if (x894_ < mPlayerLeashTime && x909_) {
    result = true;
  }
  return result;
}

bool CSporbBase::ShouldFire(CStateManager& mgr, const CTriggerData&) const {
  if (!x909_) {
    return false;
  }
  if (!xa74_24_) {
    return false;
  }
  bool result;
  if (x94e_) {
    result = x888_;
  } else {
    result = false;
    if (x888_ && mStateMachine->GetTime() > mMinTimeBetweenAttacks2) {
      result = true;
    }
  }
  return result;
}

bool CSporbBase::ShouldSpit(CStateManager&, const CTriggerData&) const { return x908_; }

bool CSporbBase::ShouldFlail(CStateManager&, const CTriggerData&) const { return mState == 8; }

bool CSporbBase::AttackExitOver(CStateManager&, const CTriggerData&) const {
  return mStateMachine->GetTime() > 1.f;
}

bool CSporbBase::AttackOver(CStateManager&, const CTriggerData&) const {
  if (x90c_ == 0) {
    return x887_ == 0;
  }
  if (x90c_ == 1) {
    return x88d_;
  }
  return false;
}

bool CSporbBase::SpitOver(CStateManager&, const CTriggerData&) const { return true; }

void CSporbBase::SelectTarget(CStateManager& mgr) {}

bool CSporbBase::IsInRange(const CActor& actor, float range) const {
  return (actor.GetTranslation() - GetTranslation()).MagSquared() < range * range;
}

float CSporbBase::GetAimAngle(CStateManager& mgr) const { return 0.f; }

void CSporbBase::UpdateAttack(CStateManager& mgr, float dt) {
  switch (x90c_) {
  case 1:
    UpdateGrabber(mgr, dt);
    break;
  case 0:
    UpdateFiring(mgr, dt);
    break;
  }
}

void CSporbBase::UpdateGrabber(CStateManager& mgr, float dt) {}

void CSporbBase::AttachPlayer(CStateManager& mgr) {}

CVector3f CSporbBase::GetTopAttachPosition() const {
  return (GetTransform() * GetScaledLocatorTransform(rstl::string_l(skTopAttachLocator)))
      .GetTranslation();
}

void CSporbBase::UpdateFiring(CStateManager& mgr, float dt) {}

void CSporbBase::ResetAttackTimers(CStateManager& mgr) {}

CVector3f CSporbBase::GetTopLaunchPosition(CStateManager& mgr) const { return GetTranslation(); }

void CSporbBase::UpdateAimBlend(CStateManager& mgr) {}

CTransform4f CSporbBase::LookAtTarget(CStateManager& mgr, const CVector3f& from) const {
  return CQuaternion::LookAt(CUnitVector3f(CVector3f::Forward()), CUnitVector3f(x914_ - from),
                             CRelAngle::FromRadians(6.2831855f))
      .BuildTransform4f(from);
}

void CSporbBase::ReleaseGrabber(CStateManager& mgr, const CVector3f& pos) {
  x910_ = 4;
  x8ac_ = 0;
  x8c0_ = pos;
  if (mProjectileId != kInvalidUniqueId) {
    if (CSporbProjectile* projectile =
            TCastToPtr< CSporbProjectile >(mgr.ObjectById(mProjectileId))) {
      projectile->mState = 9;
    }
  }
  x88d_ = true;
  x908_ = false;
  x8f0_ = 0.f;
  x8fc_ = 0.f;
  x88e_ = false;
}

CVector3f CSporbBase::GetSpitTarget(CStateManager& mgr) const { return x914_; }

void CSporbBase::SpitBall(CStateManager& mgr, float force) {}

CVector3f CSporbBase::GetPowerBombTarget(CStateManager& mgr, bool) const { return x914_; }

void CSporbBase::ResetTendril() {
  x8ac_ = 0;
  ResetTendrilParticles();
}

CDamageInfo CSporbBase::GetContactDamage() const {
  if (mState == 6 || mState == 7) {
    return CDamageInfo();
  }
  return CPatterned::GetContactDamage();
}

void CSporbBase::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                 EUserEventType type, float dt) {
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

CVector3f CSporbBase::GetAimDirection(float a, float b, float c, float d) {
  return CVector3f(d - c, a - b, 0.f).AsNormalized();
}

CVector3f CSporbBase::GetGrabberAimOffset(CStateManager& mgr) const { return CVector3f::Zero(); }

CSporbPowerBomb* CSporbBase::CreatePowerBomb(CStateManager& mgr,
                                             const TToken< CWeaponDescription >& desc,
                                             const CTransform4f& xf, const CDamageInfo& damage) {
  return nullptr;
}

CVector3f CSporbBase::PredictBallPosition(CStateManager& mgr, const CVector3f& from,
                                          float maxHeight) const {
  return CVector3f::Zero();
}

float CSporbBase::GetPowerBombFlightTime(const CVector3f& from, const CVector3f& to,
                                         float maxHeight, float gravity) const {
  const float dz = from.GetZ() - to.GetZ();
  const float upHeight = dz > 0.f ? maxHeight : -dz + maxHeight;
  const float downHeight = dz > 0.f ? dz + maxHeight : maxHeight;
  return CMath::SqrtF((2.f * upHeight) / gravity) + CMath::SqrtF((2.f * downHeight) / gravity);
}

CProjectileInfo* CSporbBase::ProjectileInfo() {
  if (mIsPowerBombGuardian) {
    return &mProjectileInfo;
  }
  return nullptr;
}

void CSporbBase::LaunchPowerBomb(const CVector3f& from, CStateManager& mgr, int maxProjectiles,
                                 const CVector3f& target, float maxHeight) {}

float CSporbBase::GetPowerBombGravity() const {
  float multiplier = 1.f;
  if (mStages.size() != 0 && mStage < mStages.size()) {
    multiplier = mStages[mStage]->mProjectileGravityMultiplier;
  }
  return 9.81f * multiplier;
}

CVector3f CSporbBase::GetScanObjectIndicatorPosition(const CStateManager& mgr) const {
  if (const CSporbProjectile* projectile =
          TCastToConstPtr< CSporbProjectile >(mgr.GetObjectById(mProjectileId))) {
    if (mState == 8 || mState == 9) {
      return xa68_;
    }
    return projectile->GetTranslation();
  }
  return GetTranslation();
}

CAABox CSporbBase::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox box = GetModelBounds();
  return box;
}

void CSporbBase::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                 const CModelFlags& flags) const {}

CAABox CSporbBase::GetModelBounds() const {
  CAABox box = CAABox::MakeMaxInvertedBox();
  box = GetModelData()->GetAnimationData()->CalcBoundingBoxFromModelVerts();
  box = box.GetTransformedAABox(CTransform4f::Translate(-GetTranslation()) * GetTransform() *
                                CTransform4f::Scale(GetModelData()->GetScale()));
  return box;
}

void CSporbBase::ResetTendrilParticles() {}

void CSporbBase::Think(float dt, CStateManager& mgr) { CPatterned::Think(dt, mgr); }


CEntity* REL_LoadSporbBase(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  return nullptr;
}

static SSporb_FuncPtrs sSporbFuncPtrs; // Guessed name.

static void SetRelLoaderFunctionToLoader() {
  sSporbFuncPtrs.mLoadBase = REL_LoadSporbBase;
  sSporbFuncPtrs.mLoadNeedle = REL_LoadSporbNeedle;
  sSporbFuncPtrs.mLoadTop = REL_LoadSporbTop;
  sSporbFuncPtrs.mLoadProjectile = REL_LoadSporbProjectile;
  SetSSporb_FuncPtrs(&sSporbFuncPtrs);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSSporb_FuncPtrs(nullptr); }
