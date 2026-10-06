#include "MetroidPrime/Enemies/CSpankWeed.hpp"

#include "Collision/CCollidableSphere.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/COBBox.hpp"
#include "Collision/CSpatialPrimitive.hpp"
#include "Collision/CollisionUtil.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSpankWeed.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

#include "rstl/StringExtras.hpp"

#include <stdio.h>

// Guessed name. Debug OBB drawing; the retail DOL body is empty.
void DrawDebugOBB(const COBBox& box, float r, float g, float b, float a);

static EMaterialTypes SolidMaterial = kMT_Unknown59;
static EMaterialTypes skExcludeMaterial1 = kMT_Character;
static EMaterialTypes skExcludeMaterial2 = kMT_Player;
static EMaterialTypes skCollisionMaterial1 = kMT_CameraPassthrough;
static EMaterialTypes skCollisionMaterial2 = kMT_Immovable;

static const char* const skArmJointNames[] = {
    "Arm_2", "Arm_3", "Arm_4",  "Arm_5",  "Arm_6",  "Arm_7",
    "Arm_8", "Arm_9", "Arm_10", "Arm_11", "Arm_12", "Arm_end",
};

// Guessed name. An unused actor that tests touching actors against spheres around the owning
// spank weed's arm joints.
class CSpankWeedCollisionActor : public CActor {
public:
  CSpankWeedCollisionActor(TUniqueId uid, TAreaId areaId, const CMaterialList& materials,
                           TUniqueId owner);

  // CEntity
  ~CSpankWeedCollisionActor() override;
  void Think(float dt, CStateManager& mgr) override;

  // CActor
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  void Touch(CActor& actor, CStateManager& mgr) override;

private:
  void UpdateBounds(CStateManager& mgr);

  TUniqueId mOwnerId;
  rstl::optional_object< CAABox > mTouchBounds;
  rstl::vector< CVector3f > mArmPositions;
};

CSpankWeedCollisionActor::~CSpankWeedCollisionActor() {}

CSpankWeedCollisionActor::CSpankWeedCollisionActor(TUniqueId uid, TAreaId areaId,
                                                   const CMaterialList& materials,
                                                   TUniqueId owner)
: CActor(uid, rstl::string_l("Spank Weed Collision "),
         CEntityInfo(areaId, CEntity::NullConnectionList, true, kInvalidEditorId), 0,
         CTransform4f::Identity(), CModelData::CModelDataNull(), materials, CActorParameters(),
         kInvalidUniqueId)
, mOwnerId(owner)
, mTouchBounds() {
  mArmPositions.reserve(12);
  SetCallTouch(false);
}

void CSpankWeedCollisionActor::Think(float dt, CStateManager& mgr) { UpdateBounds(mgr); }

void CSpankWeedCollisionActor::Touch(CActor& actor, CStateManager& mgr) {
  CActor* act = TCastToPtr< CActor >(actor);
  if (act) {
    rstl::vector< CCollidableSphere > spheres;
    spheres.reserve(12);
    bool hit = false;
    const rstl::optional_object< CAABox > touchBounds = act->GetTouchBounds();
    CSpankWeed* weed = TCastToPtr< CSpankWeed >(mgr.ObjectById(mOwnerId));
    if (!weed) {
      return;
    }
    for (uint i = 0; i < ARRAY_SIZE(skArmJointNames); ++i) {
      if (CollisionUtil::AABoxSphereIntersection(
              *touchBounds, CSphere(weed->GetTransform() * mArmPositions[i], 1.f))) {
        hit = true;
        break;
      }
    }
    if (hit) {
      if (CPlayer* player = TCastToPtr< CPlayer >(actor)) {
        if (weed->mCurDamageRemTime <= 0.f && !weed->mHitByPlayerProjectile) {
          mgr.ApplyDamage(
              mOwnerId, player->GetUniqueId(), GetUniqueId(), weed->GetContactDamage(),
              CMaterialFilter::MakeIncludeExclude(CMaterialList(SolidMaterial), CMaterialList()),
              CVector3f::Zero());
          weed->mCurDamageRemTime = weed->mDamageWaitTime;
        }
      }
    }
  }
}

rstl::optional_object< CAABox > CSpankWeedCollisionActor::GetTouchBounds() const {
  return mTouchBounds;
}

void CSpankWeedCollisionActor::UpdateBounds(CStateManager& mgr) {
  mArmPositions.clear();
  CSpankWeed* weed = TCastToPtr< CSpankWeed >(mgr.ObjectById(mOwnerId));
  if (weed) {
    const CAABox bounds = weed->GetBoundingBox();
    const float halfWidth = 0.5f * (bounds.GetMaxPoint().GetX() - bounds.GetMinPoint().GetX());
    const float halfDepth = 0.5f * (bounds.GetMaxPoint().GetZ() - bounds.GetMinPoint().GetZ());
    float minX = 0.f;
    float maxX = 0.f;
    float minZ = 0.f;
    float maxZ = 0.f;
    float minY = 0.f;
    float maxY = 0.f;
    for (uint i = 0; i < ARRAY_SIZE(skArmJointNames); ++i) {
      const CTransform4f xf = weed->GetLocatorTransform(rstl::string_l(skArmJointNames[i]));
      const CVector3f pos = xf.GetTranslation();
      mArmPositions.push_back_unsafe(pos);
      if (pos.GetX() < minX) {
        minX = pos.GetX();
      } else if (pos.GetX() > maxX) {
        maxX = pos.GetX();
      }
      if (pos.GetY() < minY) {
        minY = pos.GetY();
      } else if (pos.GetY() > maxY) {
        maxY = pos.GetY();
      }
      if (pos.GetZ() < minZ) {
        minZ = pos.GetZ();
      } else if (pos.GetZ() > maxZ) {
        maxZ = pos.GetZ();
      }
    }
    mTouchBounds = rstl::optional_object< CAABox >(
        CAABox(minX - halfWidth, minY, minZ - halfDepth, maxX + halfWidth, maxY, maxZ + halfDepth)
            .GetTransformedAABox(weed->GetTransform()));
  }
}

CSpankWeed::CSpankWeed(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                       const CTransform4f& xf, const CModelData& modelData,
                       const CActorParameters& actorParams, const CPatternedInfo& patternedInfo,
                       float maxDetectionRange, float maxHearingRange, float maxSightRange,
                       float hideTime)
: CPatterned(static_cast< EPatternedAI >(0x36), uid, name, kFT_Zero, info, xf, modelData,
             patternedInfo, kMT_Flyer, kCT_One, kBT_Restricted, actorParams)
, mMaxDetectionRange(maxDetectionRange)
, mDetectionHeightRange(patternedInfo.GetDetectionHeightRange())
, mMaxHearingRange(maxHearingRange)
, mMaxSightRange(maxSightRange)
, mHideTime(hideTime)
, mCanKnockBack(false)
, x7d8_(0.f)
, mRetreatOrigin(xf.GetTranslation())
, x7e8_(kInvalidUniqueId)
, mIsHiding(true)
, mLockonOffset(CVector3f::Zero())
, mLockonTarget(CVector3f::Zero())
, mState(-1)
, mPreviousState(-1)
, mAnimPhase(-1) {
  SetCallTouch(false);
  SetDrawShadow(false);

  const CVector3f modelScale = GetModelData()->GetScale();
  if (modelScale.GetX() != modelScale.GetY() || modelScale.GetX() != modelScale.GetZ()) {
    const float scale = modelScale.Magnitude() / CMath::SqrtF(3.f);
    ModelData()->SetScale(CVector3f(scale, scale, scale));

    char buf[1024];
    sprintf(buf,
            "WARNING: Non-uniform scale (%.2f, %.2f, %.2f) applied to Spank Weed...changing scale "
            "to (%.2f, %.2f, %.2f)\n",
            modelScale.GetX(), modelScale.GetY(), modelScale.GetZ(), scale, scale, scale);
  }

  CMaterialList exclude = GetMaterialFilter().GetExcludeList();
  exclude.Add(CMaterialList(skExcludeMaterial1, skExcludeMaterial2));
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(GetMaterialFilter().GetIncludeList(), exclude));

  const CSegId segId = GetAnimationData()->GetLocatorSegId(rstl::string_l("lockon_target_LCTR"));
  if (segId != 0xFF) {
    const CTransform4f locatorXf = GetAnimationData()->GetLocatorTransform(segId, nullptr);
    const CVector3f scale = GetModelData()->GetScale();
    const CTransform4f scaledXf = GetTransform() * (CTransform4f::Scale(scale) * locatorXf);
    mLockonTarget = scaledXf.GetTranslation();
    mLockonOffset = scaledXf.GetTranslation() - GetTranslation();
  }

  KnockBackController().EnableKnockBackPhysics(false);
}

void CSpankWeed::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const bool oldActive = GetActive();
  switch (msg.GetMessage()) {
  case kSM_Create: {
    if (!BodyController()->GetIsActive()) {
      BodyController()->Activate(mgr, pas::kAS_Invalid);
      const CAABox box = GetBoundingBox();
      const float halfWidth = 0.5f * box.GetWidth();
      const float halfDepth = 0.5f * box.GetDepth();
      const float halfHeight = 0.5f * box.GetHeight();
      SetBoundingBox(CAABox(-halfWidth, -halfHeight, -halfDepth, halfWidth, halfHeight, halfDepth));
    }
    {
      rstl::vector< CJointCollisionDescription > joints;
      if (HasAnimation() && GetAnimationData()->GetSpatialPrimitive()) {
        const CSpatialPrimitive& primitive = ***GetAnimationData()->GetSpatialPrimitive();
        const uint count = primitive.GetBoxes().size();
        joints.reserve(count);
        for (uint i = 0; i < count; ++i) {
          const CSpatialPrimitive::SBox& box = primitive.GetBoxes()[i];
          const CSegId segId = box.mFirstSegment;
          rstl::map< rstl::string, CSegId > names(GetAnimationData()->GetCharLayoutInfo()->GetNameMap());
          for (rstl::map< rstl::string, CSegId >::const_iterator it = names.begin();
               it != names.end(); ++it) {
          }
          const CJointCollisionDescription desc =
              CJointCollisionDescription::OBBFromMayaPlugInCollision(
                  segId, box.mBox.GetSize(), box.mBox.GetTransform().BuildMatrix3f(),
                  box.mBox.GetTransform().GetTranslation(),
                  rstl::string_l("box") + CStringExtras::CreateFromInteger(i), 0.001f);
          joints.push_back_unsafe(desc);
        }
      }
      mCollisionMgr = rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints,
                                                    GetActive());
      CMaterialList list;
      list.Add(CMaterialList(skCollisionMaterial1));
      list.Add(CMaterialList(skCollisionMaterial2));
      mCollisionMgr->AddMaterialList(mgr, list);
    }
    if (HasActorLights()) {
      ActorLights()->SetNeedsRelight(true);
    }
    const CVector3f bias =
        GetTransform().BuildMatrix3f() * GetScaledLocatorTransform(rstl::string_l("swoosh_LCTR")).GetTranslation();
    ActorLights()->SetActorPositionBias(bias);
    break;
  }
  case kSM_XHIT: {
    if (CCollisionActor* colAct = TCastToPtr< CCollisionActor >(mgr.ObjectById(msg.GetOriginator()))) {
      const TUniqueId touchedId = colAct->GetLastTouchedObject();
      if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(touchedId))) {
        if (mCurDamageRemTime <= 0.f && mState != 4 && mState != 6) {
          mgr.ApplyDamage(
              GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
              CMaterialFilter::MakeIncludeExclude(CMaterialList(SolidMaterial), CMaterialList()),
              CVector3f::Zero());
          mCurDamageRemTime = mDamageWaitTime;
        }
      }
    }
    break;
  }
  case kSM_Delete:
    mgr.DeleteObjectRequest(x7e8_);
    mCollisionMgr->Destroy(mgr);
    break;
  case kSM_Activate:
    if (HasActorLights()) {
      ActorLights()->SetNeedsRelight(true);
    }
    break;
  case kSM_Decrement:
    if (mState != 0 && mState != 5 && mState != 6 && mState != 4) {
      mHitByPlayerProjectile = true;
      mDamageCooldownTimer = mDamageWaitTime;
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionMgr.get()) {
      mCollisionMgr->SetPhysicsActive(mgr, false);
    }
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
  const bool active = GetActive();
  if (oldActive != active && mCollisionMgr.get()) {
    mCollisionMgr->SetActive(mgr, active);
  }
}

void CSpankWeed::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  HealthInfo()->SetHP(1000000.f);
  if (!mIsHiding) {
    const CVector3f scale = GetModelData()->GetScale();
    const CVector3f eyeOrigin = GetLocatorTransform(rstl::string_l("Eye")).GetTranslation();
    CVector3f offset = CVector3f::ByElementMultiply(scale, eyeOrigin);
    offset = GetTransform().Rotate(offset);
    MoveCollisionPrimitive(offset);
    SetTransformDirty();
  }
  mCollisionMgr->Update(dt, mgr, CCollisionActorManager::kUO_WorldSpace);
  CPatterned::Think(dt, mgr);
}

void CSpankWeed::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  if (mCanKnockBack) {
    CPatterned::KnockBack(mgr, info);
    mCanKnockBack = false;
  }
}

void CSpankWeed::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    KnockBackController().EnableFreeze(false);
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    RemoveMaterial(SolidMaterial, kMT_Scannable, mgr);
    RemoveMaterial(kMT_Orbit, kMT_Target, mgr);
    mCollisionMgr->SetActive(mgr, false);
    mIsHiding = true;
    mState = 0;
    break;
  case kStateMsg_Deactivate:
    AddMaterial(kMT_Orbit, kMT_Target, kMT_Scannable, mgr);
    SetTranslation(mRetreatOrigin);
    mCollisionMgr->SetActive(mgr, true);
    mIsHiding = false;
    KnockBackController().EnableFreeze(true);
    mPreviousState = 0;
    break;
  }
}

void CSpankWeed::FadeIn(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimPhase = 0;
    mCanKnockBack = true;
    mState = 5;
    break;
  case kStateMsg_Update:
    switch (mAnimPhase) {
    case 0:
      if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {
        mAnimPhase = 2;
      } else {
        BodyController()->CommandMgr().DeliverCmd(
            CBCStepCmd(pas::kSD_Forward, pas::kStep_Normal));
      }
      break;
    case 2:
      if (BodyController()->GetCurrentStateId() != pas::kAS_Step) {
        mAnimPhase = 3;
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 5;
    break;
  }
  SetWorldLightingDirty(true);
}

void CSpankWeed::FadeOut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimPhase = 0;
    mCanKnockBack = false;
    mState = 6;
    break;
  case kStateMsg_Update:
    switch (mAnimPhase) {
    case 0:
      if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {
        mAnimPhase = 2;
      } else {
        BodyController()->CommandMgr().DeliverCmd(
            CBCStepCmd(pas::kSD_Backward, pas::kStep_Normal));
      }
      break;
    case 2:
      if (BodyController()->GetCurrentStateId() != pas::kAS_Step) {
        mAnimPhase = 3;
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 6;
    break;
  }
}

void CSpankWeed::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    KnockBackController().EnableFreeze(true);
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
    RemoveMaterial(SolidMaterial, mgr);
    mState = 1;
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 1;
    break;
  }
}

void CSpankWeed::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    RemoveMaterial(SolidMaterial, mgr);
    mState = 2;
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 2;
    break;
  }
}

void CSpankWeed::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_Zero));
    mState = 3;
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() != pas::kAS_MeleeAttack) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_Zero));
    }
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 3;
    break;
  }
}

bool CSpankWeed::IsPlayerInRange(CStateManager& mgr, float range) const {
  const float rangeSq = range * range;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CVector3f& playerPos = mgr.GetPlayer(i)->GetTranslation();
    if (mDetectionHeightRange > 0.f &&
        !(CMath::AbsF(playerPos.GetZ() - GetTranslation().GetZ()) < mDetectionHeightRange)) {
      continue;
    }
    const CVector3f delta = playerPos - mLockonTarget;
    if (delta.MagSquared() < rangeSq) {
      return true;
    }
  }
  return false;
}

bool CSpankWeed::InDetectionRange(CStateManager& mgr, const CTriggerData& data) const {
  return IsPlayerInRange(mgr, mMaxDetectionRange);
}

bool CSpankWeed::HearPlayer(CStateManager& mgr, const CTriggerData& data) const {
  return IsPlayerInRange(mgr, mMaxHearingRange);
}

bool CSpankWeed::InRange(CStateManager& mgr, const CTriggerData& data) const {
  return IsPlayerInRange(mgr, mMaxSightRange);
}

bool CSpankWeed::Delay(CStateManager& mgr, const CTriggerData& data) const {
  if (mHitByPlayerProjectile) {
    if (mStateMachine->GetTime() > mHideTime) {
      const_cast< CSpankWeed* >(this)->mHitByPlayerProjectile = false;
      return true;
    }
    return false;
  }
  return true;
}

void CSpankWeed::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimPhase = 0;
    mState = 4;
    RemoveMaterial(kMT_Orbit, kMT_Target, mgr);
    break;
  case kStateMsg_Update:
    switch (mAnimPhase) {
    case 0:
      if (BodyController()->GetCurrentStateId() == pas::kAS_KnockBack) {
        mAnimPhase = 2;
      } else {
        BodyController()->CommandMgr().DeliverCmd(
            CBCKnockBackCmd(CVector3f::Zero(), pas::kS_Zero));
      }
      break;
    case 2:
      if (BodyController()->GetCurrentStateId() != pas::kAS_KnockBack) {
        mAnimPhase = 3;
      }
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mPreviousState = 4;
    break;
  }
}

CVector3f CSpankWeed::GetAimPosition(const CStateManager& mgr, float dt) const {
  CVector3f pos = CVector3f::Zero();
  if (dt > 0.f) {
    pos = PredictMotion(dt).GetTranslation();
  }
  const CAnimData& animData = *GetModelData()->GetAnimationData();
  const CSegId id = animData.GetLocatorSegId(rstl::string_l("lockon_target_LCTR"));
  if (id != 0xFF) {
    const CTransform4f locatorXf = animData.GetLocatorTransform(id, nullptr);
    const CVector3f scaledOrigin =
        CVector3f::ByElementMultiply(GetModelData()->GetScale(), locatorXf.GetTranslation());
    pos += GetTransform() * scaledOrigin;
  } else {
    pos += GetBoundingBox().GetCenterPoint();
  }
  return pos;
}

CVector3f CSpankWeed::GetOrbitPosition(const CStateManager& mgr) const {
  CVector3f ret = CPatterned::GetOrbitPosition(mgr);
  const float maxTime = 1.f;
  float time = rstl::min_val(mStateMachine->GetTime(), maxTime);
  CVector3f target = GetTranslation() + mLockonOffset;
  if (mState == 3 && mPreviousState == 2) {
    return CVector3f::Lerp(ret, target, time);
  } else if (mState == 2 && mPreviousState == 3) {
    return CVector3f::Lerp(target, ret, time);
  }
  return ret;
}

bool CSpankWeed::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimPhase == 3;
}

bool CSpankWeed::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Attacked(mgr, data);
}

void CSpankWeed::Render(const CStateManager& mgr) const {
  CPatterned::Render(mgr);
  const CAnimData* animData = GetModelData()->GetAnimationData();
  if (animData->GetSpatialPrimitive()) {
    const CSpatialPrimitive& primitive = ***animData->GetSpatialPrimitive();
    const uint count = primitive.GetBoxes().size();
    for (uint i = 0; i < count; ++i) {
      const CSpatialPrimitive::SBox& box = primitive.GetBoxes()[i];
      const CTransform4f locatorXf = GetScaledLocatorTransform(box.mFirstSegment);
      const COBBox obb(GetTransform() * locatorXf * box.mBox.GetTransform(), box.mBox.GetSize());
      DrawDebugOBB(obb, 1.f, 1.f, 1.f, 1.f);
    }
  }
}

CEntity* LoadSpankWeed(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSpankWeed sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSpankWeed.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CSpankWeed(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                           LdrToEntityInfo(info, sldrThis.editorProperties),
                           LdrToTransform4f(sldrThis.editorProperties), *modelData,
                           LdrToActorParameters(sldrThis.actorInformation),
                           LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.wakeUpRadius,
                           sldrThis.searchRadius, sldrThis.attackRadius, sldrThis.hurtSleepDelay);
}

static void SetFuncPtrs() {
  static SSpankWeed_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadSpankWeed;
  SetSSpankWeed_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSSpankWeed_FuncPtrs(nullptr); }

