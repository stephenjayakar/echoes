#ifndef _CELITEPIRATE
#define _CELITEPIRATE

#include "types.h"

#include "Collision/CCollidableAABox.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimationParameters.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Enemies/SPositionHistory.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrShockWaveInfo.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/TToken.hpp"

#include "rstl/auto_ptr.hpp"
#include "rstl/optional_object.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"

class CCollisionActorManager;
class CElementGen;
class CGenDescription;
class CJointCollisionDescription;
class CShockWaveInfo;
class CSkinnedModel;
class CGenericFSM2;

// Echoes reworked the Prime 1 elite pirate: the grenade launcher became a rocket launcher actor,
// the energy-absorbing claw became a light/dark body shield, and the AI runs on an FSM2.
class CElitePirateData {
public:
  CElitePirateData(CAssetId stateMachine, int initialAnim, const CDamageInfo& meleeDamage,
                   float maxMeleeRange, float minShockwaveRange, float maxShockwaveRange,
                   float minRocketRange, float maxRocketRange, float meleeChance,
                   float shockwaveChance, CAssetId darkShield, ushort darkShieldSound,
                   CAssetId darkShieldPop, CAssetId lightShield, ushort lightShieldSound,
                   CAssetId lightShieldPop, float tauntInterval, float tauntVariance,
                   float meleeWeight, float shockwaveWeight, float rocketWeight,
                   float doubleShockwaveWeight, float repeatedAttackChance,
                   float energyAttractionForce, CAssetId energyAbsorbEffect,
                   ushort energyAbsorbSound, const CActorParameters& launcherActParams,
                   const CAnimationParameters& launcherAnimParams,
                   const CAnimationParameters& possessedLauncherAnimParams, CAssetId rocket,
                   const CDamageInfo& rocketDamage, int minRocketCount, int maxRocketCount,
                   CAssetId visorElectricEffect, ushort visorElectricSound,
                   const SLdrShockWaveInfo& singleShockWave,
                   const SLdrShockWaveInfo& doubleShockWave, CAssetId shieldedModel,
                   CAssetId shieldedSkinRules);

  CAssetId GetStateMachine() const { return mStateMachine; }
  int GetInitialAnim() const { return mInitialAnim; }
  const CDamageInfo& GetMeleeDamage() const { return mMeleeDamage; }
  float GetTauntInterval() const { return mTauntInterval; }
  float GetTauntVariance() const { return mTauntVariance; }
  float GetRepeatedAttackChance() const { return mRepeatedAttackChance; }
  float GetEnergyAttractionForce() const { return mEnergyAttractionForce; }
  CAssetId GetEnergyAbsorbEffect() const { return mEnergyAbsorbEffect; }
  ushort GetEnergyAbsorbSound() const { return mEnergyAbsorbSound; }
  const CActorParameters& GetLauncherActParams() const { return mLauncherActParams; }
  const CAnimationParameters& GetLauncherAnimParams() const { return mLauncherAnimParams; }
  const CAnimationParameters& GetPossessedLauncherAnimParams() const {
    return mPossessedLauncherAnimParams;
  }
  int GetMinRocketCount() const { return mMinRocketCount; }
  int GetMaxRocketCount() const { return mMaxRocketCount; }
  CAssetId GetVisorElectricEffect() const { return mVisorElectricEffect; }
  ushort GetVisorElectricSound() const { return mVisorElectricSound; }
  float GetMeleeChance() const { return mMeleeChance; }
  float GetShockwaveChance() const { return mShockwaveChance; }
  CAssetId GetDarkShield() const { return mDarkShield; }
  ushort GetDarkShieldSound() const { return mDarkShieldSound; }
  CAssetId GetDarkShieldPop() const { return mDarkShieldPop; }
  CAssetId GetLightShield() const { return mLightShield; }
  ushort GetLightShieldSound() const { return mLightShieldSound; }
  CAssetId GetLightShieldPop() const { return mLightShieldPop; }
  const SLdrShockWaveInfo& GetSingleShockWave() const { return mSingleShockWave; }
  const SLdrShockWaveInfo& GetDoubleShockWave() const { return mDoubleShockWave; }
  float GetMeleeWeight() const { return mMeleeWeight; }
  float GetShockwaveWeight() const { return mShockwaveWeight; }
  float GetRocketWeight() const { return mRocketWeight; }
  float GetDoubleShockwaveWeight() const { return mDoubleShockwaveWeight; }
  float GetMaxMeleeRange() const { return mMaxMeleeRange; }
  float GetMinShockwaveRange() const { return mMinShockwaveRange; }
  float GetMaxShockwaveRange() const { return mMaxShockwaveRange; }
  float GetMinRocketRange() const { return mMinRocketRange; }
  float GetMaxRocketRange() const { return mMaxRocketRange; }
  CAssetId GetRocket() const { return mRocket; }
  const CDamageInfo& GetRocketDamage() const { return mRocketDamage; }
  CAssetId GetShieldedModel() const { return mShieldedModel; }
  CAssetId GetShieldedSkinRules() const { return mShieldedSkinRules; }

private:
  CAssetId mStateMachine;
  int mInitialAnim;
  CDamageInfo mMeleeDamage;
  float mTauntInterval;
  float mTauntVariance;
  float mRepeatedAttackChance;
  float mEnergyAttractionForce;
  CAssetId mEnergyAbsorbEffect;
  ushort mEnergyAbsorbSound;
  CActorParameters mLauncherActParams;
  CAnimationParameters mLauncherAnimParams;
  CAnimationParameters mPossessedLauncherAnimParams;
  int mMinRocketCount;
  int mMaxRocketCount;
  CAssetId mVisorElectricEffect;
  ushort mVisorElectricSound;
  float mMeleeChance;
  float mShockwaveChance;
  CAssetId mDarkShield;
  ushort mDarkShieldSound;
  CAssetId mDarkShieldPop;
  CAssetId mLightShield;
  ushort mLightShieldSound;
  CAssetId mLightShieldPop;
  SLdrShockWaveInfo mSingleShockWave;
  SLdrShockWaveInfo mDoubleShockWave;
  float mMeleeWeight;
  float mShockwaveWeight;
  float mRocketWeight;
  float mDoubleShockwaveWeight;
  float mMaxMeleeRange;
  float mMinShockwaveRange;
  float mMaxShockwaveRange;
  float mMinRocketRange;
  float mMaxRocketRange;
  CAssetId mRocket;
  CDamageInfo mRocketDamage;
  CAssetId mShieldedModel;
  CAssetId mShieldedSkinRules;
};
CHECK_SIZEOF(CElitePirateData, 0x190)

class CElitePirate : public CPatterned {
public:
  // Guessed names; the attack picked by the Shockwave state.
  enum EAttackType {
    kAT_Melee,
    kAT_Shockwave,
    kAT_Rocket,
    kAT_DoubleShockwave = 5,
  };
  // Guessed names; the scripted action a state is running (set while the state is active).
  enum EAction {
    kA_Invalid = -1,
    kA_None,
    kA_Alert,
    kA_ProjectileAttack,
    kA_FollowAttackPattern,
    kA_MeleeAttack,
    kA_PowerDown,
    kA_PowerUp,
    kA_Shockwave,
    kA_SpreadShot,
  };
  // Guessed names; the body shield colour.
  enum EShieldType {
    kST_Dark,
    kST_Light,
    kST_Random,
  };

  CElitePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
               const CActorParameters& actParms, const CElitePirateData& data);

  // CEntity
  ~CElitePirate() override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override { return &mCollisionAabb; }

  // CPatterned
  CProjectileInfo* ProjectileInfo() override;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  bool CanBeUnPossessed(CStateManager& mgr) const override;
  void RenderIngSnatchingTransition(const CStateManager& mgr) const override;

  // CElitePirate
  virtual void SetupStateMachineFunctions(CStateManager& mgr); // Guessed name.

  virtual bool Alerted(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool AngryAttackOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BreakProjectileAttack(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanShockwave(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ClearLineOfSight(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool DonePursuing(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool DoneTurning(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InDetectionRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InPosition(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool NotReachedTarget(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PickedSpreadShot(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PlayerInNoAttack(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PoweredDown(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PoweredUp(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ReturnedToPatrol(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ReadyToCharge(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShieldKilled(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShockwaveIsNext(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShotAt(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldAlert(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldTurn(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldFire(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldShockwave(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool SpotPlayer(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool StillAngry(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TargetNotOnMesh(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TargetUnreachable(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TooClose(CStateManager& mgr, const CTriggerData& data) const;

  virtual void Alert(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void AngryAttackBegin(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void AngryAttackEnd(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Dead(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void InvulnAlert(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void PowerDown(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void PowerUp(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Pursue(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Shielding(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void ShieldUp(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Shockwave(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void SpreadShot(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Stunned(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Taunt(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Turn(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Wait(CStateManager& mgr, EStateMsg msg, float dt);

  virtual void SelectTarget(CStateManager& mgr, float dt);
  virtual void PickAttackType(CStateManager& mgr, float dt);

  virtual void SetShieldActive(CStateManager& mgr, bool active); // Guessed name.
  virtual void ActivateGrenadeLauncher(CStateManager& mgr, bool active);
  virtual CShockWaveInfo GetShockWaveInfo() const;

private:
  // Guessed name; the light/dark body shield and its effects.
  struct SShield {
    SShield()
    : x0_(0)
    , mStartTime(-1000.f)
    , mDamage(0.f)
    , mLastHitTime(-1000.f)
    , mAlpha(0.f)
    , mType(kST_Random)
    , mElementGen(nullptr)
    , mSfx()
    , mActive(false)
    , mPendingRebuild(false) {}

    int x0_;
    float mStartTime;
    float mDamage;
    float mLastHitTime;
    float mAlpha;
    int mType;
    rstl::optional_object< TLockedToken< CGenDescription > > mDarkPop;
    rstl::optional_object< TLockedToken< CGenDescription > > mLightPop;
    rstl::auto_ptr< CElementGen > mElementGen;
    CSfxHandle mSfx;
    rstl::optional_object< TLockedToken< CSkinnedModel > > mModel;
    bool mActive : 1;
    bool mPendingRebuild : 1;
  };

  // Guessed name; the attack-pattern FSM2 resource.
  struct SStateMachineToken : public rstl::optional_object< CToken > {
    explicit SStateMachineToken(CAssetId id);
  };

  // Guessed name; the last few scripted actions.
  struct SActionHistory {
    SActionHistory() {}
    void Push(EAction action) {
      if (mActions.size() == 3) {
        mActions.erase(mActions.begin());
      }
      mActions.push_back(action);
    }

    rstl::reserved_vector< EAction, 4 > mActions;
  };

  // Guessed name; the rocket volley state.
  struct SRocketAttack {
    SRocketAttack(CAssetId rocket, const CDamageInfo& damage)
    : mTarget(CVector3f::Zero())
    , mInfo(rocket, damage)
    , mFired(0)
    , mToFire(0)
    , mFireMode(0)
    , mNextFireMode(1)
    , mBreakAttack(false) {
      mInfo.Token().Lock();
    }

    CVector3f mTarget;
    CProjectileInfo mInfo;
    int mFired;
    int mToFire;
    int mFireMode;
    int mNextFireMode;
    bool mBreakAttack;
  };

  struct SJointInfo {
    const char* mFrom;
    const char* mTo;
    float mRadius;
    float mSeparation;
  };
  struct SSphereJointInfo {
    const char* mName;
    float mRadius;
  };

  static const SJointInfo skLeftArmJointList[3];
  static const SJointInfo skRightArmJointList[3];
  static const SSphereJointInfo skSphereJointList[8];
  static const char* const skpHeadLCTR;
  static const char* const skpLauncherLCTR;
  static const char* const skpRightClawLCTR;
  static const char* const skpLeftClawLCTR;
  static const char* const skpGrenadeLauncherLCTR;
  static const char* const skpRightBallLCTR;
  static const CVector3f skExtendedClawBounds;
  static const CVector3f skLocalShieldBounds;

  void ReleaseClaimedRegion(CStateManager& mgr);
  void ClaimPathRegion(CStateManager& mgr);
  CPFArea* GetPathArea(CStateManager& mgr) const;
  int GetNearbyHintType(CStateManager& mgr) const;
  bool IsNearNoAttackHint(CStateManager& mgr) const;
  bool IsOnPath(CStateManager& mgr) const;
  void SetShotAt(bool shotAt);
  void ApplyPlayerImpulse(CStateManager& mgr, float force, float lift);
  void ProcessStompGround(CStateManager& mgr);
  void UpdateAttackTimeLeft(CStateManager& mgr);
  void UpdatePathDestination(CStateManager& mgr, const CVector3f& dest, float dt);
  void UpdateBreadCrumbTrail();
  CVector3f GetGrenadeLaunchPos(const CActor& actor) const;
  void UpdateGrenadeLauncher(CStateManager& mgr, TUniqueId& uid,
                             const rstl::string& locator) const;
  void ActivateGrenadeLauncherById(CStateManager& mgr, bool active, TUniqueId uid) const;
  void DeleteGrenadeLauncher(CStateManager& mgr);
  void CreateGrenadeLauncher(CStateManager& mgr, TUniqueId uid);
  void UpdateAILogicTimers(float dt);
  bool IsShieldUp() const;
  void SetupPathFindSearch();
  void ExtendTouchBounds(CStateManager& mgr, const rstl::reserved_vector< TUniqueId, 7 >& ids,
                         const CVector3f& bounds) const;
  bool IsArmClawCollider(TUniqueId uid, const rstl::reserved_vector< TUniqueId, 7 >& ids) const;
  bool IsArmClawCollider(const rstl::string& name, const char* locator,
                         const SJointInfo* joints, int count) const;
  void AddSphereCollisionList(const SSphereJointInfo* joints, int count,
                              rstl::vector< CJointCollisionDescription >& list);
  void AddCollisionList(const SJointInfo* joints, int count,
                        rstl::vector< CJointCollisionDescription >& list);
  void SetupCollisionActorInfo(CStateManager& mgr);
  void SetupCollisionManager(CStateManager& mgr);
  void PopShield(CStateManager& mgr);
  const char* GetShieldEffectName() const;
  void SetCurrentAction(EAction action, EStateMsg msg);
  void PursueTarget(CStateManager& mgr, EStateMsg msg, const CVector3f& target, float dt);
  bool ShouldFireLauncher(CStateManager& mgr, TUniqueId uid) const;
  void ApplyMeleeDamage(CStateManager& mgr, TUniqueId uid);
  void CreateShockWave(CStateManager& mgr, const CInt32POINode& node);
  void LaunchRocket(CStateManager& mgr);
  void RenderShield() const;
  void SetInvulnerable(CStateManager& mgr, EStateMsg msg);
  void UpdateAimBlend(float dt);
  void UpdateShieldEffect(CStateManager& mgr, float dt);
  void UpdateShieldFade(float dt);
  void AvoidObstacles(CStateManager& mgr, float dt);
  bool IsInitialAnimLocomotion(int type) const;
  CGenericFSM2* GetStateMachine();

  int x7c0_;
  CDamageVulnerability mVulnerability;
  rstl::single_ptr< CCollisionActorManager > mShieldCollisionMgr;
  CVector3f mLeftClawPos;
  CVector3f mRightClawPos;
  CVector3f mPathDestination;
  CVector3f mAlertPos;
  CElitePirateData mData;
  rstl::single_ptr< CCollisionActorManager > mCollisionActorMgr;
  CCollidableAABox mCollisionAabb;
  rstl::optional_object< TLockedToken< CGenDescription > > mEnergyAbsorbDesc;
  TUniqueId mCollisionHeadId;
  TUniqueId mLauncherId;
  rstl::reserved_vector< TUniqueId, 7 > mCollisionRJointIds;
  rstl::reserved_vector< TUniqueId, 7 > mCollisionLJointIds;
  TUniqueId mShieldCollisionId;
  TUniqueId mTargetId;
  float mInitialSpeed;
  float mSteeringSpeed;
  float mHp;
  float mAttackTimer;
  float mShotAtTimer;
  float mTime;
  float mStuckTime;
  float mPursueStartTime;
  float mLastObstacleTime;
  int mClaimedRegion;
  CPathFindSearch mPathFindSearch;
  CVector3f mTargetDestPos;
  SPositionHistory mPositionHistory;
  bool mDamageOn : 1;
  bool mShotAt : 1;
  bool mAlert : 1;
  bool xc10_27_ : 1;
  bool mAlerted : 1;
  bool mReturnedToPatrol : 1;
  bool mLauncherPossessed : 1;
  bool mInvulnAlert : 1;
  SActionHistory mActionHistory;
  int xc28_;
  EAction mCurrentAction;
  SStateMachineToken mStateMachineToken;
  int mAlertTauntType;
  int mLocomotionType;
  bool mPoweredUp : 1;
  int mMeleeSeverity;
  int mNextMeleeSeverity;
  float mLastMeleeTime;
  bool mMeleeDamageOn : 1;
  bool mMeleeKnockBack : 1;
  SShield mShield;
  int mAttackType;
  int mLastAttackType;
  CVector3f mTurnDirection;
  SRocketAttack mRocket;
  float mAimBlend;
  mutable CVector3f mAimPos;
  int mTauntType;
  int mAngryCount;
  bool mShockwaveIsNext : 1;
  bool mAngryAttackOver : 1;
  bool mAngryChoice : 1;
  bool mAngryChosen : 1;
  float mInvulnAlpha;
  bool mInvulnerable;
};
CHECK_SIZEOF(CElitePirate, 0xd30)

#endif // _CELITEPIRATE
