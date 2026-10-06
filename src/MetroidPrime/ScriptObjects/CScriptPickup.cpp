#include "MetroidPrime/ScriptObjects/CScriptPickup.hpp"

#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAnimPlaybackParms.hpp"
// #include "MetroidPrime/CArtifactDoll.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/SEchoParameters.hpp"

// #include "MetroidPrime/Cameras/CCameraManager.hpp"
// #include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
// #include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/HUD/CSamusHud.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Tweaks/CTweakGame.hpp"

#include "MetroidPrime/HUD/CHUDMemoParms.hpp"
// #include "MetroidPrime/HUD/CSamusHud.hpp"

#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrPickup.hpp"

#include "MetroidPrime/Player/CEnvironmentVariable.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Math/CAbsAngle.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Text/CStringTable.hpp"

#include "rstl/math.hpp"

static float skDrawInDistance = 30.f;
static bool sPickupSfxOwnerInit;                     // Guessed name.
static TUniqueId sPickupSfxOwner = kInvalidUniqueId; // Guessed name.

CScriptPickup::CScriptPickup(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                             const CTransform4f& xf, const CModelData& modelData,
                             const CActorParameters& aParams, const SEchoParameters& echo,
                             const CAABox& aabb, CPlayerState::EItemType itemType, int amount,
                             int capacityIncrease, int itemPercentageIncrease,
                             CAssetId pickupEffect, bool absoluteValue, bool unknown, bool autoSpin,
                             bool blinkOut, float lifeTime, float respawnTime, float fadeTime,
                             float activateDelay, float pickupEffectLifetime, float autoHomeRange,
                             float delayUntilHome, float homingSpeed, const CVector3f& orbitOffset)
: CActor(uid, name, info, 0, xf, modelData, CMaterialList(), aParams, kInvalidUniqueId)
, mItemType(itemType)
, mAmount(amount)
, mCapacity(capacityIncrease)
, mPercentageIncrease(itemPercentageIncrease)
, mLifeTime(lifeTime)
, mRespawnTime(respawnTime)
, x170(0.f)
, mFadeTime(fadeTime)
, mCurTime(0.0f)
, mPickupEffectLifetime(pickupEffectLifetime)
, mActivateDelay(activateDelay)
, mAutoHomeRange(autoHomeRange)
, mDelayUntilHome(delayUntilHome)
, mHomingSpeed(homingSpeed)
, mTransformZ(xf.GetTranslation().GetZ())
, mPickupParticleDesc()
, mTouchBounds(aabb)
, x1bc(0)
, x1c0(0)
, mOrbitOffset(orbitOffset)
, mUnknownProp(unknown)
, mGenerated(false)
, mInTractor(false)
, mAbsoluteValue(absoluteValue)
, mEnableTractorTest(false)
, mAutoSpin(autoSpin)
, mUnk2(true)
, mUnk3(false)
, mBlinkOut(blinkOut) {
  if (!sPickupSfxOwnerInit) {
    sPickupSfxOwnerInit = true;
    sPickupSfxOwner = kInvalidUniqueId;
  }

  if (pickupEffect != kInvalidAssetId) {
    mPickupParticleDesc = gpSimplePool->GetObj(SObjectTag('PART', pickupEffect));
    mPickupParticleDesc->Lock();
  }

  if (HasAnimation()) {
    AnimationData()->SetAnimation(CAnimPlaybackParms(0, -1, 1.f, true), false);
  }

  if (mFadeTime) {
    CModelFlags flags = CModelFlags::AlphaBlended(0.f);
    SetModelFlags(flags.DepthCompareUpdate(true, false));
  }

  AllocateEchoEmitter(true, CAABox(GetTranslation(), GetTranslation()), echo);
}

CScriptPickup::~CScriptPickup() {}

void CScriptPickup::PreRenderAllViewports(CStateManager& mgr) {
  CActor::PreRenderAllViewports(mgr);
  if (mUnk2) {
    mUnk2 = false;
    x1bc = 0;
  } else {
    x1bc += 1;
  }
}

void CScriptPickup::PreRender(CStateManager& mgr) {
  CActor::PreRender(mgr);
  if (!GetPreRenderClipped()) {
    mUnk2 = true;
  }
}

bool CScriptPickup::IsVisible() const {
  if (mActivateDelay >= 0.0f) {
    return false;
  }
  return !(x170 > 0.0f);
}

void CScriptPickup::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (!GetActive()) {
    return;
  }

  // if (mDelayTimer >= 0.f) {
  //   // CActor::Stop();
  //   mDelayTimer -= dt;
  //   return;
  // }

  // x270_curTime += dt;
  // if (x28c_25_inTractor && (x26c_lifeTime - x270_curTime) < 2.f) {
  //   x270_curTime = rstl::max_val(x26c_lifeTime - 2.f - FLT_EPSILON, x270_curTime - 2.f * dt);
  // }

  // CModelFlags drawFlags = CModelFlags::Normal();

  // if (x268_fadeInTime) {
  //   if (x270_curTime < x268_fadeInTime) {
  //     drawFlags =
  //         CModelFlags::AlphaBlended(x270_curTime / x268_fadeInTime).DepthCompareUpdate(true,
  //         false);
  //   } else {
  //     x268_fadeInTime = 0.f;
  //   }
  // } else if (x26c_lifeTime) {
  //   float alpha = 1.f;
  //   if (x26c_lifeTime < 2.f) {
  //     alpha = 1.f - (x26c_lifeTime / x270_curTime);
  //   } else if ((x26c_lifeTime - x270_curTime) < 2.f) {
  //     alpha = (x26c_lifeTime - x270_curTime) / 2.f;
  //   }

  //   drawFlags = CModelFlags::AlphaBlended(alpha).DepthCompareUpdate(true, false);
  // }

  // SetModelFlags(drawFlags);

  // if (HasAnimation()) {
  //   CAdvancementDeltas deltas = UpdateAnimation(dt, m_gr, true);
  //   MoveToOR(deltas.GetOffsetDelta(), dt);
  //   RotateToOR(deltas.GetOrientationDelta(), dt);
  // }

  // if (x28c_25_inTractor) {
  //   CVector3f velocity =
  //       mgr.GetPlayer()->GetTranslation() + (CVector3f::Up() * 2.f) - GetTranslation();
  //   x274_tractorTime += dt;
  //   float halfTractorTime = rstl::min_val(x274_tractorTime, 2.f) * 0.5f;
  //   velocity = velocity.AsNormalized() * (halfTractorTime * 20.f);
  //   if (x28c_26_enableTractorTest && mgr.GetPlayer()->GetPlayerGun()->GetChargeBeamFactor() <
  //                                        CPlayerGun::GetTractorBeamFactor()) {
  //     x28c_26_enableTractorTest = false;
  //     x28c_25_inTractor = false;
  //     velocity = CVector3f::Zero();
  //   }
  //   SetVelocityWR(velocity);
  // } else if (x28c_24_generated) {
  //   if (mgr.GetPlayer()->GetPlayerGun()->GetChargeBeamFactor() >
  //       CPlayerGun::GetTractorBeamFactor()) {
  //     const CFirstPersonCamera* camera = mgr.CameraManager()->FirstPersonCamera();
  //     CVector3f posDelta = GetTranslation() - camera->GetTranslation();
  //     CVector3f cameraFront = camera->GetTransform().GetColumn(kDY);
  //     float dot = CVector3f::Dot(cameraFront, posDelta.AsNormalized());
  //     float fovCos = cosine(CAbsAngle::FromDegrees(gpTweakGame->GetFirstPersonFOV()));
  //     if (dot > fovCos && posDelta.MagSquared() < skDrawInDistance * skDrawInDistance) {
  //       x28c_25_inTractor = true;
  //       x28c_26_enableTractorTest = true;
  //       x274_tractorTime = 0.f;
  //     }
  //   }
  // }

  // if (x26c_lifeTime && x270_curTime > x26c_lifeTime) {
  //   mgr.FreeScriptObject(GetUniqueId());
  // }
}

void CScriptPickup::Touch(CActor& act, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  if (!IsVisible()) {
    return;
  }
  if (CPlayer* player = TCastToPtr< CPlayer >(act)) {
    const TUniqueId playerId = player->GetUniqueId();
    int playerIndex = mgr.MaskUIdNumPlayers(playerId);
    CPlayerState* playerState = mgr.PlayerState(playerIndex);
    if (!playerState->IsPlayerAlive())
      return;

    CPlayerState::EItemType itemType = mItemType;

    if (mPickupParticleDesc) {
      mgr.AddObject(rs_new CExplosion(
          TLockedToken< CGenDescription >(*mPickupParticleDesc), mgr.AllocateUniqueId(),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
          rstl::string_l("Explosion - Pickup Effect"), GetTransform(), 0,
          CVector3f(1.f, 1.f, 1.f), CColor::White(), -1));
    }

    int previousAmount = playerState->GetItemAmount(itemType);
    playerState->AddPowerUp(CPlayerState::kIT_ItemPercentage, mPercentageIncrease);
    playerState->IncrPickUp(CPlayerState::kIT_ItemPercentage, mPercentageIncrease);
    if (!mAbsoluteValue) {
      playerState->AddPowerUp(itemType, mCapacity);
      playerState->IncrPickUp(itemType, mAmount);
    } else {
      playerState->ReInitializePowerUp(itemType, mCapacity);
      playerState->SetItemAmount(itemType, mAmount);
    }
    playerState->SetTimeLeft(itemType, mPickupEffectLifetime);
    SendScriptMsgs(kSS_Arrived, mgr, playerId, kSM_None);
    if (mRespawnTime == 0.0f) {
      mEnableTractorTest = true;
      mgr.DeleteObjectRequest(GetUniqueId());
    } else {
      SetModelFlags(CModelFlags::AlphaBlended(0.f).DepthCompareUpdate(true, false));
      x170 = mRespawnTime;
      mCurTime = 0.f;
      mFadeTime = 0.25f;
    }

    if (playerState->GetItemAmount(itemType) > previousAmount) {
      ShowAllKeysCollectedAlert(mgr, playerState, itemType);
    }

    if (mPercentageIncrease > 0) {
      int total = playerState->GetTotalPickupCount();
      int colRate = playerState->CalculateItemCollectionRate();
      if (colRate == total) {
        CAssetId id =
            gpResourceFactory
                ->GetResourceIdByName("STRG_AllPickupsFound_2")
                ->id;
              
        mgr.QueueMessage(mgr.GetHUDMessageFrameCount() + 1, id, 0.f);
        gpGameState->SystemOptions().FindEnvironmentVariable("AllPickupsFound")->Set(1);
      }
    }

    if (!mgr.IsMultiplayer() && itemType == CPlayerState::kIT_Powerbomb && mCapacity == 0) {
      CPersistentOptions& opts = gpGameState->SystemOptions();
      if (opts.FindEnvironmentVariable("PowerbombPickupMessages")->GetValue() == 0) {
        opts.FindEnvironmentVariable("PowerbombPickupMessages")->Set(1);
        CSamusHud::DisplayHudMemo(
          rstl::wstring_l(gpStringTable->GetString("FirstPowerBombPickup")),
          CHUDMemoParms(5.f, true, false, false, 1 << playerIndex, true)
        );
      }
    }
    switch (itemType) {
      case CPlayerState::kIT_SwitchVisorCombat:
        playerState->StartTransitionToVisor(CPlayerState::kPV_Combat);
        break;
      case CPlayerState::kIT_SwitchVisorScan:
        playerState->StartTransitionToVisor(CPlayerState::kPV_Scan);
        break;
      case CPlayerState::kIT_SwitchVisorDark:
        playerState->StartTransitionToVisor(CPlayerState::kPV_Dark);
        break;
      case CPlayerState::kIT_SwitchVisorEcho:
        playerState->StartTransitionToVisor(CPlayerState::kPV_Echo);
        break;
    }
  }
}

rstl::optional_object< CAABox > CScriptPickup::GetTouchBounds() const {
  const CVector3f off = GetTranslation();
  return CAABox(mTouchBounds.GetMinPoint() + off, mTouchBounds.GetMaxPoint() + off);
}

void CScriptPickup::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Activate:
    mTransformZ = GetTranslation().GetZ();
    break;
  case kSM_Create:
    if (mgr.GetScriptObjectLoaderHelper().IsGeneratingObject()) {
      mUnk3 = true;
    }
    break;
  case kSM_Delete:
    if (!mEnableTractorTest) {
      SendScriptMsgs(kSS_Dead, mgr, kSM_None);
    }
    sPickupSfxOwner = kInvalidUniqueId;
    break;
  default:
    break;
  }
  CActor::AcceptScriptMsg(mgr, msg);
}

void CScriptPickup::AddToRenderer(const CStateManager& mgr) const {
  if (IsVisible()) {
    CActor::AddToRenderer(mgr);
  }
}

CPlayerState::EItemType CScriptPickup::GetItem() const { return mItemType; }

void CScriptPickup::SetSpawned(CStateManager& mgr) {
  if (!mgr.IsMultiplayer()) {
    mUnknownProp = true;
  }
  mUnk3 = true;
}

CVector3f CScriptPickup::GetOrbitPosition(const CStateManager&) const {
  return GetTranslation() + GetTransform().Rotate(mOrbitOffset);
}

void CScriptPickup::ShowAllKeysCollectedAlert(CStateManager& mgr, CPlayerState* playerState,
                                              CPlayerState::EItemType itemType) {
  const char* message = nullptr;
  switch (itemType) {
  case CPlayerState::kIT_TempleKey1:
  case CPlayerState::kIT_TempleKey2:
  case CPlayerState::kIT_TempleKey3:
  case CPlayerState::kIT_TempleKey4:
  case CPlayerState::kIT_TempleKey5:
  case CPlayerState::kIT_TempleKey6:
  case CPlayerState::kIT_TempleKey7:
  case CPlayerState::kIT_TempleKey8:
  case CPlayerState::kIT_TempleKey9:
    if (playerState->GetItemAmount(CPlayerState::kIT_TempleKey1, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey2, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey3, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey4, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey5, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey6, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey7, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey8, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TempleKey9, true) > 0) {
      message = "STRG_AllTempleKeysFound";
    }
    break;
  case CPlayerState::kIT_AgonKey1:
  case CPlayerState::kIT_AgonKey2:
  case CPlayerState::kIT_AgonKey3:
    if (playerState->GetItemAmount(CPlayerState::kIT_AgonKey1, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_AgonKey2, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_AgonKey3, true) > 0) {
      message = "STRG_AllSandKeysFound";
    }
    break;
  case CPlayerState::kIT_TorvusKey1:
  case CPlayerState::kIT_TorvusKey2:
  case CPlayerState::kIT_TorvusKey3:
    if (playerState->GetItemAmount(CPlayerState::kIT_TorvusKey1, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TorvusKey2, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_TorvusKey3, true) > 0) {
      message = "STRG_AllSwampKeysFound";
    }
    break;
  case CPlayerState::kIT_HiveKey1:
  case CPlayerState::kIT_HiveKey2:
  case CPlayerState::kIT_HiveKey3:
    if (playerState->GetItemAmount(CPlayerState::kIT_HiveKey1, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_HiveKey2, true) > 0 &&
        playerState->GetItemAmount(CPlayerState::kIT_HiveKey3, true) > 0) {
      message = "STRG_AllCliffsKeysFound";
    }
    break;
  default:
    break;
  }

  if (message != nullptr) {
    CAssetId id = gpResourceFactory->GetResourceIdByName(message)->id;
    mgr.QueueMessage(mgr.GetHUDMessageFrameCount() + 1, id, 0.f);
  }
}

CAABox LoadCAABox(CStateManager& mgr, const TAreaId& areaId, const CVector3f& collisionSize,
                  const CVector3f& collisionOffset);

rstl::optional_object< CModelData > LdrToModelData(const CVector3f&, CAssetId asset,
                                                  const SLdrAnimationSet&, bool);

CEntity* LoadPickup(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrPickup sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrPickup.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, sldrThis.model,
                    sldrThis.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  CAABox box =
      LoadCAABox(mgr, info.GetAreaId(), sldrThis.collisionSize, sldrThis.collisionOffset);
  if (sldrThis.collisionSize == CVector3f::Zero()) {
    box = modelData->GetBounds(CTransform4f(LdrToTransform4f(sldrThis.editorProperties)));
  }
  return new CScriptPickup(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties),
      LdrToTransform4f(sldrThis.editorProperties), *modelData,
      LdrToActorParameters(sldrThis.actorInformation),
      LdrToEchoParameters(sldrThis.echoInformation), box,
      CPlayerState::EItemType(sldrThis.itemToGive.value), sldrThis.amount,
      sldrThis.capacityIncrease, sldrThis.itemPercentageIncrease, sldrThis.pickupEffect,
      sldrThis.absoluteValue, sldrThis.canHomeByDefault, sldrThis.autoSpin,
      sldrThis.blinkOut,
      sldrThis.lifetime, sldrThis.respawnTime, sldrThis.fadetime,
      sldrThis.activationDelay, sldrThis.pickupEffectLifetime, sldrThis.autoHomeRange,
      sldrThis.delayUntilHome, sldrThis.homingSpeed, CVector3f(sldrThis.orbitOffset));
}
