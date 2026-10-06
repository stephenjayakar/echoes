#include "MetroidPrime/Enemies/CParasite.hpp"

#include "Collision/CCollidableAABox.hpp"
#include "Collision/CCollisionInfoList.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrParasite.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptDynamicLight.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "REL/REL_Setup.h"

float CParasite::skAttackTime = 2.f * CMath::SqrtF(2.5f / CPhysicsActor::GravityConstant());
float CParasite::skAttackVelocity = 15.f / skAttackTime;
float CParasite::skRetreatTime = 2.f * CMath::SqrtF(2.5f / CPhysicsActor::GravityConstant());
float CParasite::skRetreatVelocity = 3.f / skRetreatTime;

CParasite::CParasite(TUniqueId uid, const rstl::string& name, EFlavorType flavor,
                     const CEntityInfo& info, const CTransform4f& xf, const CModelData& mData,
                     const CPatternedInfo& pInfo, EBodyType bodyType, float maxTelegraphReactDist,
                     float advanceWpRadius, float f3, float alignAngVel, float f5,
                     float stuckTimeThreshold, float collisionCloseMargin,
                     float parasiteSearchRadius, float parasiteSeparationDist,
                     float parasiteSeparationWeight, float parasiteAlignmentWeight,
                     float parasiteCohesionWeight, float destinationSeekWeight,
                     float forwardMoveWeight, float playerSeparationDist,
                     float playerSeparationWeight, float playerObstructionMinDist, float haltDelay,
                     bool disableMove, EType wType, const CDamageVulnerability& dVuln,
                     const CDamageInfo& dInfo, ushort haltSfx, ushort getUpSfx, ushort crouchSfx,
                     CAssetId modelRes, CAssetId skinRes, float iceZoomerJointHP,
                     float wallWalkerF6, const CDamageInfo& dInfo2, const CActorParameters& aParams)
: CWallWalker(static_cast< EPatternedAI >(39), uid, name, flavor, info, xf, mData, pInfo, kMT_Flyer,
              kCT_Zero, bodyType, aParams, pInfo.GetHalfExtent(), collisionCloseMargin,
              alignAngVel, advanceWpRadius, playerObstructionMinDist, wallWalkerF6, 0.167f, 0.6f,
              wType, disableMove, 1.5f, 0.6f, 1.5f)
, mStateProgress(-1)
, x87c_(CVector3f::Zero())
, mTargetPos(CVector3f::Zero())
, mActiveSpeed(1.f)
, mTelegraphRemTime(0.f)
, mStuckTime(0.f)
, mLastStuckPos(CVector3f::Zero())
, mCollisionActorManager(nullptr)
, mExtraModel(nullptr)
, mParasiteSeparationMove(CVector3f::Zero())
, mParasiteCohesionMove(CVector3f::Zero())
, mParasiteAlignmentMove(CVector3f::Zero())
, mOculusHaltDVuln(dVuln)
, mOculusHaltDInfo(dInfo)
, x928_(dInfo2)
, x944_(0.f)
, mMaxTelegraphReactDist(maxTelegraphReactDist)
, x94c_(f3)
, x954_(f5)
, mStuckTimeThreshold(stuckTimeThreshold)
, mParasiteSearchRadius(parasiteSearchRadius)
, mParasiteSeparationDist(parasiteSeparationDist)
, mParasiteSeparationWeight(parasiteSeparationWeight)
, mParasiteAlignmentWeight(parasiteAlignmentWeight)
, mParasiteCohesionWeight(parasiteCohesionWeight)
, mDestinationSeekWeight(destinationSeekWeight)
, mForwardMoveWeight(forwardMoveWeight)
, mPlayerSeparationDist(playerSeparationDist)
, mPlayerSeparationWeight(playerSeparationWeight)
, mUnmorphedRadius(pInfo.GetHeight() * 0.5f)
, mHaltDelay(haltDelay)
, mIceZoomerJointHP(iceZoomerJointHP)
, x990_(CVector3f::Zero())
, x99c_(CVector3f::Zero())
, x9a8_(CVector3f::Zero())
, mHaltSfx(haltSfx)
, mGetUpSfx(getUpSfx)
, mCrouchSfx(crouchSfx)
, x9ba_(kInvalidUniqueId)
, x9bc_(kInvalidUniqueId)
, x9c0_(0)
, x9c4_(0)
, mReceivedTelegraph(false)
, mJumpVelDirty(false)
, x9c8_26_(false)
, mLanded(false)
, mOnGround(true)
, x9c8_29_(false)
, mAttackOver(true)
, x9c8_31_(false)
, mHalted(false)
, mVulnerable(false)
, mOculusShotAt(false)
, mInJump(false)
, mLineOfSight(GetUniqueId(), CSegId(0xff), 0.2f, 0.05f) {
  SetCallTouch(false);
  switch (mWalkerType) {
  case kWT_Geemer:
    mKnockBackController.SetEnableFreeze(false);
  case kWT_Oculus:
    mKnockBackController.EnableKnockBackPhysics(false);
    break;
  case kWT_IceZoomer:
    mExtraModel = rs_new TLockedToken< CSkinnedModel >(
        rs_new CSkinnedModel(gpSimplePool->GetObj(SObjectTag('CMDL', modelRes)),
                             gpSimplePool->GetObj(SObjectTag('CSKR', skinRes)),
                             GetModelData()->GetAnimationData()->GetModelData()->GetLayoutInfo()));
    break;
  default:
    break;
  }
  if (mWalkerType == kWT_Oculus) {
    mKnockBackController.EnableShock(false);
    mKnockBackController.SetEnableBurn(false);
    mKnockBackController.SetEnableBurnDeath(false);
    mKnockBackController.EnableExplodeDeath(false);
  }
}

CParasite::~CParasite() {}

static TUniqueId lastParasite = kInvalidUniqueId;

void CParasite::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const TUniqueId uid = msg.GetSenderId();
  const EScriptObjectMessage message = msg.GetMessage();
  CWallWalker::AcceptScriptMsg(mgr, msg);

  switch (message) {
  case kSM_Create: {
    mBodyController->Activate(mgr, pas::kAS_Invalid);
    SetDrawShadow(false);
    mActiveSpeed = mSpeed;
    const float radius = mColSphere.GetSphere().GetRadius();
    const CVector3f extent(radius, radius, radius);
    SetBoundingBox(CAABox(-extent, extent));
    lastParasite = GetUniqueId();
    AddDoorRepulsors(mgr);
    if (mWalkerType == kWT_IceZoomer) {
      SetupIceZoomerCollision(mgr);
      SetupIceZoomerVulnerability(
          mgr, mOculusHaltDVuln,
          CHealthInfo(mIceZoomerJointHP, HealthInfo()->GetKnockBackResistance()));
    }
    break;
  }
  case kSM_Delete:
    switch (mWalkerType) {
    case kWT_IceZoomer:
      DestroyActorManager(mgr);
      break;
    case 10:
      mgr.DeleteObjectRequest(x9ba_);
      mgr.DeleteObjectRequest(x9bc_);
      break;
    }
    break;
  case kSM_AreaLoaded:
    if (mWalkerType == 10) {
      for (const SConnection* it = GetConnectionList().data();
           it != GetConnectionList().data() + GetConnectionList().size(); ++it) {
        if (it->state == kSS_GRNT && it->msg == kSM_Activate) {
          const CScriptObjectLoaderHelper::SGeneratedObject generated =
              mgr.ScriptObjectLoaderHelper().GenerateScriptObject(it->objId, mgr);
          const TUniqueId id = generated.mUniqueId;
          CEntity* entity = generated.mEntity;
          if (TCastToPtr< CScriptSafeZone >(entity)) {
            x9ba_ = id;
          } else if (TCastToPtr< CScriptDynamicLight >(entity)) {
            x9bc_ = id;
          } else {
            mgr.DeleteObjectRequest(id);
          }
        }
      }
    }
    mLineOfSight.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
    break;
  case kSM_Launching:
    if (mJumpVelDirty) {
      UpdateJumpVelocity();
      mJumpVelDirty = false;
    }
    break;
  case kSM_Activate:
    mDisableMove = false;
    switch (mWalkerType) {
    case kWT_Parasite:
      mBodyController->SetLocomotionType(pas::kLT_Lurk);
      break;
    case 10:
      UpdateShell(mgr, 0);
      break;
    }
    break;
  case kSM_Deactivate:
    if (mWalkerType == 10) {
      UpdateShell(mgr, 1);
    }
    break;
  case kSM_ResistedDamage:
    switch (mWalkerType) {
    case kWT_Oculus:
      if (const CActor* act = TCastToConstPtr< CActor >(mgr.GetObjectById(uid))) {
        const float distSq = (act->GetTranslation() - GetTranslation()).MagSquared();
        const float maxComp = rstl::max_val(
            GetTouchBounds()->GetWidth(),
            rstl::max_val(GetTouchBounds()->GetDepth(), GetTouchBounds()->GetHeight()));
        const float maxCompSq = maxComp * maxComp + 1.f;
        if (distSq < maxCompSq * maxCompSq) {
          mOculusShotAt = true;
        }
      }
      break;
    case 10: {
      int type = -1;
      if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(uid))) {
        type = weapon->GetType();
      }
      switch (type) {
      case 2:
        UpdateShell(mgr, 2);
        break;
      case 1:
        UpdateShell(mgr, 1);
        break;
      }
      break;
    }
    }
  case kSM_XXDG:
    if (mWalkerType == kWT_IceZoomer) {
      mBodyController->CommandMgr().DeliverCmd(CBCAdditiveFlinchCmd(1.f));
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionActorManager.get()) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  default:
    break;
  }
}

void CParasite::AddDoorRepulsors(CStateManager& mgr) {
  const rstl::list< CEntity* >& doors = mgr.GetDoorList();
  int count = 0;
  for (rstl::list< CEntity* >::const_iterator it = doors.begin(); it != doors.end(); ++it) {
    if (*it && (*it)->GetCurrentAreaId() == GetCurrentAreaId()) {
      ++count;
    }
  }
  mDoorRepulsors.reserve(count);
  for (rstl::list< CEntity* >::const_iterator it = doors.begin(); it != doors.end(); ++it) {
    const CActor* door = static_cast< const CActor* >(*it);
    if (door && door->GetCurrentAreaId() == GetCurrentAreaId()) {
      rstl::optional_object< CAABox > bounds = door->GetTouchBounds();
      if (bounds.valid()) {
        float diagonal = (bounds->GetMinPoint() - bounds->GetMaxPoint()).Magnitude();
        mDoorRepulsors.push_back_unsafe(CRepulsor(bounds->GetCenterPoint(), 0.75f * diagonal));
      }
    }
  }
}

void CParasite::PreThink(float dt, CStateManager& mgr) { CWallWalker::PreThink(dt, mgr); }

static EMaterialTypes skContactMaterial = kMT_Unknown59;

void CParasite::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  ++mThinkCounter;
  switch (mWalkerType) {
  case kWT_IceZoomer:
    UpdateCollisionActors(dt, mgr);
    break;
  case 10:
    if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x9ba_))) {
      act->SetTransform(GetTransform());
    }
    if (CActor* act = TCastToPtr< CActor >(mgr.ObjectById(x9bc_))) {
      act->SetTransform(GetTransform());
    }
    break;
  }

  mPlayerObstructed = false;
  const CGameArea& area = mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId());
  if (area.GetOcclusionState() != CGameArea::kOS_Visible) {
    mPlayerObstructed = true;
  }

  if (!mPlayerObstructed) {
    const CPlayer* player = mgr.GetPlayer(0);
    if ((player->GetTranslation() - GetTranslation()).Magnitude() > mPlayerObstructionMinDist) {
      mLineOfSight.Update(dt, mgr);
      if (!mLineOfSight.HasLineOfSight()) {
        mPlayerObstructed = true;
      }
    }
  }

  if (mPlayerObstructed) {
    SetMovable(false);
    return;
  }

  SetMovable(!mAlignToFloor);

  if (!mDisableMove) {
    if (close_enough(mBodyController->GetPercentageFrozen(), 0.f)) {
      static const float ksMinMoveDistance = 0.3f * dt * mSpeed;
      const CVector3f stuckDelta = GetTranslation() - mLastStuckPos;
      if (stuckDelta.MagSquared() < ksMinMoveDistance * dt) {
        mStuckTime += dt;
      } else {
        mStuckTime = 0.f;
      }

      mLastStuckPos = GetTranslation();
      if (mTelegraphRemTime > 0.f) {
        mTelegraphRemTime -= dt;
      } else {
        mTelegraphRemTime = 0.f;
      }
    }
  }

  if (mAlive) {
    CPlayer* player = mgr.Player(0);
    bool useCollisionRadius =
        player->GetMorphballTransitionState() == CPlayer::kMS_Morphed || !mAttackOver;
    float radius = useCollisionRadius ? mColSphere.GetSphere().GetRadius() : mUnmorphedRadius;

    const CVector3f extent(radius, radius, radius);
    CAABox aabox(GetTranslation() - extent, GetTranslation() + extent);
    rstl::optional_object< CAABox > plBox = player->GetTouchBounds();

    if (plBox.valid() && plBox->DoBoundsOverlap(aabox)) {
      if (!mAttackOver) {
        mAttackOver = true;
        mLanded = false;
      }

      if (mCurDamageRemTime <= 0.f) {
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
                        CMaterialFilter::MakeIncludeExclude(CMaterialList(skContactMaterial),
                                                            CMaterialList()),
                        CVector3f::Zero());
        if (mWalkerType == kWT_IceZoomer && mVulnerable) {
          CSfxManager::AddEmitter(mCrouchSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                                  false);
          CSfxManager::SfxStart(mGetUpSfx, 100, 64);
          x944_ = 2.f;
          mgr.CameraBlurPass(0, 0).SetBlur(CCameraBlurPass::kBT_LoBlur, 4.f, 0.f, false);
          mgr.CameraBlurPass(0, 0).DisableBlur(2.f);
          mgr.CameraFilterPass(0, 0).SetFilter(CCameraFilterPass::kFT_Blend,
                                                CCameraFilterPass::kFS_Fullscreen, 0.f,
                                                CColor(140, 255, 109, 63.75f), -1);
          mgr.CameraFilterPass(0, 0).DisableFilter(2.f);
        }
        mCurDamageRemTime = mDamageWaitTime;
      }
    }

    if (x944_ > 0.f) {
      mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), CDamageInfo(x928_, dt),
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(skContactMaterial),
                                                          CMaterialList()),
                      CVector3f::Zero());
      x944_ -= dt;
    }
  }

  CWallWalker::Think(dt, mgr);

  if (mDisableMove) {
    return;
  }

  if (!close_enough(mBodyController->GetPercentageFrozen(), 0.f)) {
    return;
  }

  mSpeed = mActiveSpeed;
  if (mAlignToFloor) {
    AlignToFloor(mgr, mColSphere.GetSphere().GetRadius(),
                 GetTranslation() + 2.f * (dt * GetVelocityWR()), dt);
  }

  mLanded = false;
}

CAdvancementDeltas CParasite::UpdateWalkerAnimation(CStateManager& mgr, float dt) {
  return UpdateAnimation(dt, mgr, true);
}

CVector3f CParasite::GetAimPosition(const CStateManager&, float) const { return GetTranslation(); }

void CParasite::ThinkAboutMove(float dt) {
  if (!GetMaterialList().HasMaterial(kMT_Pillar)) {
    CPatterned::ThinkAboutMove(dt);
  }
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"AnimOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::AnimOver)},
    {"Landed", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::Landed)},
    {"HitSomething",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::HitSomething)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::ShouldAttack)},
    {"Stuck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::Stuck)},
    {"AttackOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::AttackOver)},
    {"ShotAt", static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::ShotAt)},
    {"PatrolPathOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CParasite::PatrolPathOver)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Generate)},
    {"Deactivate", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Deactivate)},
    {"PathFind", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::PathFind)},
    {"TargetPatrol",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TargetPatrol)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Patrol)},
    {"Run", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Run)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Attack)},
    {"TelegraphAttack",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TelegraphAttack)},
    {"Jump", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Jump)},
    {"TargetPlayer",
     static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::TargetPlayer)},
    {"Retreat", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Retreat)},
    {"Halt", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Halt)},
    {"Crouch", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::Crouch)},
    {"GetUp", static_cast< CPatterned::StateMachine::StateFunc >(&CParasite::GetUp)},
};

bool CParasite::Stuck(CStateManager&, const CTriggerData&) const {
  return mStuckTime > mStuckTimeThreshold;
}

bool CParasite::AttackOver(CStateManager&, const CTriggerData&) const { return mAttackOver; }

bool CParasite::ShotAt(CStateManager&, const CTriggerData&) const {
  switch (mWalkerType) {
  case 10:
  case kWT_Oculus:
    return mOculusShotAt;
  default:
    return mHitByPlayerProjectile;
  }
}

bool CParasite::PatrolPathOver(CStateManager&, const CTriggerData&) const {
  return mDestObj == kInvalidUniqueId;
}

bool CParasite::IsOnGround() const { return mOnGround; }

bool CParasite::AnimOver(CStateManager&, const CTriggerData&) const {
  return mStateProgress == 2;
}

bool CParasite::Landed(CStateManager&, const CTriggerData&) const { return mLanded; }

bool CParasite::HitSomething(CStateManager& mgr, const CTriggerData&) const {
  if (mThinkCounter & 0x1) {
    return true;
  }
  return mTumbleAngle < 270.f && CloseToWall(mgr);
}

bool CParasite::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool shouldAttack = false;
  if (mReceivedTelegraph && mTelegraphRemTime > 0.1f) {
    shouldAttack = true;
  }
  return !TooClose(mgr, data) && InMaxRange(mgr, data) &&
         (shouldAttack || InDetectionRange(mgr, CTriggerData(0.f)));
}

void CParasite::Run(CStateManager&, EStateMsg, float) {}

void CParasite::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  CPhysicsActor::Stop();
  TelegraphAttack(mgr, kStateMsg_Activate, 0.f);
  SetMomentumWR(CVector3f(0.f, 0.f, -GetWeight()));
  CPatterned::Death(mgr, direction, state);
}

void CParasite::Touch(CActor& actor, CStateManager& mgr) { CPatterned::Touch(actor, mgr); }

void CParasite::UpdatePFDestination(CStateManager&) {}

void CParasite::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
  if (mWalkerType == 10) {
    SetModelFlags(GetModelFlags().UseShaderSet(x9c4_));
  }
}

void CParasite::Render(const CStateManager& mgr) const { CWallWalker::Render(mgr); }

const CDamageVulnerability* CParasite::GetDamageVulnerability() const {
  switch (mWalkerType) {
  case kWT_Oculus:
    if (mHalted) {
      return &mOculusHaltDVuln;
    }
    break;
  case kWT_IceZoomer:
    if (!mVulnerable) {
      return &CDamageVulnerability::ImmuneVulnerabilty();
    }
    break;
  case 10:
    if (x9c0_ == 1) {
      return &mOculusHaltDVuln;
    }
    break;
  }
  return CPatterned::GetDamageVulnerability();
}

EWeaponCollisionResponseTypes CParasite::GetCollisionResponseType(const CVector3f& position,
                                                                  const CVector3f& direction,
                                                                  const CWeaponMode& mode,
                                                                  int attributes) const {
  switch (mWalkerType) {
  case 10:
    if (GetDamageVulnerability()->GetEffect(mode) == 1) {
      return static_cast< EWeaponCollisionResponseTypes >(15);
    }
    break;
  }
  return CPatterned::GetCollisionResponseType(position, direction, mode, attributes);
}

CDamageInfo CParasite::GetContactDamage() const {
  switch (mWalkerType) {
  case kWT_Oculus:
    if (mHalted) {
      return mOculusHaltDInfo;
    }
    return CPatterned::GetContactDamage();
  case kWT_IceZoomer:
    if (!mVulnerable) {
      return mOculusHaltDInfo;
    }
    return CPatterned::GetContactDamage();
  default:
    return CPatterned::GetContactDamage();
  }
}

void CParasite::MassiveDeath(CStateManager& mgr) { CPatterned::MassiveDeath(mgr); }

void CParasite::MassiveFrozenDeath(CStateManager& mgr) { CPatterned::MassiveFrozenDeath(mgr); }

void CParasite::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

static void SetFuncPtrs() {
  static SParasite_FuncPtrs funcPtrs;
  funcPtrs.mLoadParasite = &LoadParasite;
  funcPtrs.mLoadBrizgee = &LoadBrizgee;
  funcPtrs.mLoadCrystallite = &LoadCrystallite;
  SetSParasite_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSParasite_FuncPtrs(nullptr); }

// ---- stubs (to be filled) ----
void CParasite::UpdateJumpVelocity() {}
bool CParasite::CloseToWall(CStateManager& mgr) const { return false; }
void CParasite::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                             CStateManager& mgr) {}
TUniqueId CParasite::RecursiveFindClosestWayPoint(CStateManager& mgr, TUniqueId id,
                                                  float& dist) const {
  return id;
}
TUniqueId CParasite::GetClosestWaypointForState(EScriptObjectState state,
                                                CStateManager& mgr) const {
  return kInvalidUniqueId;
}
void CParasite::Generate(CStateManager&, EStateMsg msg, float) {}
void CParasite::Deactivate(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {}
void CParasite::Jump(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {}
void CParasite::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {}
void CParasite::FaceTarget(CVector3f target) {}
void CParasite::Attack(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::Retreat(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {}
void CParasite::TelegraphAttack(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::Halt(CStateManager& mgr, EStateMsg msg, float) {}
void CParasite::Crouch(CStateManager&, EStateMsg msg, float) {}
void CParasite::GetUp(CStateManager&, EStateMsg msg, float) {}
void CParasite::DoFlockingBehavior(CStateManager& mgr) {}
void CParasite::SetupIceZoomerCollision(CStateManager& mgr) {}
void CParasite::DestroyActorManager(CStateManager& mgr) { mCollisionActorManager->Destroy(mgr); }
void CParasite::SetupIceZoomerVulnerability(CStateManager& mgr, const CDamageVulnerability& dVuln,
                                            const CHealthInfo& hInfo) {}
void CParasite::UpdateCollisionActors(float dt, CStateManager& mgr) {}
void CParasite::UpdateShell(CStateManager& mgr, int state) {}
CEntity* LoadParasite(CStateManager& mgr, CInputStream& input, CEntityInfo& info) { return nullptr; }
CEntity* LoadBrizgee(CStateManager& mgr, CInputStream& input, CEntityInfo& info) { return nullptr; }
CEntity* LoadCrystallite(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  return nullptr;
}
