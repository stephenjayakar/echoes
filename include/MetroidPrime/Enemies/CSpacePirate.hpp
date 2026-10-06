#ifndef _CSPACEPIRATE
#define _CSPACEPIRATE

#include "MetroidPrime/CBoneTracking.hpp"
#include "MetroidPrime/CIkChain.hpp"
#include "MetroidPrime/Enemies/CBouncyGrenade.hpp"
#include "MetroidPrime/Enemies/CBurstFire.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Enemies/CPirateRagDoll.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "rstl/list.hpp"
#include "rstl/optional_object.hpp"
#include "rstl/single_ptr.hpp"

class CMaterialList;
class CTriggerData;

// Guessed name. Weapon block of the space pirate data; the launcher fields follow SLdrSpacePirate.
struct SSpacePirateWeaponData {
  int mEquippedWeapon;
  CAssetId mGrenadeLauncher;
  CBouncyGrenadeData mGrenadeData;
  int x58_;
  float x5c_;
  float x60_;
  float x64_;
  float x68_;
};
CHECK_SIZEOF(SSpacePirateWeaponData, 0x6c)

// Prime's CSpacePirateData, built by the REL loader from SLdrSpacePirate.
class CSpacePirateData {
public:
  float mAggressionCheck;
  float mCoverCheck;
  float mSearchRadius;
  float mFallBackCheck;
  float mFallBackRadius;
  float mHearingRadius;
  uint mFlags;
  bool x1c_;
  CAssetId mProjectile;
  CDamageInfo mProjectileDamage;
  ushort mSound_Projectile;
  CDamageInfo mBladeDamage;
  float mKneelAttackChance;
  CAssetId mKneelAttackShot;
  CDamageInfo mKneelAttackDamage;
  float mDodgeCheck;
  ushort mSound_Impact;
  float mAverageNextShotTime;
  float mNextShotTimeVariation;
  float x94_;
  float x98_;
  ushort mSound_Alert;
  float mGunTrackDelay;
  int mFirstBurstCount;
  float mCloakOpacity;
  float mMaxCloakOpacity;
  float mDodgeDelayTimeMin;
  float mDodgeDelayTimeMax;
  ushort mSound_Hurled;
  ushort mSound_Death;
  float xbc_;
  float mAvoidDistance;
  float xc4_;
  SSpacePirateWeaponData mWeaponData;
};
CHECK_SIZEOF(CSpacePirateData, 0x134)

class CSpacePirate : public CPatterned {
public:
  CSpacePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CActorParameters& actParms,
               const CPatternedInfo& pInfo, const CSpacePirateData& data);

  // CEntity
  ~CSpacePirate() override;
  CEntity* TypesMatch(int typeId) const override;
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  const CDamageVulnerability* GetDamageVulnerability(const CVector3f& position,
                                                     const CVector3f& direction,
                                                     const CDamageInfo& damage) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CAi
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  bool IsListening() const override { return true; }
  bool Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) override;
  CVector3f GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                      const CVector3f& aimPos) const override;

  // CPatterned
  uchar GetModelAlphau8(const CStateManager& mgr) const override;
  CProjectileInfo* ProjectileInfo() override;
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  void SetupStateMachine(CStateManager& mgr) override;
  CRagDoll* GetRagDoll() const override;
  void SetAttackTarget(CStateManager& mgr, TUniqueId target) override;
  TUniqueId GetAttackTarget() const override { return mTargetId; }
  float GetGravityConstant() const override { return skGravityConstant; }
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;
  bool TryToBeCaptured(CStateManager& mgr) override;

  // State functions
  void Ambushing(CStateManager& mgr, EStateMsg msg, float dt);
  void WarpIn(CStateManager& mgr, EStateMsg msg, float dt);
  void WarpOut(CStateManager& mgr, EStateMsg msg, float dt);
  void PostWarpOut(CStateManager& mgr, EStateMsg msg, float dt);
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  void Crouch(CStateManager& mgr, EStateMsg msg, float dt);
  void CoverAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void Halt(CStateManager& mgr, EStateMsg msg, float dt);
  void Run(CStateManager& mgr, EStateMsg msg, float dt);
  void PathFind(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt);
  void Shuffle(CStateManager& mgr, EStateMsg msg, float dt);
  void TurnAround(CStateManager& mgr, EStateMsg msg, float dt);
  void Dodge(CStateManager& mgr, EStateMsg msg, float dt);
  void Lurk(CStateManager& mgr, EStateMsg msg, float dt);
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt);
  void Cover(CStateManager& mgr, EStateMsg msg, float dt);
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);
  void TargetCover(CStateManager& mgr, EStateMsg msg, float dt);
  void TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt);
  void Approach(CStateManager& mgr, EStateMsg msg, float dt);
  void WallHang(CStateManager& mgr, EStateMsg msg, float dt);
  void WallDetach(CStateManager& mgr, EStateMsg msg, float dt);
  void GetUp(CStateManager& mgr, EStateMsg msg, float dt);
  void Generate(CStateManager& mgr, EStateMsg msg, float dt);
  void Skid(CStateManager& mgr, EStateMsg msg, float dt);
  void DoubleSnap(CStateManager& mgr, EStateMsg msg, float dt);
  void JumpBack(CStateManager& mgr, EStateMsg msg, float dt);
  void Bounce(CStateManager& mgr, EStateMsg msg, float dt);
  void PathFindEx(CStateManager& mgr, EStateMsg msg, float dt);
  void Enraged(CStateManager& mgr, EStateMsg msg, float dt);
  void Jump(CStateManager& mgr, EStateMsg msg, float dt);
  void Deactivate(CStateManager& mgr, EStateMsg msg, float dt);
  void LaunchGrenade(CStateManager& mgr, EStateMsg msg, float dt);
  void Captured(CStateManager& mgr, EStateMsg msg, float dt);
  void RemoveFromWorld(CStateManager& mgr, EStateMsg msg, float dt);

  // Trigger functions
  bool Stuck(CStateManager& mgr, const CTriggerData& data) const;
  bool PatternShagged(CStateManager& mgr, const CTriggerData& data) const;
  bool HearShot(CStateManager& mgr, const CTriggerData& data) const;
  bool HearPlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool AggressionCheck(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverCheck(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverFind(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverBlown(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverNearlyBlown(CStateManager& mgr, const CTriggerData& data) const;
  bool CoveringFire(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool LineOfSight(CStateManager& mgr, const CTriggerData& data) const;
  bool PatternOver(CStateManager& mgr, const CTriggerData& data) const;
  bool SpotPlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDodge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const;
  bool InRange(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldMove(CStateManager& mgr, const CTriggerData& data) const;
  bool ShotAt(CStateManager& mgr, const CTriggerData& data) const;
  bool Attacked(CStateManager& mgr, const CTriggerData& data) const;
  bool HasTargetingPoint(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const;
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldStrafe(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSpecialAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool StartAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool BreakAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool LostInterest(CStateManager& mgr, const CTriggerData& data) const;
  bool BounceFind(CStateManager& mgr, const CTriggerData& data) const;
  bool OffLine(CStateManager& mgr, const CTriggerData& data) const;
  bool Landed(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const;
  bool Leash(CStateManager& mgr, const CTriggerData& data) const;
  bool HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const;
  bool IsAmbushing(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldLaunchGrenade(CStateManager& mgr, const CTriggerData& data) const;
  bool InProjectileRange(CStateManager& mgr, const CTriggerData& data) const;

  // DOL wrappers dispatch through SSpacePirate_FuncPtrs to the REL implementations.
  bool AttachActorToPirate(TUniqueId id);
  void DetachActorFromPirate();
  bool AttachActor(TUniqueId id); // Guessed name; REL side of AttachActorToPirate.
  void DetachActor();             // Guessed name; REL side of DetachActorFromPirate.

  TUniqueId GetAttachedActor() const { return mAttachedActor; }

private:
  void SetupGrenadeLauncher(const SSpacePirateWeaponData& data); // Guessed name.
  void SetEyeParticleActive(CStateManager& mgr, bool active);
  bool ShouldFrenzy(CStateManager& mgr);
  void UpdateLeashTimer(float dt);
  void UpdateCloak(float dt, CStateManager& mgr);
  void UpdateSfxEmitter(); // Guessed name.
  void UpdateAttacks(float dt, CStateManager& mgr);
  void UpdateAimBodyState(float dt, CStateManager& mgr);
  TUniqueId UpdateTarget(CStateManager& mgr); // Guessed name.
  bool FireProjectile(float dt, CStateManager& mgr);
  void LaunchGrenadeProjectile(CStateManager& mgr); // Guessed name.
  void ComputeLaunchSpeedAndAngle(const CVector3f& target, const CVector3f& origin,
                                  float& angleOut, float& speedOut) const; // Guessed name.
  CVector3f GetGrenadeTargetPosition(const CStateManager& mgr,
                                     CActor* target) const; // Guessed name.
  void RenderGrenadeLauncher(const CStateManager& mgr, const CTransform4f& xf,
                             const CModelFlags& flags) const; // Guessed name.
  TUniqueId ChooseAttackTarget(CStateManager& mgr); // Guessed name.
  void StartWarpOut(CStateManager& mgr, bool flag); // Guessed name.
  void SquadAdd(CStateManager& mgr);
  void SquadRemove(CStateManager& mgr);
  void SquadReset(CStateManager& mgr);
  void SetTeamAiTarget(CStateManager& mgr); // Guessed name.
  bool CheckTargetable(CStateManager& mgr);
  void SetVelocityForJump();
  void CheckForProjectiles(CStateManager& mgr);
  bool LineOfSightTest(CStateManager& mgr, const CVector3f& eyePos, const CVector3f& targetPos,
                       const CMaterialList& excludeList);
  void UpdateCantSeePlayer(CStateManager& mgr, float dt);
  void UpdateHeldPosition(CStateManager& mgr, float dt);
  void AvoidActors(CStateManager& mgr);
  void CheckBlade(CStateManager& mgr);
  CVector3f GetTargetPos(CStateManager& mgr);
  void SetCinematicCollision(CStateManager& mgr);
  void SetNonCinematicCollision(CStateManager& mgr);

  static const float skGravityConstant;
  static const float skRadii[14];
  static const SBurst skBurstsQuick[];
  static const SBurst skBurstsStandard[];
  static const SBurst skBurstsFrenzied[];
  static const SBurst skBurstsJumping[];
  static const SBurst skBurstsInjured[];
  static const SBurst skBurstsSeated[];
  static const SBurst skBurstsQuickOOV[];
  static const SBurst skBurstsStandardOOV[];
  static const SBurst skBurstsFrenziedOOV[];
  static const SBurst skBurstsJumpingOOV[];
  static const SBurst skBurstsInjuredOOV[];
  static const SBurst skBurstsSeatedOOV[];
  static const SBurst* skBursts[];
  static rstl::list< TUniqueId > mChargePlayerList;

  CSpacePirateData mPirateData;

  bool mPendingAmbush : 1;
  bool mCeilingAmbush : 1;
  bool mNonAggressive : 1;
  bool mMelee : 1;
  bool mNoShuffleCloseCheck : 1;
  bool mOnlyAttackInRange : 1;
  bool x8f4_30_ : 1;
  bool mNoKnockbackImpulseReset : 1;
  bool mNoMeleeAttack : 1;
  bool mBreakAttack : 1;
  bool mSeated : 1;
  bool mShadowPirate : 1;
  bool mAlertBeforeCloak : 1;
  bool mNoBreakDodge : 1;
  bool mFloatingCorpse : 1;
  bool mRagdollNoAiCollision : 1;
  bool mTrooper : 1;
  mutable bool mHearNoise : 1;
  bool mEnableMeleeAttack : 1;
  bool x8f6_27_ : 1;
  bool x8f6_28_ : 1;
  bool mEnableRetreat : 1;
  bool mShuffleClose : 1;
  bool mInAttackState : 1;
  bool mEnablePatrol : 1;
  bool mEnableAim : 1;
  bool mHearPlayerFire : 1;
  bool mInProjectilePath : 1;
  bool mNoPlayerLos : 1;
  bool mInWallHang : 1;
  bool mJumpVelSet : 1;
  bool mPrevInCineCam : 1;
  bool mPendingFrenzyChance : 1;
  bool mAppliedBladeDamage : 1;
  bool mAlwaysAggressive : 1;
  bool mCoverCheck : 1;
  bool mEnableDodge : 1;
  bool mNoPlayerDodge : 1;
  bool mAllEnergyDrained : 1;
  mutable bool mMayStartAttack : 1;
  bool x8f9_24_ : 1;
  bool mUseJumpBackJump : 1;
  bool mStarted : 1;
  bool mInRange : 1;
  bool mSatUp : 1;
  bool mEnableBreakDodge : 1;
  bool mCloseMelee : 1;
  bool mSentAttackMsg : 1;
  bool mNormalDodge : 1;
  bool x8fa_25_ : 1;
  bool x8fa_26_ : 1;
  bool x8fa_27_ : 1;
  bool x8fa_28_ : 1;
  bool x8fa_29_ : 1;
  bool x8fa_30_ : 1;
  bool x8fa_31_ : 1;

  int mFrenzyFrames;
  TUniqueId mCoverPoint;
  TUniqueId mPreviousCoverPoint;
  float mSteeringSpeed;
  CVector3f mTargetDelta;
  CVector3f mCoverPointRearDir;
  CPathFindSearch mPathFindSearch;
  float mUnkTimer;
  float mSteeringDelayTimer;
  uint xa14_;
  float mInitialHP;
  float mCoverRange;
  CSegId mHeadSeg;
  uint xa24_;
  pas::ETauntType mTaunt;
  CBoneTracking mBoneTracking;
  pas::ECoverDirection mCoverDir;
  uchar mPad[4];
  float mIntoJumpDist;
  float mEyeHeight;
  float mTimeNoPlayerLos;
  float xa7c_;
  float xa80_;
  TUniqueId mAttachedActor;
  CSegId mGunSeg;
  CSegId mElbowSeg;
  CSegId mWristSeg;
  CSegId mSwooshSeg;
  CSegId mLeftHipSeg;  // Guessed name; "L_hip" locator.
  CSegId mRightHipSeg; // Guessed name; "R_hip" locator.
  CSegId mCollarSeg;   // Guessed name; "Collar" locator.
  float mAttackRemTime;
  TUniqueId mTargetId;
  CBurstFire mBurstFire;
  float mJumpHeight;
  CVector3f mPatrolDestPos;
  pas::EStepDirection mSkidDir;
  float mStrafeDelayTimer;
  pas::ESeverity mMeleeSeverity;
  TUniqueId mJumpPoint;
  pas::EStepDirection mDodgeDir;
  float mDodgeDist;
  float mBreakDodgeDist;
  float mTimeSinceHitByPlayer;
  float mLowHealthFrenzyTimer;
  float mRagdollDelayTimer;
  rstl::single_ptr< CPirateRagDoll > mRagDoll;
  CIkChain mIkChain;
  float mCloakDelayTimer;
  float mElectricParticleTimer;
  float mCloakStepTime;
  float mShadowPirateAlpha;
  float mMinCloakAlpha;
  float mMaxCloakAlpha;
  float mDodgeDelayTimer;
  float mAimDelayTimer;
  float xb9c_;
  TUniqueId mTeamAiMgrId;
  CVector2f mHeldPosition;
  float mHoldPositionTime;
  float mLeashTimer;
  CVector3f xbb4_;
  mutable uint xbc0_;
  CVector3f xbc4_;
  float xbd0_;
  rstl::optional_object< CProjectileInfo > mProjectileInfo;
  CSfxHandle xc00_;
  int xc04_;
  rstl::optional_object< CModelData > mGrenadeLauncherModel; // Guessed name.
  int xc58_;
  CPlane xc5c_;
};
CHECK_SIZEOF(CSpacePirate, 0xc70)

#endif
