#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Graphics/CLight.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameLight.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Cameras/CCinematicCamera.hpp"
#include "MetroidPrime/Cameras/CGameCamera.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSafeZone.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"

#include "rstl/algorithm.hpp"

CEntity* REL_LoadSafeZone(CStateManager& mgr, CInputStream& input, CEntityInfo& info);
CEntity* REL_LoadSafeZoneCrystal(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

// The REL carries its own weak copy of the base destructor and inlines it here.
inline CScriptTriggerEllipsoid::~CScriptTriggerEllipsoid() {}

CScriptSafeZone::~CScriptSafeZone() {}

void CScriptSafeZone::SetActive(const bool active) { CActor::SetActive(active); }

CLight CScriptSafeZone::BuildLight() const {
  const float radius = mActivation * GetScale().GetX();
  if (mGenerateMobileLight) {
    CLight light(CLight::BuildPoint(CVector3f::Zero(),
                                    CColor::Lerp(CColor::Black(), CColor::White(), 1.f)));
    light.SetAttenuation(0.0001f, 1.f / (5000.f * radius), 1.f / (0.5f * radius));
    return light;
  }
  CLight light(CLight::BuildHard(CVector3f::Zero(), CColor::White(), radius));
  return light;
}

void CScriptSafeZone::SetZoneType(EZoneType type) {
  if (mZoneType == type) {
    return;
  }
  mZoneType = type;
  switch (mZoneType) {
  case kZT_Normal:
    mCurrentInfo = &mNormalInfo;
    break;
  case kZT_Hurtful:
    mCurrentInfo = &mHurtfulInfo;
    break;
  case kZT_Echo:
    mCurrentInfo = &mEchoInfo;
    break;
  }
}

void CScriptSafeZone::PlaySound(CStateManager& mgr, ushort sfx, uint flags) {
  const int areaId = mgr.mNextAreaId.Value();
  ProcessSoundEvent(flags | sfx, 1.f, 0, 1.f, 75.f, CSegId(0), 0x2000, 0x2000, 0.f, 0x14, 0x7f,
                    GetDistanceToCamera(mgr), GetTranslation(), areaId, mgr, true);
}

void CScriptSafeZone::PlayDeactivateSound(CStateManager& mgr) {
  StopLoopedSounds();
  PlaySound(mgr, mCurrentInfo->x4_, 0);
}

void CScriptSafeZone::PlayActivateSound(CStateManager& mgr) {
  mLoopSoundDelay = 0.f;
  StopLoopedSounds();
  PlaySound(mgr, mCurrentInfo->x0_, 0);
}

void CScriptSafeZone::SetLowPassFilter(bool enabled) {
  if (mFilterSoundEffects) {
    if (enabled) {
      if (mLowPassFilterId == 0) {
        mLowPassFilterId = CSfxManager::AddLowPassAreaFilter(mLowPassFrequency, 0.f);
      }
    } else {
      CSfxManager::RemoveLowPassAreaFilter(mLowPassFilterId);
      mLowPassFilterId = 0;
    }
  }
}

void CScriptSafeZone::UpdateLoopSound(float dt, CStateManager& mgr) {
  if (mLoopSoundDelay >= mCurrentInfo->xc_) {
    if (!FindLoopedSound(mCurrentInfo->x2_)) {
      PlaySound(mgr, mCurrentInfo->x2_, 0x80000000);
    }
  } else {
    mLoopSoundDelay = rstl::min_val(mLoopSoundDelay + dt, mCurrentInfo->xc_);
  }
}

void CScriptSafeZone::UpdateFlash(float dt) {
  if (mFlashTimer > 0.f) {
    mFlashTimer -= dt / mFlashBrightness;
    if (mFlashTimer < 0.f) {
      mFlashTimer = 0.f;
    }
  }
}

void CScriptSafeZone::UpdateInsideAlpha(float dt) {
  if (mCameraInside) {
    mInsideTime += dt;
    if (mInsideTime < mInsideFadeStart) {
      mInsideAlpha = 1.f;
    } else if (mInsideTime < mInsideFadeTime) {
      const float t = 1.f - (mInsideTime - mInsideFadeStart) / (mInsideFadeTime - mInsideFadeStart);
      mInsideAlpha = t + (1.f - t) * mInsideFadeMinAlpha;
    } else {
      mInsideAlpha = mInsideFadeMinAlpha;
    }
  } else {
    mInsideTime = 0.f;
    mInsideAlpha = 1.f;
  }
}

void CScriptSafeZone::ApplyInsideFilter(CStateManager& mgr) {
  CCameraFilterPass& pass = mgr.CameraFilterPass(0, 10);
  if (pass.GetCurrentType() == CCameraFilterPass::kFT_Passthru) {
    pass.SetFilter(CCameraFilterPass::kFT_Blend, CCameraFilterPass::kFS_Fullscreen, 0.f,
                   mInsideFilterColor, kInvalidAssetId);
    pass.DisableFilter(mInsideFilterTime);
  }
}

void CScriptSafeZone::ApplyFog(CStateManager& mgr, const CSafeZoneFog& fog) {
  if (fog.mEnabled) {
    const TAreaId aid = GetCurrentAreaId();
    CGameArea::CAreaFog* areaFog = mgr.World()->Area(aid)->GetPostConstructed()->mAreaFog.get();
    if (fog.mMode != kRFM_None) {
      areaFog->FadeFog(fog.mMode, fog.mColor, fog.mRange, fog.mColorRate, fog.mRangeRate);
    } else {
      areaFog->RollFogOut(fog.mRangeRate.GetX(), fog.mColorRate, fog.mColor);
    }
  }
}

void CScriptSafeZone::PreRenderAllViewports(CStateManager& mgr) {
  float x = mActivation * GetScale().GetX();
  float y = mActivation * GetScale().GetY();
  float z = mActivation * GetScale().GetZ();
  if (x < mMinRadius) {
    x = mMinRadius;
  }
  if (y < mMinRadius) {
    y = mMinRadius;
  }
  if (z < mMinRadius) {
    z = mMinRadius;
  }
  const CVector3f center = GetTouchBounds()->GetCenterPoint();
  const CAABox bounds(center - CVector3f(x, y, z), center + CVector3f(x, y, z));
  SetRenderBounds(bounds);
  SetOtherBounds(bounds);
  UpdatePortalSystemState(mgr);
}

void CScriptSafeZone::InhabitantAdded(CActor& actor, CStateManager& mgr) {
  CScriptTrigger::InhabitantAdded(actor, mgr);
  CGameProjectile* proj = TCastToPtr< CGameProjectile >(actor);
  if (proj && HasInhabitant(proj->GetOwnerId())) {
    TUniqueId id = proj->GetUniqueId();
    if (rstl::find(mProjectiles.begin(), mProjectiles.end(), id) == mProjectiles.end()) {
      mProjectiles.push_back(id);
    }
  } else {
    HandleProjectile(actor, mgr);
  }
  mgr.SendScriptMsg(&actor, GetUniqueId(), kSM_XENZ, kInvalidUniqueId);
  UpdatePlayerInside(actor, true, mgr);
}

void CScriptSafeZone::InhabitantExited(CActor& actor, CStateManager& mgr) {
  CScriptTrigger::InhabitantExited(actor, mgr);
  HandleProjectile(actor, mgr);
  mgr.SendScriptMsg(&actor, GetUniqueId(), kSM_XEXZ, kInvalidUniqueId);
  UpdatePlayerInside(actor, false, mgr);
}

void CScriptSafeZone::UpdatePlayerInside(CActor& actor, bool inside, CStateManager& mgr) {
  if (TCastToPtr< CGameCamera >(actor) &&
      TCastToPtr< CGameCamera >(actor)->CameraManager(mgr).GetCurrentCameraId(true) ==
          actor.GetUniqueId()) {
    mCameraInside = inside;
    if (!mIgnoreCinematicCamera || !TCastToPtr< CCinematicCamera >(actor)) {
      if (mCameraInside) {
        PlaySound(mgr, mCurrentInfo->x6_, 0x40000000);
      } else {
        PlaySound(mgr, mCurrentInfo->x8_, 0x40000000);
      }
    } else {
      mFogDirty = true;
    }
  }
}

void CScriptSafeZone::AddToRenderer(const CStateManager& mgr) const {
  if (GetActive()) {
    CScriptTriggerEllipsoid::AddToRenderer(mgr);
    EnsureRendered(mgr);
  }
  for (rstl::list< rstl::auto_ptr< CElementGen > >::const_iterator it = mImpactGens.begin();
       it != mImpactGens.end(); ++it) {
    gpRender->AddParticleGen(**it);
  }
}

bool CScriptSafeZone::ShouldSendScriptMsgs(CActor& actor, CStateManager& mgr) const {
  return TCastToPtr< CPlayer >(actor) != nullptr;
}

void CScriptSafeZone::Render(const CStateManager& mgr) const {
  if (mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
    RenderDarkVisorSpot(mgr);
  }
  CScriptTriggerEllipsoid::Render(mgr);
}

void CScriptSafeZone::InhabitantIdle(CActor& actor, CStateManager& mgr, float dt) {
  CScriptTrigger::InhabitantIdle(actor, mgr, dt);
  if (IsAI(mgr, actor)) {
    ApplyDamageTo(mgr, dt, actor.GetUniqueId());
  }
}

void CScriptSafeZone::ApplyDamageTo(CStateManager& mgr, float dt, TUniqueId id) {
  CDamageInfo damage = mZoneType == kZT_Normal ? CDamageInfo(mNormalDamage, dt)
                                               : CDamageInfo(mHurtfulDamage, dt);
  mgr.ApplyDamage(GetUniqueId(), id, GetUniqueId(), damage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), CMaterialList()),
                  CVector3f::Zero());
}

void CScriptSafeZone::UpdateObstruction(CStateManager& mgr, bool enable) {
  if (mObstructionType != -1) {
    ModifyObstruction(mgr, -1, mObstructionType);
    mObstructionType = -1;
  }
  if (enable) {
    mObstructionRadius = GetScale().GetX();
    mObstructionPos = GetTranslation();
    mObstructionType = mZoneType;
    ModifyObstruction(mgr, 1, mObstructionType);
  }
}

void CScriptSafeZone::ModifyObstruction(CStateManager& mgr, int delta, int type) {
  const float radiusSq = mObstructionRadius * mObstructionRadius;
  CPFArea* area = mgr.World()->Area(GetCurrentAreaId())->GetPostConstructed()->mPathArea;
  if (area != nullptr) {
    const CTransform4f& transform = area->GetTransform();
    const CVector3f point =
        transform.TransposeRotate(mObstructionPos - transform.GetTranslation());
    for (int i = 0; i < area->GetNumRegions(); ++i) {
      CPFRegion* region = area->GetRegionPtr(i);
      if ((region->GetCentroid() - point).MagSquared() <= radiusSq) {
        region->ModifyObstructionCount(static_cast< EPathFindObstructions >(type != 0), delta);
      }
    }
    mgr.InformListeners(mObstructionPos, static_cast< EListenNoiseType >(4));
  }
}

bool CScriptSafeZone::IsHurtful() const {
  return mZoneType == kZT_Hurtful || mZoneType == kZT_Echo;
}

SSafeZone_FuncPtrs REL_loader_SafeZone;

void SetRelLoaderFunctionToLoader() {
  REL_loader_SafeZone.mLoadSafeZone = REL_LoadSafeZone;
  REL_loader_SafeZone.mLoadSafeZoneCrystal = REL_LoadSafeZoneCrystal;
  REL_loader_SafeZone.mApplyRenderEffect =
      static_cast< void (CEntity::*)(CStateManager&) >(&CScriptSafeZone::ApplyRenderEffect);
  SetSSafeZone_FuncPtrs(&REL_loader_SafeZone);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSSafeZone_FuncPtrs(nullptr); }
