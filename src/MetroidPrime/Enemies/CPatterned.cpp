#include "MetroidPrime/Enemies/CPatterned.hpp"

#include "Kyoto/Animation/CCharAnimTime.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CActorModelParticles.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CSimpleShadow.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "rstl/math.hpp"
#include "MetroidPrime/CGenericFSM2State.hpp"

#include <float.h>

const float CPatterned::skDamageHitTime = 0.33f;
const float CPatterned::skActorApproachDistance = 3.f;
const CColor CPatterned::skDamageColor(0.5f, 0.f, 0.f, 1.f);
const CColor CPatterned::skHitsWithoutDamageColor(0.5f, 0.5f, 0.f, 1.f);

static CMaterialList gkPatternedFlyerMaterialList(kMT_Character, kMT_Unknown59, kMT_Orbit,
                                                  kMT_Target, kMT_SeekerTarget);
static CMaterialList gkPatternedGroundMaterialList =
    CMaterialList(kMT_GroundCollider).Union(gkPatternedFlyerMaterialList);

CPatterned::CPatterned(EPatternedAI character, TUniqueId uid, const rstl::string& name,
                       EFlavorType flavor, const CEntityInfo& info, const CTransform4f& xf,
                       const CModelData& modelData, const CPatternedInfo& pinfo,
                       EMovementType movement, EColliderType collider, EBodyType body,
                       const CActorParameters& params)
: CAi(uid, name, info, 4, xf, modelData,
      CAABox(pinfo.mBodyOrigin.GetX() - pinfo.mHalfExtent * modelData.GetScale().GetX(),
             pinfo.mBodyOrigin.GetY() - pinfo.mHalfExtent * modelData.GetScale().GetY(),
             pinfo.mBodyOrigin.GetZ(),
             pinfo.mBodyOrigin.GetX() + pinfo.mHalfExtent * modelData.GetScale().GetX(),
             pinfo.mBodyOrigin.GetY() + pinfo.mHalfExtent * modelData.GetScale().GetY(),
             pinfo.mBodyOrigin.GetZ() + pinfo.mHeight * modelData.GetScale().GetZ()),
      pinfo.mMass, pinfo.mHealthInfo, pinfo.mDamageVulnerability,
      movement == kMT_Flyer ? gkPatternedFlyerMaterialList : gkPatternedGroundMaterialList,
      pinfo.mStateMachineId, pinfo.mStateMachine2Id, params, pinfo.mStepUpHeight, 0.8f)
, mDestObj(kInvalidUniqueId)
, mDestPos(CVector3f::Zero())
, mReflectedDestPos(CVector3f::Zero())
, mInPosition(false)
, mVerticalMovement(movement == kMT_Flyer)
, mSolidCollision(false)
, mBlockingCollision(false)
, mOnGround(movement != kMT_Flyer)
, mOnStaticGround(false)
, mPrevOnGround(true)
, mEnergyAttractor(false)
, mLookAtDeathDir(true)
, x34d_25_(false)
, x34d_26_(true)
, mStateMachine(pinfo.mStateMachine2Id == kInvalidAssetId
                    ? static_cast< StateMachine* >(rs_new TStateMachineState< CPatterned >)
                    : static_cast< StateMachine* >(rs_new CGenericFSM2State< CPatterned >))
, mCharacterType(character)
, mCreatureSize(pinfo.mCreatureSize)
, mIngPossessionBlend(0.f)
, mIngPossessionTarget(0.f)
, mIngPossessionDelay(0.f)
, mIngPossessionDuration(0.5f)
, mIngVulnerability(pinfo.mIngPossessionData.ingVulnerability)
, mMoveVec(CVector3f::Zero())
, mFaceVec(CVector3f::Zero())
, mInitialAnimation(pinfo.mAnimationParameters.GetInitialAnimation())
, mLatestLeashPosition(CVector3f::Zero())
, mSpeed(pinfo.mSpeed)
, mTurnSpeed(pinfo.mTurnSpeed)
, mDetectionRange(pinfo.mDetectionRange)
, mDetectionHeightRange(pinfo.mDetectionHeightRange)
, mDetectionAngle(cosf(CMath::Deg2Rad(pinfo.mDetectionAngle)))
, mMinAttackRange(pinfo.mMinAttackRange)
, mMaxAttackRange(pinfo.mMaxAttackRange)
, mAverageAttackTime(pinfo.mAverageAttackTime)
, mAttackTimeVariation(pinfo.mAttackTimeVariation)
, mLeashRadius(pinfo.mLeashRadius)
, mPlayerLeashRadius(pinfo.mPlayerLeashRadius)
, mPlayerLeashTime(pinfo.mPlayerLeashTime)
, mCurPlayerLeashTime(0.f)
, mXDamageThreshold(pinfo.mXDamageThreshold)
, mFrozenXDamageThreshold(pinfo.mFrozenXDamageThreshold)
, mXDamageDelay(pinfo.mXDamageDelay)
, mLastHP(0.f)
, mAlphaDelta(0.f)
, mPendingFireDamage(0.f)
, mPendingShockDamage(0.f)
, mBurnThinkRateTimer(0.f)
, mFlavor(flavor)
, mHitByPlayerProjectile(false)
, mAlive(true)
, x420_26_(false)
, mFadeToDeath(false)
, mPendingMassiveDeath(false)
, mPendingMassiveFrozenDeath(false)
, mIsFlyer(movement == kMT_Flyer)
, mPathOverCount(0)
, mLaggedBurnDeath(false)
, x421_26_(false)
, mPendingDeath(false)
, mLostMassiveFrozenHP(false)
, mDieIf80PercFrozen(false)
, mIsMakingBigStrike(false)
, mDrawParticles(true)
, mEnableStateMachine(true)
, mStateControlledMassiveDeath(true)
, x422_26_(0)
, x422_28_(false)
, x422_29_(false)
, x422_30_(false)
, mStopPhysics(false)
, x423_24_(false)
, mSuppressKnockBack(false)
, mContactDamage(pinfo.mContactDamageInfo)
, mCurDamageRemTime(0.f)
, mDamageWaitTime(pinfo.mDamageWaitTime)
, mDamageCooldownTimer(-1.f)
, mColor(0.f, 0.f, 0.f, 1.f)
, mDamageColor(skDamageColor)
, mPosDelta(CVector3f::Zero())
, mRotDelta(CQuaternion::NoRotation())
, mNormalModel(GetAnimationData()->GetModelData())
, mDeathSfx(pinfo.mDeathSfx)
, mIceShatterSfx(pinfo.mIceShatterSfx)
, mIceVocalSfx(pinfo.mIceVocalSfx)
, mFrozenSfx(pinfo.mFrozenSfx)
, mIngPossessionData(pinfo.mIngPossessionData)
, mKnockBackController(pinfo.mKnockBackRules)
, mLatestPredictedTranslation(CVector3f::Zero())
, mPredictedLeashTime(0.f)
, mIntoFreezeDuration(pinfo.mIntoFreezeDuration)
, mOutOfFreezeDuration(pinfo.mOutOfFreezeDuration)
, mFreezeDuration(pinfo.mFreezeDuration)
, mPreThinkDt(0.f)
, mDamageDuration(0.f)
, mColliderType(collider)
, mFadeOnDeathTime(3.f)
, mDeathExplosionOffset(pinfo.mDeathExplosionOffset)
, mIceDeathExplosionOffset(pinfo.mIceDeathExplosionOffset)
, mMoveScale(1.f, 1.f, 1.f)
, mIngSnatchingPlane(CVector3f::Zero(), CVector3f::Forward())
, mDisintegrationOrigin(CVector3f::Zero()) {
  fn_800747a4(mIngPossessionData.ingPossessedModel, mIngPossessionData.ingPossessedSkinRules);
  if (pinfo.mDeathExplosionParticle != kInvalidAssetId) {
    mDeathExplosionParticle =
        gpSimplePool->GetObj(SObjectTag('PART', pinfo.mDeathExplosionParticle));
    mDeathExplosionParticle->Lock();
  }
  if (pinfo.mDeathExplosionElectric != kInvalidAssetId) {
    mDeathExplosionElectric =
        gpSimplePool->GetObj(SObjectTag('ELSC', pinfo.mDeathExplosionElectric));
    mDeathExplosionElectric->Lock();
  }
  if (pinfo.mIceDeathExplosionParticle != kInvalidAssetId) {
    mIceDeathExplosionParticle =
        gpSimplePool->GetObj(SObjectTag('PART', pinfo.mIceDeathExplosionParticle));
    mIceDeathExplosionParticle->Lock();
  }
  if (mContactDamage.GetRadius() > 0.f) {
    mContactDamage.SetRadius(0.f);
  }
  SetRenderParticleDatabaseInside(false);
  if (HasModelData()) {
    BuildBodyController(body);
    mLockOnTarget = GetAnimationData()->GetLocatorSegId(rstl::string("lockon_target_LCTR"));
  }
  if (mIngPossessionData.darkScanInfo != kInvalidAssetId) {
    mIngScanInfo = rs_new TLockedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', mIngPossessionData.darkScanInfo)));
  }
  // TODO: Enable the actor's damage/echo flags and apply pinfo.mEchoParameters.
}

void CPatterned::BuildBodyController(EBodyType) {
  // TODO: Construct the body controller and configure the additive-reaction knockback options.
}

static const CPatterned::StateMachine::STriggerFunction triggers[] = {
    {"Leash", &CPatterned::Leash},
    {"SpotPlayer", &CPatterned::SpotPlayer},
    {"PlayerSpot", &CPatterned::PlayerSpot},
    {"InRange", &CPatterned::InRange},
    {"InMaxRange", &CPatterned::InMaxRange},
    {"InDetectionRange", &CPatterned::InDetectionRange},
    {"PathShagged", &CPatterned::PathShagged},
    {"PathOver", &CPatterned::PathOver},
    {"PathFound", &CPatterned::PathFound},
    {"Delay", &CPatterned::Delay},
    {"RandomDelay", &CPatterned::RandomDelay},
    {"FixedDelay", &CPatterned::FixedDelay},
    {"HasPatrolPath", &CPatterned::HasPatrolPath},
    {"Attacked", &CPatterned::Attacked},
    {"OffLine", &CPatterned::OffLine},
    {"AnimOver", &CPatterned::AnimOver},
    {"NoPathNodes", &CPatterned::NoPathNodes},
    {"TooClose", &CPatterned::TooClose},
    {"Landed", &CPatterned::Landed},
    {"InPosition", &CPatterned::InPosition},
    {"Stuck", &CPatterned::Stuck},
    {"CodeTrigger", &CPatterned::CodeTrigger},
    {"Random", &CPatterned::Random},
    {"FixedRandom", &CPatterned::FixedRandom},
};
static const CPatterned::StateMachine::SStateFunction states[] = {
    {"Start", &CPatterned::Start},
    {"Dead", &CPatterned::Dead},
    {"PathFind", &CPatterned::PathFind},
    {"Patrol", &CPatterned::Patrol},
};

void CPatterned::SetupStateMachine(CStateManager&) {
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(triggers, ARRAY_SIZE(triggers));
  stateMachine->SetStateFunctions(states, ARRAY_SIZE(states));
}

void CPatterned::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CAi::AcceptScriptMsg(mgr, msg);
  // TODO: Restore registration, floor, activation, deletion and damage-message handling.
}

void CPatterned::SetDestPos(const CVector3f& position) { mDestPos = position; }

CVector3f CPatterned::GetGunEyePos() const {
  CVector3f translation = GetTranslation();
  const CAABox& bounds = GetBaseBoundingBox();
  translation[kDZ] += 0.6f * (bounds.GetMaxPoint().GetZ() - bounds.GetMinPoint().GetZ());
  return translation;
}

bool CPatterned::ApplyBoneTracking() const {
  if (mAlive && !GetBodyController()->IsFrozen() &&
      !(mKnockBackController.GetFlinchRemainingTime() > 0.f)) {
    return true;
  }
  return false;
}

float CPatterned::GetAnimationDistance(const CPASAnimParmData& parms) const {
  float distance = 1.f;
  const rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(parms, -1);
  if (best.first > FLT_EPSILON) {
    const CAnimData* animData = GetAnimationData();
    distance = animData->GetAnimationDuration(best.second);
    distance *= animData->GetAverageVelocity(best.second);
  }
  return distance;
}

float CPatterned::GetAnimationDuration(const CPASAnimParmData& parms) const {
  const rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(parms, -1);
  if (best.first > FLT_EPSILON) {
    return GetAnimationData()->GetAnimationDuration(best.second);
  }
  return 0.f;
}

void CPatterned::SetupPlayerCollision(bool) {
  // TODO: Update the player's material in the include/exclude filter for the collider mode.
}

CScriptCoverPoint* CPatterned::GetCoverPoint(CStateManager& mgr, const TUniqueId id) const {
  CScriptCoverPoint* point = nullptr;
  if (id != kInvalidUniqueId) {
    point = TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(id));
  }
  return point;
}

void CPatterned::ReleaseCoverPoint(CStateManager& mgr, TUniqueId& id, bool retainCooldown) {
  if (CScriptCoverPoint* point = GetCoverPoint(mgr, id)) {
    point->SetInUse(false);
    if (!retainCooldown) {
      point->ResetCooldown();
    }
    id = kInvalidUniqueId;
  }
}

void CPatterned::SetCoverPoint(CScriptCoverPoint* point, TUniqueId& id) {
  point->SetInUse(true);
  id = point->GetUniqueId();
}

void CPatterned::Death(CStateManager&, const CVector3f&, EScriptObjectState) {
  // TODO: Select massive/frozen/animated death, change materials, and send the requested state.
}

void CPatterned::IssueDeathBodyCommand(CStateManager&, const CVector3f&) {
  // TODO: Submit the appropriate die, fall or hurled body-state command.
}

void CPatterned::CreateXDamageParticles(CStateManager&) const {
  // TODO: Instantiate the configured particle/electric death effects at the scaled locator.
}

void CPatterned::fn_8007850c(CStateManager&) {
  // TODO: Instantiate the ice-death explosion with the actor's scaled offset.
}

void CPatterned::MassiveDeath(CStateManager&) {
  // TODO: Death effects, sound, script notification and object deletion.
}

void CPatterned::MassiveFrozenDeath(CStateManager&) {
  // TODO: Ice-death effects, sound, script notification and object deletion.
}

void CPatterned::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  const CHealthInfo* health = GetHealthInfo();
  if (!mLaggedBurnDeath && health != nullptr && !mSuppressKnockBack) {
    mKnockBackController.KnockBack(mgr, *this, info);
  }
}

void CPatterned::ApplyKnockBackFollowUp(CStateManager&, const CVector3f&,
                                       CKnockBackMgr::EFollowUp, float, float, TUniqueId,
                                       TUniqueId) {
  // TODO: Apply the knockback rule's follow-up (freeze, burn, shock, death or disintegration).
}

void CPatterned::UpdateAlphaDelta(CStateManager& mgr, float dt) {
  if (mAlphaDelta == 0.f) {
    return;
  }
  float alpha = mColor.GetAlpha() + dt * mAlphaDelta;
  if (alpha > 1.f) {
    mAlphaDelta = 0.f;
    alpha = 1.f;
  } else if (alpha < 0.f) {
    mAlphaDelta = 0.f;
    alpha = 0.f;
    if (mFadeToDeath) {
      DeathDelete(mgr);
    }
  }
  Shadow()->SetUserAlpha(alpha);
  mColor.SetAlpha(alpha);
  AnimationData()->GetParticleDB().SetModulationColorAllActiveEffects(
      CColor(1.f, 1.f, 1.f, alpha));
}

void CPatterned::UpdateHitDamageTime(float dt) {
  if (mDamageCooldownTimer > 0.f) {
    mDamageCooldownTimer = CMath::Max(0.f, mDamageCooldownTimer - dt);
  }
  // TODO: Restore frozen/hit color interpolation and the actor-light damage response.
}

void CPatterned::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  // TODO: Restore death, body/animation, continuous damage, movement and leash updates.
}

void CPatterned::InitializeStateMachine(CStateManager& mgr) {
  if (mStateMachine->HasState()) {
    return;
  }
  if (mStateMachine->GetType() == 1) {
    CGenericFSM2* machine = GetStateMachine2();
    if (!machine) {
      return;
    }
    static_cast< CGenericFSM2State< CPatterned >* >(mStateMachine.get())->Setup(*machine);
  } else {
    CStateMachine* machine = GetStateMachine();
    if (!machine) {
      return;
    }
    static_cast< TStateMachineState< CPatterned >* >(mStateMachine.get())->Setup(machine);
  }
  SetupStateMachine(mgr);
  mStateMachine->SetState(mgr, *this, rstl::string("Start"));
}

void CPatterned::Touch(CActor& actor, CStateManager& mgr) {
  if (!mAlive) {
    return;
  }
  if (CGameProjectile* projectile = TCastToPtr< CGameProjectile >(actor)) {
    if (TCastToPtr< CPlayer >(const_cast< CEntity* >(mgr.GetObjectById(projectile->GetOwnerId())))) {
      mHitByPlayerProjectile = true;
    }
  }
}

void CPatterned::CollidedWith(const TUniqueId&, const CCollisionInfoList&, CStateManager&) {
  // TODO: Recover ground/static-ground flags, collision response and linked script messages.
}

void CPatterned::ThinkAboutMove(float) {
  // TODO: Apply scaled animation translation/rotation and account for frozen/disabled movement.
}

void CPatterned::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                                 float dt) {
  // TODO: Restore projectile, material, damage-window, movement and body-state events.
  CActor::DoUserAnimEvent(mgr, node, type, dt);
}

void CPatterned::Burn(CStateManager&, float, float) {
  // TODO: Check knockback options and Light vulnerability before starting fire damage.
}

void CPatterned::Shock(CStateManager&, float, float) {
  // TODO: Check knockback options and Annihilator vulnerability before starting electric damage.
}

void CPatterned::Freeze(CStateManager&, const CVector3f&, CUnitVector3f, float, float) {
  // TODO: Freeze the body controller and start model frost effects.
}

float CPatterned::GetDeathTimeScale() const {
  return rstl::max_val(mLaggedBurnDeath ? mBurnThinkRateTimer / 1.5f : 1.f, 0.1f);
}

void CPatterned::DeathDelete(CStateManager& mgr) {
  mSuppressKnockBack = true;
  if (!mStateMachine->HasState()) {
    InitializeStateMachine(mgr);
  }
  SendScriptMsgs(kSS_Dead, mgr, GetUniqueId(), kSM_None);
  if (GetBodyController()->IsElectrocuting()) {
    mPendingShockDamage = 0.f;
    BodyController()->DouseElectrocuting();
    mgr.ActorModelParticles()->StopElectric(*this);
  }
  mgr.DeleteObjectRequest(GetUniqueId());
}

CDamageInfo CPatterned::GetContactDamage() const {
  if (!mAlive) {
    return CDamageInfo();
  }
  return mContactDamage;
}

CTransform4f CPatterned::GetLctrTransform(const rstl::string& name) const {
  return GetTransform() * GetScaledLocatorTransform(name);
}

CTransform4f CPatterned::GetLctrTransform(const CSegId& id) const {
  CTransform4f locator = GetAnimationData()->GetLocatorTransform(id, nullptr);
  CVector3f scaled =
      CVector3f::ByElementMultiply(GetModelData()->GetScale(), locator.GetTranslation());
  return GetTransform() * CTransform4f(locator.BuildMatrix3f(), scaled);
}

CVector3f CPatterned::GetAimPosition(const CStateManager& mgr, float dt) const {
  // TODO: Use the lock-on locator, scaled bounds and predicted motion as the target does.
  return CPhysicsActor::GetAimPosition(mgr, dt);
}

CVector3f CPatterned::GetOrbitPosition(const CStateManager& mgr) const {
  return GetAimPosition(mgr, 0.f);
}

void CPatterned::PreRender(CStateManager& mgr) {
  CActor::PreRender(mgr);
  // TODO: Restore actor lighting, frozen/possession models and particle preparation.
}

bool CPatterned::CanRenderUnsorted(const CStateManager& mgr) const {
  return GetAnimationData()->GetParticleDB().AreAnySystemsDrawnWithModel()
             ? false
             : CActor::CanRenderUnsorted(mgr);
}

void CPatterned::PreRenderAllViewports(CStateManager& mgr) {
  CActor::PreRenderAllViewports(mgr);
  // TODO: Restore particle-light updates and frozen model effects.
}

void CPatterned::Render(const CStateManager& mgr) const {
  // TODO: Restore model flags, damage color and possession-transition rendering.
  CPhysicsActor::Render(mgr);
}

bool CPatterned::IsBeingSnatched() const {
  return mIngPossessionBlend > 0.f && mIngPossessionBlend < 1.f && mIngModel.valid() == true;
}

void CPatterned::RenderSystemsToBeDrawnFirst(const CStateManager&, uint mask, uint target) const {
  if (mDrawParticles) {
    GetAnimationData()->GetParticleDB().RenderSystemsToBeDrawnFirstPOICheck(mask, target);
  }
}

void CPatterned::RenderSystemsToBeDrawnLast(const CStateManager&, uint mask, uint target) const {
  if (mDrawParticles) {
    GetAnimationData()->GetParticleDB().RenderSystemsToBeDrawnLastPOICheck(mask, target);
  }
}

void CPatterned::fn_80074e54(const CModelFlags& flags) const {
  const CAnimData* animData = GetAnimationData();
  const CModelFlags iceFlags = flags.UseShaderSet(0);
  if (animData->GetIceModel().valid()) {
    animData->Render(**animData->GetIceModel().data(), iceFlags);
  }
}

void CPatterned::RenderIngSnatchingTransition(const CStateManager&) const {
  // TODO: Render the normal/possessed models on opposite sides of the snatching plane.
}

CVector3f CPatterned::GetIngSnatchingNormal(float) const { return CVector3f::Up(); }

CVector3f CPatterned::GetIngSnatchingPoint(float t) const {
  const CAABox bounds = GetBoundingBox();
  const float height = bounds.GetMaxPoint().GetZ() - bounds.GetMinPoint().GetZ();
  return GetTranslation() + height * ((1.f - t) * GetIngSnatchingNormal(t));
}

float CPatterned::GetIngSnatchingModelOverlapSize() const { return 0.f; }

void CPatterned::fn_800747a4(CAssetId, CAssetId) {
  // TODO: Build the possessed skinned model using the actor's shared character layout.
}

bool CPatterned::CanBeIngPossessed(CStateManager&) const { return mAlive && mIngModel.valid(); }

bool CPatterned::CanBeUnPossessed(CStateManager&) const { return true; }

void CPatterned::SetIngPossessed(bool possessed, CStateManager& mgr) {
  SetIngPossessed(possessed, 1.f, mgr);
  // TODO: Derive possession delay/duration from the current animation's EventStart/EventStop POIs.
}

void CPatterned::SetIngPossessed(bool possessed, float duration, CStateManager&) {
  if (!IsIngPossessed() && possessed) {
    if (mIngPossessionData.unknown_0xb68c0aa3) {
      *HealthInfo() = LdrToHealthInfo(mIngPossessionData.ingPossessedHealth);
    }
    mIngPossessionDelay = 0.f;
    mIngPossessionDuration = duration;
  }
  mIngPossessionTarget = possessed ? 1.f : 0.f;
}

bool CPatterned::IsIngPossessed() const {
  return mIngPossessionBlend > 0.f || mIngPossessionTarget > 0.f;
}

void CPatterned::UpdateIngPossession(float dt) {
  if (mIngPossessionBlend < mIngPossessionTarget) {
    const float delta = mIngPossessionDuration > 0.f ? dt / mIngPossessionDuration : 1.f;
    if (mIngPossessionDelay <= 0.f) {
      mIngPossessionBlend = rstl::min_val(mIngPossessionBlend + delta, 1.f);
      if (mIngPossessionBlend == 1.f && mIngModel) {
        AnimationData()->SetSkinnedModel(*mIngModel);
      }
    } else {
      mIngPossessionDelay -= dt;
    }
  } else if (mIngPossessionBlend > mIngPossessionTarget) {
    mIngPossessionBlend = rstl::max_val(0.f, mIngPossessionBlend - dt);
    if (mIngPossessionBlend == 0.f) {
      AnimationData()->SetSkinnedModel(mNormalModel);
    }
  }
}

const CDamageVulnerability* CPatterned::GetDamageVulnerability() const {
  if (mIngPossessionTarget > mIngPossessionBlend) {
    return &CDamageVulnerability::ImmuneVulnerabilty();
  }
  if (IsIngPossessed() && mIngPossessionData.unknown_0xb68c0aa3) {
    return &mIngVulnerability;
  }
  return CAi::GetDamageVulnerability();
}

const CDamageVulnerability* CPatterned::GetDamageVulnerability(const CVector3f&, const CVector3f&,
                                                               const CDamageInfo&) const {
  return GetDamageVulnerability();
}

CScannableObjectInfo* CPatterned::GetScannableObjectInfo() const {
  if (IsIngPossessed() && !mIngScanInfo.null()) {
    return **mIngScanInfo;
  }
  return CActor::GetScannableObjectInfo();
}

CEnergyProjectile* CPatterned::LaunchProjectile(const CTransform4f&, CStateManager&, int, uint,
                                                bool, const CImpactVisorEffect&, const CVector3f&) {
  // TODO: Check projectile resource/count limits, construct the energy projectile and add it.
  return nullptr;
}

EWeaponCollisionResponseTypes CPatterned::GetCollisionResponseType(const CVector3f& position,
                                                                   const CVector3f& direction,
                                                                   const CWeaponMode& mode,
                                                                   int attributes) const {
  if (GetBodyController()->IsFrozen() && mode.GetType() == kWT_Dark) {
    return kWCR_None;
  }
  return CAi::GetCollisionResponseType(position, direction, mode, attributes);
}

void CPatterned::PreThink(float dt, CStateManager& mgr) {
  mPreThinkDt = dt;
  CEntity::PreThink(dt, mgr);
}

void CPatterned::AddToRenderer(const CStateManager& mgr) const {
  if (mDrawParticles && HasModelData()) {
    uint mask;
    uint target;
    mgr.GetCharacterRenderMaskAndTarget(mask, target);
    if (const CAnimData* animData = GetAnimationData()) {
      animData->GetParticleDB().AddToRendererClippedMasked(mgr.GetFrustumPlanes(), mask, target);
    }
  }
  CActor::AddToRenderer(mgr);
}

bool CPatterned::IsOnStaticGround() const { return mOnStaticGround; }

bool CPatterned::TryToBeCaptured(CStateManager&) { return false; }

CCharAnimTime CPatterned::GetTimeOfUserEventForAnimation(const CPASAnimParmData& parms,
                                                         EUserEventType type) const {
  const rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(parms, -1);
  if (best.first > FLT_EPSILON) {
    return GetAnimationData()->GetTimeOfUserEventForAnimation(best.second, type);
  }
  return CCharAnimTime(CCharAnimTime::kT_Infinity, 1.f);
}

int CPatterned::GetNumUserEventsForAnimation(const CPASAnimParmData& parms,
                                             EUserEventType type) const {
  const rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(parms, -1);
  if (best.first > FLT_EPSILON) {
    return GetAnimationData()->CountUserEventsForAnimation(best.second, type);
  }
  return 0;
}

float CPatterned::GetAverageAttackTime() const {
  const float timeScale = GetBodyController()->GetTimeScale();
  if (timeScale > 0.f) {
    return mAverageAttackTime / timeScale;
  }
  return mAverageAttackTime;
}

void CPatterned::AddParticleEffect(CStateManager&, const CTransform4f&, float, CAssetId, uint,
                                   int) {
  // TODO: Add the PART effect to the animation's particle database.
}

CAABox CPatterned::GetScanVisorRenderBounds(const CStateManager&) const {
  return CAABox::Identity();
}

CPatterned::~CPatterned() {}
