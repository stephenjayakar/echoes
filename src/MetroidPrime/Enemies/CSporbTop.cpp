#include "MetroidPrime/Enemies/CSporbTop.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSporbBase.hpp"
#include "MetroidPrime/Enemies/CSporbProjectile.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSporbTop.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "Kyoto/Math/CRelAngle.hpp"

CSporbTop::CSporbTop(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& mData,
                     const CPatternedInfo& pInfo, const CActorParameters& aParms)
: CPatterned(static_cast< EPatternedAI >(0x48), uid, name, kFT_Zero, info, xf, mData, pInfo,
             kMT_Flyer, kCT_One, static_cast< EBodyType >(5), aParms)
, mState(-1)
, mOrbitPosition(GetTranslation())
, mFreezeDuration(0.f)
, mCollisionPrimitive(CSphere(pInfo.GetBodyOrigin(),
                              rstl::max_val(pInfo.GetHalfExtent(), pInfo.GetHeight() / 2.f)),
                      GetMaterialList())
, mGenerateType(3)
, mBaseId(kInvalidUniqueId)
, mProjectileId(kInvalidUniqueId)
, x800_24_(false) {
  SetDrawShadow(false);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_Player);
  include.Remove(kMT_Character);
  include.Remove(kMT_ExcludeFromRadar);
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(include, CMaterialList(kMT_Character, kMT_Player)));
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::AnimOver)},
    {"ShouldPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldPatrol)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldAttack)},
    {"ShouldFire", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldFire)},
    {"ShouldReload",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldReload)},
    {"ShouldClose", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldClose)},
    {"ShouldSpit", static_cast< CPatterned::StateMachine::TriggerFunc >(&CSporbTop::ShouldSpit)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Sleep)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Patrol)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Attack)},
    {"WakeUp", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::WakeUp)},
    {"GoToSleep", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::GoToSleep)},
    {"Fire", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Fire)},
    {"Reload", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Reload)},
    {"Close", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Close)},
    {"Spit", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Spit)},
    {"Flinch", static_cast< CPatterned::StateMachine::StateFunc >(&CSporbTop::Flinch)},
};

void CSporbTop::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

void CSporbTop::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
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
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

void CSporbTop::Think(float dt, CStateManager& mgr) { CPatterned::Think(dt, mgr); }

void CSporbTop::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

void CSporbTop::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

void CSporbTop::AddToRenderer(const CStateManager& mgr) const { CPatterned::AddToRenderer(mgr); }

void CSporbTop::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbTop::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    AddMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbTop::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbTop::WakeUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Zero, -1));
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
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

void CSporbTop::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Six, -1));
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

void CSporbTop::GoToSleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_One, -1));
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);
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

void CSporbTop::Fire(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(
        CBCGenerateCmd(static_cast< pas::EGenerateType >(mGenerateType), -1));
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(static_cast< pas::EGenerateType >(mGenerateType), -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSporbTop::Spit(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbTop::Close(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Internal6);
    break;
  case kStateMsg_Update:
  case kStateMsg_Deactivate:
    break;
  }
}

void CSporbTop::Reload(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*GetBodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mState = 4;
    break;
  }
}

bool CSporbTop::ShouldPatrol(CStateManager&, const CTriggerData&) const { return mState >= 3; }

bool CSporbTop::ShouldAttack(CStateManager&, const CTriggerData&) const { return mState >= 4; }

bool CSporbTop::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CSporbTop::ShouldFire(CStateManager&, const CTriggerData&) const { return mState >= 5; }

bool CSporbTop::ShouldReload(CStateManager&, const CTriggerData&) const { return mState >= 6; }

bool CSporbTop::ShouldClose(CStateManager&, const CTriggerData&) const { return mState == 0; }

bool CSporbTop::ShouldSpit(CStateManager&, const CTriggerData&) const { return mState == 1; }

void CSporbTop::OnBaseEvent() {}

CVector3f CSporbTop::GetOrbitPosition(const CStateManager&) const { return mOrbitPosition; }

void CSporbTop::Freeze(CStateManager& mgr, const CVector3f& position, CUnitVector3f direction,
                       float duration, float intoFreezeDuration) {
  if (!GetBodyController()->IsFrozen()) {
    mFreezeDuration = duration;
    CPatterned::Freeze(mgr, position, direction, duration, intoFreezeDuration);
  }
}

const CCollisionPrimitive* CSporbTop::GetCollisionPrimitive() const {
  return &mCollisionPrimitive;
}

rstl::optional_object< CAABox > CSporbTop::GetTouchBounds() const {
  return GetCollisionPrimitive()->CalculateAABox(GetTransform());
}

void CSporbTop::StartFlinch(CStateManager& mgr) {
  mState = 7;
  mStateMachine->SetState(mgr, *this, rstl::string_l("Flinch"));
}

void CSporbTop::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                const CModelFlags& flags) const {
  CSporbTop* self = const_cast< CSporbTop* >(this);
  const CTransform4f oldXf = GetTransform();
  const CModelFlags oldFlags = GetModelFlags();
  self->SetTransformRaw(xf);
  self->SetModelFlags(flags);
  CPatterned::Render(mgr);
  if (const CSporbProjectile* projectile =
          TCastToConstPtr< CSporbProjectile >(mgr.GetObjectById(mProjectileId))) {
    projectile->GetModelData()->Render(mgr, xf, projectile->GetActorLights(), flags);
  }
  self->SetTransformRaw(oldXf);
  self->SetModelFlags(oldFlags);

  TUniqueId scanningId = mgr.GetPlayer(0)->GetScanningObject();
  const CSporbProjectile* scanned =
      TCastToConstPtr< CSporbProjectile >(mgr.GetObjectById(scanningId));
  if (scanningId == GetUniqueId() || scanned != nullptr) {
    if (const CSporbBase* base = TCastToConstPtr< CSporbBase >(mgr.GetObjectById(mBaseId))) {
      CSegId locator =
          base->GetModelData()->GetAnimationData()->GetLocatorSegId(
              rstl::string_l(CSporbBase::skTopAttachLocator));
      CTransform4f locatorXf = base->GetScaledLocatorTransform(locator);
      base->ScanVisorRender(mgr, xf * locatorXf.GetInverse(), flags);
    }
  }
}

CAABox CSporbTop::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox box = GetModelBounds();
  if (const CSporbBase* base = TCastToConstPtr< CSporbBase >(mgr.GetObjectById(mBaseId))) {
    CTransform4f locatorXf =
        base->GetScaledLocatorTransform(rstl::string_l(CSporbBase::skTopAttachLocator));
    CTransform4f xf = CTransform4f::Translate(-base->GetTranslation()) * base->GetTransform() *
                      locatorXf;
    CAABox baseBox = base->GetModelBounds();
    box.Include(baseBox.GetTransformedAABox(CTransform4f::Translate(-xf.GetTranslation())));
    return box;
  }
  return box;
}

CAABox CSporbTop::GetModelBounds() const {
  CAABox box = GetModelData()->GetAnimationData()->CalcBoundingBoxFromModelVerts();
  box = box.GetTransformedAABox(CTransform4f::Translate(-GetTranslation()) * GetTransform() *
                                CTransform4f::Scale(GetModelData()->GetScale()));
  return box;
}

void CSporbTop::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  if (mAlive) {
    BodyController()->SetTimeScale(1.f);
    const CWeaponMode deathWeapon = GetHealthInfo()->GetCauseOfDeathWeapon();
    bool massiveDeath = true;
    if (deathWeapon.GetType() == kWT_Light || deathWeapon.GetType() == kWT_PowerBomb ||
        (deathWeapon.IsComboed() &&
         (deathWeapon.GetType() == kWT_Dark || deathWeapon.GetType() == kWT_Annihilator))) {
      massiveDeath = false;
    }
    if (massiveDeath) {
      mPendingMassiveDeath = true;
      if (mLookAtDeathDir && mXDamageDelay <= 0.f && direction.IsNonZero()) {
        const CVector3f pos = GetTranslation();
        const CVector3f target = pos - direction;
        const CTransform4f deathXf = CTransform4f::LookAt(pos, target) *
                                     CTransform4f::RotateX(CRelAngle::FromRadians(0.7853982f));
        SetTransform(deathXf);
      }
    } else {
      if (mStateMachine->HasState()) {
        mStateMachine->SetState(mgr, *this, rstl::string_l("Dead"));
      }
      RemoveMaterial(kMT_GroundCollider, mgr);
    }
    mAlive = false;
    SetHighlightedInDarkVisor(false);
    IssueDeathBodyCommand(mgr, direction);
    if (state != kSS_InvalidState) {
      SendScriptMsgs(state, mgr, GetUniqueId(), kSM_None);
    }
  }
}

CEntity* REL_LoadSporbTop(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSporbTop sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSporbTop.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }
  return rs_new CSporbTop(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                          LdrToEntityInfo(info, sldrThis.editorProperties),
                          LdrToTransform4f(sldrThis.editorProperties), *modelData,
                          LdrToPatternedInfo(sldrThis.patterned, nullptr),
                          LdrToActorParameters(sldrThis.actorInformation));
}

CSporbTop::~CSporbTop() {}
