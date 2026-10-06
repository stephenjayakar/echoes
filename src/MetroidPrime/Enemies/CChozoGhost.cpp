#include "MetroidPrime/Enemies/CChozoGhost.hpp"

#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrChozoGhost.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "REL/REL_Setup.h"

#include <float.h>

const rstl::string CChozoGhost::skSpeedSwooshName = rstl::string_l("SpeedSwoosh");

CChozoGhost::CBehaveChance::CBehaveChance(float lurk, float taunt, float attack, float move,
                                          float lurkTime, float chargeAttack, uint numBolts)
: mLurk(lurk)
, mTaunt(taunt)
, mAttack(attack)
, mMove(move)
, mLurkTime(lurkTime)
, mChargeAttack(chargeAttack)
, mNumBolts(numBolts) {
  const float average = 1.f / (mLurk + mTaunt + mAttack + mMove);
  mLurk *= average;
  mTaunt *= average;
  mAttack *= average;
  mMove *= average;
}

CChozoGhost::EBehaveType CChozoGhost::CBehaveChance::GetBehave(const EBehaveType type,
                                                               CStateManager& mgr) const {
  float lurkChance = mLurk;
  float tauntChance = mTaunt;
  float attackChance = mAttack;
  switch (type) {
  case kBT_Lurk: {
    const float delta = lurkChance / 3.f;
    lurkChance = 0.f;
    tauntChance += delta;
    attackChance += delta;
  } break;
  case kBT_Taunt: {
    const float delta = tauntChance / 3.f;
    tauntChance = 0.f;
    lurkChance += delta;
    attackChance += delta;
  } break;
  case kBT_Attack: {
    const float delta = attackChance / 3.f;
    attackChance = 0.f;
    lurkChance += delta;
    tauntChance += delta;
  } break;
  case kBT_Move: {
    const float delta = mMove / 3.f;
    lurkChance += delta;
    tauntChance += delta;
    attackChance += delta;
  } break;
  default:
    break;
  }

  const float rnd = mgr.Random()->Float();
  EBehaveType ret = kBT_Move;
  if (rnd < lurkChance) {
    ret = kBT_Lurk;
  } else if (rnd - lurkChance < tauntChance) {
    ret = kBT_Taunt;
  } else if (rnd - lurkChance - tauntChance < attackChance) {
    ret = kBT_Attack;
  }
  return ret;
}

CChozoGhost::CChozoGhost(
    const TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CModelData& mData, const CActorParameters& actParms, const CPatternedInfo& pInfo,
    const float hearingRadius, const float fadeOutDelay, const float attackDelay,
    const float freezeTime, const CAssetId wpsc1, const CDamageInfo& dInfo1, const CAssetId wpsc2,
    const CDamageInfo& dInfo2, const CBehaveChance& chance1, const CBehaveChance& chance2,
    const CBehaveChance& chance3, const ushort soundImpact, const float f1, const ushort sfxFadeIn,
    const ushort sfxFadeOut, const uint w1, const float f2, const uint w2,
    const float hurlRecoverTime, const CAssetId projectileVisor, const ushort soundProjectileVisor,
    const float f3, const float f4, const uint nearChance, const uint midChance)
: CPatterned(static_cast< EPatternedAI >(4), uid, name, kFT_Zero, info, xf, mData, pInfo,
             kMT_Flyer, kCT_Zero, static_cast< EBodyType >(1), actParms)
, mHearingRadius(hearingRadius)
, mFadeOutDelay(fadeOutDelay)
, mAttackDelay(attackDelay)
, mFreezeTime(freezeTime)
, mProjectileInfo1(wpsc1, dInfo1)
, mProjectileInfo2(wpsc2, dInfo2)
, mBehaveChance1(chance1)
, mBehaveChance2(chance2)
, mBehaveChance3(chance3)
, mSoundImpact(soundImpact)
, x62c_(f1)
, mSfxFadeIn(sfxFadeIn)
, mSfxFadeOut(sfxFadeOut)
, x634_(f2)
, mHurlRecoverTime(hurlRecoverTime)
, x63c_(w2)
, mSoundProjectileVisor(soundProjectileVisor)
, x654_(f3)
, x658_(f4)
, mNearChance(nearChance)
, mMidChance(midChance)
, mBehaviorEnabled(w1 & 1)
, mFlinch((w1 >> 1) & 1)
, mAlert(false)
, mOnGround(false)
, x664_28_(false)
, mFadedIn(false)
, mFadedOut(false)
, x664_31_(false)
, x665_24_(true)
, x665_25_(false)
, mShouldSwoosh(false)
, mInRange(false)
, mAggressive(false)
, x668_(0.f)
, x66c_(0.f)
, x670_(0.f)
, mCoverPoint(kInvalidUniqueId)
, mFloorLevel(0.f)
, mAttackType(-1)
, mBehaveType(mBehaviorEnabled ? kBT_Attack : kBT_None)
, mLurkDelay(1.f)
, mBoneTracking(*GetAnimationData(), rstl::string_l("Head_1"), 80.f * M_PIF / 180.f,
                CRelAngle::FromDegrees(180.f).AsRadians(), 0)
, mTeamMgr(kInvalidUniqueId)
, mSpaceWarpTime(0.f)
, mSpaceWarpPosition(CVector3f::Zero())
, x6d8_(1) {
  mProjectileInfo1.Token().Lock();
  mProjectileInfo2.Token().Lock();

  const CPASAnimParmData jumpAnimParms(static_cast< pas::EAnimationState >(13),
                                       CPASAnimParm::FromEnum(3), CPASAnimParm::FromEnum(0));
  x668_ = GetModelData()->GetScale().GetZ() * GetAnimationDistance(jumpAnimParms);
  const CPASAnimParmData slideAnimParms(static_cast< pas::EAnimationState >(15),
                                        CPASAnimParm::FromEnum(1),
                                        CPASAnimParm::FromReal32(90.f));
  x66c_ = GetModelData()->GetScale().GetY() * GetAnimationDistance(slideAnimParms);
  const CPASAnimParmData meleeAnimParms(static_cast< pas::EAnimationState >(7),
                                        CPASAnimParm::FromEnum(2), CPASAnimParm::FromEnum(1));
  x670_ = GetModelData()->GetScale().GetZ() * GetAnimationDistance(meleeAnimParms);

  if (projectileVisor != kInvalidAssetId) {
    mProjectileVisor = gpSimplePool->GetObj(SObjectTag('PART', projectileVisor));
  }

  KnockBackController().EnableBurn(false);
  KnockBackController().EnableLaggedBurnDeath(false);
  KnockBackController().EnableShock(false);
  KnockBackController().EnableFreeze(false);
  SetDrawShadow(false);
}

void CChozoGhost::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_AreaLoaded:
    if (GetActive()) {
      AddToTeam(mgr);
    }
    break;
  case kSM_Activate:
    AddToTeam(mgr);
    break;
  case kSM_Alert:
    if (!mAlert) {
      mAlert = true;
      mHitByPlayerProjectile = true;
    }
    break;
  case kSM_Action:
    if (mFlinch) {
      mAggressive = true;
    }
    break;
  case kSM_Deactivate:
  case kSM_Delete:
    RemoveFromTeam(mgr);
    break;
  case kSM_Falling:
  case kSM_Launching:
    if (!mVerticalMovement) {
      SetMomentumWR(CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass()));
    }
    break;
  default:
    break;
  }
}

static EMaterialTypes SolidMaterial = kMT_Unknown59;

void CChozoGhost::Touch(CActor& act, CStateManager& mgr) {
  if (IsVisibleEnough(mgr)) {
    if (CPlayer* player = TCastToPtr< CPlayer >(act)) {
      if (mCurDamageRemTime <= 0.f) {
        mgr.ApplyDamage(
            GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
            CMaterialFilter::MakeIncludeExclude(CMaterialList(SolidMaterial), CMaterialList()),
            CVector3f::Zero());
        mCurDamageRemTime = mDamageWaitTime;
      }
    }
  }
  CPatterned::Touch(act, mgr);
}

bool CChozoGhost::CanBeShot(const CStateManager& mgr, int w1) { return IsVisibleEnough(mgr); }

EWeaponCollisionResponseTypes CChozoGhost::GetCollisionResponseType(const CVector3f&,
                                                                    const CVector3f&,
                                                                    const CWeaponMode&, int) const {
  return kWCR_ChozoGhost;
}

const CDamageVulnerability* CChozoGhost::GetDamageVulnerability() const {
  if (x665_24_) {
    return &CDamageVulnerability::PassThroughVulnerabilty();
  }

  return CPatterned::GetDamageVulnerability();
}

uchar CChozoGhost::GetModelAlphau8(const CStateManager& mgr) const {
  uchar ret = 255;
  if (!GetAlive()) {
    ret = mColor.GetAlphau8();
  }

  return ret;
}

void CChozoGhost::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
}

void CChozoGhost::Render(const CStateManager& mgr) const {
  if (mSpaceWarpTime > 0.f) {
    mgr.DrawSpaceWarp(mSpaceWarpPosition,
                      CMath::FastSinR(M_PIF * mSpaceWarpTime / mFadeOutDelay));
  }
  CPatterned::Render(mgr);
}

bool CChozoGhost::IsVisibleEnough(const CStateManager& mgr) const {
  return GetModelAlphau8(mgr) > 31;
}

CVector3f CChozoGhost::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                 const CVector3f& aimPos) const {
  return GetTranslation();
}

void CChozoGhost::AddToTeam(CStateManager& mgr) {
  if (mTeamMgr == kInvalidUniqueId) {
    mTeamMgr = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }

  if (mTeamMgr == kInvalidUniqueId) {
    return;
  }

  if (CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamMgr))) {
    teamMgr->JoinTeam(*this, CTeamAiRole::ETeamAiRole(2), CTeamAiRole::ETeamAiRole(3),
                      CTeamAiRole::ETeamAiRole(-1));
  }
}

void CChozoGhost::RemoveFromTeam(CStateManager& mgr) {
  if (mTeamMgr == kInvalidUniqueId) {
    return;
  }

  CScriptTeamAiMgr* teamMgr = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamMgr));
  if (teamMgr && teamMgr->IsPartOfTeam(GetUniqueId())) {
    teamMgr->QuitTeam(GetUniqueId());
    mTeamMgr = kInvalidUniqueId;
  }
}

void CChozoGhost::FloatToLevel(const float f1, const float dt) {
  CVector3f translation = GetTranslation();
  const float floatAmt = ((f1 - translation.GetZ()) * 4.f);
  translation.SetZ(translation.GetZ() + floatAmt * dt);
  SetTranslation(translation);
}

bool CChozoGhost::IsOnGround() const { return mOnGround; }

void CChozoGhost::FindBestAnchor(CStateManager& mgr) {}

const CChozoGhost::CBehaveChance& CChozoGhost::ChooseBehaveChanceRange(CStateManager& mgr) const {
  const float dist = (GetTranslation() - mgr.GetPlayer(0)->GetTranslation()).Magnitude();
  if (dist < x654_) {
    return mBehaveChance1;
  }
  if (dist < x658_) {
    return mBehaveChance2;
  }

  return mBehaveChance3;
}

void CChozoGhost::SetWarpPosition(CStateManager& mgr, const CVector3f& dir) {}

void CChozoGhost::InActive(CStateManager& mgr, EStateMsg msg, float arg) {}

bool CChozoGhost::AIStage(CStateManager& mgr, const CTriggerData& data) const {
  return static_cast< int >(data.GetFloat()) == x63c_;
}

void CChozoGhost::Growth(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Generate(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::WallDetach(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Run(CStateManager& mgr, EStateMsg msg, float arg) {}

bool CChozoGhost::InRange(CStateManager& mgr, const CTriggerData& data) const { return mInRange; }

void CChozoGhost::SelectTarget(CStateManager& mgr, EStateMsg msg, float arg) {
  switch (msg) {
  case kStateMsg_Activate: {
    FindBestAnchor(mgr);
    break;
  }
  }
}

bool CChozoGhost::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mBehaveType == kBT_Attack;
}

void CChozoGhost::Attack(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Land(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Shuffle(CStateManager& mgr, EStateMsg msg, float arg) {}

bool CChozoGhost::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  return mBehaveType == kBT_Taunt;
}

void CChozoGhost::Taunt(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Hurled(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Lurk(CStateManager& mgr, EStateMsg msg, float arg) {
  switch (msg) {
  case kStateMsg_Activate:
    mStateMachine->SetDelay(mLurkDelay);
    break;
  case kStateMsg_Update:
    FloatToLevel(mFloorLevel, arg);
    break;
  }
}

bool CChozoGhost::ShouldMove(CStateManager& mgr, const CTriggerData& data) const {
  return mBehaveType == kBT_Move;
}

bool CChozoGhost::Leash(CStateManager& mgr, const CTriggerData& data) const {
  return mPlayerInLeashRange || CPatterned::Leash(mgr, data);
}

bool CChozoGhost::ShouldFlinch(CStateManager& mgr, const CTriggerData& data) const {
  return mFlinch;
}

bool CChozoGhost::AggressionCheck(CStateManager& mgr, const CTriggerData& data) const {
  return mAggressive;
}

void CChozoGhost::Deactivate(CStateManager& mgr, EStateMsg msg, float arg) {}

void CChozoGhost::Dead(CStateManager& mgr, EStateMsg msg, float arg) {
  switch (msg) {
  case kStateMsg_Activate:
    ReleaseCoverPoint(mgr, mCoverPoint, true);
    mAlphaDelta = 4.f;
    mFadedOut = false;
    mFadedIn = false;
    mBoneTracking.SetActive(false);
    Stop();
    break;
  case kStateMsg_Update:
    Stop();
    SetMomentumWR(CVector3f::Zero());
    break;
  }
}

CProjectileInfo* CChozoGhost::ProjectileInfo() {
  if (mAttackType == 2) {
    return &mProjectileInfo1;
  }
  return &mProjectileInfo2;
}

void CChozoGhost::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                  EUserEventType type, float dt) {
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

void CChozoGhost::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  CPatterned::KnockBack(mgr, info);
}

void CChozoGhost::PreThink(float dt, CStateManager& mgr) {
  mBoneTracking.PreThink(*AnimationData());
  CPatterned::PreThink(dt, mgr);
}

void CChozoGhost::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  CPatterned::Think(dt, mgr);
  mBoneTracking.Think(dt);
  mSpaceWarpTime = CMath::Max(0.f, mSpaceWarpTime - dt);
  SetValidTarget(0, IsVisibleEnough(mgr));
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::ShouldAttack)},
    {"InRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::InRange)},
    {"ShouldTaunt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::ShouldTaunt)},
    {"ShouldMove", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::ShouldMove)},
    {"AIStage", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::AIStage)},
    {"Leash", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::Leash)},
    {"ShouldFlinch", static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::ShouldFlinch)},
    {"AggressionCheck",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CChozoGhost::AggressionCheck)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Start", &CPatterned::Start},
    {"InActive", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::InActive)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Attack)},
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Generate)},
    {"Run", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Run)},
    {"SelectTarget", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::SelectTarget)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Dead)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Deactivate)},
    {"Shuffle", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Shuffle)},
    {"Taunt", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Taunt)},
    {"Lurk", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Lurk)},
    {"Hurled", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Hurled)},
    {"Growth", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Growth)},
    {"WallDetach", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::WallDetach)},
    {"Land", static_cast< CPatterned::StateMachine::StateFunc >(&CChozoGhost::Land)},
};

void CChozoGhost::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

CEntity* LoadChozoGhost(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrChozoGhost sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrChozoGhost.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CChozoGhost::CBehaveChance nearChance(
      sldrThis.near.lurk, sldrThis.near.heckle, sldrThis.near.attack, sldrThis.near.move,
      sldrThis.near.lurkTime, sldrThis.near.chargeAttack, sldrThis.near.numBolts);
  const CChozoGhost::CBehaveChance midChance(
      sldrThis.mid.lurk, sldrThis.mid.heckle, sldrThis.mid.attack, sldrThis.mid.move,
      sldrThis.mid.lurkTime, sldrThis.mid.chargeAttack, sldrThis.mid.numBolts);
  const CChozoGhost::CBehaveChance farChance(
      sldrThis.far.lurk, sldrThis.far.heckle, sldrThis.far.attack, sldrThis.far.move,
      sldrThis.far.lurkTime, sldrThis.far.chargeAttack, sldrThis.far.numBolts);

  return rs_new CChozoGhost(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.hearingRadius,
      sldrThis.fadeOutDelay, sldrThis.attackDelay, sldrThis.freezeTime,
      sldrThis.unknown_0x54151870, LdrToDamageInfo(sldrThis.damageInfo),
      sldrThis.unknown_0x3a58089c, LdrToDamageInfo(sldrThis.damageInfo_0x1ff047a9), nearChance,
      midChance, farChance, sldrThis.sound_Impact, sldrThis.disablePlayerGunTime,
      sldrThis.sound_PhazeIn, sldrThis.sound_PhazeOut, sldrThis.unknown_0xec76940c,
      sldrThis.projectileStopHomingRange, sldrThis.unknown_0xfe9eac26, sldrThis.hurlRecoverTime,
      sldrThis.projectileVisorEffect, sldrThis.sound_ProjectileVisor,
      sldrThis.nearToMidDistance, sldrThis.midToFarDistance, sldrThis.nearChance,
      sldrThis.midChance);
}

static void SetFuncPtrs() {
  static SChozoGhost_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadChozoGhost;
  SetSChozoGhost_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSChozoGhost_FuncPtrs(nullptr); }
