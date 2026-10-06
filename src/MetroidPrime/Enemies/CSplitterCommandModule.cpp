#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"

#include "Collision/CCollisionInfoList.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CParticleGenInfo.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CSplitterBeamEffect.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <float.h>

// Guessed type name; one entry per sphere collision actor.
struct SSphereJointInfo {
  const char* name;
  float radius;
};

static const SSphereJointInfo skSphereJoints[] = {
    {"Skeleton_Root", 3.f},
};
static const char* const skShieldJoint = "Skeleton_Root";
static const char* const skBeamLocator = "Beam_LCTR";
static const char* const skLightShield = "LightShield";
static const char* const skDarkShield = "DarkShield";
static const char* const skLaserMuzzle = "LaserMuzzle";
static const char* const skAlertEye = "AlertEye";

CSplitterCommandModule::CSplitterCommandModule(TUniqueId uid, const rstl::string& name,
                                               const CEntityInfo& info, const CTransform4f& xf,
                                               const CModelData& mData,
                                               const CActorParameters& aParms,
                                               const CPatternedInfo& pInfo,
                                               const CSplitterCommandModuleData& data)
: CPatterned(kPAI_SplitterCommandModule, uid, name, kFT_Zero, info, xf, mData, pInfo, kMT_Flyer,
             kCT_One, kBT_AiMovedFlyer, aParms)
, mData(data)
, mPathFindSearch(nullptr, 3, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center)
, mCollisionActorManager(nullptr)
, mLaserPulseProjectileInfo(data.laserPulseProjectile, data.mLaserPulseDamage)
, mLaserSweepProjectileInfo(data.laserSweepBeamInfo.weaponSystem, CDamageInfo())
, mVulnerability(*CPatterned::GetDamageVulnerability())
, xedc_(0)
, mShieldType(kST_None)
, mLastShieldType(kST_None)
, mHoverDistance(0.f)
, xeec_(-1)
, xef0_(0.f)
, xef4_(0.f)
, mBeamLocator()
, mMainChassisId(kInvalidUniqueId)
, xefc_(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, xf00_(kInvalidUniqueId)
, mLaserSweepBeamId(kInvalidUniqueId)
, xf04_(CColor::White())
, xf08_(0.f)
, xf0c_(0.f)
, xf10_(0.f)
, xf14_(3.f)
, xf18_(0.f)
, xf1c_(0.f)
, xf20_(0)
, mFaceDirection(xf.GetForward())
, mDodgeDirection(pas::kSD_Invalid)
, mLaserSweepState(-1)
, mLaserSweepStart(CVector3f::Zero())
, mLaserSweepEnd(CVector3f::Zero())
, mLaserSweepDirection(CVector3f::Zero())
, mLaserSweepSfx()
, mShieldSfx()
, mScanSfx()
, mBeamEffectId(kInvalidUniqueId)
, xf6a_24_(true)
, xf6a_25_(false)
, xf6a_26_(false)
, xf6a_27_(true)
, xf6a_28_(false)
, xf6a_29_(false)
, xf6a_30_(true)
, xf6a_31_(false) {
  mLaserPulseProjectileInfo.Token().Lock();
  mLaserSweepProjectileInfo.Token().Lock();
  const CAnimData* animData = GetModelData()->GetAnimationData();
  mBeamLocator = animData->GetLocatorSegId(rstl::string_l(skBeamLocator));
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Unknown59), CMaterialList(kMT_CollisionActor, kMT_Character)));
  const CPASAnimParmData parms(pas::kAS_Step, CPASAnimParm::FromEnum(pas::kSD_Right),
                               CPASAnimParm::FromEnum(pas::kStep_Dodge),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter(),
                               CPASAnimParm::NoParameter(), CPASAnimParm::NoParameter());
  mHoverDistance = GetModelData()->GetScale().GetX() * GetAnimationDistance(parms);
}

CSplitterCommandModule::~CSplitterCommandModule() {}

void CSplitterCommandModule::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_Create:
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    SetupCollisionActors(mgr);
    mLastShieldType = mgr.Random()->Range(0.f, 100.f) < 50.f ? kST_Light : kST_Dark;
    break;
  case kSM_Delete:
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->Destroy(mgr);
    }
    if (mBeamEffectId != kInvalidUniqueId) {
      mgr.DeleteObjectRequest(mBeamEffectId);
    }
    StopLaserSweep(mgr);
    break;
  case kSM_Activate:
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->SetActive(mgr, true);
    }
    break;
  case kSM_Deactivate:
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->SetActive(mgr, false);
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_Alert:
    mHitByPlayerProjectile = true;
    break;
  case kSM_AreaLoaded:
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    FindDockingTarget(mgr);
    break;
  case kSM_Damage:
    OnDamaged(msg.GetSenderId());
    break;
  default:
    break;
  }
}

void CSplitterCommandModule::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  CPatterned::Think(dt, mgr);
  mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  UpdateDocking(mgr);
  UpdateTimers(dt, mgr);
  UpdateShields(mgr);
  UpdateLaserSweep(dt, mgr);
  UpdateBeamEffect(dt, mgr);
  UpdateStuckTimer(dt, mgr);
}

void CSplitterCommandModule::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                             EUserEventType type, float dt) {
  bool handled = false;
  switch (type) {
  case kUE_BreakLockOn:
    if (xf6a_27_ && (mData.unknown_0xbd80fd94 & 4) != 0) {
      if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId))) {
        player->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource, mgr);
      }
    }
    handled = true;
    break;
  case kUE_Projectile:
    FireLaserPulse(mgr, node.GetLocatorName());
    handled = true;
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

void CSplitterCommandModule::Render(const CStateManager& mgr) const {
  if (GetAlive()) {
    CPatterned::Render(mgr);
  }
}

void CSplitterCommandModule::PreRender(CStateManager& mgr) {
  if (GetAlive()) {
    CPatterned::PreRender(mgr);
  }
}

void CSplitterCommandModule::AddToRenderer(const CStateManager& mgr) const {
  if (GetAlive()) {
    CPatterned::AddToRenderer(mgr);
  }
}

void CSplitterCommandModule::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                          CStateManager& mgr) {
  CPatterned::CollidedWith(id, list, mgr);
  if (!xf6a_31_ && id == kInvalidUniqueId) {
    static const CMaterialList skWorldMaterials(kMT_Wall, kMT_Floor, kMT_Ceiling);
    for (int i = 0; i < list.GetCount(); ++i) {
      if (list[i].GetMaterialLeft().SharesMaterials(skWorldMaterials)) {
        xf6a_31_ = true;
        break;
      }
    }
  }
}

void CSplitterCommandModule::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {}

const CDamageVulnerability* CSplitterCommandModule::GetDamageVulnerability() const {
  return &mVulnerability;
}

CVector3f CSplitterCommandModule::GetAimPosition(const CStateManager& mgr, float dt) const {
  CVector3f aimPos = CPatterned::GetAimPosition(mgr, dt);
  if (xedc_ == 2) {
    if (const CSplitterMainChassis* chassis =
            TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mMainChassisId))) {
      const CVector3f chassisAimPos = chassis->GetAimPosition(mgr, dt);
      aimPos = CVector3f::Lerp(aimPos, chassisAimPos, rstl::min_val(xf18_, 1.f));
    }
  } else if (xf1c_ < 1.f) {
    if (const CSplitterMainChassis* chassis =
            TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mMainChassisId))) {
      const CVector3f chassisAimPos = chassis->GetAimPosition(mgr, dt);
      aimPos = CVector3f::Lerp(chassisAimPos, aimPos, rstl::min_val(xf1c_, 1.f));
    }
  }
  return aimPos;
}

bool CSplitterCommandModule::Listen(CStateManager& mgr, const CVector3f& position,
                                    EListenNoiseType type) {
  bool ret = false;
  if (GetAlive()) {
    switch (type) {
    case kLNT_Unknown4:
      if ((position - GetTranslation()).MagSquared() < 400.f) {
        ret = true;
        xf6a_25_ = true;
      }
      break;
    }
  }
  return ret;
}

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::StateOver)},
    {"InDetectionRange", static_cast< CPatterned::StateMachine::TriggerFunc >(
                             &CSplitterCommandModule::InDetectionRange)},
    {"IsScanning",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::IsScanning)},
    {"IsInitiallyDocked", static_cast< CPatterned::StateMachine::TriggerFunc >(
                              &CSplitterCommandModule::IsInitiallyDocked)},
    {"IsDocked",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::IsDocked)},
    {"HasDockingPath", static_cast< CPatterned::StateMachine::TriggerFunc >(
                           &CSplitterCommandModule::HasDockingPath)},
    {"HasDockingTarget", static_cast< CPatterned::StateMachine::TriggerFunc >(
                             &CSplitterCommandModule::HasDockingTarget)},
    {"DockingPathOver", static_cast< CPatterned::StateMachine::TriggerFunc >(
                            &CSplitterCommandModule::DockingPathOver)},
    {"HasTarget",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::HasTarget)},
    {"InHoverRange",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::InHoverRange)},
    {"InLaserPulseRange", static_cast< CPatterned::StateMachine::TriggerFunc >(
                              &CSplitterCommandModule::InLaserPulseRange)},
    {"ShouldDodge",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::ShouldDodge)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::ShouldAttack)},
    {"ShouldFireAgain", static_cast< CPatterned::StateMachine::TriggerFunc >(
                            &CSplitterCommandModule::ShouldFireAgain)},
    {"ShouldLaserSweep", static_cast< CPatterned::StateMachine::TriggerFunc >(
                             &CSplitterCommandModule::ShouldLaserSweep)},
    {"PathOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::PathOver)},
    {"PathShagged",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::PathShagged)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Start", static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Start)},
    {"Idle", static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Idle)},
    {"SpawnIdle",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::SpawnIdle)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Dead)},
    {"Scanning",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Scanning)},
    {"FaceTarget",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::FaceTarget)},
    {"PathFind",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::PathFind)},
    {"Hover", static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Hover)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::Dodge)},
    {"LaserPulse",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::LaserPulse)},
    {"LaserSweep",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::LaserSweep)},
    {"LostChassisReaction", static_cast< CPatterned::StateMachine::StateFunc >(
                                &CSplitterCommandModule::LostChassisReaction)},
    {"SeekMainChassis",
     static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::SeekMainChassis)},
    {"FollowDockingPath", static_cast< CPatterned::StateMachine::StateFunc >(
                              &CSplitterCommandModule::FollowDockingPath)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"NotifyDocking",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::NotifyDocking)},
    {"SelectTarget",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::SelectTarget)},
    {"SetTargetDest",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::SetTargetDest)},
    {"SetDockingDest",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::SetDockingDest)},
    {"FindBestDodgeDirection", static_cast< CPatterned::StateMachine::CodeFunc >(
                                   &CSplitterCommandModule::FindBestDodgeDirection)},
    {"RaiseShields",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::RaiseShields)},
    {"ResetAttackTimes",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::ResetAttackTimes)},
};

void CSplitterCommandModule::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

bool CSplitterCommandModule::CanBeIngPossessed(CStateManager& mgr) const {
  if (mMainChassisId != kInvalidUniqueId) {
    return false;
  }
  return CPatterned::CanBeIngPossessed(mgr);
}

void CSplitterCommandModule::SetChassisDocked(CStateManager& mgr) {
  mVulnerability = CDamageVulnerability::ReflectVulnerabilty();
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
  xedc_ = 2;
  xf18_ = 0.f;
  UpdateAlertEffect(mgr);
}

void CSplitterCommandModule::SetChassisReleased(CStateManager& mgr) {
  mVulnerability = *CPatterned::GetDamageVulnerability();
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
  if (xedc_ == 2) {
    xf1c_ = 0.f;
  }
  xedc_ = 0;
  xf18_ = 0.f;
  UpdateAlertEffect(mgr);
}

void CSplitterCommandModule::SetChassisDetaching(CStateManager& mgr) {
  mVulnerability = CDamageVulnerability::ReflectVulnerabilty();
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
  if (xedc_ == 2) {
    xf1c_ = 0.f;
  }
  xedc_ = 1;
  xf18_ = 0.f;
  UpdateAlertEffect(mgr);
}

void CSplitterCommandModule::SetChassisProtected(CStateManager& mgr) {
  mVulnerability = CDamageVulnerability::ReflectVulnerabilty();
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false);
  xedc_ = 3;
  xf18_ = 0.f;
  UpdateAlertEffect(mgr);
}

void CSplitterCommandModule::StartLaserSweep(const CVector3f& start, const CVector3f& end) {
  if (mMainChassisId == kInvalidUniqueId) {
    return;
  }
  mLaserSweepState = 0;
  mLaserSweepStart = start;
  mLaserSweepEnd = end;
}

CVector3f CSplitterCommandModule::GetBeamPosition() const {
  const CTransform4f xf = GetLctrTransform(mBeamLocator);
  return xf.GetTranslation();
}

void CSplitterCommandModule::AutoDestruct(float time) {
  if (GetAlive() && !xf6a_29_) {
    xf6a_29_ = true;
    xef0_ = time;
  }
}

bool CSplitterCommandModule::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.IsOver();
}

bool CSplitterCommandModule::InDetectionRange(CStateManager& mgr,
                                              const CTriggerData& data) const {
  if (IsScanning(mgr, data) && CPatterned::InDetectionRange(mgr, data)) {
    CVector3f delta = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
    delta.SetZ(0.f);
    const float dist = delta.Magnitude();
    return CVector3f::Dot(delta, GetTransform().GetForward()) > dist * mDetectionAngle;
  }
  return false;
}

bool CSplitterCommandModule::IsScanning(CStateManager& mgr, const CTriggerData& data) const {
  if (const CSplitterMainChassis* chassis =
          TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mMainChassisId))) {
    return chassis->GetHeadHealth() > 0.f;
  }
  return false;
}

bool CSplitterCommandModule::IsInitiallyDocked(CStateManager& mgr,
                                               const CTriggerData& data) const {
  return mData.unknown_0xbd80fd94 & 1;
}

bool CSplitterCommandModule::IsDocked(CStateManager& mgr, const CTriggerData& data) const {
  return mMainChassisId != kInvalidUniqueId;
}

bool CSplitterCommandModule::HasDockingPath(CStateManager& mgr, const CTriggerData& data) const {
  return GetConnectedObject(mgr, kSS_Connect, kSM_Follow) != kInvalidUniqueId;
}

bool CSplitterCommandModule::HasDockingTarget(CStateManager& mgr,
                                              const CTriggerData& data) const {
  const CSplitterMainChassis* chassis =
      TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(xefc_));
  bool ret = false;
  if (chassis && chassis->GetAlive() && chassis->GetActive() && !chassis->IsDocked()) {
    ret = true;
  }
  return ret;
}

bool CSplitterCommandModule::DockingPathOver(CStateManager& mgr,
                                             const CTriggerData& data) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

bool CSplitterCommandModule::HasTarget(CStateManager& mgr, const CTriggerData& data) const {
  return mTargetId != kInvalidUniqueId;
}

bool CSplitterCommandModule::InHoverRange(CStateManager& mgr, const CTriggerData& data) const {
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const float range = 0.5f * (mData.minLaserPulseRange + mData.maxLaserPulseRange);
    const CVector3f delta = target->GetTranslation() - GetTranslation();
    const float distSq = delta.MagSquared();
    return distSq <= range * range;
  }
  return false;
}

bool CSplitterCommandModule::InLaserPulseRange(CStateManager& mgr,
                                               const CTriggerData& data) const {
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const CVector3f delta = target->GetTranslation() - GetTranslation();
    const float distSq = delta.MagSquared();
    const float minRangeSq = mData.minLaserPulseRange * mData.minLaserPulseRange;
    const float maxRangeSq = mData.maxLaserPulseRange * mData.maxLaserPulseRange;
    bool ret = false;
    if (distSq >= minRangeSq && distSq <= maxRangeSq) {
      ret = true;
    }
    return ret;
  }
  return false;
}

bool CSplitterCommandModule::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  return mDodgeDirection != pas::kSD_Invalid;
}

bool CSplitterCommandModule::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return xf0c_ <= 0.f;
}

bool CSplitterCommandModule::ShouldFireAgain(CStateManager& mgr, const CTriggerData& data) const {
  return xf20_ > 0;
}

bool CSplitterCommandModule::ShouldLaserSweep(CStateManager& mgr,
                                              const CTriggerData& data) const {
  return mLaserSweepState != -1;
}

bool CSplitterCommandModule::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (!xf6a_24_ || CPatterned::PathOver(mgr, data)) {
    ret = true;
  }
  return ret;
}

bool CSplitterCommandModule::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  bool ret = false;
  if (xf6a_25_ || CPatterned::PathShagged(mgr, data)) {
    ret = true;
  }
  return ret;
}

void CSplitterCommandModule::Start(CStateManager& mgr, EStateMsg msg, float dt) {}

void CSplitterCommandModule::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
  case kStateMsg_Update:
    mFaceDirection = GetTransform().GetForward();
    break;
  }
}

void CSplitterCommandModule::SpawnIdle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mFaceDirection = CVector3f::Zero();
    break;
  case kStateMsg_Update:
    if (xefc_ == kInvalidUniqueId) {
      xefc_ = FindConnectedObject(mgr, kSS_Connect, kSM_Attach);
    }
    break;
  }
}

void CSplitterCommandModule::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    StopLaserSweep(mgr);
    DeathDelete(mgr);
    break;
  }
}

void CSplitterCommandModule::Scanning(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    SetHitByPlayerProjectile(false);
    mFaceDirection = CVector3f::Zero();
    UpdateAlertEffect(mgr);
    break;
  case kStateMsg_Update: {
    if (CSplitterMainChassis* chassis =
            TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(mMainChassisId))) {
      if (mHitByPlayerProjectile || InDetectionRange(mgr, CTriggerData(0.f))) {
        chassis->SetHitByPlayerProjectile(true);
      }
    }
    const float turnSpeed = dt * mData.scanningTurnSpeed;
    const CVector3f forward(GetTransform().GetForward().ToVec2f(), 0.f);
    const CVector3f right(GetTransform().GetRight().ToVec2f(), 0.f);
    if (forward.IsMagnitudeSafe() && right.IsMagnitudeSafe()) {
      mFaceDirection = CVector3f::Slerp(forward.AsNormalized(), right.AsNormalized(),
                                        CRelAngle::FromDegrees(turnSpeed));
    }
    break;
  }
  case kStateMsg_Deactivate:
    UpdateAlertEffect(mgr);
    break;
  }
}

void CSplitterCommandModule::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mFaceDirection = GetTransform().GetForward();
    break;
  case kStateMsg_Update:
    if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
        CVector3f toTarget = target->GetTranslation() - GetTranslation();
        toTarget.SetZ(0.f);
        if (toTarget.IsMagnitudeSafe()) {
          const CVector3f forward = GetTransform().GetForward();
          const float maxTurn = dt * mData.maxTurnSpeed;
          const float angleDiff = CVector3f::GetAngleDiff(forward, toTarget);
          if (angleDiff < CRelAngle::FromDegrees(maxTurn).AsRadians()) {
            mFaceDirection = toTarget.AsNormalized();
          } else {
            mFaceDirection =
                CVector3f::Slerp(forward, toTarget.AsNormalized(), CRelAngle::FromDegrees(maxTurn));
          }
        }
      }
    }
    break;
  }
}

void CSplitterCommandModule::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (xf6a_24_) {
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  }
  switch (msg) {
  case kStateMsg_Activate:
    xf6a_25_ = false;
    break;
  case kStateMsg_Update: {
    CVector3f moveVector = BodyController()->GetCommandMgr().GetMoveVector();
    moveVector += GetSeparation(mgr);
    if (moveVector.IsMagnitudeSafe()) {
      const CVector3f move = (dt * mData.maxLinearVelocity) * moveVector;
      const CVector3f dest = GetTranslation() + move;
      MoveTo(dest, dt);
      const TUniqueId faceTargetId = mPathFindNavigation.GetFaceTarget();
      CVector3f faceDir = move;
      if (const CActor* faceTarget = static_cast< const CActor* >(mgr.GetObjectById(faceTargetId))) {
        faceDir = faceTarget->GetTranslation() - GetTranslation();
        faceDir.SetZ(0.f);
      }
      BodyController()->FaceDirection(faceDir, dt);
    }
    break;
  }
  case kStateMsg_Deactivate:
    xf6a_24_ = true;
    break;
  }
}

void CSplitterCommandModule::Hover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update:
    if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
        CVector3f delta = target->GetAimPosition(mgr, 0.f) - GetTranslation();
        const float dz = delta.GetZ();
        delta.SetZ(0.f);
        BodyController()->FaceDirection(delta, dt);
        const float maxMove = dt * mData.maxLinearVelocity;
        if (fabsf(dz) > maxMove) {
          const CVector3f dest =
              GetTranslation() + CVector3f(0.f, 0.f, dz > 0.f ? maxMove : -maxMove);
          MoveTo(dest, dt);
        }
      }
    }
    break;
  }
}

void CSplitterCommandModule::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    xf6a_27_ = true;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(mDodgeDirection, pas::kStep_Dodge));
    } else if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
        CVector3f toTarget = target->GetTranslation() - GetTranslation();
        toTarget.SetZ(0.f);
        BodyController()->FaceDirection(toTarget, dt);
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    xf6a_27_ = false;
    break;
  }
}

void CSplitterCommandModule::LaserPulse(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    xeec_ = 0;
    --xf20_;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCProjectileAttackCmd(
          pas::kS_One, GetTranslation() + GetTransform().GetForward(), false));
    } else if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
        const CVector3f aimPos = target->GetAimPosition(mgr, 0.f);
        const CVector3f faceDir(aimPos.GetX() - GetTranslation().GetX(),
                                aimPos.GetY() - GetTranslation().GetY(), 0.f);
        BodyController()->FaceDirection(faceDir, dt);
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSplitterCommandModule::LaserSweep(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mLaserSweepDirection = GetTransform().GetForward();
    if (mgr.GetObjectById(mTargetId)) {
      const CVector3f toStart = mLaserSweepStart - GetTranslation();
      const CVector3f toEnd = mLaserSweepEnd - GetTranslation();
      if (CVector3f::GetAngleDiff(mLaserSweepDirection, toEnd) <
          CVector3f::GetAngleDiff(mLaserSweepDirection, toStart)) {
        const CVector3f start = mLaserSweepStart;
        mLaserSweepStart = mLaserSweepEnd;
        mLaserSweepEnd = start;
      }
    }
    CSfxManager::AddEmitter(mData.sound_LaserChargeUp, GetTranslation(), 127,
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
    break;
  case kStateMsg_Update:
    if (dt > 0.f) {
      const CVector3f target = mLaserSweepState == 0 ? mLaserSweepStart : mLaserSweepEnd;
      const CTransform4f lctrXf = GetLctrTransform(mBeamLocator);
      const CVector3f delta = target - lctrXf.GetTranslation();
      const CVector3f flatDelta = delta.DropZ();
      if (flatDelta.IsMagnitudeSafe()) {
        const CVector3f forward = GetTransform().GetForward();
        const float maxTurn = dt * mData.laserSweepTurnSpeed;
        const float angleDiff = CVector3f::GetAngleDiff(forward, flatDelta);
        if (angleDiff < CRelAngle::FromDegrees(maxTurn).AsRadians()) {
          mFaceDirection = flatDelta.AsNormalized();
          mLaserSweepDirection = delta.AsNormalized();
          if (mLaserSweepState == 0) {
            mLaserSweepState = 1;
            FireLaserSweep(mgr, target);
          } else {
            mLaserSweepState = -1;
          }
        } else {
          mFaceDirection = CVector3f::Slerp(forward, flatDelta.AsNormalized(),
                                            CRelAngle::FromDegrees(maxTurn));
          mLaserSweepDirection = CVector3f::Slerp(mLaserSweepDirection.AsNormalized(),
                                                  delta.AsNormalized(),
                                                  CRelAngle::FromDegrees(maxTurn));
        }
      } else {
        mLaserSweepState = -1;
      }
    }
    break;
  case kStateMsg_Deactivate:
    StopLaserSweep(mgr);
    mLaserSweepState = -1;
    break;
  }
}

void CSplitterCommandModule::LostChassisReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    xf6a_26_ = true;
    break;
  case kStateMsg_Update:
    ReorientUpright(dt);
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(GetTransform().GetForward(), pas::kS_Two));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSplitterCommandModule::SeekMainChassis(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    xf6a_28_ = false;
    break;
  case kStateMsg_Update:
    if (const CSplitterMainChassis* chassis =
            TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(xefc_))) {
      const CVector3f dockPos = chassis->GetDockPosition();
      const CVector3f delta = dockPos - GetTranslation();
      const float speed = dt * mData.maxLinearVelocity;
      if (delta.MagSquared() > speed * speed && delta.IsMagnitudeSafe()) {
        const CVector3f dest = GetTranslation() + speed * delta.AsNormalized();
        MoveTo(dest, dt);
      } else {
        SetTranslation(dockPos);
        mAnimationState.SetState(CAnimationState::kAS_Over);
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

void CSplitterCommandModule::FollowDockingPath(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  switch (msg) {
  case kStateMsg_Activate:
    mWaypointNavigation.SetDestination(GetConnectedObject(mgr, kSS_Connect, kSM_Follow));
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    break;
  case kStateMsg_Update: {
    const CVector3f moveVector = BodyController()->GetCommandMgr().GetMoveVector();
    const float speed = dt * mData.maxLinearVelocity;
    if (moveVector.IsMagnitudeSafe()) {
      const CVector3f dest = GetTranslation() + speed * moveVector;
      MoveTo(dest, dt);
      BodyController()->FaceDirection(moveVector, dt);
    }
    break;
  }
  }
}

void CSplitterCommandModule::NotifyDocking(CStateManager& mgr, int arg) {
  if (CSplitterMainChassis* chassis =
          TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(xefc_))) {
    if (chassis->Dock(mgr, GetUniqueId())) {
      mMainChassisId = xefc_;
      xefc_ = kInvalidUniqueId;
      SetNextDrawNode(mMainChassisId);
      if (!xf6a_30_ || (mData.unknown_0xbd80fd94 & 1) == 0) {
        CSfxManager::AddEmitter(mData.sound_Docking, GetTranslation(), 127,
                                GetCurrentAreaId().Value(), true, false,
                                CSfxManager::kMedPriority);
      }
      xf6a_30_ = false;
    }
  }
  BodyController()->SetLocomotionType(pas::kLT_Relaxed);
}

void CSplitterCommandModule::SelectTarget(CStateManager& mgr, int arg) {
  if (mTargetId == kInvalidUniqueId) {
    mTargetId = mgr.GetPlayer(0)->GetUniqueId();
  }
  UpdateAlertEffect(mgr);
  SendScriptMsgs(kSS_Attack, mgr, GetUniqueId());
}

void CSplitterCommandModule::SetTargetDest(CStateManager& mgr, int arg) {
  CVector3f dest = GetTranslation();
  xf6a_24_ = false;
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    dest = target->GetAimPosition(mgr, 0.f) + 4.f * CVector3f::Up();
    xf6a_24_ = true;
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(mTargetId);
}

void CSplitterCommandModule::SetDockingDest(CStateManager& mgr, int arg) {
  CVector3f dest = GetTranslation();
  xf6a_24_ = false;
  if (CSplitterMainChassis* chassis =
          TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(xefc_))) {
    dest = chassis->GetDockPosition() + 4.f * CVector3f::Up();
    xf6a_24_ = true;
    chassis->SetHitByPlayerProjectile(true);
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
}

void CSplitterCommandModule::FindBestDodgeDirection(CStateManager& mgr, int arg) {
  mDodgeDirection = pas::kSD_Invalid;
  if ((mData.unknown_0xbd80fd94 & 2) != 0 && xeec_ < mData.maxDodges) {
    if (xeec_ < mData.minDodges || mgr.Random()->Range(0.f, 100.f) <= mData.dodgeChance) {
      mDodgeDirection = FindDodgeDirection(mgr);
      ++xeec_;
    } else {
      xeec_ = mData.maxDodges;
    }
  }
}

void CSplitterCommandModule::RaiseShields(CStateManager& mgr, int arg) { xf6a_28_ = true; }

void CSplitterCommandModule::ResetAttackTimes(CStateManager& mgr, int arg) {
  xf0c_ = mData.minLaserPulseAttackTime;
  xf20_ = mgr.Random()->Range(1, mData.maxLaserPulseShots);
}

void CSplitterCommandModule::FindDockingTarget(CStateManager& mgr) {
  xefc_ = FindConnectedObject(mgr, kSS_Connect, kSM_Attach);
}

void CSplitterCommandModule::SetupCollisionActors(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(ARRAY_SIZE(skSphereJoints));
  const CAnimData* animData = GetModelData()->GetAnimationData();
  for (int i = 0; i < ARRAY_SIZE(skSphereJoints); ++i) {
    const SSphereJointInfo& joint = skSphereJoints[i];
    const CSegId segId = animData->GetLocatorSegId(rstl::string_l(joint.name));
    const CJointCollisionDescription desc = CJointCollisionDescription::SphereCollision(
        segId, CVector3f::Zero(), joint.radius, rstl::string_l(joint.name), 1000.f);
    joints.push_back(desc);
  }
  mCollisionActorManager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      if (desc.GetName() == rstl::string_l(skShieldJoint)) {
        xf00_ = id;
      }
    }
  }
  SetShieldState(mgr, false);
}

void CSplitterCommandModule::UpdateTimers(float dt, CStateManager& mgr) {
  xf10_ -= dt;
  xf0c_ -= dt;
  xf14_ -= dt;
  xf18_ += dt;
  xf1c_ += dt;
  if (xf08_ > 0.f) {
    xf08_ = rstl::max_val(xf08_ - dt, 0.f);
    xf04_ = CColor::Lerp(CColor::White(), mDamageColor, rstl::min_val(1.f, xf08_));
  }
  if (xf6a_29_ && GetAlive()) {
    xef0_ -= dt;
    if (xef0_ <= 0.f) {
      CPatterned::Death(mgr, GetTransform().GetForward(), kSS_DeathRattle);
    }
  }
}

void CSplitterCommandModule::UpdateDocking(CStateManager& mgr) {
  if (mMainChassisId != kInvalidUniqueId) {
    const CSplitterMainChassis* chassis =
        TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mMainChassisId));
    if (chassis && chassis->GetDockedCommandModule() == GetUniqueId()) {
      if (mTargetId != chassis->GetTargetId()) {
        mTargetId = chassis->GetTargetId();
        UpdateAlertEffect(mgr);
      }
      xf14_ = 3.f;
    } else {
      mMainChassisId = kInvalidUniqueId;
      SetNextDrawNode(kInvalidUniqueId);
    }
  } else if ((mData.unknown_0xbd80fd94 & 8) != 0 && xf14_ <= 0.f &&
             xefc_ == kInvalidUniqueId) {
    FindChassisToDock(mgr);
    xf14_ = 3.f;
  } else if (xefc_ != kInvalidUniqueId) {
    const CSplitterMainChassis* chassis =
        TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(xefc_));
    if (!chassis || !chassis->GetAlive()) {
      xefc_ = kInvalidUniqueId;
    }
  }
}

void CSplitterCommandModule::UpdateShields(CStateManager& mgr) {
  if (CParticleGenInfo* effect = GetShieldEffect()) {
    effect->SetModulationColor(xf04_);
  }
  CSfxManager::UpdateEmitter(mShieldSfx, GetTranslation(), GetTransform().GetForward(), 127);
  if (CCollisionActor* colAct = TCastToPtr< CCollisionActor >(mgr.ObjectById(xf00_))) {
    if (mCollisionActorManager.get() != nullptr) {
      mCollisionActorManager->SetActive(mgr, mMainChassisId == kInvalidUniqueId);
    }
    switch (mShieldType) {
    case kST_Light:
    case kST_Dark:
      if (!xf6a_28_ || colAct->HealthInfo()->GetHP() <= 0.f) {
        mShieldType = kST_None;
        xf10_ = mData.resetShieldTime;
        SetShieldState(mgr, false);
      }
      break;
    case kST_None:
    default:
      if (xf6a_28_ && xf10_ <= 0.f) {
        mShieldType = mLastShieldType == kST_Light ? kST_Dark : kST_Light;
        mLastShieldType = mShieldType;
        SetShieldState(mgr, true);
      }
      break;
    }
  }
}

void CSplitterCommandModule::SetShieldState(CStateManager& mgr, bool playSound) {
  CAnimData* animData = AnimationData();
  if (playSound) {
    CSfxManager::AddEmitter(mData.sound_ShieldOn, GetTranslation(), 127,
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  if (mShieldSfx) {
    CSfxManager::RemoveEmitter(mShieldSfx);
    mShieldSfx = CSfxHandle();
  }
  if (CCollisionActor* colAct = TCastToPtr< CCollisionActor >(mgr.ObjectById(xf00_))) {
    colAct->HealthInfo()->SetHP(mData.shieldHP);
    switch (mShieldType) {
    case kST_Light:
      colAct->AddMaterial(kMT_Character, mgr);
      AddMaterial(kMT_Unknown54, mgr);
      colAct->SetDamageVulnerability(mData.mLightShieldVulnerability);
      animData->SetEffectState(rstl::string_l(skLightShield), true, mgr);
      animData->SetEffectState(rstl::string_l(skDarkShield), false, mgr);
      mShieldSfx = CSfxManager::AddEmitter(mData.sound_LightShield, GetTranslation(), 127,
                                           GetCurrentAreaId().Value(), true, true,
                                           CSfxManager::kMedPriority);
      break;
    case kST_Dark:
      colAct->RemoveMaterial(kMT_Character, mgr);
      AddMaterial(kMT_Unknown54, mgr);
      colAct->SetDamageVulnerability(mData.mDarkShieldVulnerability);
      animData->SetEffectState(rstl::string_l(skLightShield), false, mgr);
      animData->SetEffectState(rstl::string_l(skDarkShield), true, mgr);
      mShieldSfx = CSfxManager::AddEmitter(mData.sound_DarkShield, GetTranslation(), 127,
                                           GetCurrentAreaId().Value(), true, true,
                                           CSfxManager::kMedPriority);
      break;
    case kST_None:
    default:
      colAct->RemoveMaterial(kMT_Character, mgr);
      RemoveMaterial(kMT_Unknown54, mgr);
      colAct->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
      animData->SetEffectState(rstl::string_l(skLightShield), false, mgr);
      animData->SetEffectState(rstl::string_l(skDarkShield), false, mgr);
      break;
    }
  }
}

void CSplitterCommandModule::OnDamaged(TUniqueId sender) {
  if (sender == xf00_) {
    xf08_ = 1.f;
  }
}

CParticleGenInfo* CSplitterCommandModule::GetShieldEffect() {
  if (mShieldType != kST_None) {
    const rstl::string name(mShieldType == kST_Light ? rstl::string_l(skLightShield)
                                                     : rstl::string_l(skDarkShield));
    return AnimationData()->GetFirstParticleEffect(name);
  }
  return nullptr;
}

void CSplitterCommandModule::ReorientUpright(float dt) {
  if (!xf6a_26_) {
    return;
  }
  const CVector3f pos = GetTranslation();
  const float maxAngle = M_2PIF * dt;
  const CVector3f up = CVector3f::Up();
  const CVector3f curUp = GetTransform().GetUp();
  CVector3f flatForward = GetTransform().GetForward();
  flatForward.SetZ(0.f);
  if (CVector3f::GetAngleDiff(up, curUp) <= maxAngle) {
    if (flatForward.IsNonZero()) {
      SetTransform(CTransform4f::LookAt(pos, pos + flatForward, CVector3f::Up()));
      xf6a_26_ = false;
      return;
    }
  }
  const CVector3f newUp =
      CVector3f::Slerp(curUp.AsNormalized(), up.AsNormalized(), CRelAngle::FromRadians(maxAngle));
  const CQuaternion arc = CQuaternion::ShortestRotationArc(curUp, newUp);
  const CQuaternion rot = CQuaternion::FromMatrix(GetTransform()) * arc;
  SetTransform(rot.BuildTransform4f(pos));
}

pas::EStepDirection CSplitterCommandModule::FindDodgeDirection(CStateManager& mgr) {
  const float rangeSq = mHoverDistance * mHoverDistance;
  const CVector3f pos = GetTranslation();
  bool leftClear = true;
  bool rightClear = true;
  pas::EStepDirection dir = pas::kSD_Invalid;
  const CVector3f right = GetTransform().GetRight();
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CEntity* ent = list[i];
    if (ent && ent != this && ent->GetCurrentAreaId() == GetCurrentAreaId()) {
      const CVector3f delta = static_cast< const CActor* >(ent)->GetTranslation() - pos;
      if (delta.MagSquared() < rangeSq) {
        if (CVector3f::Dot(delta, right) >= 0.f) {
          if (rightClear && CVector3f::GetAngleDiff(right, delta) < M_PIF / 3.f) {
            rightClear = false;
          }
        } else if (leftClear && CVector3f::GetAngleDiff(-right, delta) < M_PIF / 3.f) {
          leftClear = false;
        }
      }
    }
  }
  if (rightClear) {
    rightClear = IsDodgeClear(mgr, right, mHoverDistance);
  }
  if (leftClear) {
    leftClear = IsDodgeClear(mgr, -right, mHoverDistance);
  }
  if (leftClear && rightClear) {
    if (mgr.Random()->Next() & 0x4000) {
      leftClear = false;
    } else {
      rightClear = false;
    }
  }
  if (leftClear) {
    dir = pas::kSD_Left;
  } else if (rightClear) {
    dir = pas::kSD_Right;
  }
  return dir;
}

bool CSplitterCommandModule::IsDodgeClear(CStateManager& mgr, const CVector3f& dir, float dist) {
  const CVector3f center = GetBoundingBox().GetCenterPoint();
  const CVector3f end = center + dist * dir;
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Unknown59), CMaterialList(kMT_CollisionActor));
  if (mgr.RayCollideWorld(center, end, filter, this) &&
      mPathFindSearch.OnPath(end) == CPathFindSearch::kR_Success) {
    return true;
  }
  return false;
}

CVector3f CSplitterCommandModule::GetSeparation(CStateManager& mgr) {
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  CVector3f separation = CVector3f::Zero();
  float count = 0.f;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (const CPatterned* ai = TCastToConstPtr< CPatterned >(list[i])) {
      if (ai != this && ai->GetCurrentAreaId() == GetCurrentAreaId()) {
        const CSplitterCommandModule* module = TCastToConstPtr< CSplitterCommandModule >(ai);
        if (!module || module->mMainChassisId == kInvalidUniqueId) {
          const float radius = 5.f * GetModelData()->GetScale().GetX();
          const CVector3f away = mSteeringBehaviors.Separation(*this, ai->GetTranslation(), radius);
          if (away.IsMagnitudeSafe()) {
            separation += away.AsNormalized();
            count += 1.f;
          }
        }
      }
    }
  }
  if (count > 0.f) {
    separation *= 1.f / count;
  }
  return separation;
}

void CSplitterCommandModule::FindChassisToDock(CStateManager& mgr) {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  const CVector3f pos = GetTranslation();
  float bestDistSq = FLT_MAX;
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    if (CSplitterMainChassis* chassis = TCastToPtr< CSplitterMainChassis >(list[i])) {
      if (chassis->GetCurrentAreaId() == GetCurrentAreaId() &&
          chassis->RequestDocking(GetUniqueId())) {
        const CVector3f delta = chassis->GetTranslation() - pos;
        const float distSq = delta.MagSquared();
        if (distSq < bestDistSq) {
          if (CSplitterMainChassis* prev =
                  TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(xefc_))) {
            prev->CancelDockingRequest(GetUniqueId());
          }
          bestDistSq = distSq;
          xefc_ = chassis->GetUniqueId();
        }
      }
    }
  }
}

void CSplitterCommandModule::UpdateStuckTimer(float dt, CStateManager& mgr) {
  if (xf6a_31_) {
    xef4_ += dt;
    if (xef4_ >= 5.f) {
      AutoDestruct(0.f);
    }
    xf6a_31_ = false;
  } else {
    xef4_ = 0.f;
  }
}

void CSplitterCommandModule::MoveTo(const CVector3f& pos, float dt) {
  if (xef4_ > 0.f) {
    SetTranslation(pos);
  } else {
    const CVector3f delta = pos - GetTranslation();
    MoveInOneFrameOR(GetTransform().TransposeRotate(delta), dt);
  }
}

void CSplitterCommandModule::FireLaserPulse(CStateManager& mgr, const rstl::string& locator) {
  if (!GetAlive()) {
    return;
  }
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const CTransform4f lctrXf = GetLctrTransform(locator);
    const CVector3f muzzlePos = lctrXf.GetTranslation();
    const CVector3f aimPos = target->GetAimPosition(mgr, 0.f);
    const CVector3f delta = aimPos - muzzlePos;
    const CVector3f forward = GetTransform().GetForward();
    CVector3f offset = CVector3f::Dot(forward, delta) * forward;
    offset.SetZ(delta.GetZ());
    const CTransform4f lookAtXf =
        CTransform4f::LookAt(muzzlePos, muzzlePos + offset, CVector3f::Up());
    LaunchProjectile(lookAtXf, mgr, 6, CWeapon::kPA_None, false, CImpactVisorEffect(),
                     CVector3f(1.f, 1.f, 1.f));
  }
}

void CSplitterCommandModule::FireLaserSweep(CStateManager& mgr, const CVector3f& target) {
  const CBeamInfo beamInfo = TLdrToBeamInfo(mData.laserSweepBeamInfo, 0x91);
  const TUniqueId uid = mgr.AllocateUniqueId();
  CPlasmaProjectile* beam = rs_new CPlasmaProjectile(
      mLaserSweepProjectileInfo.Token(), rstl::string_l("LaserSweepBeam"), kWT_Light, beamInfo,
      CTransform4f::Identity(), kMT_NoPlatformCollision, mData.mLaserSweepDamage, uid,
      GetCurrentAreaId(), GetUniqueId(), CWeaponAssetInfo(), false, 0x21000);
  if (beam) {
    const CTransform4f lctrXf = GetLctrTransform(mBeamLocator);
    const CTransform4f fireXf =
        CTransform4f::LookAt(lctrXf.GetTranslation(), target, CVector3f::Up());
    beam->Fire(fireXf, mgr, false);
    beam->SetNextDrawNode(mMainChassisId);
    mgr.AddObject(*beam);
    const int areaId = GetCurrentAreaId().Value();
    CAudioSys::C3DEmitterParmData parms(150.f, 0.1f, 1, 127, 35);
    parms.mPos = beam->GetCurrentPos();
    parms.mDir = CVector3f::Up();
    parms.mSfxId = mData.sound_LaserSweep;
    mLaserSweepSfx = CSfxManager::AddEmitter(parms, areaId, true, true, CSfxManager::kMedPriority);
    mLaserSweepBeamId = uid;
  }
  UpdateAlertEffect(mgr);
}

void CSplitterCommandModule::UpdateLaserSweep(float dt, CStateManager& mgr) {
  if (CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(mLaserSweepBeamId))) {
    const CTransform4f lctrXf = GetLctrTransform(mBeamLocator);
    const CVector3f beamPos = lctrXf.GetTranslation();
    const CTransform4f beamXf =
        CTransform4f::LookAt(beamPos, beamPos + mLaserSweepDirection, CVector3f::Up());
    beam->UpdateFx(beamXf, dt, mgr);
    CSfxManager::UpdateEmitter(mLaserSweepSfx, beam->GetCurrentPos(), CVector3f::Up(), 127);
  }
}

void CSplitterCommandModule::StopLaserSweep(CStateManager& mgr) {
  if (CPlasmaProjectile* beam = static_cast< CPlasmaProjectile* >(mgr.ObjectById(mLaserSweepBeamId))) {
    beam->ResetBeam(mgr, false);
    CSfxManager::RemoveEmitter(mLaserSweepSfx);
    mgr.DeleteObjectRequest(mLaserSweepBeamId);
  }
  mLaserSweepBeamId = kInvalidUniqueId;
  mLaserSweepSfx = CSfxHandle();
  if (GetAlive()) {
    UpdateAlertEffect(mgr);
  }
}

void CSplitterCommandModule::UpdateAlertEffect(CStateManager& mgr) {
  CAnimData* animData = AnimationData();
  if (xedc_ == 2 || xedc_ == 1) {
    animData->SetEffectState(rstl::string_l(skAlertEye), false, mgr);
    animData->SetEffectState(rstl::string_l(skLaserMuzzle), false, mgr);
  } else if (mLaserSweepBeamId != kInvalidUniqueId) {
    animData->SetEffectState(rstl::string_l(skAlertEye), false, mgr);
    animData->SetEffectState(rstl::string_l(skLaserMuzzle), true, mgr);
  } else if (mTargetId != kInvalidUniqueId || IsScanning(mgr, CTriggerData(0.f))) {
    animData->SetEffectState(rstl::string_l(skAlertEye), true, mgr);
    animData->SetEffectState(rstl::string_l(skLaserMuzzle), false, mgr);
  } else {
    animData->SetEffectState(rstl::string_l(skAlertEye), false, mgr);
    animData->SetEffectState(rstl::string_l(skLaserMuzzle), false, mgr);
  }
}

void CSplitterCommandModule::UpdateBeamEffect(float dt, CStateManager& mgr) {
  CSplitterBeamEffect* beam = static_cast< CSplitterBeamEffect* >(mgr.ObjectById(mBeamEffectId));
  const CTransform4f beamXf = GetBeamEffectTransform();
  if (beam) {
    beam->SetTransform(beamXf);
  }
  if (IsScanning(mgr, CTriggerData(0.f))) {
    if (!beam && mDetectionRange > 0.f) {
      mBeamEffectId = mgr.AllocateUniqueId();
      beam = rs_new CSplitterBeamEffect(
          mBeamEffectId,
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
          rstl::string_l("Splitter Beam Effect"), beamXf, 64,
          CAbsAngle::FromDegrees(mData.unknown_0x9ec51fe4.angle), mDetectionRange,
          mData.unknown_0x9ec51fe4.cloudColor1, mData.unknown_0x9ec51fe4.cloudColor2,
          mData.unknown_0x9ec51fe4.addColor1, mData.unknown_0x9ec51fe4.addColor2,
          mData.unknown_0x9ec51fe4.cloudScale, mData.unknown_0x9ec51fe4.fadeOffSize,
          mData.unknown_0x9ec51fe4.openSpeed);
      mgr.AddObject(*beam);
      mScanSfx = CSfxManager::AddEmitter(mData.sound_Scanning, GetTranslation(), 127,
                                         GetCurrentAreaId().Value(), true, true,
                                         CSfxManager::kMedPriority);
    }
    if (beam) {
      beam->SetExpanding(true);
    }
    CSfxManager::UpdateEmitter(mScanSfx, GetTranslation(), GetTransform().GetForward(), 127);
  } else if (beam) {
    beam->SetExpanding(false);
    if (beam->IsFullyClosed()) {
      mgr.DeleteObjectRequest(mBeamEffectId);
      mBeamEffectId = kInvalidUniqueId;
    }
  } else {
    mBeamEffectId = kInvalidUniqueId;
    CSfxManager::RemoveEmitter(mScanSfx);
    mScanSfx = CSfxHandle();
  }
}

CTransform4f CSplitterCommandModule::GetBeamEffectTransform() const {
  CTransform4f xf = GetTransform();
  xf.SetTranslation(xf * GetLocatorTransform(rstl::string_l("Beam_LCTR")).GetTranslation());
  return xf;
}

CEntity* REL_LoadSplitterCommandModule(CStateManager& mgr, CInputStream& input,
                                       CEntityInfo& info) {
  SLdrSplitterCommandModule sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CSplitterCommandModuleData data(sldrThis.commandModuleProperties);
  return rs_new CSplitterCommandModule(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties),
      LdrToTransform4f(sldrThis.editorProperties), *modelData,
      LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, &sldrThis.commandModuleProperties.ingPossessionData),
      data);
}
