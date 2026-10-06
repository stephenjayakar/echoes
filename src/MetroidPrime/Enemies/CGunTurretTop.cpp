#include "MetroidPrime/Enemies/CGunTurretTop.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CGameLight.hpp"
#include "MetroidPrime/CParticleDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Cameras/CGameCamera.hpp"
#include "MetroidPrime/Enemies/CGunTurretBase.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGunTurretTop.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Graphics/CLight.hpp"

CGunTurretTop::CGunTurretTop(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                             const CTransform4f& xf, const CModelData& modelData,
                             const CPatternedInfo& patternedInfo,
                             const CActorParameters& actorParameters, float powerUpTime,
                             float powerDownTime, CAssetId gfChargeEffect,
                             CAssetId pirateChargeEffect, const CColor& lightColor,
                             ushort powerUpSfx, ushort powerDownSfx)
: CPatterned(static_cast< EPatternedAI >(0x44), uid, name, kFT_Zero, info, xf, modelData,
             patternedInfo, kMT_Flyer, kCT_One, static_cast< EBodyType >(5), actorParameters)
, mBaseId(kInvalidUniqueId)
, mState(kS_Sleep)
, mHealthInfo(patternedInfo.GetHealthInfo())
, mShouldAttack(false)
, mPowerUpTime(powerUpTime)
, mPowerDownTime(powerDownTime)
, mStateTime(0.f)
, mAdditiveAnim(0)
, mTarget(CVector3f::Zero())
, mShouldPatrol(false)
, x809_(false)
, mGFChargeEffect(gfChargeEffect)
, mPirateChargeEffect(pirateChargeEffect)
, mLightId(kInvalidUniqueId)
, mLightColor(lightColor)
, mPowerUpSfx(powerUpSfx)
, mPowerDownSfx(powerDownSfx)
, mSfxFallOff(0.1f)
, mSfxMaxDistance(1000.f) {
  mKnockBackController.EnableKnockBackPhysics(false);
  SetDrawShadow(false);

  rstl::pair< float, int > best = GetAnimationData()->GetPASDatabase().FindBestAnimation(
      CPASAnimParmData(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(5), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                       CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter()),
      -1);
  if (best.first > FLT_EPSILON) {
    mAdditiveAnim = best.second;
  }

  rstl::vector< CAssetId > parts;
  parts.reserve(2);
  if (mGFChargeEffect != kInvalidAssetId) {
    parts.push_back(mGFChargeEffect);
  }
  if (mPirateChargeEffect != kInvalidAssetId) {
    parts.push_back(mPirateChargeEffect);
  }
  AnimationData()->GetParticleDB().CacheParticleDesc(CCharacterInfo::CParticleResData(
      parts, rstl::vector< CAssetId >(), rstl::vector< CAssetId >(), rstl::vector< CAssetId >(),
      rstl::vector< CAssetId >(), rstl::vector< CAssetId >()));
}

void CGunTurretTop::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_AIUpdateDisabled:
  case kSM_XHIT:
  case kSM_Decrement:
  case kSM_Alert:
  case kSM_Activate:
    break;
  case kSM_AreaLoaded: {
    rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
    for (; it != GetConnectionList().end(); ++it) {
      if (it->state == kSS_APRC) {
        TUniqueId id = mgr.GetIdForScript(it->objId);
        if (CGunTurretBase* base = TCastToPtr< CGunTurretBase >(mgr.ObjectById(id))) {
          mBaseId = id;
          mSfxFallOff = base->GetSfxFallOff();
          mSfxMaxDistance = base->GetSfxMaxDistance();
        }
      }
    }
    break;
  }
  case kSM_Create: {
    if (!BodyController()->GetIsActive()) {
      BodyController()->SetLocomotionType(pas::kLT_Internal7);
      BodyController()->Activate(mgr, pas::kAS_Invalid);
    }
    TUniqueId lightId = mgr.AllocateUniqueId();
    rstl::string lightName("");
    CGameLight* light = rs_new CGameLight(lightId, GetCurrentAreaId(), false, lightName,
                                          GetTransform(), GetUniqueId(),
                                          CLight::BuildPoint(CVector3f::Zero(), mLightColor), 0,
                                          0, 0.f);
    light->SetActive(false);
    mgr.AddObject(light);
    mLightId = lightId;
    AddMaterial(kMT_Scannable, mgr);
    break;
  }
  case kSM_Delete:
    mgr.DeleteObjectRequest(mLightId);
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::ShouldAttack)},
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::AnimOver)},
    {"Attacked", static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::Attacked)},
    {"PowerUpOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::PowerUpOver)},
    {"PowerDownOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::PowerDownOver)},
    {"ShouldPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CGunTurretTop::ShouldPatrol)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Sleep", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretTop::Sleep)},
    {"PowerUp", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretTop::PowerUp)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretTop::Attack)},
    {"PowerDown", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretTop::PowerDown)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CGunTurretTop::Patrol)},
};

void CGunTurretTop::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

bool CGunTurretTop::ShouldAttack(CStateManager&, const CTriggerData&) const {
  return mShouldAttack;
}

bool CGunTurretTop::ShouldPatrol(CStateManager&, const CTriggerData&) const {
  return mShouldPatrol;
}

bool CGunTurretTop::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Attacked(mgr, data);
}

bool CGunTurretTop::PowerUpOver(CStateManager&, const CTriggerData&) const {
  return mStateMachine->GetTime() > mPowerUpTime;
}

bool CGunTurretTop::PowerDownOver(CStateManager&, const CTriggerData&) const {
  return mStateMachine->GetTime() > mPowerDownTime;
}

bool CGunTurretTop::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

void CGunTurretTop::Sleep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_Sleep;
    BodyController()->SetLocomotionType(pas::kLT_Internal7);
    if (CGameLight* light = TCastToPtr< CGameLight >(mgr.ObjectById(mLightId))) {
      light->SetActive(false);
    }
    AnimationData()->DelAdditiveAnimation(mAdditiveAnim);
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretTop::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_Patrol;
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    AnimationData()->DelAdditiveAnimation(mAdditiveAnim);
    if (CGameLight* light = TCastToPtr< CGameLight >(mgr.ObjectById(mLightId))) {
      light->SetActive(true);
    }
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretTop::PowerUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_PowerUp;
    mStateTime = 0.f;
    ProcessSoundEvent(mPowerUpSfx, 1.f, 0, mSfxFallOff, mSfxMaxDistance, CSegId(), 0, 0, 0.f,
                      20, 127, GetClosestCameraDistanceSq(mgr), GetTranslation(),
                      mgr.GetNextAreaId().Value(), mgr, true);
    break;
  case kStateMsg_Update: {
    mStateTime += dt;
    float t = mStateTime / mPowerUpTime;
    AnimationData()->AddAdditiveAnimation(mAdditiveAnim, t < 1.f ? t : 1.f, true, false);
    break;
  }
  }
}

void CGunTurretTop::PowerDown(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_PowerDown;
    mStateTime = 0.f;
    StopLoopedSounds();
    ProcessSoundEvent(mPowerDownSfx, 1.f, 0, mSfxFallOff, mSfxMaxDistance, CSegId(), 0, 0, 0.f,
                      20, 127, GetClosestCameraDistanceSq(mgr), GetTranslation(),
                      mgr.GetNextAreaId().Value(), mgr, true);
    break;
  case kStateMsg_Update: {
    mStateTime += dt;
    float t = mStateTime / mPowerDownTime;
    AnimationData()->AddAdditiveAnimation(mAdditiveAnim, 1.f - (t < 1.f ? t : 1.f), true, false);
    break;
  }
  }
}

void CGunTurretTop::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kS_Attack;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    break;
  }
}

void CGunTurretTop::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

void CGunTurretTop::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

void CGunTurretTop::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                    const CModelFlags& flags) const {
  const CModelData* modelData = GetModelData();
  if (!modelData->IsNull()) {
    modelData->Render(CModelData::kWM_Normal, xf, nullptr, flags);
  }
  if (mgr.GetPlayer(0)->GetOrbitTargetId() == GetUniqueId()) {
    if (const CGunTurretBase* base =
            TCastToConstPtr< CGunTurretBase >(mgr.GetObjectById(mBaseId))) {
      CSegId seg = base->GetAnimationData()->GetLocatorSegId(CGunTurretBase::skConnectLocator);
      CTransform4f locXf = base->GetScaledLocatorTransform(seg);
      base->ScanVisorRender(mgr, xf * locXf.GetInverse(), flags);
    }
  }
}

CAABox CGunTurretTop::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  CAABox bounds = GetModelBounds();
  if (const CGunTurretBase* base = TCastToConstPtr< CGunTurretBase >(mgr.GetObjectById(mBaseId))) {
    CTransform4f locXf = base->GetScaledLocatorTransform(CGunTurretBase::skConnectLocator);
    CTransform4f xf = GetTransform() * CTransform4f::Translate(-base->GetTranslation()) * locXf;
    CAABox baseBounds = base->GetModelBounds();
    CAABox box = baseBounds.GetTransformedAABox(CTransform4f::Translate(-locXf.GetTranslation()));
    bounds.AccumulateBounds(box.GetMinPoint());
    bounds.AccumulateBounds(box.GetMaxPoint());
  }
  return bounds;
}

CAABox CGunTurretTop::GetModelBounds() const {
  CAABox box = GetAnimationData()->CalcBoundingBoxFromModelVerts();
  return box.GetTransformedAABox(GetTransform() *
                                 CTransform4f::Translate(-GetTranslation()) *
                                 CTransform4f::Scale(GetModelData()->GetScale()));
}

void CGunTurretTop::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  if (CGameLight* light = TCastToPtr< CGameLight >(mgr.ObjectById(mLightId))) {
    light->SetTransform(GetTransform() * GetScaledLocatorTransform("light_LCTR"));
  }

  CPatterned::Think(dt, mgr);

  if (mHitByPlayerProjectile) {
    if (CGunTurretBase* base = TCastToPtr< CGunTurretBase >(mgr.ObjectById(mBaseId))) {
      base->SetGunHit(true);
    }
    mHitByPlayerProjectile = false;
  }
}

void CGunTurretTop::Touch(CActor& actor, CStateManager& mgr) { CPatterned::Touch(actor, mgr); }

void CGunTurretTop::Death(CStateManager& mgr, const CVector3f& direction,
                          EScriptObjectState state) {
  AnimationData()->SetEffectState("Generator", false, mgr);
  SetActive(false);
  mHitByPlayerProjectile = false;
  RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  if (CGunTurretBase* base = TCastToPtr< CGunTurretBase >(mgr.ObjectById(mBaseId))) {
    base->DestroyGun(mgr);
  }
  StopLoopedSounds();
  AnimationData()->GetParticleDB().DestroyAllActiveParticles();
  AnimationData()->GetParticleDB().ClearAllNonPersistentEffects(&mgr);
  SendScriptMsgs(state, mgr);
}

void CGunTurretTop::Revive(CStateManager& mgr) {
  mStateMachine->SetState(mgr, *this, "Start");
  SetActive(true);
  *HealthInfo() = mHealthInfo;
  mHitByPlayerProjectile = false;
  mDamageCooldownTimer = 0.f;
  SetModelFlags(CModelFlags(CModelFlags::kT_Opaque, 1.f));
  mAlphaDelta = 0.f;
  mColor = CColor(0.f, 0.f, 0.f, 1.f);
  BodyController()->DouseElectrocuting();
  BodyController()->DouseFlames();
  BodyController()->UnFreeze();
}

float CGunTurretTop::GetClosestCameraDistanceSq(const CStateManager& mgr) const {
  float closest = FLT_MAX;
  const CVector3f& pos = GetTranslation();
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CGameCamera* camera = mgr.GetCameraManager(i)->GetCurrentCamera(mgr, true);
    float distSq = (camera->GetTranslation() - pos).MagSquared();
    if (distSq < closest) {
      closest = distSq;
    }
  }
  return closest;
}

void CGunTurretTop::SetChargeEffect(CStateManager& mgr, bool active, bool pirate) {
  AnimationData()->SetEffectState(pirate ? rstl::string_l("PirateCharge")
                                         : rstl::string_l("GFCharge"),
                                  active, mgr);
}

CEntity* LoadGunTurretTop(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrGunTurretTop sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrGunTurretTop.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CGunTurretTop(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToPatternedInfo(sldrThis.patterned, nullptr),
      LdrToActorParameters(sldrThis.actorInformation), sldrThis.powerUpTime,
      sldrThis.powerDownTime, sldrThis.pART, sldrThis.pART_0xaf6e671a, sldrThis.lightColor,
      sldrThis.sound, sldrThis.sound_0x5d9ed447);
}

rstl::optional_object< CAABox > CGunTurretTop::GetTouchBounds() const {
  return GetBoundingBox();
}

CGunTurretTop::~CGunTurretTop() {}
