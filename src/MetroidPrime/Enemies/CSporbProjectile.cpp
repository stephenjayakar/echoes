#include "MetroidPrime/Enemies/CSporbProjectile.hpp"

#include "Collision/CCollisionInfoList.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "Kyoto/Animation/CCharacterInfo.hpp"
#include "Kyoto/Particles/CParticleData.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSporbTop.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSporbProjectile.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>

CSporbProjectile::CSporbProjectile(TUniqueId uid, const rstl::string& name,
                                   const CEntityInfo& info, const CTransform4f& xf,
                                   const CModelData& mData, const CPatternedInfo& pInfo,
                                   const CActorParameters& aParms, CAssetId ballSpitEffect,
                                   CAssetId ballEscapeEffect)
: CPatterned(static_cast< EPatternedAI >(0x49), uid, name, kFT_Zero, info, xf, mData, pInfo,
             kMT_Flyer, kCT_One, static_cast< EBodyType >(5), aParms)
, mState(-1)
, mCaptureState(0)
, mCollisionPrimitive(CSphere(pInfo.GetBodyOrigin(),
                              rstl::max_val(pInfo.GetHalfExtent(), pInfo.GetHeight() / 2.f)),
                      GetMaterialList())
, mCaptureRadius(2.f * mCollisionPrimitive.GetSphere().GetRadius())
, mHoldingBall(false)
, mBallSpitEffect(ballSpitEffect)
, mBallEscapeEffect(ballEscapeEffect)
, mEffectCount(0)
, mTopId(kInvalidUniqueId) {
  SetDrawShadow(false);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_Player);
  include.Remove(kMT_Character);
  include.Add(kMT_NoPlatformCollision);
  include.Add(kMT_CameraPassthrough);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      include, CMaterialList(kMT_Character, kMT_Player, kMT_AIPassthrough)));
  mCollisionPrimitive.SetMaterial(include);

  const CAABox& baseBox = GetBaseBoundingBox();
  const float halfX = (baseBox.GetMaxPoint().GetX() - baseBox.GetMinPoint().GetX()) / 2.f;
  const float halfZ = (baseBox.GetMaxPoint().GetZ() - baseBox.GetMinPoint().GetZ()) / 2.f;
  const float halfY = (baseBox.GetMaxPoint().GetY() - baseBox.GetMinPoint().GetY()) / 2.f;
  SetBoundingBox(CAABox(-halfX, -halfZ, -halfY, halfX, halfZ, halfY));
  mDisabledAnimationDeltas = kADF_Translation | kADF_Rotation;

  rstl::vector< CAssetId > parts;
  parts.reserve(2);
  if (mBallSpitEffect != kInvalidAssetId) {
    parts.push_back_unsafe(mBallSpitEffect);
  }
  if (mBallEscapeEffect != kInvalidAssetId) {
    parts.push_back_unsafe(mBallEscapeEffect);
  }
  AnimationData()->GetParticleDB().CacheParticleDesc(CCharacterInfo::CParticleResData(
      parts, rstl::vector< CAssetId >(), rstl::vector< CAssetId >(), rstl::vector< CAssetId >(),
      rstl::vector< CAssetId >(), rstl::vector< CAssetId >()));
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::AnimOver)},
    {"ShouldPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldPatrol)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldAttack)},
    {"ShouldFire",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldFire)},
    {"ShouldReload",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldReload)},
    {"ShouldLaunch",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldLaunch)},
    {"ShouldClose",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldClose)},
    {"ShouldOpen",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldOpen)},
    {"ShouldSpit",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbProjectile::ShouldSpit)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Sleep)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Patrol)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Attack)},
    {"WakeUp", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::WakeUp)},
    {"GoToSleep",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::GoToSleep)},
    {"Fire", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Fire)},
    {"Reload", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Reload)},
    {"Launch", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Launch)},
    {"Close", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Close)},
    {"Open", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Open)},
    {"Spit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbProjectile::Spit)},
};

void CSporbProjectile::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

void CSporbProjectile::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_AIUpdateDisabled:
  case kSM_Increment:
  case kSM_Decrement:
    break;
  case kSM_AreaLoaded:
    for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
         it != GetConnectionList().end(); ++it) {
    }
    break;
  case kSM_Create:
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    *DamageVulnerability() = CDamageVulnerability::ImmuneVulnerabilty();
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    RemoveMaterial(kMT_ExcludeFromRadar, mgr);
    AddMaterial(kMT_CameraPassthrough, mgr);
    RemoveMaterial(kMT_Target, mgr);
    AddMaterial(kMT_Unknown54, mgr);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

void CSporbProjectile::Think(float dt, CStateManager& mgr) {
  const CTransform4f& xf = GetTransform();
  const CTransform4f locatorXf =
      xf * GetScaledLocatorTransform(rstl::string_l("ball_attach_LCTR"));
  const CVector3f locatorPos = locatorXf.GetTranslation();
  const CVector3f moveDelta = locatorPos - xf.GetTranslation();
  MoveCollisionPrimitive(moveDelta);
  CPatterned::Think(dt, mgr);

  const CPlayer* player = mgr.GetPlayer(0);
  const CVector3f ballPos = player->GetAimPosition(mgr, 0.f);
  const CPlayer::EPlayerMorphBallState morphState =
      player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
          ? player->GetMorphballTransitionState()
          : CPlayer::kMS_Unmorphed;
  if (morphState == CPlayer::kMS_Morphed && mState == 0) {
    const CVector3f delta = ballPos - locatorPos;
    if (mCaptureState == 0) {
      if (delta.MagSquared() <= mCaptureRadius * mCaptureRadius) {
        RemoveMaterial(kMT_Unknown59, mgr);
        mCaptureState = 2;
        mHoldingBall = true;
      }
    } else if (mCaptureState == 2) {
      if (delta.MagSquared() > mCaptureRadius * mCaptureRadius) {
        mCaptureState = 0;
        AddMaterial(kMT_Unknown59, mgr);
        mHoldingBall = false;
      }
    }
  }
}

void CSporbProjectile::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

void CSporbProjectile::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

void CSporbProjectile::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

void CSporbProjectile::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mState = 5;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mState = 6;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mState = 7;
    mCaptureState = 0;
    AddMaterial(kMT_Unknown59, mgr);
    mHoldingBall = false;
    x7ed_ = false;
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::WakeUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::GoToSleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Launch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal8);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Close(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mHoldingBall) {
      BodyController()->SetLocomotionType(pas::kLT_Internal6);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
    }
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Open(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Fire(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    BodyController()->SetLocomotionType(pas::kLT_Combat);
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

void CSporbProjectile::Spit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbProjectile::Reload(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mState = 7;
    break;
  }
}

bool CSporbProjectile::ShouldPatrol(CStateManager&, const CTriggerData&) const {
  return mState >= 6;
}

bool CSporbProjectile::ShouldAttack(CStateManager&, const CTriggerData&) const {
  return mState >= 7;
}

bool CSporbProjectile::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CSporbProjectile::ShouldFire(CStateManager&, const CTriggerData&) const {
  return mState >= 8;
}

bool CSporbProjectile::ShouldReload(CStateManager&, const CTriggerData&) const {
  return mState >= 9;
}

bool CSporbProjectile::ShouldLaunch(CStateManager&, const CTriggerData&) const {
  return mState == 0;
}

bool CSporbProjectile::ShouldClose(CStateManager&, const CTriggerData&) const {
  return mState == 1;
}

bool CSporbProjectile::ShouldOpen(CStateManager&, const CTriggerData&) const {
  return mState == 2;
}

bool CSporbProjectile::ShouldSpit(CStateManager&, const CTriggerData&) const {
  return mState == 4;
}

void CSporbProjectile::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                    CStateManager& mgr) {
  static const CMaterialList skWorldTypes(kMT_Unknown59, kMT_Ceiling, kMT_Wall, kMT_Floor);
  if (id == kInvalidUniqueId) {
    for (int i = 0; i < list.GetCount(); ++i) {
      const CCollisionInfo& info = list[i];
      if (info.GetMaterialLeft().SharesMaterials(skWorldTypes) &&
          !info.GetMaterialLeft().HasMaterial(kMT_AIPassthrough)) {
        mCaptureState = 1;
        mHoldingBall = false;
      }
    }
  }
  CPatterned::CollidedWith(id, list, mgr);
}

void CSporbProjectile::CreateSpitEffect(CStateManager& mgr) {
  char name[100];
  sprintf(name, "SPORB_TOP_EFFECT%d-%d", mBallSpitEffect, mEffectCount++);
  const CSegId locator =
      GetAnimationData()->GetCharLayoutInfo()->GetSegIdFromString(rstl::string_l("ball_attach_LCTR"));
  AnimationData()->GetParticleDB().AddParticleEffect(
      CPOINode::GetHashForString(name), 0x40,
      CParticleData(0, SObjectTag('PART', mBallSpitEffect),
                    locator == CSegId::Invalid() ? CSegId(0) : locator, 1.f,
                    CParticleData::kPM_ContinuousSystem),
      GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
}

void CSporbProjectile::CreateEscapeEffect(CStateManager& mgr) {
  char name[100];
  sprintf(name, "SPORB_TOP_EFFECT%d-%d", mBallEscapeEffect, mEffectCount++);
  const CSegId locator =
      GetAnimationData()->GetCharLayoutInfo()->GetSegIdFromString(rstl::string_l("ball_attach_LCTR"));
  AnimationData()->GetParticleDB().AddParticleEffect(
      CPOINode::GetHashForString(name), 0x40,
      CParticleData(0, SObjectTag('PART', mBallEscapeEffect),
                    locator == CSegId::Invalid() ? CSegId(0) : locator, 1.f,
                    CParticleData::kPM_ContinuousEmitter),
      GetModelData()->GetScale(), &mgr, GetCurrentAreaId(), false, 0);
}

rstl::optional_object< CAABox > CSporbProjectile::GetTouchBounds() const {
  return GetCollisionPrimitive()->CalculateAABox(GetTransform());
}

CAABox CSporbProjectile::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  if (mTopId != kInvalidUniqueId) {
    if (const CSporbTop* top = TCastToConstPtr< CSporbTop >(mgr.GetObjectById(mTopId))) {
      return top->GetScanVisorRenderBounds(mgr);
    }
  }
  return CPatterned::GetScanVisorRenderBounds(mgr);
}

void CSporbProjectile::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                       const CModelFlags& flags) const {
  if (mTopId != kInvalidUniqueId) {
    if (const CSporbTop* top = TCastToConstPtr< CSporbTop >(mgr.GetObjectById(mTopId))) {
      top->ScanVisorRender(mgr, xf, flags);
    }
  }
}

CEntity* REL_LoadSporbProjectile(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSporbProjectile sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSporbProjectile.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }
  return rs_new CSporbProjectile(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                 LdrToEntityInfo(info, sldrThis.editorProperties),
                                 LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                 LdrToPatternedInfo(sldrThis.patterned, nullptr),
                                 LdrToActorParameters(sldrThis.actorInformation),
                                 sldrThis.ballSpitParticleEffect,
                                 sldrThis.ballEscapeParticleEffect);
}

CSporbProjectile::~CSporbProjectile() {}
