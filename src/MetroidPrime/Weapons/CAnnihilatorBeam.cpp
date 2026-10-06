#include "MetroidPrime/Weapons/CAnnihilatorBeam.hpp"

#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"

CAnnihilatorBeam::CAnnihilatorBeam(TUniqueId playerId, const CVector3f& scale, int flags)
: CGunWeapon(kWT_Annihilator, playerId, scale, flags)
, mShotDelayTimer(0.f)
, mShotDelay(0.f)
, mLightingResetDelayTimer(0.f)
, mProjectileSpeed(0.f)
, mProjectileTurnRate(0.f)
, mLightingArea(kInvalidAreaId)
, mEffectLoaded(false)
, mWorldLightingDimmed(false) {}

CAnnihilatorBeam::~CAnnihilatorBeam() {}

void CAnnihilatorBeam::ReInitVariables() {
  mChargeGenerator = nullptr;
  mEffectLoaded = false;
  mEnabledSecondaryEffect = kSFT_None;
}

void CAnnihilatorBeam::PostRenderGunFx(const CStateManager& mgr, const CTransform4f& xf) {
  if (mChargeGenerator.get() && mEnabledSecondaryEffect != kSFT_None) {
    mChargeGenerator->Render();
  }
  CGunWeapon::PostRenderGunFx(mgr, xf);
}

void CAnnihilatorBeam::UpdateGunFx(bool shotSmoke, float dt, const CStateManager& mgr,
                                   const CTransform4f& xf) {}

void CAnnihilatorBeam::Update(float dt, CStateManager& mgr) {}

void CAnnihilatorBeam::Fire(const TCachedToken< CWeaponDescription >& projectile, bool underwater,
                            float dt, CPlayerState::EChargeStage chargeState,
                            const CTransform4f& xf, CStateManager& mgr, TUniqueId homingTarget,
                            uint projectileAttributes, ushort soundId, TUniqueId* projectileId,
                            CSfxHandle* soundHandle, float chargeFactor1, float chargeFactor2) {}

void CAnnihilatorBeam::Load(CStateManager& mgr, bool subtypeBasePose) {
  CGunWeapon::Load(mgr, subtypeBasePose);
  mChargeEffect->Lock();
}

void CAnnihilatorBeam::Unload(CStateManager& mgr) {
  CGunWeapon::Unload(mgr);
  if (!mgr.IsMultiplayer()) {
    mChargeEffect->Unlock();
  }
  ResetWorldLighting(mgr);
  ReInitVariables();
}

void CAnnihilatorBeam::ReleaseResources(CStateManager& mgr) {
  CGunWeapon::ReleaseResources(mgr);
  if (!mgr.IsMultiplayer()) {
    mChargeEffect->Unlock();
  }
  ResetWorldLighting(mgr);
  mChargeGenerator = nullptr;
  mEnabledSecondaryEffect = kSFT_None;
}

bool CAnnihilatorBeam::IsLoaded() const {
  bool ret = false;
  if (CGunWeapon::IsLoaded() && mEffectLoaded) {
    ret = true;
  }
  return ret;
}

void CAnnihilatorBeam::EnableSecondaryFx(ESecondaryFxType type) {}

void CAnnihilatorBeam::InitializeResources(CStateManager& mgr) {}

void CAnnihilatorBeam::SetWorldLighting(CStateManager& mgr, TAreaId areaId, float speed,
                                        float target) {
  if (mWorldLightingDimmed && mLightingArea != areaId && mLightingArea != kInvalidAreaId) {
    CGameArea* area = mgr.World()->Area(mLightingArea);
    if (area->IsLoaded()) {
      area->SetWeaponWorldLighting(2.f, 1.f);
    }
  }
  mLightingArea = areaId;
  mWorldLightingDimmed = target != 1.f;
  if (mLightingArea != kInvalidAreaId) {
    CGameArea* area = mgr.World()->Area(mLightingArea);
    if (area->IsLoaded()) {
      area->SetWeaponWorldLighting(speed, target);
    }
  }
}

void CAnnihilatorBeam::ResetWorldLighting(CStateManager& mgr) {
  if (mWorldLightingDimmed && !mgr.IsMultiplayer()) {
    SetWorldLighting(mgr, GetPlayer(mgr)->GetCurrentAreaId(), 2.f, 1.f);
  }
}

void CAnnihilatorBeam::FireProjectile(const TCachedToken< CWeaponDescription >& projectile,
                                      bool underwater, float dt,
                                      CPlayerState::EChargeStage chargeState,
                                      const CTransform4f& xf, CStateManager& mgr,
                                      TUniqueId homingTarget, uint projectileAttributes,
                                      ushort soundId, float damageFactor, float projectileScale,
                                      float projectileFactor) {}
