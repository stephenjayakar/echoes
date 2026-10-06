#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Factories/CScannableObjectInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

#include <float.h>

// Spine joints, head first; the last entry is the skeleton root.
static const char* const skSpineJoints[] = {
    "Head_1", "Spine_6", "Spine_5", "Spine_4", "Spine_3", "Spine_2", "Spine_1", "Skeleton_Root",
};
static const char* const skRootJoint = "Skeleton_Root";
static const char* const skHeadJoint = "Head_1";
static CVector3f skJawsTouchBounds(4.5f, 4.5f, 1.5f); // Guessed name.

CSandBoss::CSandBoss(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& mData,
                     const CPatternedInfo& pInfo, const CActorParameters& aParms,
                     const SLdrSandBossData& data)
: CPatterned(kPAI_SandBoss, uid, name, kFT_Zero, info, xf, mData, pInfo, kMT_Ground, kCT_One,
             kBT_BiPedal, aParms)
, mData(data)
, mCollisionActorManager(nullptr)
, mBoneTracking(*GetModelData()->GetAnimationData(), skHeadJoint, 1.0471976f, 1.5707964f, 1)
, mRound(0)
, xde8_(pInfo.GetTurnSpeed())
, mScanInfo(nullptr)
, mSyncState(0)
, mCinematicState(0)
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
, mArmorSegIds(CSegId())
, xf0c_(CColor::Black())
, xf10_(0.f)
, xf14_(0.f)
, mChargeBeamTimer(data.unknown_0x7619e561.doubleCharge.minChargeBeamAttackTime)
, mDarkBeamTimer(data.minDarkBeamAttackTime)
, mStampedeTimer(0.f)
, mStampedeHP(data.stampedeProperties.breakStampedeHP)
, xf28_(0.f)
, mHeadArmorHP(data.headArmorHP)
, xf30_(0.f)
, xf34_(0.f)
, mBeamAngle(0.f)
, mBeamTurnDirection(1)
, mBeamTurnTimer(0.f)
, mAttackOrder(0)
, mRepeaterShots(0)
, mAttachedArmorModels(rstl::optional_object< CModelData >())
, mStampedeArmorModels(rstl::optional_object< CModelData >())
, mArmorStates(kArmor_None)
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
    mArmorSegIds[i] = animData->GetLocatorSegId(skSpineJoints[i]);
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
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
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
      if (mRound == 0 && mData.commandIndex == 0) {
        if (CActor* ent = static_cast< CActor* >(mgr.ObjectById(xe86_))) {
          mgr.SetBossParams(ent->GetUniqueId(), ent->GetHealthInfo()->GetInitialHP(),
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
    if (msg.GetSenderId() == GetUniqueId() || mSyncState == 0) {
      ++mRound;
    }
    break;
  default:
    break;
  }
}

void CSandBoss::PreThink(float dt, CStateManager& mgr) {
  if (IsLeader(mgr)) {
    if (CActor* sphere = static_cast< CActor* >(mgr.ObjectById(xe8a_))) {
      CTransform4f xf = GetLctrTransform(CSegId(1));
      const CVector3f center = sphere->GetModelData()->GetBounds().GetCenterPoint();
      xf.AddTranslation(xf.Rotate(-center));
      sphere->SetTransform(xf);
    }
    const CQuaternion rot = GetRotation() * x14d0_.BuildInverted();
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
      if (other != nullptr && other != this && other->x165d_24_) {
        other->SetTransform((other->x14d0_ * rot).BuildTransform4f(other->GetTranslation()));
      }
    }
  }
  mBoneTracking.PreThink(*AnimationData());
  CPatterned::PreThink(dt, mgr);
}

void CSandBoss::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                EUserEventType type, float dt) {
  bool handled = false;
  switch (type) {
  case kUE_Activate:
    SetArmorVisible(node.GetLocatorName(), true);
    UpdateCollisionActorResponses(mgr);
    break;
  case kUE_Deactivate:
    SetArmorVisible(node.GetLocatorName(), false);
    UpdateCollisionActorResponses(mgr);
    break;
  case kUE_ObjectDrop:
    ReleasePlayer(mgr);
    handled = true;
    break;
  case kUE_ObjectPickUp:
    if (x165d_28_) {
      if (CScriptWaypoint* wp = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(xe84_))) {
        const CTransform4f lctrXf = GetLctrTransform(node.GetLocatorName());
        CTransform4f xf = GetTransform();
        xf.SetTranslation(lctrXf.GetTranslation());
        wp->SetTransform(xf);
        SendScriptMsgs(kSS_InternalState00, mgr, GetUniqueId(), kSM_None);
      }
    }
    handled = true;
    break;
  case kUE_Projectile:
    if (x165c_28_) {
      FireDarkBeam(mgr);
      handled = true;
    }
    break;
  case kUE_DamageOn:
    BreakArmor(mgr);
    handled = true;
    break;
  case kUE_BeginAction:
    if (x165d_26_) {
      x165d_27_ = true;
    } else if (IsLeader(mgr)) {
      if (x165c_30_) {
        FireDoubleChargeBeams(mgr, node.GetLocatorName());
      } else if (x165c_31_) {
        FireTripleChargeBeams(mgr, node.GetLocatorName());
      }
    }
    handled = true;
    break;
  case kUE_IkLock:
    x165d_24_ = true;
    handled = true;
    break;
  case kUE_IkRelease:
    x165d_24_ = false;
    handled = true;
    break;
  case kUE_ScreenShake:
    ShakeCamera(mgr, node.GetLocatorName());
    break;
  case kUE_TakeOff:
    if (GetCoverPoint(mgr, xe90_) != nullptr) {
      SendScriptMsgs(kSS_Arrived, mgr, kInvalidUniqueId, kSM_None);
    }
    SpawnSandFountain(mgr, node.GetLocatorName());
    break;
  case kUE_Landing:
    SpawnSandFountain(mgr, node.GetLocatorName());
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

void CSandBoss::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                const CModelFlags& flags) const {
  if (x165d_24_) {
    RenderSphere(mgr, xf, flags);
    const CQuaternion inv = CQuaternion::FromMatrix(GetTransform()).BuildInverted();
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
      if (other != nullptr && other->x165d_24_) {
        const CQuaternion rot = CQuaternion::FromMatrix(other->GetTransform()) * inv;
        other->RenderModelAndArmor(mgr, rot.BuildTransform4f() * xf, flags);
      }
    }
  } else {
    RenderModelAndArmor(mgr, xf, flags);
  }
}

CAABox CSandBoss::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  if (x165d_24_) {
    CAABox box = GetModelBounds();
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
      if (other != nullptr && other != this && other->x165d_24_) {
        const CAABox otherBox = other->GetModelBounds();
        box.AccumulateBounds(otherBox.GetMinPoint());
        box.AccumulateBounds(otherBox.GetMaxPoint());
      }
    }
    return box;
  }
  return GetModelBounds();
}

CAABox CSandBoss::GetModelBounds() const {
  CAABox box = CAABox::MakeMaxInvertedBox();
  box = GetModelData()->GetAnimationData()->CalcBoundingBoxFromModelVerts();
  const CVector3f scale = GetModelData()->GetScale();
  box = box.GetTransformedAABox(CTransform4f::Translate(-GetTranslation()) * GetTransform() *
                                CTransform4f::Scale(scale));
  return box;
}

void CSandBoss::RenderSphere(const CStateManager& mgr, const CTransform4f& xf,
                             const CModelFlags& flags) const {
  if (const CActor* sphere = static_cast< const CActor* >(mgr.GetObjectById(xe8a_))) {
    CTransform4f sphereXf = xf * GetScaledLocatorTransform(CSegId(1));
    const CVector3f center = sphere->GetModelData()->GetBounds().GetCenterPoint();
    sphereXf.AddTranslation(sphereXf.Rotate(-center));
    sphere->GetModelData()->Render(mgr, sphereXf, sphere->GetActorLights(), flags);
  }
}

void CSandBoss::RenderModelAndArmor(const CStateManager& mgr, const CTransform4f& xf,
                                    const CModelFlags& flags) const {
  if (!NullModel()) {
    GetModelData()->Render(CModelData::kWM_Normal, xf, nullptr, flags);
  }
  RenderArmor(mgr, xf, flags, flags);
}

void CSandBoss::RenderArmor(const CStateManager& mgr, const CTransform4f& xf,
                            const CModelFlags& flags, const CModelFlags& headFlags) const {
  for (int i = 0; i < 8; ++i) {
    rstl::optional_object< CModelData > model;
    if (mArmorStates[i] == kArmor_Attached) {
      model = mAttachedArmorModels[i];
    } else if (mArmorStates[i] == kArmor_Stampede) {
      model = mStampedeArmorModels[i];
    }
    if (model) {
      const CTransform4f armorXf = xf * GetScaledLocatorTransform(mArmorSegIds[i]);
      model->Render(mgr, armorXf, GetActorLights(), i == 0 ? headFlags : flags);
    }
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
  if (!x165d_29_) {
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
  const CTransform4f xf = GetLctrTransform(mArmorSegIds[0]);
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
  if (mRound >= GetStage() && mRound < 3) {
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
  return mStampedeHP <= 0.f;
}

bool CSandBoss::ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (!x165e_24_ && !x165e_25_) {
    if (IsLeader(mgr)) {
      if (GetNumActiveBosses(mgr) == 1) {
        return mDarkBeamTimer <= 0.f;
      }
      if (mAttackOrder > 0 && mDarkBeamTimer <= 0.f) {
        const TUniqueId id = SelectDarkBeamBoss(mgr);
        if (id == GetUniqueId()) {
          return true;
        }
        if (id != kInvalidUniqueId) {
          if (CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(id))) {
            other->QueryDarkBeamAttack(mgr);
          }
        }
      }
    } else {
      return x165e_26_;
    }
  }
  return false;
}

bool CSandBoss::ShouldRepeaterAttack(CStateManager& mgr, const CTriggerData& data) const {
  const CPlayer* player = mgr.GetPlayer(0);
  if (GetNumActiveSafeZones(mgr) == 0) {
    return true;
  }
  CVector3f diff = player->GetTranslation() - GetTranslation();
  diff.SetZ(0.f);
  if (CVector3f::GetAngleDiff(GetTransform().GetForward(), diff) > 1.2217305f) {
    return false;
  }
  if (mgr.GetSafeZoneManager()->IsObjectInSafeZone(*player, mgr)) {
    return mgr.Random()->Range(0.f, 100.f) > mData.unknown_0x2b42dddf;
  }
  return mgr.Random()->Range(0.f, 100.f) > mData.unknown_0x1562e0d6;
}

bool CSandBoss::FoundSafeZoneTarget(CStateManager& mgr, const CTriggerData& data) const {
  return xe88_ != kInvalidUniqueId;
}

bool CSandBoss::ShouldTripleCharge(CStateManager& mgr, const CTriggerData& data) const {
  if (GetNumActiveBosses(mgr) == 3) {
    if (IsLeader(mgr)) {
      if (mChargeBeamTimer <= 0.f && x165e_27_ && mAttackOrder <= 0) {
        for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
          CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
          if (other != nullptr && other != this && other->x165d_24_ && !other->x165e_27_) {
            return false;
          }
        }
        for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
          CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
          if (other != nullptr && other != this && other->x165d_24_ &&
              !other->QueryTripleCharge(mgr)) {
            for (const TUniqueId* it2 = mOtherBosses.begin(); it2 != it; ++it2) {
              CSandBoss* prev = TCastToPtr< CSandBoss >(mgr.ObjectById(*it2));
              if (prev != nullptr && prev != this && prev->x165d_24_) {
                prev->BodyController()->CommandMgr().DeliverCmd(
                    CBodyStateCmd(kBSC_ExitState));
                prev->mAnimationState.SetState(CAnimationState::kAS_Over);
              }
            }
            return false;
          }
        }
        return true;
      }
    } else if (x165e_25_) {
      return true;
    }
  }
  return false;
}

bool CSandBoss::ShouldDoubleCharge(CStateManager& mgr, const CTriggerData& data) const {
  if (GetNumActiveBosses(mgr) == 2) {
    if (IsLeader(mgr)) {
      if (mChargeBeamTimer <= 0.f && x165e_27_ && mAttackOrder <= 0) {
        const CPlayer* player = mgr.GetPlayer(0);
        const CVector3f diff = player->GetTranslation() - GetTranslation();
        if (CVector3f::Dot(GetTransform().GetForward(), diff) > 0.f) {
          for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
            CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
            if (other != nullptr && other != this && other->x165d_24_) {
              if (CVector3f::Dot(other->GetTransform().GetForward(), diff) > 0.f) {
                return other->QueryDoubleCharge(mgr);
              }
              return false;
            }
          }
        }
      }
    } else if (x165e_24_) {
      return x165e_27_;
    }
  }
  return false;
}

bool CSandBoss::ShouldSnapJaws(CStateManager& mgr, const CTriggerData& data) const {
  if (!x165e_24_) {
    const CVector3f diff = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
    const float distSq = diff.MagSquared();
    const float minSq = mMinAttackRange * mMinAttackRange;
    const float maxSq = mMaxAttackRange * mMaxAttackRange;
    if (distSq >= minSq && distSq <= maxSq) {
      return CVector3f::GetAngleDiff(GetTransform().GetForward(), diff) < 0.5235988f;
    }
  }
  return false;
}

bool CSandBoss::ShouldDestroySphere(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (x165d_24_ && GetNumActiveBosses(mgr) == 1) {
    ret = true;
  }
  return ret;
}

bool CSandBoss::IsFacingPlayer(CStateManager& mgr, const CTriggerData& data) const {
  const CVector3f diff = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  return CVector3f::GetAngleDiff(GetTransform().GetForward(), diff) <= 0.34906584f;
}

bool CSandBoss::IsBallInSuckRange(CStateManager& mgr, const CTriggerData& data) const {
  if (x165d_27_ && mgr.GetPlayer(0)->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    return IsInSuckRange(*mgr.GetPlayer(0));
  }
  return false;
}

bool CSandBoss::IsLastStage(CStateManager& mgr, const CTriggerData& data) const {
  return mRound == 2;
}

bool CSandBoss::HeadArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return mHeadArmorHP <= 0.f;
}

bool CSandBoss::ArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return x165d_25_;
}

bool CSandBoss::SyncStampede(CStateManager& mgr, const CTriggerData& data) const {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->mSyncState != 1 && other->mSyncState != 0) {
      return false;
    }
  }
  return true;
}

bool CSandBoss::SyncAttachToSphere(CStateManager& mgr, const CTriggerData& data) const {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->mSyncState != 2 && other->mSyncState != 0) {
      return false;
    }
  }
  return true;
}

void CSandBoss::Recover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    x165d_27_ = false;
    break;
  case kStateMsg_Update:
    if (mStateMachine->GetTime() > 2.5f) {
      x165d_26_ = false;
    }
    break;
  }
}

void CSandBoss::UnderGround(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mCollisionActorManager->SetActive(mgr, false);
    mStampedeHP = mData.stampedeProperties.breakStampedeHP;
    x165c_24_ = true;
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
    break;
  case kStateMsg_Deactivate:
    x165c_24_ = false;
    x165d_26_ = false;
    break;
  }
}

void CSandBoss::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    FaceDeathWaypoint(mgr);
    SendScriptMsgs(kSS_DeathRattle, mgr, GetUniqueId(), kSM_None);
    SendScriptMsgs(kSS_Dead, mgr, GetUniqueId(), kSM_None);
    ReleasePlayer(mgr);
    mgr.SetBossParams(kInvalidUniqueId, 0.f, 0);
    mgr.DeleteObjectRequest(GetUniqueId());
    x165d_29_ = true;
    break;
  }
}

void CSandBoss::Deactivate(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Update:
    if (mStateMachine->GetTime() > 3.f) {
      mgr.DeleteObjectRequest(GetUniqueId());
    }
    break;
  }
}

void CSandBoss::RotateToPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mBoneTracking.SetAngSpeed(1.5707964f);
    mBoneTracking.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    break;
  case kStateMsg_Update: {
    const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
    if (AreSpheresUnlocked(mgr) && IsLeader(mgr)) {
      TurnTowards(playerPos, mgr, dt);
    }
    UpdateTurnLocomotion(playerPos);
    break;
  }
  case kStateMsg_Deactivate:
    mBoneTracking.SetActive(false);
    mBoneTracking.SetAngSpeed(3.1415927f);
    break;
  }
}

void CSandBoss::SuckBall(CStateManager& mgr, EStateMsg msg, float dt) {
  CPlayer* player = mgr.GetPlayer(0);
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->SetLocomotionType(pas::kLT_Internal5);
    player->EnableLeaveMorphBall(false);
    player->RemoveMaterial(kMT_Unknown59, mgr);
    break;
  case kStateMsg_Update:
    if (PullPlayerToMouth(*player, dt)) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSandBoss::BallInMouth(CStateManager& mgr, EStateMsg msg, float dt) {
  CPlayer* player = mgr.GetPlayer(0);
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    SendScriptMsgs(kSS_Entered, mgr, player->GetUniqueId(), kSM_None);
    BodyController()->SetLocomotionType(pas::kLT_Internal6);
    player->AttachActorToPlayer(GetUniqueId(), false);
    break;
  case kStateMsg_Update:
    AttachPlayerToMouth(*player);
    UpdateSpitOut(mgr);
    if (mStateMachine->GetTime() >= mData.spitMorphballTime) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x165d_27_ = false;
    break;
  }
}

void CSandBoss::DestroySphere(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    FaceDeathWaypoint(mgr);
    SendScriptMsgs(kSS_Retreat, mgr, GetUniqueId(), kSM_None);
    break;
  case kStateMsg_Update:
    if (mgr.GetPlayer(0)->GetAttachedActorId() == GetUniqueId()) {
      AttachPlayerToMouth(*mgr.GetPlayer(0));
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Five, -1));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
    }
    const float time = mStateMachine->GetTime();
    if (time < 5.f) {
      UpdateDamageFlash(time);
    }
    break;
  case kStateMsg_Deactivate:
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), *it, kSM_Increment));
    }
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    x165d_24_ = false;
    break;
  }
}

void CSandBoss::JumpOffSphere(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x165d_28_ = true;
    mgr.GetPlayer(0)->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::EPlayerOrbitRequest(8),
                                              mgr);
    RemoveMaterial(kMT_Orbit, mgr);
    break;
  case kStateMsg_Update:
    if (mgr.GetPlayer(0)->GetAttachedActorId() == GetUniqueId()) {
      AttachPlayerToMouth(*mgr.GetPlayer(0));
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
    }
    const float time = mStateMachine->GetTime();
    if (time < 5.f) {
      UpdateDamageFlash(time);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    x165d_24_ = false;
    x165d_28_ = false;
    mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), GetUniqueId(), kSM_Increment));
    break;
  }
}

void CSandBoss::SpitOutBall(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x165e_28_ = true;
    break;
  case kStateMsg_Update:
    if (mgr.GetPlayer(0)->GetAttachedActorId() == GetUniqueId()) {
      AttachPlayerToMouth(*mgr.GetPlayer(0));
      UpdateSpitOut(mgr);
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::EGenerateType(9), -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x165e_28_ = false;
    break;
  }
}

void CSandBoss::AttachToSphere(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (const CActor* sphere = static_cast< const CActor* >(mgr.GetObjectById(xe8a_))) {
      SetTranslation(sphere->GetTranslation());
      SetTransform(x14d0_.BuildTransform4f(GetTranslation()));
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mCollisionActorManager->SetActive(mgr, true);
    SetCollisionActorVulnerability(mgr, CDamageVulnerability::ReflectVulnerabilty(),
                                   CDamageVulnerability::ReflectVulnerabilty());
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    mAttackOrder = 0;
    x165d_30_ = true;
    break;
  case kStateMsg_Update:
    if (mStateMachine->GetTime() >= GetAttachDelay()) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
        BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
      } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
        BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    x165d_24_ = true;
    x165d_25_ = false;
    break;
  }
}

void CSandBoss::SuckAir(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->SetLocomotionType(pas::kLT_Internal5);
    x165d_26_ = true;
    break;
  case kStateMsg_Update:
    if (mStateMachine->GetTime() < mData.suckAirTime) {
      if (BodyController()->GetCurrentStateId() != pas::kAS_Locomotion) {
        BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted));
      }
      if (x165d_27_) {
        CPlayer* player = mgr.GetPlayer(0);
        if (player->GetMorphballTransitionState() != CPlayer::kMS_Morphed &&
            IsInSuckRange(*player)) {
          const CTransform4f xf = GetLctrTransform(mArmorSegIds[0]);
          const CVector3f diff = player->GetTranslation() - xf.GetTranslation();
          const float mag = diff.Magnitude();
          if (!(fabs(mag - 0.f) < 0.00001f)) {
            player->ApplyImpulseWR(dt * ((150.f * player->GetMass()) * ((1.f / mag) * -diff)),
                                   CAxisAngle::Identity());
            player->UseCollisionImpulses();
            player->SetMinimalAccelerationTimer(2.f * dt);
            mgr.PlayerState(0)->StaticInterference().AddSource(GetUniqueId(), 0.1f, 0.1f);
          }
        }
      }
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSandBoss::DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mBoneTracking.SetActive(true);
    mBoneTracking.SetAngSpeed(1.5707964f);
    mBoneTracking.SetTarget(xe88_);
    x165c_28_ = true;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(xe88_))) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCProjectileAttackCmd(pas::kS_Zero, target->GetTranslation(), false));
      }
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    if (BodyController()->GetCurrentStateId() == pas::kAS_ProjectileAttack) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted));
    }
    mBoneTracking.SetActive(false);
    mBoneTracking.SetAngSpeed(3.1415927f);
    x165c_28_ = false;
    xe88_ = kInvalidUniqueId;
    SyncAttackOrder(mgr, -1);
    break;
  }
}

void CSandBoss::RepeaterAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mBoneTracking.SetActive(true);
    mBoneTracking.SetAngSpeed(1.5707964f);
    mBoneTracking.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    x165c_29_ = true;
    x165c_28_ = x165c_29_;
    mRepeaterShots = 0;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLoopAttackCmd(pas::kLAT_One, true));
    }
    if (mRepeaterShots >= 3) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    }
    if (AreSpheresUnlocked(mgr) && IsLeader(mgr)) {
      FaceTarget(mgr.GetPlayer(0)->GetTranslation(), dt);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    }
    mBoneTracking.SetActive(false);
    mBoneTracking.SetAngSpeed(3.1415927f);
    x165c_29_ = false;
    x165c_28_ = x165c_29_;
    SyncAttackOrder(mgr, -1);
    break;
  }
}

void CSandBoss::DoubleCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x165c_30_ = true;
    BodyController()->SetTurnSpeed(mData.unknown_0x7619e561.doubleCharge.turnSpeed);
    mBeamAngle = -1.5707964f;
    mBeamTurnDirection = 1;
    mBeamTurnTimer = 0.f;
    UpdateBeamTurn(mgr, mData.unknown_0x7619e561.doubleCharge, dt);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack && IsLeader(mgr)) {
      if (!mChargeBeams.empty()) {
        const float delta =
            dt * ((M_PIF / 180.f) * mData.unknown_0x7619e561.doubleCharge.turnSpeed);
        float angle;
        if (mBeamTurnDirection == 1) {
          angle = 1.5707964f - delta;
        } else {
          angle = 1.5707964f + delta;
        }
        const CVector3f dir =
            GetTransform().Rotate(CVector3f(CMath::FastCosR(angle), CMath::FastSinR(angle), 0.f));
        FaceTarget(GetTranslation() + dir, dt);
      }
      if (mStateMachine->GetTime() > mData.unknown_0x7619e561.doubleCharge.duration ||
          (GetNumFiringBeams(mgr) < 2 && !mChargeBeams.empty())) {
        StopChargeBeams(mgr);
      }
      UpdateDoubleChargeBeams(mgr, dt);
      UpdateBeamTurn(mgr, mData.unknown_0x7619e561.doubleCharge, dt);
    }
    break;
  case kStateMsg_Deactivate:
    x165c_30_ = false;
    mAttackOrder = mData.unknown_0x7619e561.doubleCharge.unknown_0x8d4f3b88;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    StopChargeBeams(mgr);
    BodyController()->SetTurnSpeed(xde8_);
    break;
  }
}

void CSandBoss::TripleCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x165c_31_ = true;
    mBeamAngle = -1.5707964f;
    mBeamTurnDirection = 1;
    mBeamTurnTimer = 0.f;
    BodyController()->SetTurnSpeed(mData.unknown_0x7619e561.tripleCharge.turnSpeed);
    UpdateBeamTurn(mgr, mData.unknown_0x7619e561.tripleCharge, dt);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack && IsLeader(mgr)) {
      if (!mChargeBeams.empty()) {
        const float delta =
            dt * ((M_PIF / 180.f) * mData.unknown_0x7619e561.tripleCharge.turnSpeed);
        float angle;
        if (mBeamTurnDirection == 1) {
          angle = 1.5707964f - delta;
        } else {
          angle = 1.5707964f + delta;
        }
        const CVector3f dir =
            GetTransform().Rotate(CVector3f(CMath::FastCosR(angle), CMath::FastSinR(angle), 0.f));
        FaceTarget(GetTranslation() + dir, dt);
      }
      if (mStateMachine->GetTime() > mData.unknown_0x7619e561.tripleCharge.duration ||
          (GetNumFiringBeams(mgr) < 3 && !mChargeBeams.empty())) {
        StopChargeBeams(mgr);
      }
      UpdateTripleChargeBeams(mgr, dt);
      UpdateBeamTurn(mgr, mData.unknown_0x7619e561.tripleCharge, dt);
    }
    break;
  case kStateMsg_Deactivate:
    x165c_31_ = false;
    mAttackOrder = mData.unknown_0x7619e561.tripleCharge.unknown_0x8d4f3b88;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    StopChargeBeams(mgr);
    BodyController()->SetTurnSpeed(xde8_);
    break;
  }
}

void CSandBoss::SnapJaws(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x165c_26_ = true;
    SetCollisionActorExtendedTouchBounds(mgr, skJawsTouchBounds);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_One));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x165c_26_ = false;
    SetCollisionActorExtendedTouchBounds(mgr, CVector3f::Zero());
    if (BodyController()->GetCurrentStateId() == pas::kAS_MeleeAttack) {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_NextState));
    }
    break;
  }
}

void CSandBoss::SeekStampedePoint(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x165c_25_ = false;
    break;
  case kStateMsg_Update:
    if (const CScriptCoverPoint* cover = GetCoverPoint(mgr, xe8e_)) {
      const CVector3f pos = GetTranslation();
      const CVector3f diff = cover->GetTranslation() - pos;
      const float mag = diff.Magnitude();
      const float step = 100.f * dt;
      if (mag > step) {
        SetTranslation(pos + step * ((1.f / mag) * diff));
      }
      if (mStampedeTimer >= GetStampedeSpeed(mgr)) {
        x165c_25_ = true;
      }
    }
    break;
  case kStateMsg_Deactivate:
    if (const CScriptCoverPoint* cover = GetCoverPoint(mgr, xe8e_)) {
      const TUniqueId wpId = cover->FindConnectedObject(mgr, kSS_Arrived, kSM_Next);
      if (const CScriptAIWaypoint* wp =
              TCastToConstPtr< CScriptAIWaypoint >(mgr.GetObjectById(wpId))) {
        const CVector3f coverPos = cover->GetTranslation();
        CVector3f target = coverPos + (wp->GetTranslation() - coverPos);
        target.SetZ(coverPos.GetZ() + 0.f);
        SetTransform(CTransform4f::LookAt(coverPos, target, CVector3f::Up()));
        xf28_ = wp->GetTranslation().GetZ();
      }
      xf28_ = cover->GetTranslation().GetZ();
    }
    break;
  }
}

void CSandBoss::Stampede(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mCollisionActorManager->SetActive(mgr, true);
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    RemoveMaterial(kMT_GroundCollider, mgr);
    mVerticalMovement = true;
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
      if (other != nullptr && other != this) {
        other->mStampedeTimer = 0.f;
      }
    }
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    }
    UpdateStampedeMovement(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
    AddMaterial(kMT_GroundCollider, mgr);
    mVerticalMovement = false;
    mStampedeTimer = 0.f;
    break;
  }
}

void CSandBoss::SetTripleChargeArmor(CStateManager& mgr, float dt) {
  for (int i = 0; i < 8; ++i) {
    mArmorStates[i] = kArmor_Attached;
  }
  SetCollisionActorVulnerability(mgr, x162c_, x162c_);
  UpdateCollisionActorResponses(mgr);
  mDamageColor = skHitsWithoutDamageColor;
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(15), true);
}

void CSandBoss::SetDoubleChargeArmor(CStateManager& mgr, float dt) {
  for (int i = 0; i < 8; ++i) {
    mArmorStates[i] = kArmor_Attached;
  }
  SetCollisionActorVulnerability(mgr, x15fc_, x15fc_);
  UpdateCollisionActorResponses(mgr);
  mDamageColor = skHitsWithoutDamageColor;
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(15), true);
}

void CSandBoss::SetStampedeArmor(CStateManager& mgr, float dt) {
  for (int i = 0; i < 8; ++i) {
    mArmorStates[i] = kArmor_Stampede;
  }
  SetCollisionActorVulnerability(mgr, mStampedeVulnerability, mStampedeVulnerability);
  UpdateCollisionActorResponses(mgr);
  mDamageColor = skHitsWithoutDamageColor;
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(15), true);
}

void CSandBoss::SetAttachedArmor(CStateManager& mgr, float dt) {
  for (int i = 0; i < 8; ++i) {
    mArmorStates[i] = kArmor_Attached;
  }
  SetCollisionActorVulnerability(mgr, mDamageVulnerability,
                                 CDamageVulnerability::ReflectVulnerabilty());
  UpdateCollisionActorResponses(mgr);
  mDamageColor = skHitsWithoutDamageColor;
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(15), true);
}

void CSandBoss::SetSuckAirArmor(CStateManager& mgr, float dt) {
  mArmorStates[0] = kArmor_None;
  for (int i = 1; i < 8; ++i) {
    mArmorStates[i] = kArmor_Attached;
  }
  SetCollisionActorVulnerability(mgr, mSuckAirVulnerability,
                                 CDamageVulnerability::ReflectVulnerabilty());
  UpdateCollisionActorResponses(mgr);
  mDamageColor = skDamageColor;
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(15), false);
  SetVisorOrbitableFlags(CVisorParameters::EVisorOrbitableFlags(2), true);
}

void CSandBoss::RemovePlayerCollision(CStateManager& mgr, float dt) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc =
        mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (CCollisionActor* act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      CMaterialFilter filter = act->GetMaterialFilter();
      filter.ExcludeList().Add(kMT_Player);
      act->SetMaterialFilter(filter);
    }
  }
}

void CSandBoss::RestorePlayerCollision(CStateManager& mgr, float dt) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc =
        mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (CCollisionActor* act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      CMaterialFilter filter = act->GetMaterialFilter();
      filter.ExcludeList().Remove(kMT_Player);
      act->SetMaterialFilter(filter);
    }
  }
  mgr.GetPlayer(0)->EnableLeaveMorphBall(true);
}

void CSandBoss::SelectSafeZoneTarget(CStateManager& mgr, float dt) {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  const CVector3f pos = GetTranslation();
  const CVector3f forward = GetTransform().GetForward();
  float bestDistSq = 3.4028235e38f;
  xe88_ = kInvalidUniqueId;
  for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
       it != GetConnectionList().end(); ++it) {
    if (it->state == kSS_Attack && it->msg == kSM_Attach) {
      const TUniqueId id = mgr.GetIdForScript(it->objId);
      const CScriptSafeZone* zone = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(id));
      if (zone != nullptr && zone->GetActive()) {
        const CVector3f zonePos = zone->GetTranslation();
        if (CVector3f::GetAngleDiff(forward, zonePos - pos) <= 1.2217305f) {
          const float distSq = (zonePos - playerPos).MagSquared();
          if (distSq < bestDistSq) {
            bestDistSq = distSq;
            xe88_ = zone->GetUniqueId();
          }
        }
      }
    }
  }
}

void CSandBoss::WaitStampede(CStateManager& mgr, float dt) { mSyncState = 1; }

void CSandBoss::WaitAttachToSphere(CStateManager& mgr, float dt) { mSyncState = 2; }

void CSandBoss::LockSphere(CStateManager& mgr, float dt) { x165d_31_ = true; }

void CSandBoss::UnLockSphere(CStateManager& mgr, float dt) { x165d_31_ = false; }

void CSandBoss::AllowSyncAttacks(CStateManager& mgr, float dt) { x165e_27_ = true; }

void CSandBoss::PreventSyncAttacks(CStateManager& mgr, float dt) { x165e_27_ = false; }

void CSandBoss::Deactivate(CStateManager& mgr, float dt) { x165c_27_ = true; }

void CSandBoss::EndStampede(CStateManager& mgr, float dt) { x165c_27_ = false; }

void CSandBoss::ResetHeadArmorHP(CStateManager& mgr, float dt) { mHeadArmorHP = mData.headArmorHP; }

void CSandBoss::ResetAttackTimes(CStateManager& mgr, float dt) {
  switch (GetNumActiveBosses(mgr)) {
  case 3:
    mChargeBeamTimer = mData.unknown_0x7619e561.tripleCharge.minChargeBeamAttackTime +
                       mgr.Random()->Range(0.f, mData.unknown_0x7619e561.tripleCharge
                                                    .chargeBeamAttackTimeVariance);
    mDarkBeamTimer = mData.unknown_0x7619e561.tripleCharge.minDarkBeamAttackTime +
                     mgr.Random()->Range(0.f, mData.unknown_0x7619e561.tripleCharge
                                                  .darkBeamAttackTimeVariance);
    break;
  case 2:
    mChargeBeamTimer = mData.unknown_0x7619e561.doubleCharge.minChargeBeamAttackTime +
                       mgr.Random()->Range(0.f, mData.unknown_0x7619e561.doubleCharge
                                                    .chargeBeamAttackTimeVariance);
    mDarkBeamTimer = mData.unknown_0x7619e561.doubleCharge.minDarkBeamAttackTime +
                     mgr.Random()->Range(0.f, mData.unknown_0x7619e561.doubleCharge
                                                  .darkBeamAttackTimeVariance);
    break;
  case 1:
  default:
    mChargeBeamTimer = 3.4028235e38f;
    mDarkBeamTimer =
        mData.minDarkBeamAttackTime + mgr.Random()->Range(0.f, mData.darkBeamAttackTimeVariance);
    break;
  }
  x165d_30_ = false;
}

void CSandBoss::SetCinematicStateUnderGround(CStateManager& mgr, float dt) {
  mCinematicState = kCS_UnderGround;
}

void CSandBoss::SetCinematicStateStampede(CStateManager& mgr, float dt) {
  mCinematicState = kCS_Stampede;
}

void CSandBoss::SetCinematicStateNormal(CStateManager& mgr, float dt) {
  mCinematicState = kCS_Normal;
}

void CSandBoss::SetCinematicStateStunned(CStateManager& mgr, float dt) {
  mCinematicState = kCS_Stunned;
}

void CSandBoss::SetCinematicStateChoking(CStateManager& mgr, float dt) {
  mCinematicState = kCS_Choking;
}

void CSandBoss::SetCinematicStateExitSphere(CStateManager& mgr, float dt) {
  mCinematicState = kCS_ExitSphere;
}

bool CSandBoss::QueryDoubleCharge(CStateManager& mgr) {
  if (x165e_27_) {
    x165e_24_ = true;
    mStateMachine->Update(mgr, *this, 0.f);
    x165e_24_ = false;
  }
  return x165c_30_;
}

bool CSandBoss::QueryTripleCharge(CStateManager& mgr) {
  x165e_25_ = true;
  mStateMachine->Update(mgr, *this, 0.f);
  x165e_25_ = false;
  return x165c_31_;
}

bool CSandBoss::QueryDarkBeamAttack(CStateManager& mgr) {
  x165e_26_ = true;
  mStateMachine->Update(mgr, *this, 0.f);
  x165e_26_ = false;
  return x165c_28_;
}

bool CSandBoss::IsStampeding() const {
  bool ret = false;
  if (!x165c_28_ && GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion) {
    ret = true;
  }
  return ret;
}

int CSandBoss::GetStage() const { return mData.commandIndex; }

void CSandBoss::UpdateCollisionActorResponses(CStateManager& mgr) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc =
        mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (CCollisionActor* act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      act->SetWeaponCollisionResponseType(EWeaponCollisionResponseTypes(47));
      for (int j = 0; j < 8; ++j) {
        if (desc.GetName() == skSpineJoints[j]) {
          if (j == 0) {
            if (mArmorStates[j] == kArmor_Attached) {
              act->SetWeaponCollisionResponseType(EWeaponCollisionResponseTypes(77));
            }
          } else if (mArmorStates[j] == kArmor_Attached) {
            act->SetWeaponCollisionResponseType(EWeaponCollisionResponseTypes(107));
          }
          break;
        }
      }
    }
  }
}

void CSandBoss::SetCollisionActorVulnerability(CStateManager& mgr,
                                               const CDamageVulnerability& headVuln,
                                               const CDamageVulnerability& bodyVuln) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc =
        mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (CCollisionActor* act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      if (id == xe8c_) {
        act->SetDamageVulnerability(headVuln);
      } else {
        act->SetDamageVulnerability(bodyVuln);
      }
    }
  }
}

void CSandBoss::SetCollisionActorExtendedTouchBounds(CStateManager& mgr,
                                                     const CVector3f& extents) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc =
        mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (CCollisionActor* act = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      act->SetExtendedTouchBounds(extents);
    }
  }
}

bool CSandBoss::IsInSuckRange(const CPlayer& player) const {
  const CTransform4f xf = GetLctrTransform(mArmorSegIds[0]);
  const CVector3f diff = player.GetTranslation() - xf.GetTranslation();
  if (diff.MagSquared() < mData.suckMorphballRange * mData.suckMorphballRange) {
    return CVector3f::GetAngleDiff(GetTransform().GetForward(),
                                   CVector3f(CVector2f(diff.GetX(), diff.GetY()), 0.f)) <=
           0.2617994f;
  }
  return false;
}

bool CSandBoss::IsLeader(const CStateManager& mgr) const {
  if (x165d_24_) {
    const int stage = mData.commandIndex;
    for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
      const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
      if (other != nullptr && other != this && other->x165d_24_ &&
          other->mData.commandIndex < stage) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool CSandBoss::AreSpheresUnlocked(const CStateManager& mgr) const {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->x165d_31_) {
      return false;
    }
  }
  return true;
}

int CSandBoss::GetNumActiveBosses(const CStateManager& mgr) const {
  int count = 0;
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->x165d_24_) {
      ++count;
    }
  }
  return count;
}

void CSandBoss::GetActiveBosses(CStateManager& mgr,
                                rstl::reserved_vector< TUniqueId, 3 >& bosses) {
  bosses.clear();
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
    if (other != nullptr && other->x165d_24_) {
      bosses.push_back(*it);
    }
  }
}

int CSandBoss::GetNumActiveSafeZones(const CStateManager& mgr) const {
  int count = 0;
  for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
       it != GetConnectionList().end(); ++it) {
    if (it->state == kSS_Attack && it->msg == kSM_Attach) {
      const TUniqueId id = mgr.GetIdForScript(it->objId);
      const CScriptSafeZone* zone = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(id));
      if (zone != nullptr && zone->GetActive()) {
        ++count;
      }
    }
  }
  return count;
}

float CSandBoss::GetStampedeSpeed(const CStateManager& mgr) const {
  int count = 0;
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(*it));
    if (other != nullptr && other->x165c_27_) {
      ++count;
    }
  }
  if (count == 3) {
    return mData.stampedeProperties.unknown_0x5fb66017;
  } else if (count == 2) {
    return mData.stampedeProperties.unknown_0xc2b98161;
  } else {
    return mData.stampedeProperties.unknown_0xbed8a4ba;
  }
}

void CSandBoss::UpdateTurnLocomotion(const CVector3f& target) {
  const CVector3f diff = target - GetTranslation();
  const float angle = CVector3f::GetAngleDiff(GetTransform().GetForward(), diff);
  if (angle < 0.61086524f || angle > 2.5307274f) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  } else if (CVector3f::Dot(GetTransform().GetRight(), diff) > 0.f) {
    BodyController()->SetLocomotionType(pas::kLT_Internal10);
  } else {
    BodyController()->SetLocomotionType(pas::kLT_Internal11);
  }
}

void CSandBoss::UpdateDamageFlash(float time) {
  const float t = fabs(CMath::FastCosR(3.1415927f * time));
  mDamageColor = CColor::Lerp(CColor(0.f, 0.f, 0.f, 1.f), skDamageColor, t);
  TakeDamage(CVector3f::Zero(), 0.f);
}

bool CSandBoss::PullPlayerToMouth(CPlayer& player, float dt) {
  if (player.GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    const CTransform4f xf = GetLctrTransform(mArmorSegIds[0]);
    const CVector3f diff = xf.GetTranslation() - player.GetTranslation();
    const float step = 60.f * dt;
    if (diff.MagSquared() < step * step || !diff.IsMagnitudeSafe()) {
      AttachPlayerToMouth(player);
      return true;
    }
    player.Stop();
    player.SetTranslation(player.GetTranslation() + step * diff.AsNormalized());
  }
  return false;
}

void CSandBoss::AttachPlayerToMouth(CPlayer& player) {
  const CTransform4f xf = GetLctrTransform(mArmorSegIds[0]);
  player.SetTranslation(xf.GetTranslation());
  player.Stop();
  x14a0_ = player.GetTransform().GetRotation();
}

void CSandBoss::FaceDeathWaypoint(CStateManager& mgr) {
  const TUniqueId id = FindConnectedObject(mgr, kSS_Dead, kSM_Attach);
  if (const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id))) {
    const CVector3f pos = GetTranslation();
    CVector3f target = wp->GetTranslation();
    target.SetZ(pos.GetZ());
    SetTransform(CTransform4f::LookAt(pos, target, CVector3f::Up()));
  }
}

int CSandBoss::GetNumFiringBeams(CStateManager& mgr) {
  int count = 0;
  for (const SChargeBeam* it = mChargeBeams.begin(); it != mChargeBeams.end(); ++it) {
    CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(it->mBeamId));
    if (beam != nullptr && beam->IsFiring() &&
        beam->GetExpansionState() != CPlasmaProjectile::kES_Release) {
      ++count;
    }
  }
  return count;
}

int CSandBoss::SelectFacingBoss(CStateManager& mgr,
                                const rstl::reserved_vector< TUniqueId, 3 >& bosses,
                                CVector3f target) const {
  const int count = bosses.size();
  float bestAngle = -3.1415927f;
  int best = -1;
  for (int i = 0; i < count; ++i) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(bosses[i]));
    if (other != nullptr) {
      const float angle = CVector3f::GetAngleDiff(target - other->GetTranslation(),
                                                  other->GetTransform().GetForward());
      if (angle > bestAngle) {
        bestAngle = angle;
        best = i;
      }
    }
  }
  return best;
}

int CSandBoss::FindLungingBoss(CStateManager& mgr,
                               const rstl::reserved_vector< TUniqueId, 3 >& bosses) const {
  const int count = bosses.size();
  int ret = -1;
  for (int i = 0; i < count; ++i) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(bosses[i]));
    if (other != nullptr && other->x165c_29_) {
      ret = i;
      break;
    }
  }
  return ret;
}

void CSandBoss::TurnTowards(const CVector3f& target, CStateManager& mgr, float dt) {
  rstl::reserved_vector< TUniqueId, 3 > bosses;
  GetActiveBosses(mgr, bosses);
  switch (bosses.size()) {
  case 3: {
    const int idx = FindLungingBoss(mgr, bosses);
    if (idx != -1) {
      TurnWithBoss(target, mgr, bosses[idx], dt);
    }
    const CVector3f pos = target;
    bosses.erase(bosses.begin() + SelectFacingBoss(mgr, bosses, pos));
    TurnBetweenBosses(target, mgr, bosses[0], bosses[1], dt);
    break;
  }
  case 2: {
    const int idx = FindLungingBoss(mgr, bosses);
    if (idx != -1) {
      TurnWithBoss(target, mgr, bosses[idx], dt);
    }
    TurnBetweenBosses(target, mgr, bosses[0], bosses[1], dt);
    break;
  }
  default:
    FaceTarget(target, dt);
    break;
  }
}

void CSandBoss::TurnBetweenBosses(const CVector3f& target, CStateManager& mgr, TUniqueId id1,
                                  TUniqueId id2, float dt) {
  const CSandBoss* boss1 = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(id1));
  const CSandBoss* boss2 = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(id2));
  if (boss1 != nullptr && boss2 != nullptr) {
    const CVector3f pos = GetTranslation();
    CVector3f diff = target - pos;
    diff.SetZ(0.f);
    const CVector3f right = boss1->GetTransform().GetRight();
    const CVector3f sum = boss1->GetTransform().GetForward() + boss2->GetTransform().GetForward();
    const CVector3f dir = sum.IsMagnitudeSafe() ? sum.AsNormalized() : right;
    float angle = CVector3f::GetAngleDiff(dir, diff);
    if (angle > 0.17453292f) {
      if (CVector3f::Dot(CVector3f(sum.GetY(), -sum.GetX(), sum.GetZ()), diff) > 0.f) {
        angle = -angle;
      }
      const CVector3f rotated = GetTransform().Rotate(
          CVector3f(CMath::FastCosR(1.5707964f + angle), CMath::FastSinR(angle), 0.f));
      FaceTarget(pos + diff.Magnitude() * rotated, dt);
    }
  }
}

void CSandBoss::FaceTarget(const CVector3f& target, float dt) {
  if (dt > 0.f) {
    const CVector3f diff = target - GetTranslation();
    if (diff.IsMagnitudeSafe()) {
      BodyController()->FaceDirection(diff.AsNormalized(), dt);
    }
  }
}

void CSandBoss::TurnWithBoss(const CVector3f& target, CStateManager& mgr, TUniqueId id,
                             float dt) {
  if (const CSandBoss* other = TCastToConstPtr< CSandBoss >(mgr.GetObjectById(id))) {
    const CVector3f pos = GetTranslation();
    CVector3f diff = target - pos;
    diff.SetZ(0.f);
    float angle = CVector3f::GetAngleDiff(other->GetTransform().GetForward(), diff);
    if (angle > 0.17453292f) {
      if (CVector3f::Dot(other->GetTransform().GetRight(), diff) > 0.f) {
        angle = -angle;
      }
      const CVector3f rotated = GetTransform().Rotate(
          CVector3f(CMath::FastCosR(1.5707964f + angle), CMath::FastSinR(angle), 0.f));
      FaceTarget(pos + diff.Magnitude() * rotated, dt);
    }
  }
}

void CSandBoss::SyncAttackOrder(CStateManager& mgr, int offset) {
  int minOrder = mAttackOrder;
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
    if (other != nullptr && other != this && other->x165d_24_) {
      if (other->mAttackOrder < minOrder) {
        minOrder = other->mAttackOrder;
      }
    }
  }
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
    if (other != nullptr && other->x165d_24_) {
      other->mAttackOrder = minOrder + offset;
      other->ResetAttackTimes(mgr, 0.f);
    }
  }
}

void CSandBoss::UpdateBeamTurn(CStateManager& mgr, const SLdrSandBossChargeBeamData& data,
                               float dt) {
  if (mChargeBeams.size() != 0) {
    mBeamTurnTimer -= dt;
  }
  if (mBeamTurnTimer <= 0.f) {
    const float maxInterval = data.changeDirectionInterval + data.changeDirectionVariance;
    if (data.duration - mStateMachine->GetTime() > maxInterval) {
      if (mgr.Random()->Range(0.f, 100.f) <= data.changeDirectionChance) {
        mBeamTurnDirection = mBeamTurnDirection != 1;
      }
      mBeamTurnTimer =
          data.changeDirectionInterval + mgr.Random()->Range(0.f, data.changeDirectionVariance);
    } else {
      mBeamTurnTimer = maxInterval;
    }
  }
}

void CSandBoss::StopChargeBeams(CStateManager& mgr) {
  for (const TUniqueId* it = mOtherBosses.begin(); it != mOtherBosses.end(); ++it) {
    CSandBoss* other = TCastToPtr< CSandBoss >(mgr.ObjectById(*it));
    if (other != nullptr && other->x165d_24_) {
      other->BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
      other->mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  }
  ResetChargeBeams(mgr);
  switch (xdfc_) {
  case 2:
    break;
  case 0:
  case 1:
    xdfc_ = 2;
    xf14_ = mData.unknown_0x7619e561.chargeBeamInfo.shutdownTime;
    break;
  }
}

void CSandBoss::ResetChargeBeams(CStateManager& mgr) {
  for (const SChargeBeam* it = mChargeBeams.begin(); it != mChargeBeams.end(); ++it) {
    if (CEntity* beam = mgr.ObjectById(it->mBeamId)) {
      static_cast< CPlasmaProjectile* >(beam)->ResetBeam(mgr, false);
    }
  }
}

float CSandBoss::GetAttachDelay() const { return 0.5f * float(mData.commandIndex); }

SSandBoss_FuncPtrs REL_loader_SandBoss;

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
