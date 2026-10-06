#include "MetroidPrime/Enemies/CBabyMetroid.hpp"

#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver", static_cast< TriggerFunc >(&CMetroid::StateOver)},
    {"AttackOver", static_cast< TriggerFunc >(&CMetroid::AttackOver)},
    {"LostInterest", static_cast< TriggerFunc >(&CMetroid::LostInterest)},
    {"PatternShagged", static_cast< TriggerFunc >(&CMetroid::PatternShagged)},
    {"Attacked", static_cast< TriggerFunc >(&CMetroid::Attacked)},
    {"ShotAt", static_cast< TriggerFunc >(&CMetroid::ShotAt)},
    {"ShouldAttack", static_cast< TriggerFunc >(&CMetroid::ShouldAttack)},
    {"InAttackPosition", static_cast< TriggerFunc >(&CMetroid::InAttackPosition)},
    {"InPosition", static_cast< TriggerFunc >(&CMetroid::InPosition)},
    {"InRange", static_cast< TriggerFunc >(&CMetroid::InRange)},
    {"InDetectionRange", static_cast< TriggerFunc >(&CMetroid::InDetectionRange)},
    {"SpotPlayer", static_cast< TriggerFunc >(&CMetroid::SpotPlayer)},
    {"AggressionCheck", static_cast< TriggerFunc >(&CMetroid::AggressionCheck)},
    {"ShouldTurn", static_cast< TriggerFunc >(&CMetroid::ShouldTurn)},
    {"Leash", static_cast< TriggerFunc >(&CMetroid::Leash)},
    {"ShouldWallHang", static_cast< TriggerFunc >(&CMetroid::ShouldWallHang)},
    {"ShouldDodge", static_cast< TriggerFunc >(&CMetroid::ShouldDodge)},
    {"AnimOver", static_cast< TriggerFunc >(&CBabyMetroid::AnimOver)},
    {"ShouldSeekEnergySource",
     static_cast< TriggerFunc >(&CBabyMetroid::ShouldSeekEnergySource)},
    {"AbsorbFinished", static_cast< TriggerFunc >(&CBabyMetroid::AbsorbFinished)},
    {"InEnergySourcePosition",
     static_cast< TriggerFunc >(&CBabyMetroid::InEnergySourcePosition)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Patrol", static_cast< StateFunc >(&CMetroid::Patrol)},
    {"Generate", static_cast< StateFunc >(&CBabyMetroid::Generate)},
    {"SelectTarget", static_cast< StateFunc >(&CMetroid::SelectTarget)},
    {"PathFind", static_cast< StateFunc >(&CMetroid::PathFind)},
    {"TurnAround", static_cast< StateFunc >(&CMetroid::TurnAround)},
    {"TelegraphAttack", static_cast< StateFunc >(&CMetroid::TelegraphAttack)},
    {"Attack", static_cast< StateFunc >(&CBabyMetroid::Attack)},
    {"WallHang", static_cast< StateFunc >(&CMetroid::WallHang)},
    {"Dodge", static_cast< StateFunc >(&CMetroid::Dodge)},
    {"ExitHive", static_cast< StateFunc >(&CBabyMetroid::ExitHive)},
    {"AbsorbEnergy", static_cast< StateFunc >(&CBabyMetroid::AbsorbEnergy)},
    {"TransformIntoMetroid", static_cast< StateFunc >(&CBabyMetroid::TransformIntoMetroid)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"SetTargetDest", static_cast< CodeFunc >(&CMetroid::SetTargetDest)},
    {"SetPatrolDest", static_cast< CodeFunc >(&CMetroid::SetPatrolDest)},
    {"SetEnergySourceDest", static_cast< CodeFunc >(&CBabyMetroid::SetEnergySourceDest)},
};

CBabyMetroid::CBabyMetroid(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& mData,
                           const CPatternedInfo& pInfo, const CActorParameters& aParms,
                           const CMetroidData& metroidData)
: CMetroid(uid, name, info, xf, mData, pInfo, aParms, metroidData)
, xa48_(metroidData.xa4_)
, xa4c_(metroidData.xa0_)
, xa50_(0.f)
, xa64_(kInvalidUniqueId)
, xa66_(kInvalidUniqueId)
, xa68_(kInvalidUniqueId)
, mInitialScale(mData.GetScale().GetX())
, mBabyMetroidScale(metroidData.mBabyMetroidScale)
, xa74_(metroidData.xa8_)
, xa78_(0.f)
, mChanceToDodge(metroidData.mChanceToDodge)
, mDodgeCheckTimeInterval(metroidData.mDodgeCheckTimeInterval)
, xa84_(0.f)
, mGrowthVulnerability(metroidData.mBabyMetroidGrowthVulnerability)
, mShouldSeekEnergySource(false)
, xac8_25_(false) {
  ModelData()->SetScale(CVector3f(mInitialScale, mInitialScale, mInitialScale));
  const CMaterialFilter& filter = GetMaterialFilter();
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(filter.GetIncludeList(), CMaterialList(kMT_Character)));
  if (metroidData.mBabyMetroidTransformationParticleEffect != kInvalidAssetId) {
    mTransformationParticle = rstl::optional_object< TLockedToken< CGenDescription > >(
        TLockedToken< CGenDescription >(gpSimplePool->GetObj(
            SObjectTag('PART', metroidData.mBabyMetroidTransformationParticleEffect))));
  }
}

CBabyMetroid::~CBabyMetroid() {}

void CBabyMetroid::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CMetroid::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

void CBabyMetroid::Render(const CStateManager& mgr) const { CMetroid::Render(mgr); }

bool CBabyMetroid::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::AnimOver(mgr, data);
}

bool CBabyMetroid::ShouldSeekEnergySource(CStateManager&, const CTriggerData&) const {
  return mShouldSeekEnergySource;
}

bool CBabyMetroid::AbsorbFinished(CStateManager&, const CTriggerData&) const {
  return xa50_ >= xa4c_;
}

bool CBabyMetroid::InEnergySourcePosition(CStateManager&, const CTriggerData&) const {
  const CVector3f delta = x7c0_ - GetTranslation();
  return delta.MagSquared() < 4.f;
}

void CBabyMetroid::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mState = kAiState_Over;
    break;
  default:
    break;
  }
}
