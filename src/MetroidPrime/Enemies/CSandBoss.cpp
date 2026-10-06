#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Factories/CScannableObjectInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

// Spine joints, head first; the last entry is the skeleton root.
static const char* const skSpineJoints[] = {
    "Head_1", "Spine_6", "Spine_5", "Spine_4", "Spine_3", "Spine_2", "Spine_1", "Skeleton_Root",
};
static const char* const skRootJoint = "Skeleton_Root";
static const char* const skHeadJoint = "Head_1";

CSandBoss::CSandBoss(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& mData,
                     const CPatternedInfo& pInfo, const CActorParameters& aParms,
                     const SLdrSandBossData& data)
: CPatterned(kPAI_SandBoss, uid, name, kFT_Zero, info, xf, mData, pInfo, kMT_Ground, kCT_One,
             kBT_BiPedal, aParms)
, mData(data)
, mCollisionActorManager(nullptr)
, mBoneTracking(*GetModelData()->GetAnimationData(), skHeadJoint, 1.0471976f, 1.5707964f, 1)
, xde4_(0)
, xde8_(pInfo.GetTurnSpeed())
, mScanInfo(nullptr)
, xdf0_(0)
, xdf4_(0)
, xdf8_(0)
, xdfc_(-1)
, mDarkBeamInfo(data.darkBeamProjectile, LdrToDamageInfo(data.darkBeamDamage))
, mChargeBeamInfo(data.unknown_0x7619e561.chargeBeamInfo.weaponSystem, CDamageInfo())
, mHeadArmorExplosion(data.headArmorExplosion != kInvalidAssetId
                          ? rstl::optional_object< TLockedToken< CGenDescription > >(
                                gpSimplePool->GetObj(SObjectTag('PART', data.headArmorExplosion)))
                          : rstl::optional_object_null())
, mStampedeArmorExplosion(
      data.stampedeProperties.stampedeArmorExplosion != kInvalidAssetId
          ? rstl::optional_object< TLockedToken< CGenDescription > >(gpSimplePool->GetObj(
                SObjectTag('PART', data.stampedeProperties.stampedeArmorExplosion)))
          : rstl::optional_object_null())
, mStampedeSandFountain(
      data.stampedeProperties.stampedeSandFountainFx != kInvalidAssetId
          ? rstl::optional_object< TLockedToken< CGenDescription > >(gpSimplePool->GetObj(
                SObjectTag('PART', data.stampedeProperties.stampedeSandFountainFx)))
          : rstl::optional_object_null())
, xe80_(kInvalidUniqueId)
, xe82_(kInvalidUniqueId)
, xe84_(kInvalidUniqueId)
, xe86_(kInvalidUniqueId)
, xe88_(kInvalidUniqueId)
, xe8a_(kInvalidUniqueId)
, xe8c_(kInvalidUniqueId)
, xe8e_(kInvalidUniqueId)
, xe90_(kInvalidUniqueId)
, xef8_(kInvalidUniqueId)
, mSpineSegIds(CSegId())
, xf0c_(CColor::Black())
, xf10_(0.f)
, xf14_(0.f)
, xf18_(data.unknown_0x7619e561.doubleCharge.minChargeBeamAttackTime)
, xf1c_(data.minDarkBeamAttackTime)
, xf20_(0.f)
, xf24_(data.stampedeProperties.breakStampedeHP)
, xf28_(0.f)
, xf2c_(data.headArmorHP)
, xf30_(0.f)
, xf34_(0.f)
, xf38_(0.f)
, xf3c_(1)
, xf40_(0.f)
, xf44_(0)
, xf48_(0)
, mAttachedArmorModels(rstl::optional_object< CModelData >())
, mStampedeArmorModels(rstl::optional_object< CModelData >())
, mArmorStates(kArmor_Attached)
, mNormalSkinnedModel(GetModelData()->GetAnimationData()->GetModelData())
, mTailArmorSkinnedModel(rs_new CSkinnedModel(
      TLockedToken< CModel >(gpSimplePool->GetObj(SObjectTag('CMDL', data.withTailArmorModel))),
      TLockedToken< CSkinRules >(
          gpSimplePool->GetObj(SObjectTag('CSKR', data.withTailArmorSkinRules))),
      GetModelData()->GetAnimationData()->GetModelData()->GetLayoutInfo()))
, x14a0_(CTransform4f::Identity())
, x14d0_(CQuaternion::FromMatrix(GetTransform().GetRotation()))
, mSnapJawDamage(LdrToDamageInfo(data.snapJawDamage))
, mSpitOutDamage(LdrToDamageInfo(data.spitOutDamage))
, mStampedeDamage(LdrToDamageInfo(data.stampedeProperties.stampedeDamage))
, mDoubleChargeDamage(LdrToDamageInfo(data.unknown_0x7619e561.doubleCharge.damage))
, mTripleChargeDamage(LdrToDamageInfo(data.unknown_0x7619e561.tripleCharge.damage))
, mDamageVulnerability(LdrToDamageVulnerability(data.damageVulnerability))
, mStampedeVulnerability(LdrToDamageVulnerability(data.stampedeVulnerability))
, mSuckAirVulnerability(LdrToDamageVulnerability(data.suckAirVulnerability))
, x15fc_(CDamageVulnerability::ReflectVulnerabilty())
, x162c_(CDamageVulnerability::ReflectVulnerabilty())
, x165c_24_(false)
, x165c_25_(false)
, x165c_26_(false)
, x165c_27_(false)
, x165c_28_(false)
, x165c_29_(false)
, x165c_30_(false)
, x165c_31_(false)
, x165d_24_(false)
, x165d_25_(false)
, x165d_26_(false)
, x165d_27_(false)
, x165d_28_(false)
, x165d_29_(false)
, x165d_30_(true)
, x165d_31_(false)
, x165e_24_(false)
, x165e_25_(false)
, x165e_26_(false)
, x165e_27_(false)
, x165e_28_(false) {
  mDarkBeamInfo.Token().Lock();
  x1478_.reserve(16);
  mKnockBackController.EnableKnockBackPhysics(false);
  mKnockBackController.EnableAllAnimReactions(false);

  const CAnimData* animData = GetModelData()->GetAnimationData();
  for (int i = 0; i < 8; ++i) {
    mSpineSegIds[i] = animData->GetLocatorSegId(skSpineJoints[i]);
  }
  mHeadSegId = animData->GetLocatorSegId(skHeadJoint);

  SetupArmorModels();
  SetDrawShadow(false);

  if (data.scannableInfo1 != kInvalidAssetId) {
    mScanInfo = rs_new TLockedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', data.scannableInfo1)));
  }
}

CSandBoss::~CSandBoss() {}

void CSandBoss::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  const bool wasActive = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_Create:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    SetupCollisionActors(mgr);
    break;
  case kSM_Delete:
    mCollisionActorManager->Destroy(mgr);
    break;
  case kSM_Activate:
    if (!wasActive) {
      if (mCollisionActorManager.get() != nullptr) {
        mCollisionActorManager->SetActive(mgr, true);
      }
      if (xde4_ == 0 && mData.commandIndex == 0) {
        if (CActor* ent = static_cast< CActor* >(mgr.ObjectById(xe86_))) {
          mgr.SetBossParams(ent->GetUniqueId(), ent->GetHealthInfo()->GetHP(),
                            gpStringTable->GetStringIndex("BossSandBoss"));
        }
      }
    }
    break;
  case kSM_Deactivate:
    if (wasActive) {
      if (mCollisionActorManager.get() != nullptr) {
        mCollisionActorManager->SetActive(mgr, false);
      }
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_AreaLoaded:
    FindOtherBosses(mgr);
    break;
  case kSM_XHIT:
    OnCollisionActorHit(mgr, msg.GetSenderId());
    break;
  case kSM_Damage:
    OnCollisionActorDamage(mgr, msg.GetSenderId());
    break;
  case kSM_Increment:
    if (msg.GetSenderId() == GetUniqueId() || xdf0_ == 0) {
      ++xde4_;
    }
    break;
  default:
    break;
  }
}

void CSandBoss::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  CPatterned::Think(dt, mgr);
  mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  mBoneTracking.Think(dt);
  UpdateArmorColor(dt, mgr);
  UpdateStampede(mgr, dt);
  UpdateCinematicState(mgr);
}

void CSandBoss::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  mBoneTracking.PreRender(mgr, *AnimationData(), GetTransform(), GetModelData()->GetScale(),
                          *BodyController());
}

void CSandBoss::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  if (!x165c_24_) {
    CPatterned::Render(mgr);
    RenderArmor(mgr, GetTransform(), GetModelFlags(),
                CModelFlags(CModelFlags::kT_Two, 0,
                            CModelFlags::EFlags(CModelFlags::kF_DepthCompare |
                                                CModelFlags::kF_DepthUpdate),
                            xf0c_));
  }
}

void CSandBoss::Render(const CStateManager& mgr) const {
  if (!x165d_26_) {
    uint mask = 0;
    uint target = 0;
    if (mDrawParticles) {
      mgr.GetCharacterRenderMaskAndTarget(mask, target);
    }
    CPatterned::RenderSystemsToBeDrawnLast(mgr, mask, target);
  }
}

void CSandBoss::RenderSystemsToBeDrawnLast(const CStateManager& mgr, uint mask,
                                           uint target) const {}

void CSandBoss::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);
  const rstl::optional_object< CAABox > bounds =
      GetModelData()->GetAnimationData()->GetParticleDB().GetTotalBounds();
  if (bounds) {
    CAABox box = GetOtherBounds();
    box.AccumulateBounds(bounds->GetMinPoint());
    box.AccumulateBounds(bounds->GetMaxPoint());
    SetOtherBounds(box);
    SetRenderBounds(box);
  }
}

CAABox CSandBoss::GetSortingBounds(const CStateManager& mgr) const {
  const CAABox bounds = GetModelData()->GetBounds(GetTransform());
  const CVector3f center = bounds.GetCenterPoint();
  const CVector3f extents = 0.25f * (bounds.GetMaxPoint() - bounds.GetMinPoint());
  return CAABox(center - extents, center + extents);
}

const CDamageVulnerability* CSandBoss::GetDamageVulnerability() const {
  return &CDamageVulnerability::PassThroughVulnerabilty();
}

CVector3f CSandBoss::GetAimPosition(const CStateManager& mgr, float dt) const {
  CVector3f predicted = CVector3f::Zero();
  if (dt > 0.f) {
    predicted = PredictMotion(dt).GetTranslation();
  }
  const CTransform4f xf = GetLctrTransform(mHeadSegId);
  float z = predicted.GetZ() + xf.GetTranslation().GetZ();
  if (x165c_27_) {
    const float minZ = GetTranslation().GetZ();
    if (!(minZ < z)) {
      z = minZ;
    }
  }
  return CVector3f(predicted.GetX() + xf.GetTranslation().GetX(),
                   predicted.GetY() + xf.GetTranslation().GetY(), z);
}

CProjectileInfo* CSandBoss::ProjectileInfo() { return &mDarkBeamInfo; }

CScannableObjectInfo* CSandBoss::GetScannableObjectInfo() const {
  if (!x165c_27_ && mScanInfo.get() != nullptr) {
    return **mScanInfo;
  }
  return CPatterned::GetScannableObjectInfo();
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::StateOver)},
    {"IsUnderGround", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::IsUnderGround)},
    {"IsRecovered", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::IsRecovered)},
    {"CanStartNewRound", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::CanStartNewRound)},
    {"BeginStampede", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::BeginStampede)},
    {"FoundStampedePoint", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::FoundStampedePoint)},
    {"StampedeOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::StampedeOver)},
    {"ShouldDarkBeamAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldDarkBeamAttack)},
    {"ShouldRepeaterAttack", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldRepeaterAttack)},
    {"FoundSafeZoneTarget", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::FoundSafeZoneTarget)},
    {"ShouldTripleCharge", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldTripleCharge)},
    {"ShouldDoubleCharge", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldDoubleCharge)},
    {"ShouldSnapJaws", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldSnapJaws)},
    {"ShouldDestroySphere", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ShouldDestroySphere)},
    {"IsFacingPlayer", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::IsFacingPlayer)},
    {"IsBallInSuckRange", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::IsBallInSuckRange)},
    {"IsLastStage", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::IsLastStage)},
    {"HeadArmorDestroyed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::HeadArmorDestroyed)},
    {"ArmorDestroyed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::ArmorDestroyed)},
    {"SyncStampede", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::SyncStampede)},
    {"SyncAttachToSphere", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSandBoss::SyncAttachToSphere)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Recover", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::Recover)},
    {"UnderGround", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::UnderGround)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::Dead)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::Deactivate)},
    {"AttachToSphere", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::AttachToSphere)},
    {"RotateToPlayer", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::RotateToPlayer)},
    {"SuckAir", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SuckAir)},
    {"SuckBall", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SuckBall)},
    {"BallInMouth", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::BallInMouth)},
    {"DestroySphere", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::DestroySphere)},
    {"JumpOffSphere", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::JumpOffSphere)},
    {"SpitOutBall", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SpitOutBall)},
    {"DarkBeamAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::DarkBeamAttack)},
    {"RepeaterAttack", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::RepeaterAttack)},
    {"DoubleCharge", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::DoubleCharge)},
    {"TripleCharge", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::TripleCharge)},
    {"SnapJaws", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SnapJaws)},
    {"SelectStampedePoint", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SelectStampedePoint)},
    {"SeekStampedePoint", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::SeekStampedePoint)},
    {"Stampede", static_cast< CPatterned::StateMachine::StateFunc >(&CSandBoss::Stampede)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"SetTripleChargeArmor", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetTripleChargeArmor)},
    {"SetDoubleChargeArmor", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetDoubleChargeArmor)},
    {"SetStampedeArmor", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetStampedeArmor)},
    {"SetAttachedArmor", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetAttachedArmor)},
    {"SetSuckAirArmor", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetSuckAirArmor)},
    {"RemovePlayerCollision", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::RemovePlayerCollision)},
    {"RestorePlayerCollision", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::RestorePlayerCollision)},
    {"SelectSafeZoneTarget", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SelectSafeZoneTarget)},
    {"WaitStampede", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::WaitStampede)},
    {"WaitAttachToSphere", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::WaitAttachToSphere)},
    {"LockSphere", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::LockSphere)},
    {"UnLockSphere", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::UnLockSphere)},
    {"AllowSyncAttacks", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::AllowSyncAttacks)},
    {"PreventSyncAttacks", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::PreventSyncAttacks)},
    {"Deactivate", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::Deactivate)},
    {"EndStampede", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::EndStampede)},
    {"PlayHeadArmorExplosion", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::PlayHeadArmorExplosion)},
    {"ResetHeadArmorHP", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::ResetHeadArmorHP)},
    {"ResetAttackTimes", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::ResetAttackTimes)},
    {"SetCinematicStateUnderGround", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateUnderGround)},
    {"SetCinematicStateStampede", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateStampede)},
    {"SetCinematicStateNormal", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateNormal)},
    {"SetCinematicStateStunned", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateStunned)},
    {"SetCinematicStateChoking", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateChoking)},
    {"SetCinematicStateExitSphere", static_cast< CPatterned::StateMachine::CodeFunc >(&CSandBoss::SetCinematicStateExitSphere)},
};

void CSandBoss::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

bool CSandBoss::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.IsOver();
}

bool CSandBoss::IsUnderGround(CStateManager& mgr, const CTriggerData& data) const {
  return x165c_24_;
}

bool CSandBoss::IsRecovered(CStateManager& mgr, const CTriggerData& data) const {
  return x165d_26_ == false;
}

bool CSandBoss::CanStartNewRound(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (xde4_ >= GetStage() && xde4_ < 3) {
    ret = true;
  }
  return ret;
}

bool CSandBoss::BeginStampede(CStateManager& mgr, const CTriggerData& data) const {
  return x165c_25_;
}

bool CSandBoss::FoundStampedePoint(CStateManager& mgr, const CTriggerData& data) const {
  return GetCoverPoint(mgr, xe8e_) != nullptr;
}

bool CSandBoss::StampedeOver(CStateManager& mgr, const CTriggerData& data) const {
  return xf28_ <= 0.f;
}

bool CSandBoss::FoundSafeZoneTarget(CStateManager& mgr, const CTriggerData& data) const {
  return xe88_ != kInvalidUniqueId;
}

bool CSandBoss::IsLastStage(CStateManager& mgr, const CTriggerData& data) const {
  return xde4_ == 2;
}

bool CSandBoss::HeadArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return xf2c_ <= 0.f;
}

bool CSandBoss::ArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return x165d_25_;
}

bool CSandBoss::SyncStampede(CStateManager& mgr, const CTriggerData& data) const {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->xdf0_ != 1 && other->xdf0_ != 0) {
      return false;
    }
  }
  return true;
}

bool CSandBoss::SyncAttachToSphere(CStateManager& mgr, const CTriggerData& data) const {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->xdf0_ != 2 && other->xdf0_ != 0) {
      return false;
    }
  }
  return true;
}

void CSandBoss::WaitStampede(CStateManager& mgr, float dt) { xdf0_ = 1; }

void CSandBoss::WaitAttachToSphere(CStateManager& mgr, float dt) { xdf0_ = 2; }

void CSandBoss::Deactivate(CStateManager& mgr, float dt) { x165c_27_ = true; }

void CSandBoss::EndStampede(CStateManager& mgr, float dt) { x165c_27_ = false; }

void CSandBoss::ResetHeadArmorHP(CStateManager& mgr, float dt) { xf2c_ = mData.headArmorHP; }

void CSandBoss::SetCinematicStateExitSphere(CStateManager& mgr, float dt) { xdf4_ = 6; }

void CSandBoss::SetCinematicStateChoking(CStateManager& mgr, float dt) { xdf4_ = 5; }

int CSandBoss::GetStage() const { return mData.commandIndex; }

static SSandBoss_FuncPtrs REL_loader_SandBoss;

CEntity* REL_LoadSandBoss(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSandBoss sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSandBoss.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CSandBoss(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                          LdrToEntityInfo(info, sldrThis.editorProperties),
                          LdrToTransform4f(sldrThis.editorProperties), *modelData,
                          LdrToPatternedInfo(sldrThis.patterned, nullptr),
                          LdrToActorParameters(sldrThis.actorInformation),
                          sldrThis.sandBossProperties);
}

void SetRelLoaderFunctionToLoader() {
  REL_loader_SandBoss.mLoader = REL_LoadSandBoss;
  SetSSandBoss_FuncPtrs(&REL_loader_SandBoss);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSSandBoss_FuncPtrs(nullptr); }
