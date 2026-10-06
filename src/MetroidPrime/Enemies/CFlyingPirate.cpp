#include "MetroidPrime/Enemies/CFlyingPirate.hpp"

#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CFrustumPlanes.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWater.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrFlyingPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

static const SBurst skBurstsFlying[] = {
    {10, {3, 4, 11, 12, -1, 0, 0, 0}, 0.1f, 0.05f},
    {20, {2, 3, 4, 5, -1, 0, 0, 0}, 0.1f, 0.05f},
    {20, {10, 11, 12, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {25, {15, 16, 1, 2, -1, 0, 0, 0}, 0.1f, 0.05f},
    {25, {5, 6, 7, 8, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsFlyingOutOfView[] = {
    {5, {3, 4, 11, 12, -1, 0, 0, 0}, 0.1f, 0.05f},
    {10, {2, 3, 4, 5, -1, 0, 0, 0}, 0.1f, 0.05f},
    {10, {10, 11, 12, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {40, {15, 16, 1, 2, -1, 0, 0, 0}, 0.1f, 0.05f},
    {35, {5, 6, 7, 8, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsLanded[] = {
    {30, {3, 4, 5, 11, 12, 4, -1, 0}, 0.1f, 0.05f},  {20, {2, 3, 4, 5, 4, 3, -1, 0}, 0.1f, 0.05f},
    {20, {5, 4, 3, 13, 12, 11, -1, 0}, 0.1f, 0.05f}, {30, {1, 2, 3, 4, 5, 6, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

static const SBurst skBurstsLandedOutOfView[] = {
    {10, {6, 5, 4, 14, 13, 12, -1, 0}, 0.1f, 0.05f},
    {20, {14, 13, 12, 11, 10, 9, -1, 0}, 0.1f, 0.05f},
    {20, {14, 15, 16, 11, 10, 9, -1, 0}, 0.1f, 0.05f},
    {50, {11, 10, 9, 8, 7, 6, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.f, 0.f},
};

const float CFlyingPirate::skGravityConstant = 50.f;
const float CFlyingPirate::skAquaGravityConstant = 5.f;

static EMaterialTypes skSolidMaterial = kMT_Unknown59;

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"HearShot", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::HearShot)},
    {"HearPlayer",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::HearPlayer)},
    {"ShouldSpecialAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShouldSpecialAttack)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShouldAttack)},
    {"LineOfSight",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::LineOfSight)},
    {"PatternOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::PatternOver)},
    {"PatternShagged",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::PatternShagged)},
    {"SpotPlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::SpotPlayer)},
    {"ShouldDodge",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShouldDodge)},
    {"ShotAt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShotAt)},
    {"Attacked", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::Attacked)},
    {"CoverCheck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::CoverCheck)},
    {"CoverFind", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::CoverFind)},
    {"Landed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::Landed)},
    {"ShouldMove", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShouldMove)},
    {"Stuck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::Stuck)},
    {"DeathOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::DeathOver)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::AnimOver)},
    {"InRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::InRange)},
    {"InPosition", static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::InPosition)},
    {"AggressionCheck",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::AggressionCheck)},
    {"ShouldRetreat",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::ShouldRetreat)},
    {"HasAttackPattern",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CFlyingPirate::HasAttackPattern)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Attack)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::PathFind)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Patrol)},
    {"TargetPatrol",
     static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::TargetPatrol)},
    {"TurnAround", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::TurnAround)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Dodge)},
    {"Lurk", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Lurk)},
    {"Taunt", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Taunt)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Dead)},
    {"GetUp", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::GetUp)},
    {"GetUpNow", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::GetUpNow)},
    {"Jump", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Jump)},
    {"ProjectileAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::ProjectileAttack)},
    {"Land", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Land)},
    {"Walk", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Walk)},
    {"Retreat", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Retreat)},
    {"Explode", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Explode)},
    {"Enraged", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Enraged)},
    {"Bounce", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Bounce)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::Deactivate)},
    {"ChooseWaypoint",
     static_cast< CPatterned::StateMachine::StateFunc >(&CFlyingPirate::ChooseWaypoint)},
};

static const SBurst* skBursts[] = {
    skBurstsFlying, skBurstsFlyingOutOfView, skBurstsLanded, skBurstsLandedOutOfView, nullptr,
};

static rstl::string skJetPack = rstl::string_l("JetPack");
static rstl::string skScubaGear = rstl::string_l("ScubaGear");
static rstl::string skScubaBubbles = rstl::string_l("ScubaBubbles");
static rstl::string skSparks = rstl::string_l("Sparks");
static rstl::string skLandingSmoke = rstl::string_l("LandingSmoke");
static rstl::string skEyes = rstl::string_l("Eyes");

CFlyingPirate::CFlyingPirateData::CFlyingPirateData(
    float maxCoverDistance, float hearingDistance, uint type, CAssetId projectile,
    const CDamageInfo& projectileDamage, ushort gunSfx, CAssetId missile,
    const CDamageInfo& missileDamage, CAssetId wpsc, float knockBackDelay, float flyingHeight,
    CAssetId rocketPackExplosion, const CDamageInfo& rocketPackExplosionDamage,
    float spiralChance, float minimumMissileTime, float missileTimeVariation, float flightThrust,
    ushort impactSfx, ushort spiralSfx, float coverCheckChance, float intraBurstShotTime,
    float intraBurstShotVariation, CAssetId landingCloudDirt, CAssetId landingCloudDust,
    CAssetId landingCloudSnow, ushort hurledSfx, ushort deathSfx, float aggressionChance,
    float jumpAggressionChance, float projectileHomingDistance, float unknown_0xccf05648,
    float unknown_0x2a90f9a9, float unknown_0x9ca8f357, float unknown_0x7ac85cb6)
: mMaxCoverDistance(maxCoverDistance)
, mHearingDistance(hearingDistance)
, mType(type)
, mProjectile(projectile)
, mProjectileDamage(projectileDamage)
, mGunSfx(gunSfx)
, mMissile(missile)
, mMissileDamage(missileDamage)
, mWpsc(wpsc)
, mWpscDamage(CDamageInfo())
, mKnockBackDelay(knockBackDelay)
, mFlyingHeight(flyingHeight)
, mRocketPackExplosion(rocketPackExplosion)
, mDInfo(rocketPackExplosionDamage)
, mSpiralChance(spiralChance)
, mMinimumMissileTime(minimumMissileTime)
, mMissileTimeVariation(missileTimeVariation)
, mFlightThrust(flightThrust)
, mRagDollSfx1(impactSfx)
, mRagDollSfx2(spiralSfx)
, mCoverCheckChance(coverCheckChance)
, mIntraBurstShotTime(intraBurstShotTime)
, mIntraBurstShotVariation(intraBurstShotVariation)
, mParticleGen1(landingCloudDirt)
, mParticleGen2(landingCloudDust)
, mParticleGen3(landingCloudSnow)
, mKnockBackSfx(hurledSfx)
, mDeathSfx(deathSfx)
, mAggressionChance(aggressionChance)
, mJumpAggressionChance(jumpAggressionChance)
, mProjectileHomingDistance(projectileHomingDistance)
, unknown_0xccf05648(unknown_0xccf05648)
, unknown_0x2a90f9a9(unknown_0x2a90f9a9)
, unknown_0x9ca8f357(unknown_0x9ca8f357)
, unknown_0x7ac85cb6(unknown_0x7ac85cb6) {}

CFlyingPirate::CFlyingPirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                             const CTransform4f& xf, const CModelData& modelData,
                             const CActorParameters& actParms, const CPatternedInfo& pInfo,
                             const CFlyingPirateData& data)
: CPatterned(static_cast< EPatternedAI >(21), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, kBT_AiMovedFlyer, actParms)
, mData(data)
, mGunProjectileInfo(data.mProjectile, data.mProjectileDamage)
, mAltProjectileInfo1(data.mMissile, data.mMissileDamage)
, mAltProjectileInfo2(data.mWpsc, data.mWpscDamage)
, mParticleGenDesc(gpSimplePool->GetObj(SObjectTag('PART', data.mRocketPackExplosion)))
, mCurrentCoverPoint(kInvalidUniqueId)
, mPathFindSearch(nullptr, (mData.mType & 2) ? 4 : 3, pInfo.GetPathfindingIndex(),
                  pInfo.GetHalfExtent() * modelData.GetScale().GetX(),
                  pInfo.GetHeight() * modelData.GetScale().GetZ(), 1, CPFRegion::kRP_Center)
, x78c_(0.f)
, x790_(0)
, mInitialHealth(pInfo.GetHealthInfo().GetHP())
, mHeadSegId(CSegId::Invalid())
, mBoneTracking(*GetModelData()->GetAnimationData(), rstl::string_l("Head_1"),
                CMath::Deg2Rad(80.f), CMath::Deg2Rad(180.f), kBTF_None)
, mGunSegId(CSegId::Invalid())
, x7e4_(1.f)
, mTargetId(kInvalidUniqueId)
, mBurstFire(skBursts, 0)
, mDodgeDirection(pas::kSD_Invalid)
, mHeight(3.f)
, x854_(3.4028235e38f)
, x858_(3.4028235e38f)
, mAttackObjectId(kInvalidUniqueId)
, x860_(15.f)
, x86c_(0.f)
, x870_(CVector3f::Zero())
, x87c_(CVector3f::Zero())
, x888_(10.f)
, mRagDollTimer(3.f)
, mTeamAiMgr(kInvalidUniqueId)
, mPitchBend(1.f)
, x898_(1.f)
, mPatrolTarget(kInvalidUniqueId)
, x8a4_(0.f)
, mLineOfSightTracker(GetUniqueId(), CSegId::Invalid(), 0.2f, 0.05f)
, xbb0_(0)
, xbb4_(11)
, xbb8_24_(false)
, mIsFlyingPirate(mData.mType & 1)
, mIsAquaPirate(mData.mType & 2)
, mHearShot(false)
, mCanPatrol(false)
, x6a0_28_(false)
, mCheckForProjectiles(false)
, x6a0_30_(false)
, mPrevInCineCam(false)
, x6a1_25_(false)
, mIsAttackingObject(false)
, x6a1_27_(false)
, x6a1_28_(false)
, mIsMoving(false)
, mSpinToDeath(false)
, mStopped(false)
, mAggressive(false)
, mAggressionChecked(false)
, mJetpackActive(false)
, mSparksActive(false)
, x6a2_28_(false)
, xbba_29_(false)
, xbba_30_(false) {
  mGunProjectileInfo.Token().Lock();
  mAltProjectileInfo1.Token().Lock();
  mAltProjectileInfo2.Token().Lock();
  const CAnimData* animData = GetModelData()->GetAnimationData();
  mHeadSegId = animData->GetLocatorSegId(rstl::string_l("Head_1"));
  mGunSegId = animData->GetLocatorSegId(rstl::string_l("L_gun_LCTR"));
  mMissileSegments.push_back(animData->GetLocatorSegId(rstl::string_l("L_Missile_LCTR")));
  mMissileSegments.push_back(animData->GetLocatorSegId(rstl::string_l("R_Missile_LCTR")));
  mBoneTracking.SetDisableTrackingDistance(25.f * GetModelData()->GetScale().GetZ());
  const CPASAnimParmData parms(pas::kAS_Step, CPASAnimParm::FromEnum(3), CPASAnimParm::FromEnum(1));
  mHeight = GetModelData()->GetScale().GetX() * GetAnimationDistance(parms);
  if (mData.mParticleGen1 != kInvalidAssetId && mData.mParticleGen2 != kInvalidAssetId &&
      mData.mParticleGen3 != kInvalidAssetId) {
    mParticleGenDescs.push_back(gpSimplePool->GetObj(SObjectTag('PART', mData.mParticleGen1)));
    mParticleGenDescs.push_back(gpSimplePool->GetObj(SObjectTag('PART', mData.mParticleGen2)));
    mParticleGenDescs.push_back(gpSimplePool->GetObj(SObjectTag('PART', mData.mParticleGen3)));
    for (int i = 0; i < mParticleGenDescs.size(); ++i) {
      mParticleGens.push_back(rs_new CElementGen(mParticleGenDescs[i]));
      mParticleGens[i]->SetParticleEmission(false);
    }
  }
  KnockBackController().SetLocomotionDuringElectrocution(true);
  mOnGround = !mIsFlyingPirate;
  mLineOfSightTracker.SetSegment(mHeadSegId);
  mLineOfSightTracker.SetRayFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Unknown59, kMT_Character),
      CMaterialList(kMT_Player, kMT_CollisionActor, kMT_NoPlatformCollision,
                    kMT_ExcludeFromLineOfSightTest)));
}

void CFlyingPirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  switch (message) {
  case kSM_Alert:
    if (GetActive()) {
      mHitByPlayerProjectile = true;
    }
    break;
  case kSM_Activate:
    AddToTeam(mgr);
    break;
  case kSM_Deactivate:
  case kSM_Delete:
    RemoveFromTeam(mgr);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_AreaLoaded:
    for (AUTO(it, GetConnectionList().begin()); it != GetConnectionList().end(); ++it) {
      if (it->state == kSS_Retreat) {
        const TUniqueId id = mgr.GetIdForScript(it->objId);
        if (CScriptCoverPoint* cover = TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(id))) {
          cover->Reserve(GetUniqueId());
        }
      } else if (it->state == kSS_Patrol && it->msg == kSM_Follow) {
        mCanPatrol = true;
      } else if (it->state == kSS_Attack && it->msg == kSM_Action) {
        mAttackObjectId = mgr.GetIdForScript(it->objId);
      }
    }
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    if (GetActive()) {
      AddToTeam(mgr);
    }
    UpdateParticleEffects(mgr, 0.f, mIsFlyingPirate);
    AnimationData()->SetEffectState(skEyes, true, mgr);
    mLineOfSightTracker.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    break;
  case kSM_Create: {
    const float range = mData.mMissileTimeVariation;
    const float delay = mData.mMinimumMissileTime;
    x86c_ = range * mgr.Random()->Float() + delay;
    break;
  }
  case kSM_Falling:
    if (GetBodyController()->GetPercentageFrozen() == 0.f && !mFadeToDeath && !mSpinToDeath) {
      SetMomentumWR(CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass()));
    }
    mBurstFire.SetBurstType(0);
    break;
  case kSM_Landed:
    mBurstFire.SetBurstType(2);
    break;
  case kSM_Launching:
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, mCurrentCoverPoint)) {
      mVerticalMovement = false;
      SetMomentumWR(CVector3f(0.f, 0.f, -GetMass() * GetGravityConstant()));
      AddMaterial(kMT_GroundCollider, mgr);
      SetDestPos(cover->GetTranslation());
      const CVector3f delta = cover->GetTranslation() - GetTranslation();
      if (delta.GetZ() < 0.f) {
        CVector3f velocity = GetVelocityWR();
        const float gravity = GetGravityConstant();
        const float root =
            CMath::SqrtF(-(2.f * gravity * delta.GetZ() - velocity.GetZ() * velocity.GetZ()));
        const float time = (-velocity.GetZ() + root) / gravity;
        if (time > 0.f) {
          const CVector2f normal = CVector2f(delta.GetX(), delta.GetY()).AsNormalized();
          const float speed = CVector2f(delta.GetX(), delta.GetY()).Magnitude() / time;
          velocity.SetX(speed * normal.GetX());
          velocity.SetY(speed * normal.GetY());
          SetVelocityWR(velocity);
          x870_ = CVector3f::Zero();
          x87c_ = CVector3f::Zero();
          x898_ = 1.f;
        }
      }
    }
    break;
  case kSM_Start:
    mStopped = false;
    break;
  case kSM_Stop:
    mStopped = true;
    break;
  case kSM_SetToZero:
    x6a2_28_ = true;
    break;
  }
}

bool CFlyingPirate::Listen(CStateManager& mgr, const CVector3f& pos, EListenNoiseType type) {
  bool heard = false;
  if (mAlive) {
    const float hearingDistance = mData.mHearingDistance * mData.mHearingDistance;
    const CVector3f delta = pos - GetTranslation();
    if (delta.MagSquared() < hearingDistance &&
        (mDetectionHeightRange == 0.f ||
         delta.GetZ() * delta.GetZ() < mDetectionHeightRange * mDetectionHeightRange)) {
      mHearShot = true;
      heard = true;
    }
    if (type == kLNT_PlayerFire) {
      mCheckForProjectiles = true;
    }
  }
  const bool result = heard;
  return result;
}

void CFlyingPirate::DeliverGetUp() {
  if (BodyController()->GetCurrentStateId() == pas::kAS_LieOnGround) {
    BodyController()->CommandMgr().DeliverCmd(CBCGetupCmd(pas::kGetup_Zero));
  }
}

void CFlyingPirate::UpdateParticleEffects(CStateManager& mgr, float intensity, const bool active) {
  CAnimData* animData = AnimationData();
  const rstl::string& name = mIsAquaPirate ? skScubaGear : skJetPack;
  if (active != mJetpackActive) {
    animData->SetEffectState(name, active, mgr);
    if (mIsAquaPirate) {
      animData->SetEffectState(skScubaBubbles, active, mgr);
    }
    mJetpackActive = active;
  }
  if (active) {
    animData->SetEffectComponentExternalParam(
        name, 0,
        intensity * (mData.unknown_0x2a90f9a9 - mData.unknown_0xccf05648) +
            mData.unknown_0xccf05648);
    animData->SetEffectComponentExternalParam(
        name, 1,
        intensity * (mData.unknown_0x7ac85cb6 - mData.unknown_0x9ca8f357) +
            mData.unknown_0x9ca8f357);
  }
  if (!mIsAquaPirate) {
    bool sparks = active && intensity > 0.8f;
    if (sparks != mSparksActive) {
      animData->SetEffectState(skSparks, sparks, mgr);
      mSparksActive = sparks;
    }
  }
}

void CFlyingPirate::UpdateLandingSmoke(CStateManager& mgr, bool active) {
  if (active) {
    if (!mParticleGens.empty()) {
      float particleLevel = GetTranslation().GetZ() - 5.f;
      CScriptCoverPoint* cover = GetCoverPoint(mgr, mCurrentCoverPoint);
      if (cover != nullptr) {
        particleLevel = cover->GetTranslation().GetZ() - 1.f;
      }
      const CRayCastResult result = mgr.RayStaticIntersection(
          GetTranslation(), CVector3f::Down(), GetTranslation().GetZ() - particleLevel,
          CMaterialFilter::MakeInclude(CMaterialList(kMT_Unknown59)));
      int index = 1;
      if (result.IsValid()) {
        const CMaterialList& material = result.GetMaterial();
        if (material.HasMaterial(kMT_Dirt) || material.HasMaterial(kMT_Organic) ||
            material.HasMaterial(kMT_Sand)) {
          index = 0;
        }
        particleLevel = GetTranslation().GetZ() - result.GetTime();
      }
      mParticleGens[index]->SetParticleEmission(true);
      const CVector3f& origin = GetTranslation();
      mParticleGens[index]->SetTranslation(
          CVector3f(origin.GetX(), origin.GetY(), particleLevel));
    }
    AnimationData()->SetEffectState(skLandingSmoke, true, mgr);
  } else {
    for (int i = 0; i < mParticleGens.size(); ++i) {
      mParticleGens[i]->SetParticleEmission(false);
    }
    AnimationData()->SetEffectState(skLandingSmoke, false, mgr);
  }
}

CVector3f CFlyingPirate::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                   const CVector3f& aimPos) const {
  return GetTranslation();
}

void CFlyingPirate::AddToTeam(CStateManager& mgr) {
  if (mTeamAiMgr == kInvalidUniqueId) {
    mTeamAiMgr = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamAiMgr != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgr))) {
      team->JoinTeam(*this, CTeamAiRole::ETeamAiRole(2), CTeamAiRole::ETeamAiRole(3),
                     CTeamAiRole::ETeamAiRole(-1));
    }
  }
}

void CFlyingPirate::RemoveFromTeam(CStateManager& mgr) {
  if (mTeamAiMgr != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgr))) {
      if (team->IsPartOfTeam(GetUniqueId())) {
        team->QuitTeam(GetUniqueId());
        mTeamAiMgr = kInvalidUniqueId;
      }
    }
  }
}

void CFlyingPirate::CheckForProjectiles(CStateManager& mgr) {
  if (mCheckForProjectiles) {
    const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
    const CVector3f extent(5.f, 5.f, 5.f);
    const CAABox box(playerPos - extent, playerPos + extent);
    x6a0_30_ = false;
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    mgr.BuildNearList(nearList, box, CMaterialFilter::MakeInclude(CMaterialList(kMT_Projectile)),
                      nullptr);
    for (int i = 0; i < nearList.size(); ++i) {
      if (const CGameProjectile* const projectile =
              TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(nearList[i]))) {
        CVector3f delta = GetBoundingBox().GetCenterPoint() - projectile->GetTranslation();
        if (delta.IsMagnitudeSafe()) {
          if (CVector3f::Dot(GetTransform().GetForward(), delta) < 0.f) {
            delta.Normalize();
            CVector3f movement = projectile->GetTranslation() - projectile->GetPreviousPos();
            if (movement.IsMagnitudeSafe()) {
              movement.Normalize();
              if (CVector3f::Dot(movement, delta) > 0.939f) {
                x6a0_30_ = true;
              }
            }
          }
        } else {
          x6a0_30_ = true;
        }
        if (x6a0_30_) {
          break;
        }
      }
    }
    mCheckForProjectiles = false;
  }
}

bool CFlyingPirate::LineOfSightTest(CStateManager& mgr, const CVector3f& start,
                                    const CVector3f& end, const CMaterialList& exclude) {
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), exclude);
  return mgr.RayCollideWorld(start, end, filter, this);
}

CVector3f CFlyingPirate::AvoidActors(CStateManager& mgr) {
  CVector3f separation = CVector3f::Zero();
  const CVector3f extent(8.f, 8.f, 8.f);
  const CAABox box(GetTranslation() - extent, GetTranslation() + extent);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, box, CMaterialFilter::MakeInclude(CMaterialList(kMT_Character)),
                    this);
  for (int i = 0; i < nearList.size(); ++i) {
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(nearList[i]))) {
      separation += mSteeringBehaviors.Separation(*this, actor->GetTranslation(), 10.f);
    }
  }
  CVector3f delta = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  delta.SetZ(0.f);
  separation += mSteeringBehaviors.Separation(*this, GetTranslation() + delta, 20.f);
  return separation;
}

CVector3f CFlyingPirate::GetTargetPos(CStateManager& mgr) {
  if (mTargetId != mgr.GetPlayer(0)->GetUniqueId()) {
    if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId))) {
      if (actor->GetActive()) {
        return actor->GetTranslation();
      }
    }
    mBoneTracking.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    mTargetId = mgr.GetPlayer(0)->GetUniqueId();
  }
  return mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f);
}

void CFlyingPirate::MassiveDeath(CStateManager& mgr) {
  CExplosion* explosion = rs_new CExplosion(
      mParticleGenDesc, mgr.AllocateUniqueId(),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true), rstl::string_l(""),
      GetTransform(), 0, CVector3f(1.5f, 1.5f, 1.5f), CColor::White(), -1);
  if (explosion != nullptr) {
    mgr.AddObject(*explosion);
    mgr.ApplyDamageToWorld(
        GetUniqueId(), *this, GetTranslation(), mData.mDInfo,
        CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), CMaterialList()));
    for (int i = 0; i < mgr.GetNumPlayers(); ++i) {
    }
  }
  CPatterned::MassiveDeath(mgr);
}

bool CFlyingPirate::PatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

bool CFlyingPirate::HearShot(CStateManager& mgr, const CTriggerData& data) const {
  const bool heard = mHearShot;
  const_cast< CFlyingPirate* >(this)->mHearShot = false;
  return heard;
}

void CFlyingPirate::GetUpNow(CStateManager& mgr, EStateMsg msg, float dt) {}

bool CFlyingPirate::LineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return mLineOfSightTracker.HasLineOfSight();
}

bool CFlyingPirate::Landed(CStateManager& mgr, const CTriggerData& data) const {
  return GetBodyController()->GetCurrentStateId() == pas::kAS_LieOnGround;
}

bool CFlyingPirate::AggressionCheck(CStateManager& mgr, const CTriggerData& data) const {
  return mAggressive;
}

void CFlyingPirate::Deactivate(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mPendingDeath = true;
  }
}

CProjectileInfo* CFlyingPirate::ProjectileInfo() { return &mGunProjectileInfo; }

void CFlyingPirate::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

void CFlyingPirate::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);
}

void CFlyingPirate::PreThink(float dt, CStateManager& mgr) {
  mBoneTracking.PreThink(*AnimationData());
  CPatterned::PreThink(dt, mgr);
}

void CFlyingPirate::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

CEntity* REL_LoadFlyingPirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrFlyingPirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrFlyingPirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CFlyingPirate::CFlyingPirateData data(
      sldrThis.searchRadius, sldrThis.hearingRadius, sldrThis.unknown_0x20daf45e,
      sldrThis.projectile, LdrToDamageInfo(sldrThis.projectileDamage), sldrThis.sound_Projectile,
      sldrThis.missile, LdrToDamageInfo(sldrThis.missileDamage), sldrThis.wPSC,
      sldrThis.hurlRecoverTime, sldrThis.hoverHeight, sldrThis.rocketPackExplosion,
      LdrToDamageInfo(sldrThis.rocketPackExplosionDamage), sldrThis.spiralChance,
      sldrThis.minimumMissileTime, sldrThis.missileTimeVariation, sldrThis.flightThrust,
      sldrThis.sound_Impact, sldrThis.sound_Spiral, sldrThis.landChance,
      sldrThis.intraBurstShotTime, sldrThis.intraBurstShotVariation, sldrThis.landingCloudDirt,
      sldrThis.landingCloudDust, sldrThis.landingCloudSnow, sldrThis.sound_Hurled,
      sldrThis.sound_Death, sldrThis.doubleAttackChance, sldrThis.unknown_0x3427d27f,
      sldrThis.stopHomingRange, sldrThis.unknown_0xccf05648, sldrThis.unknown_0x2a90f9a9,
      sldrThis.unknown_0x9ca8f357, sldrThis.unknown_0x7ac85cb6);

  return rs_new CFlyingPirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                              LdrToEntityInfo(info, sldrThis.editorProperties),
                              LdrToTransform4f(sldrThis.editorProperties), *modelData,
                              LdrToActorParameters(sldrThis.actorInformation),
                              LdrToPatternedInfo(sldrThis.patterned, nullptr), data);
}

static SFlyingPirate_FuncPtrs REL_loader_FlyingPirate;

void SetRelLoaderFunctionToLoader() {
  REL_loader_FlyingPirate.mLoader = REL_LoadFlyingPirate;
  SetSFlyingPirate_FuncPtrs(&REL_loader_FlyingPirate);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSFlyingPirate_FuncPtrs(nullptr); }
