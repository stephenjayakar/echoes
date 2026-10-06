#ifndef _CSPLITTERCOMMANDMODULE
#define _CSPLITTERCOMMANDMODULE

#include "Kyoto/Animation/CSegId.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "rstl/single_ptr.hpp"

class CCollisionActorManager;
class CSplitterMainChassis;

// Runtime copy of the loader record with converted damage data (layout from the REL copy
// constructor).
struct CSplitterCommandModuleData : public SLdrSplitterCommandModuleData {
  CSplitterCommandModuleData(const SLdrSplitterCommandModuleData& data)
  : SLdrSplitterCommandModuleData(data)
  , mLaserPulseDamage(LdrToDamageInfo(data.laserPulseDamage))
  , mLaserSweepDamage(LdrToDamageInfo(data.laserSweepDamage))
  , mLightShieldVulnerability(LdrToDamageVulnerability(data.lightShieldVulnerability))
  , mDarkShieldVulnerability(LdrToDamageVulnerability(data.darkShieldVulnerability)) {}
  ~CSplitterCommandModuleData() {}

  CDamageInfo mLaserPulseDamage;
  CDamageInfo mLaserSweepDamage;
  CDamageVulnerability mLightShieldVulnerability;
  CDamageVulnerability mDarkShieldVulnerability;
};
CHECK_SIZEOF(CSplitterCommandModuleData, 0x5AC)

// Original class name from the Wii SEL exports (TypesMatch__22CSplitterCommandModuleCFi,
// AutoDestruct__22CSplitterCommandModuleFf). Member names are guessed.
class CSplitterCommandModule : public CPatterned {
public:
  CSplitterCommandModule(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                         const CTransform4f& xf, const CModelData& mData,
                         const CActorParameters& aParms, const CPatternedInfo& pInfo,
                         const CSplitterCommandModuleData& data);
  ~CSplitterCommandModule() override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CPhysicsActor
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  // CAi
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  bool IsListening() const override { return true; }
  bool Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) override;

  // CPatterned
  CProjectileInfo* ProjectileInfo() override { return &mLaserPulseProjectileInfo; }
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  void SetupStateMachine(CStateManager& mgr) override;
  bool CanBeIngPossessed(CStateManager& mgr) const override;

  void AutoDestruct(float time);

  // Triggers
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;
  bool InDetectionRange(CStateManager& mgr, const CTriggerData& data) const;
  bool IsScanning(CStateManager& mgr, const CTriggerData& data) const;
  bool IsInitiallyDocked(CStateManager& mgr, const CTriggerData& data) const;
  bool IsDocked(CStateManager& mgr, const CTriggerData& data) const;
  bool HasDockingPath(CStateManager& mgr, const CTriggerData& data) const;
  bool HasDockingTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool DockingPathOver(CStateManager& mgr, const CTriggerData& data) const;
  bool HasTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool InHoverRange(CStateManager& mgr, const CTriggerData& data) const;
  bool InLaserPulseRange(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDodge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFireAgain(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldLaserSweep(CStateManager& mgr, const CTriggerData& data) const;
  bool PathOver(CStateManager& mgr, const CTriggerData& data) const;
  bool PathShagged(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Start(CStateManager& mgr, EStateMsg msg, float dt);
  void Idle(CStateManager& mgr, EStateMsg msg, float dt);
  void SpawnIdle(CStateManager& mgr, EStateMsg msg, float dt);
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);
  void Scanning(CStateManager& mgr, EStateMsg msg, float dt);
  void FaceTarget(CStateManager& mgr, EStateMsg msg, float dt);
  void PathFind(CStateManager& mgr, EStateMsg msg, float dt);
  void Hover(CStateManager& mgr, EStateMsg msg, float dt);
  void Dodge(CStateManager& mgr, EStateMsg msg, float dt);
  void LaserPulse(CStateManager& mgr, EStateMsg msg, float dt);
  void LaserSweep(CStateManager& mgr, EStateMsg msg, float dt);
  void LostChassisReaction(CStateManager& mgr, EStateMsg msg, float dt);
  void SeekMainChassis(CStateManager& mgr, EStateMsg msg, float dt);
  void FollowDockingPath(CStateManager& mgr, EStateMsg msg, float dt);

  // Code functions
  void NotifyDocking(CStateManager& mgr, int arg);
  void SelectTarget(CStateManager& mgr, int arg);
  void SetTargetDest(CStateManager& mgr, int arg);
  void SetDockingDest(CStateManager& mgr, int arg);
  void FindBestDodgeDirection(CStateManager& mgr, int arg);
  void RaiseShields(CStateManager& mgr, int arg);
  void ResetAttackTimes(CStateManager& mgr, int arg);

  // Guessed names.
  void StartLaserSweep(const CVector3f& start, const CVector3f& end);
  CVector3f GetBeamPosition() const;

private:
  // Guessed names.
  void UpdateDocking(CStateManager& mgr);
  void UpdateAutoDestruct(float dt, CStateManager& mgr);
  void UpdateShields(CStateManager& mgr);
  void UpdateLaserSweep(float dt, CStateManager& mgr);
  void UpdateBeamEffect(float dt, CStateManager& mgr);
  void UpdateStuckTimer(float dt, CStateManager& mgr);
  void UpdateAlertEffect(CStateManager& mgr);
  int FindDodgeDirection(CStateManager& mgr);
  void MoveTo(const CVector3f& pos, float dt);
  void FireLaserPulse(CStateManager& mgr, const rstl::string& locator);

  CSplitterCommandModuleData mData;
  CPathFindSearch mPathFindSearch;
  rstl::single_ptr< CCollisionActorManager > mCollisionActorManager;
  CProjectileInfo mLaserPulseProjectileInfo;
  CProjectileInfo mLaserSweepProjectileInfo;
  CDamageVulnerability mVulnerability;
  int xedc_;
  int xee0_;
  int xee4_;
  float mHoverDistance;
  int xeec_;
  float xef0_;
  float xef4_;
  CSegId mBeamLocator;
  TUniqueId mMainChassisId;
  TUniqueId xefc_;
  TUniqueId mTargetId;
  TUniqueId xf00_;
  TUniqueId xf02_;
  CColor xf04_;
  float xf08_;
  float xf0c_;
  float xf10_;
  float xf14_;
  float xf18_;
  float xf1c_;
  int xf20_;
  CVector3f mFaceDirection;
  int xf30_;
  int xf34_;
  CVector3f xf38_;
  CVector3f xf44_;
  CVector3f xf50_;
  int xf5c_;
  int xf60_;
  int xf64_;
  TUniqueId xf68_;
  bool xf6a_24_ : 1;
  bool xf6a_25_ : 1;
  bool xf6a_26_ : 1;
  bool xf6a_27_ : 1;
  bool xf6a_28_ : 1;
  bool xf6a_29_ : 1;
  bool xf6a_30_ : 1;
  bool xf6a_31_ : 1;
};
CHECK_SIZEOF(CSplitterCommandModule, 0xF70)

#endif // _CSPLITTERCOMMANDMODULE
