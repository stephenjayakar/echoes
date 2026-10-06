#include "MetroidPrime/Enemies/CBlogg.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrBlogg.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

// Guessed name. Armour vulnerability for the Blogg's collision actors; while the Blogg is
// Ing-possessed its possessed armour vulnerability is used instead.
class CBloggArmorVulnerability : public CNonUniformVulnerability {
public:
  CBloggArmorVulnerability(const CDamageVulnerability& armor,
                           const CDamageVulnerability& possessedArmor, bool unknown)
  : mArmor(armor), mPossessedArmor(possessedArmor), mOwner(nullptr), x68_(unknown) {}

  const CDamageVulnerability* GetDamageVulnerability(const CDamageVulnerability* vulnerability,
                                                     const CVector3f& point,
                                                     const CVector3f& direction,
                                                     const CDamageInfo& info) const override {
    if (mOwner && mOwner->IsIngPossessed()) {
      return &mOwner->GetIngPossessedArmorVulnerability();
    }
    return &mArmor;
  }

  bool GetCollisionResponseType(const CVector3f& point, const CVector3f& direction,
                                const CWeaponMode& mode, int attributes,
                                EWeaponCollisionResponseTypes& type) const override {
    switch (mArmor.GetEffect(mode)) {
    case 1:
      type = kWCR_EnemyShielded;
      break;
    default:
      type = kWCR_EnemyNormal;
      break;
    }
    return true;
  }

  void SetOwner(CBlogg* owner) { mOwner = owner; }

protected:
  CDamageVulnerability mArmor;
  CDamageVulnerability mPossessedArmor;
  CBlogg* mOwner;
  bool x68_;
};

// Guessed name. The head collision actor: hits inside the mouth angle use the Blogg's own
// vulnerability, everything else hits the armour.
class CBloggMouthVulnerability : public CBloggArmorVulnerability {
public:
  CBloggMouthVulnerability(const CDamageVulnerability& armor,
                           const CDamageVulnerability& possessedArmor, bool unknown)
  : CBloggArmorVulnerability(armor, possessedArmor, unknown), mResponseType(kWCR_EnemyShielded) {}

  const CDamageVulnerability* GetDamageVulnerability(const CDamageVulnerability* vulnerability,
                                                     const CVector3f& point,
                                                     const CVector3f& direction,
                                                     const CDamageInfo& info) const override {
    switch (mArmor.GetEffect(info.GetWeaponMode())) {
    case 1:
      mResponseType = kWCR_EnemyShielded;
      break;
    default:
      mResponseType = kWCR_EnemyNormal;
      break;
    }
    if (mOwner && mOwner->IsInMouthAngle(direction) && mOwner->GetIsGrabbingBall() == 0) {
      mResponseType = kWCR_EnemyNormal;
      return mOwner->GetDamageVulnerability();
    }
    return CBloggArmorVulnerability::GetDamageVulnerability(vulnerability, point, direction, info);
  }

  bool GetCollisionResponseType(const CVector3f& point, const CVector3f& direction,
                                const CWeaponMode& mode, int attributes,
                                EWeaponCollisionResponseTypes& type) const override {
    type = mResponseType;
    return true;
  }

private:
  mutable EWeaponCollisionResponseTypes mResponseType;
};

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldPatrol", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldPatrol)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::AnimOver)},
    {"ShouldPrepareToAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldPrepareToAttack)},
    {"InAttackPosition",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::InAttackPosition)},
    {"InValidPosition",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::InValidPosition)},
    {"IsFacingPlayer",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::IsFacingPlayer)},
    {"IsPlayerStunned",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::IsPlayerStunned)},
    {"ProjectileAttackDelay",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ProjectileAttackDelay)},
    {"ShouldCharge", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldCharge)},
    {"IsChargeOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::IsChargeOver)},
    {"CanMeleeAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CanMeleeAttack)},
    {"CanRangedAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CanRangedAttack)},
    {"CanTaunt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CanTaunt)},
    {"InProjectileRange",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::InProjectileRange)},
    {"CollidedWithWall",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CollidedWithWall)},
    {"CanBitePlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CanBitePlayer)},
    {"InMeleeRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::InMeleeRange)},
    {"InBiteRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::InBiteRange)},
    {"CantMoveToPlayer",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CantMoveToPlayer)},
    {"ShouldEndPursuit",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldEndPursuit)},
    {"ShouldEndBallPursuit",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldEndBallPursuit)},
    {"PlayerInBallMode",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::PlayerInBallMode)},
    {"DetectBall", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::DetectBall)},
    {"CanGrabBall", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::CanGrabBall)},
    {"BallGrabbed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::BallGrabbed)},
    {"IsPlayerReachable",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::IsPlayerReachable)},
    {"ShouldAbortBallGrab",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CBlogg::ShouldAbortBallGrab)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::Patrol)},
    {"MoveToAttackPosition",
     static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::MoveToAttackPosition)},
    {"MoveToValidPosition",
     static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::MoveToValidPosition)},
    {"FacePlayer", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::FacePlayer)},
    {"ProjectileAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::ProjectileAttack)},
    {"ChargeTelegraph",
     static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::ChargeTelegraph)},
    {"ChargeAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::ChargeAttack)},
    {"MeleeAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::MeleeAttack)},
    {"Stunned", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::Stunned)},
    {"Taunt", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::Taunt)},
    {"MoveToPlayer", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::MoveToPlayer)},
    {"GrabBall", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::GrabBall)},
    {"Thrash", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::Thrash)},
    {"SpitBall", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::SpitBall)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CBlogg::Dead)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"ComputeAttackPositions",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CBlogg::ComputeAttackPositions)},
    {"ComputeTauntProbability",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CBlogg::ComputeTauntProbability)},
    {"EndMeleePursuit",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CBlogg::EndMeleePursuit)},
};

static EMaterialTypes skIncludeMaterial = kMT_Unknown59;
static EMaterialTypes skExcludeMaterial1 = static_cast< EMaterialTypes >(0x21);
static EMaterialTypes skExcludeMaterial2 = static_cast< EMaterialTypes >(0x2f);
static EMaterialTypes skExcludeMaterial3 = static_cast< EMaterialTypes >(0x1c);
static EMaterialTypes skExcludeMaterial4 = static_cast< EMaterialTypes >(0x20);

static float skUnknownAngle = 0.5235988f;
static CVector3f skUnknownOffset(0.f, 0.f, 5.f);
static float skForwardSpeed;
static float skSideSpeed;

inline SBloggAttackCount::SBloggAttackCount(const SLdrBloggStruct& data)
: mMin(data.min_________________________)
, mMax(data.max_________________________)
, x4_(data.unknown_0x6e603df2)
, x8_(data.unknown_0x1e74f1ec)
, xc_(data.unknown_0xecba9fb2) {}

CBlogg::CBlogg(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
               const CActorParameters& actParms, float minAttackAngle, float maxAttackAngle,
               float minProjectileDelay, float maxProjectileDelay, uchar unknown0xa19d5f62,
               CAssetId projectile, const CDamageInfo& projectileDamage,
               float bodyDamageMultiplier, float mouthDamageMultiplier, float mouthDamageAngle,
               const CDamageVulnerability& armorVulnerability, float chargeDamageRadius,
               float chargeDamage, float biteDamage, float ballSpitDamage,
               float fishAttractionRadius, float fishAttractionPriority, float aggressiveness,
               float unknown0x479ccc37, float unknown0x689a803f, float unknown0x800a2b0d,
               float chargeTurnSpeed, float chargeSpeedMultiplier, float maxMeleeRange,
               float maxBallDetectionRange, float maxPlayerPursuitTime, float maxBallPursuitTime,
               ushort mouthOpenSound, float minDelayBetweenMeleeAttacks, float maxCollisionTime,
               bool isMegaBlogg, float projectileBlurRadius, float projectileBlurTime,
               const CDamageVulnerability& ingPossessedArmorVulnerability,
               const SBloggAttackCount& attackCount1, const SBloggAttackCount& attackCount2,
               const SBloggAttackCount& attackCount3)
: CPatterned(static_cast< EPatternedAI >(0x4c), uid, name, kFT_Zero, info, xf, mData, pInfo,
             kMT_Flyer, kCT_One, kBT_PitchableFlyer, actParms)
, x7c4_(0)
, x7c8_(0)
, x7cc_(0)
, x7d0_(0)
, x7d4_(0)
, x7d8_(0)
, x7dc_(0.f)
, x7e0_(0.f)
, x7e4_(0.f)
, x7e8_(0.f)
, x7ec_(xf.GetTranslation())
, x7f8_(xf.GetTranslation())
, mContactDamageInfo(pInfo.GetContactDamage())
, mCollisionActorManager(nullptr)
, mMinAttackAngle(minAttackAngle * (M_PIF / 180.f))
, mMaxAttackAngle(maxAttackAngle * (M_PIF / 180.f))
, mMinAttackRange(pInfo.GetMinAttackRange())
, mMaxAttackRange(pInfo.GetMaxAttackRange())
, x834_(mMinAttackAngle)
, x838_(mMinAttackRange)
, x83c_(CVector3f::Zero())
, x848_(kInvalidUniqueId)
, mPathFindSearch(nullptr, 4, 0, 1.f, 1.f, 0, CPFRegion::kRP_Center)
, x938_(CVector3f::Forward())
, mMinProjectileDelay(minProjectileDelay)
, mMaxProjectileDelay(maxProjectileDelay)
, mProjectileDelay(minProjectileDelay)
, x950_(unknown0xa19d5f62)
, x951_(false)
, mProjectileInfo(projectile, projectileDamage)
, x97c_(CVector3f::Zero())
, x988_(GetSpeed())
, x98c_(3.f)
, x994_(kInvalidUniqueId)
, x996_(kInvalidUniqueId)
, mAttackPositions(16, CVector3f::Zero())
, mBodyDamageMultiplier(bodyDamageMultiplier)
, mMouthDamageMultiplier(mouthDamageMultiplier)
, mArmorVulnerability(armorVulnerability)
, mIngPossessedArmorVulnerability(ingPossessedArmorVulnerability)
, xac4_(kInvalidUniqueId)
, mMouthDamageAngle(mouthDamageAngle * (M_PIF / 180.f))
, mChargeDamageRadius(chargeDamageRadius)
, mChargeDamage(chargeDamage)
, mChargeTurnSpeed(chargeTurnSpeed)
, mChargeSpeedMultiplier(chargeSpeedMultiplier)
, mBiteDamage(biteDamage)
, mBallSpitDamage(ballSpitDamage)
, mMaxMeleeRange(maxMeleeRange)
, mMaxBallDetectionRange(maxBallDetectionRange)
, mMaxPlayerPursuitTime(maxPlayerPursuitTime)
, mMaxBallPursuitTime(maxBallPursuitTime)
, xaf4_(0.f)
, xaf8_(0.f)
, mFishAttractionRadius(fishAttractionRadius)
, mFishAttractionPriority(fishAttractionPriority)
, mAggressiveness(aggressiveness)
, xb08_(unknown0x479ccc37)
, xb0c_(unknown0x689a803f)
, xb10_(unknown0x800a2b0d)
, xb1c_(0.f)
, mMaxCollisionTime(maxCollisionTime)
, xb24_(0.f)
, xb28_(0.f)
, xb2c_(10.f)
, mMouthOpenSound(mouthOpenSound)
, xb34_(pInfo.GetSpeed())
, xb48_(0.f)
, mMinDelayBetweenMeleeAttacks(minDelayBetweenMeleeAttacks)
, mMouthVulnerability(rs_new CBloggMouthVulnerability(armorVulnerability, armorVulnerability, true))
, mArmorNonUniformVulnerability(
      rs_new CBloggArmorVulnerability(armorVulnerability, armorVulnerability, true))
, mProjectileBlurRadius(projectileBlurRadius)
, mProjectileBlurTime(projectileBlurTime)
, xb68_(0.f)
, mLineOfSight(GetUniqueId(), CSegId(1), 0.3f, 0.f)
, xbb0_(0)
, xbb1_(false)
, xbb2_(false)
, xbc4_24_(false)
, xbc4_25_(false)
, xbc4_26_(false)
, xbc4_27_(false)
, xbc4_28_(false)
, xbc4_29_(false)
, xbc4_30_(false)
, mIsMegaBlogg(isMegaBlogg)
, xbc5_24_(false)
, xbc5_25_(false)
, xbc5_26_(false)
, xbc5_27_(true)
, xbc5_28_(true)
, xbc5_29_(false)
, xbc5_30_(false) {
  mProjectileInfo.Token().Lock();
  const CPASDatabase& pas = GetModelData()->GetAnimationData()->GetPASDatabase();

  rstl::pair< float, int > best = pas.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveAim, CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(0)),
      -1);
  if (best.first > FLT_EPSILON) {
    x7c4_ = best.second;
  }
  best = pas.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveAim, CPASAnimParm::FromEnum(1), CPASAnimParm::FromEnum(0)),
      -1);
  if (best.first > FLT_EPSILON) {
    x7c8_ = best.second;
  }
  best = pas.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveAim, CPASAnimParm::FromEnum(2), CPASAnimParm::FromEnum(0)),
      -1);
  if (best.first > FLT_EPSILON) {
    x7cc_ = best.second;
  }
  best = pas.FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveAim, CPASAnimParm::FromEnum(3), CPASAnimParm::FromEnum(0)),
      -1);
  if (best.first > FLT_EPSILON) {
    x7d0_ = best.second;
  }
  best = pas.FindBestAnimation(CPASAnimParmData(pas::kAS_Unknown26), -1);
  if (best.first > FLT_EPSILON) {
    x7d8_ = best.second;
  }

  const CPASAnimParmData forwardParms(pas::kAS_Locomotion, CPASAnimParm::FromEnum(2),
                                      CPASAnimParm::FromEnum(3));
  skForwardSpeed = GetAnimationDistance(forwardParms) / GetAnimationDuration(forwardParms);
  const CPASAnimParmData sideParms(pas::kAS_Locomotion, CPASAnimParm::FromEnum(2),
                                   CPASAnimParm::FromEnum(2));
  skSideSpeed = GetAnimationDistance(sideParms) / GetAnimationDuration(sideParms);

  SetDrawShadow(false);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(skIncludeMaterial),
      CMaterialList(skExcludeMaterial1, skExcludeMaterial2, skExcludeMaterial3,
                    skExcludeMaterial4)));
  BodyController()->BodyStateInfo().SetMaximumPitch(1.3962634f);

  rstl::ncrc_ptr< CBloggMouthVulnerability > mouth =
      rstl::rc_ptr< CBloggMouthVulnerability >(mMouthVulnerability);
  if (mouth) {
    mouth->SetOwner(this);
  }
  rstl::ncrc_ptr< CBloggArmorVulnerability > armor =
      rstl::rc_ptr< CBloggArmorVulnerability >(mArmorNonUniformVulnerability);
  if (armor) {
    armor->SetOwner(this);
  }

  KnockBackController().EnableAllAnimReactions(false);
  KnockBackController().EnableKnockBackPhysics(false);
  KnockBackController().EnableBurn(false);
  KnockBackController().EnableFreeze(false);
  KnockBackController().EnableSlow(false);

  if (mIsMegaBlogg) {
    mAttackCounts.reserve(3);
    mAttackCounts.push_back_unsafe(attackCount1);
    mAttackCounts.push_back_unsafe(attackCount2);
    mAttackCounts.push_back_unsafe(attackCount3);
  }
}

CBlogg::~CBlogg() {}

void CBlogg::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

CVector3f CBlogg::GetIngSnatchingNormal(float t) const { return -GetTransform().GetForward(); }

CEntity* REL_LoadBlogg(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrBlogg sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrBlogg.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CBlogg(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
      LdrToActorParameters(sldrThis.actorInformation), sldrThis.minAttackAngle,
      sldrThis.maxAttackAngle, sldrThis.minDelayBetweenProjectileAttacks,
      sldrThis.maxDelayBetweenProjectileAttacks, sldrThis.unknown_0xa19d5f62,
      sldrThis.projectileParticleEffect, LdrToDamageInfo(sldrThis.projectileDamage),
      sldrThis.bodyDamageMultiplier, sldrThis.mouthDamageMultiplier, sldrThis.mouthDamageAngle,
      LdrToDamageVulnerability(sldrThis.armorVulnerability), sldrThis.chargeDamageRadius,
      sldrThis.chargeDamage, sldrThis.biteDamage, sldrThis.ballSpitDamage,
      sldrThis.fishAttractionRadius, sldrThis.fishAttractionPriority, sldrThis.aggressiveness,
      sldrThis.unknown_0x479ccc37, sldrThis.unknown_0x689a803f, sldrThis.unknown_0x800a2b0d,
      sldrThis.chargeTurnSpeed, sldrThis.chargeSpeedMultiplier, sldrThis.maxMeleeRange,
      sldrThis.maxBallDetectionRange, sldrThis.maxPlayerPursuitTime, sldrThis.maxBallPursuitTime,
      sldrThis.mouthOpenSound, sldrThis.minDelayBetweenMeleeAttacks, sldrThis.maxCollisionTime,
      sldrThis.isMegaBlogg, sldrThis.projectileBlurRadius, sldrThis.projectileBlurTime,
      LdrToDamageVulnerability(sldrThis.ingPossessedArmorVulnerability),
      SBloggAttackCount(sldrThis.bloggStruct), SBloggAttackCount(sldrThis.bloggStruct_0x97dd1aa7),
      SBloggAttackCount(sldrThis.bloggStruct_0xf2ba21e1));
}

SBlogg_FuncPtrs REL_loader_Blogg;

void SetRelLoaderFunctionToLoader() {
  REL_loader_Blogg.mLoader = REL_LoadBlogg;
  SetSBlogg_FuncPtrs(&REL_loader_Blogg);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSBlogg_FuncPtrs(nullptr); }
