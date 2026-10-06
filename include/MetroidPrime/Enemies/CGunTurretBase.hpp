#ifndef _CGUNTURRETBASE
#define _CGUNTURRETBASE

#include "types.h"

#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "Collision/CCollidableAABox.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/TToken.hpp"

class CGenDescription;
class CGunTurretTop;

class CGunTurretBase : public CPatterned {
public:
  // Guessed names; the pole/base half of a pirate/GF gun turret.
  enum EState {
    kS_Sleep,
    kS_Spawn,
    kS_Patrol,
    kS_Attack,
    kS_Withdraw,
    kS_Flinch,
    kS_IntoPan,
    kS_PanLeft,
    kS_PanRight,
    kS_AttackExit,
    kS_OpenDoor,
    kS_CloseDoor,
  };

  CGunTurretBase(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                 const CTransform4f& xf, const CModelData& modelData,
                 const CPatternedInfo& patternedInfo, const CDamageInfo& attackDamage,
                 float hurtSleepDelay, float gunAimTurnSpeed, float gunLockOnTurnSpeed,
                 float minTimeBetweenAttacks, float maxTimeBetweenAttacks,
                 float minTimeBetweenShots, float maxTimeBetweenShots, float maxPitchAngleUp,
                 const CActorParameters& actorParameters, bool gunRespawns,
                 uchar minShotsInABurst, uchar maxShotsInABurst, bool isPirateTurret,
                 CAssetId crscId, CAssetId pirateProjectile, CAssetId pirateProjectileEffect,
                 bool unknown5cf1, bool unknown479d, float maxPitchAngleDown,
                 float unknownFc03, float unknown8a35, float unknownD49b, float attackDelay,
                 float patrolDelay, float withdrawDelay, float detectionHeightUp,
                 float detectionHeightDown, float shotAngleVariance, float attackLeashTime,
                 ushort gfFireShotSfx, ushort pirateFireShotSfx, ushort lockOnSfx,
                 ushort gunPanSfx, ushort gfGunChargeSfx, ushort pirateGunChargeSfx,
                 ushort gunLowerLoopedSfx, ushort gunLowerOffSfx, ushort gunRaiseLoopedSfx,
                 ushort gunRaiseOffSfx, ushort pirateGunDeathLowerLoopedSfx,
                 ushort gfGunDeathLowerLoopedSfx, ushort poleSparksSfx, float unknown80ce,
                 float sfxFallOff, float sfxMaxDistance);

  // CEntity
  ~CGunTurretBase() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override { return &mCollisionPrimitive; }

  // CPatterned
  void SetupStateMachine(CStateManager& mgr) override;
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  void Sleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Spawn(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  void Withdraw(CStateManager& mgr, EStateMsg msg, float dt);
  void Flinch(CStateManager& mgr, EStateMsg msg, float dt);
  void IntoPan(CStateManager& mgr, EStateMsg msg, float dt);
  void PanLeft(CStateManager& mgr, EStateMsg msg, float dt);
  void PanRight(CStateManager& mgr, EStateMsg msg, float dt);
  void AttackExit(CStateManager& mgr, EStateMsg msg, float dt);
  void OpenDoor(CStateManager& mgr, EStateMsg msg, float dt);
  void CloseDoor(CStateManager& mgr, EStateMsg msg, float dt);

  bool ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldPan(CStateManager& mgr, const CTriggerData& data) const;
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool SpawnOver(CStateManager& mgr, const CTriggerData& data) const;
  bool WithdrawOver(CStateManager& mgr, const CTriggerData& data) const;
  bool Attacked(CStateManager& mgr, const CTriggerData& data) const;
  bool Delay(CStateManager& mgr, const CTriggerData& data) const;
  bool PatrolDelay(CStateManager& mgr, const CTriggerData& data) const;
  bool WithdrawDelay(CStateManager& mgr, const CTriggerData& data) const;
  bool GunDestroyed(CStateManager& mgr, const CTriggerData& data) const;
  bool AttackExitDone(CStateManager& mgr, const CTriggerData& data) const;

  void DestroyGun(CStateManager& mgr);
  void SetGunHit(bool hit) { mGunHit = hit; }
  float GetSfxFallOff() const { return mSfxFallOff; }
  float GetSfxMaxDistance() const { return mSfxMaxDistance; }
  CAABox GetModelBounds() const;

  static const char* const skConnectLocator;

private:
  float GetClosestCameraDistanceSq(CStateManager& mgr) const;
  CVector3f GetGunFirePosition(CStateManager& mgr) const;
  void LaunchProjectile(CStateManager& mgr);
  void ResetAttack(CStateManager& mgr);
  void UpdateAttack(CStateManager& mgr, float dt);
  void LowerGunPartial(float dt);
  void RaiseGunPartial(float dt);
  bool InDetectionHeight(const CActor& actor, float up, float down) const;

  float mDetectionRange;
  float mMaxAttackRange;
  float mMinAttackRange;
  CDamageInfo mAttackDamage;
  int mState;
  float mHurtSleepDelay;
  TUniqueId mTopId;
  bool mGunDestroyed;
  bool mGunRespawns;
  CVector3f mTargetPos;
  CVector3f mOriginalFront;
  int x80c_;
  float mGunAimTurnSpeed;
  float mGunLockOnTurnSpeed;
  CQuaternion mGunRotation;
  CQuaternion mTargetGunRotation;
  float mMinTimeBetweenAttacks;
  float mMaxTimeBetweenAttacks;
  float mTimeBetweenAttacks;
  float mMinTimeBetweenShots;
  float mMaxTimeBetweenShots;
  float mTimeBetweenShots;
  uchar mMinShotsInABurst;
  uchar mMaxShotsInABurst;
  uchar mShotsInBurst;
  float mAttackTimer;
  float mShotTimer;
  uint mShotCount;
  bool mIsPirateTurret;
  TCachedToken< CGenDescription > mCrsc;
  CAssetId mPirateProjectile;
  rstl::optional_object< TLockedToken< CGenDescription > > mPirateProjectileEffect;
  CProjectileInfo mProjectileInfo;
  float x8ac_;
  float x8b0_;
  bool mFiring;
  uchar mEffectIndex;
  bool mInBurst;
  TUniqueId mHitTarget;
  bool mHitTargetValid;
  TUniqueId mLastHitTarget;
  bool x8be_;
  bool x8bf_;
  float mMaxPitchAngleUp;
  float mMaxPitchAngleDown;
  float xfc03_;
  float mLowerSpeed;
  float mRaiseSpeed;
  float mAttackDelay;
  float mPatrolDelay;
  float mWithdrawDelay;
  float mShotAngleVariance;
  bool mGunHit;
  float mDelayTimer;
  float mLowerDuration;
  float mRaiseDuration;
  float mPanDuration;
  CDamageVulnerability mGunVulnerability;
  ushort mGFFireShotSfx;
  ushort mPirateFireShotSfx;
  ushort mLockOnSfx;
  ushort mGunPanSfx;
  ushort mGFGunChargeSfx;
  ushort mPirateGunChargeSfx;
  ushort mGunRaiseLoopedSfx;
  ushort mGunRaiseOffSfx;
  ushort mGunLowerLoopedSfx;
  ushort mGunLowerOffSfx;
  ushort mGFGunDeathLowerLoopedSfx;
  ushort mPirateGunDeathLowerLoopedSfx;
  ushort mPoleSparksSfx;
  float mDetectionHeightUp;
  float mDetectionHeightDown;
  bool x94c_;
  bool x94d_;
  float x950_;
  bool x954_;
  float x958_;
  float x95c_;
  float mAttackLeashTime;
  float mAttackLeashTimer;
  int x968_;
  int x96c_;
  TUniqueId x970_;
  CCollidableAABox mCollisionPrimitive;
  int mAdditiveAnim;
  float mMaxLowerAmount;
  float mLowerAmount;
  float mSfxFallOff;
  float mSfxMaxDistance;
  CAABox x9b4_;
  bool x9cc_24_ : 1;
  bool x9cc_25_ : 1;
};
CHECK_SIZEOF(CGunTurretBase, 0x9d0)

#endif // _CGUNTURRETBASE
