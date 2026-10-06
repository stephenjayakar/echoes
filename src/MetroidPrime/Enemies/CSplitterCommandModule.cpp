#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"

#include "Collision/CCollisionInfoList.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/TCastTo.hpp"

static const char* const skBeamLocator = "Beam_LCTR";

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
, xee0_(0)
, xee4_(0)
, mHoverDistance(0.f)
, xeec_(-1)
, xef0_(0.f)
, xef4_(0.f)
, mBeamLocator()
, mMainChassisId(kInvalidUniqueId)
, xefc_(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, xf00_(kInvalidUniqueId)
, xf02_(kInvalidUniqueId)
, xf04_(CColor::White())
, xf08_(0.f)
, xf0c_(0.f)
, xf10_(0.f)
, xf14_(3.f)
, xf18_(0.f)
, xf1c_(0.f)
, xf20_(0)
, mFaceDirection(xf.GetForward())
, xf30_(-1)
, xf34_(-1)
, xf38_(CVector3f::Zero())
, xf44_(CVector3f::Zero())
, xf50_(CVector3f::Zero())
, xf5c_(0)
, xf60_(0)
, xf64_(0)
, xf68_(kInvalidUniqueId)
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
  mBeamLocator = GetModelData()->GetAnimationData()->GetLocatorSegId(rstl::string_l(skBeamLocator));
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

void CSplitterCommandModule::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  CPatterned::Think(dt, mgr);
  mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  UpdateDocking(mgr);
  UpdateAutoDestruct(dt, mgr);
  UpdateShields(mgr);
  UpdateLaserSweep(dt, mgr);
  UpdateBeamEffect(dt, mgr);
  UpdateHover(dt, mgr);
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
    FireLaserPulse(node.GetLocatorName(), mgr);
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
      aimPos = CVector3f::Lerp(aimPos, chassis->GetAimPosition(mgr, dt),
                               rstl::min_val(xf18_, 1.f));
    }
  } else if (xf1c_ < 1.f) {
    if (const CSplitterMainChassis* chassis =
            TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mMainChassisId))) {
      aimPos = CVector3f::Lerp(chassis->GetAimPosition(mgr, dt), aimPos,
                               rstl::min_val(xf1c_, 1.f));
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

void CSplitterCommandModule::StartLaserSweep(const CVector3f& start, const CVector3f& end) {
  if (mMainChassisId == kInvalidUniqueId) {
    return;
  }
  xf34_ = 0;
  xf38_ = start;
  xf44_ = end;
}

CVector3f CSplitterCommandModule::GetBeamPosition() const {
  return GetLctrTransform(mBeamLocator).GetTranslation();
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
    return delta.Magnitude() * mDetectionAngle < CVector3f::Dot(GetTransform().GetForward(), delta);
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
    return (target->GetTranslation() - GetTranslation()).MagSquared() <= range * range;
  }
  return false;
}

bool CSplitterCommandModule::InLaserPulseRange(CStateManager& mgr,
                                               const CTriggerData& data) const {
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mTargetId))) {
    const float distSq = (target->GetTranslation() - GetTranslation()).MagSquared();
    bool ret = false;
    if (distSq >= mData.minLaserPulseRange * mData.minLaserPulseRange &&
        distSq <= mData.maxLaserPulseRange * mData.maxLaserPulseRange) {
      ret = true;
    }
    return ret;
  }
  return false;
}

bool CSplitterCommandModule::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  return xf30_ != -1;
}

bool CSplitterCommandModule::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return xf0c_ <= 0.f;
}

bool CSplitterCommandModule::ShouldFireAgain(CStateManager& mgr, const CTriggerData& data) const {
  return xf20_ > 0;
}

bool CSplitterCommandModule::ShouldLaserSweep(CStateManager& mgr,
                                              const CTriggerData& data) const {
  return xf34_ != -1;
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

void CSplitterCommandModule::ResetAttackTimes(CStateManager& mgr, int arg) {}

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
