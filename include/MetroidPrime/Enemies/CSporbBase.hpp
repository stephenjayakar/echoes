#ifndef _CSPORBBASE
#define _CSPORBBASE

#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/TToken.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CPowerBombGuardianStageData.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "rstl/auto_ptr.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"
#include "rstl/vector.hpp"

class CGenDescription;
class CSporbPowerBomb;
class CWeaponDescription;
class CPASAnimParmData;

// Guessed name: the grabber tendril's particle system, a CElementGen that reports a fixed
// bounding box (set by the base) instead of its particles' bounds.
class CSporbTendrilGen : public CElementGen {
public:
  CSporbTendrilGen(TToken< CGenDescription > desc)
  : CElementGen(desc, kMOT_Normal, kOSF_One), mBounds(CAABox::MakeMaxInvertedBox()) {}
  ~CSporbTendrilGen() override {}

  rstl::optional_object< CAABox > GetBounds() override { return mBounds; }

  void SetBounds(const CAABox& bounds) { mBounds = bounds; }

private:
  CAABox mBounds;
};
CHECK_SIZEOF(CSporbTendrilGen, 0x350)

// Original class name from the Wii SEL exports (TypesMatch__10CSporbBaseCFi, TCastToPtr<10CSporbBase>).
// The rooted Sporb body (the Power Bomb Guardian when configured): owns the top, the grabber
// tendril, the morph-ball pod (CSporbProjectile) and fires needles or power bombs.
class CSporbBase : public CPatterned {
public:
  typedef rstl::vector< rstl::auto_ptr< CPowerBombGuardianStageData > > StageList;

  // Guessed name; one of the four tracked needle/bomb slots.
  struct SShot {
    SShot() : mTimer(0.f), mId(kInvalidUniqueId) {}
    float mTimer;
    TUniqueId mId;
  };

  CSporbBase(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
             const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
             const CActorParameters& aParms, float minTimeBetweenAttacks,
             float maxTimeBetweenAttacks, float minTimeBetweenShots, float maxTimeBetweenShots,
             uchar minShotsInABurst, uchar maxShotsInABurst, float shotAngleVariance,
             const CVector3f& attackAimOffset, float grabberOutAcceleration,
             float grabberInAcceleration, float initialGrabberOutSpeed,
             float initialGrabberInSpeed, float grabberAttachTime, float minGrabberGrabTime,
             float maxGrabberGrabTime, float spitForce, CAssetId tendrilParticleEffect,
             ushort grabberFireSfx, ushort grabberFlightSfx, ushort grabberHitPlayerSfx,
             ushort grabberHitWorldSfx, ushort grabberRetractSfx,
             ushort grabberRetractMissedPlayerSfx, ushort morphballSpitSfx,
             ushort grabberExplosionSfx, ushort ballEscapeSfx, ushort needleTelegraphSfx,
             ushort grabberTelegraphSfx, float spitDamage, float grabDamage,
             float ballEscapeDamage, float maxGrabberGrabRange, float minGrabberGrabRange,
             bool isPowerBombGuardian, CAssetId powerBombProjectileParticle,
             const CDamageInfo& powerBombProjectileDamage, float maxPowerBombProjectileHeight,
             float powerBombProjectileFuseTime, ushort powerBombProjectileSfx, float x9f4,
             float x9f8, float powerBombProjectileStartDamageTime,
             float powerBombProjectileEndDamageTime, const StageList& stages, float xa08,
             float xa5c, float xa60, float powerBombProjectileDamageWaitTime);

  // CEntity
  ~CSporbBase() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetScanObjectIndicatorPosition(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CPatterned
  CProjectileInfo* ProjectileInfo() override;
  CDamageInfo GetContactDamage() const override;
  void SetupStateMachine(CStateManager& mgr) override;
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  CAABox GetModelBounds() const; // Guessed name.

  // Triggers
  bool ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFire(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSpit(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFlail(CStateManager& mgr, const CTriggerData& data) const;
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool AttackOver(CStateManager& mgr, const CTriggerData& data) const;
  bool SpitOver(CStateManager& mgr, const CTriggerData& data) const;
  bool AttackExitOver(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Sleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  void WakeUp(CStateManager& mgr, EStateMsg msg, float dt);
  void GoToSleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Fire(CStateManager& mgr, EStateMsg msg, float dt);
  void ContinueFire(CStateManager& mgr, EStateMsg msg, float dt);
  void AttackExit(CStateManager& mgr, EStateMsg msg, float dt);
  void FakeDeath(CStateManager& mgr, EStateMsg msg, float dt);
  void FakeDead(CStateManager& mgr, EStateMsg msg, float dt);
  void Flail(CStateManager& mgr, EStateMsg msg, float dt);
  void ContinueFlail(CStateManager& mgr, EStateMsg msg, float dt);
  void Spit(CStateManager& mgr, EStateMsg msg, float dt);
  void ContinueSpit(CStateManager& mgr, EStateMsg msg, float dt);
  void SpitExit(CStateManager& mgr, EStateMsg msg, float dt);
  void Flinch(CStateManager& mgr, EStateMsg msg, float dt);

  static const char* const skTopAttachLocator; // Guessed name.

private:
  // Guessed names for the helpers below.
  void ResetTendrilParticles();
  float GetPowerBombGravity() const;
  void LaunchPowerBomb(const CVector3f& from, CStateManager& mgr, int maxProjectiles,
                       const CVector3f& target, float maxHeight);
  float GetPowerBombFlightTime(const CVector3f& from, const CVector3f& to, float maxHeight,
                               float gravity) const;
  CVector3f PredictBallPosition(CStateManager& mgr, const CVector3f& from, float maxHeight) const;
  CSporbPowerBomb* CreatePowerBomb(CStateManager& mgr, const TToken< CWeaponDescription >& desc,
                                   const CTransform4f& xf, const CDamageInfo& damage);
  CVector3f GetGrabberAimOffset(CStateManager& mgr) const;
  static CVector3f GetAimDirection(float a, float b, float c, float d);
  void ResetTendril();
  CVector3f GetPowerBombTarget(CStateManager& mgr, bool) const;
  void SpitBall(CStateManager& mgr, float force);
  CVector3f GetSpitTarget(CStateManager& mgr) const;
  void ReleaseGrabber(CStateManager& mgr, const CVector3f& pos);
  CTransform4f LookAtTarget(CStateManager& mgr, const CVector3f& from) const;
  void UpdateAimBlend(CStateManager& mgr);
  CVector3f GetTopLaunchPosition(CStateManager& mgr) const;
  void ResetAttackTimers(CStateManager& mgr);
  void UpdateFiring(CStateManager& mgr, float dt);
  CVector3f GetTopAttachPosition() const;
  void AttachPlayer(CStateManager& mgr);
  void UpdateGrabber(CStateManager& mgr, float dt);
  void UpdateAttack(CStateManager& mgr, float dt);
  float GetAimAngle(CStateManager& mgr) const;
  bool IsInRange(const CActor& actor, float range) const;
  void SelectTarget(CStateManager& mgr);
  void DecayAimBlend();
  void SetStage(uchar stage);

  int mState;
  TUniqueId mTopId;
  TUniqueId mProjectileId;
  int mGenerateAnims[5]; // Guessed name; anim ids found for generate states 5, 4, 7, 6, 0x17.
  float mDetectionRange;
  float mMaxAttackRange;
  float mMinAttackRange;
  float x7e8_;
  float x7ec_;
  float x7f0_;
  float x7f4_;
  float x7f8_;
  float x7fc_;
  TUniqueId x800_;
  CDamageInfo mSpitDamage;
  CDamageInfo mGrabDamage;
  CDamageInfo mBallEscapeDamage;
  float mMaxTimeBetweenShots2;
  float x85c_;
  float x860_;
  float mMinTimeBetweenAttacks2;
  float mMinTimeBetweenShots2;
  uchar mMinShotsInABurst2;
  float mMinTimeBetweenAttacks;
  float mMaxTimeBetweenAttacks;
  float mMinTimeBetweenShots;
  float mMaxTimeBetweenShots;
  float x880_;
  uchar mMinShotsInABurst;
  uchar mMaxShotsInABurst;
  uchar x886_;
  uchar x887_;
  bool x888_;
  bool x889_;
  bool x88a_;
  bool x88b_;
  bool x88c_;
  bool x88d_;
  bool x88e_;
  ushort x890_;
  ushort x892_;
  float x894_;
  TEditorId x898_;
  float mShotAngleVariance;
  CVector3f mAttackAimOffset;
  int x8ac_;
  float mInitialGrabberOutSpeed;
  float mInitialGrabberInSpeed;
  float mGrabberOutAcceleration;
  float mGrabberInAcceleration;
  CVector3f x8c0_;
  float x8cc_;
  float x8d0_;
  float x8d4_;
  float mGrabberAttachTime;
  float x8dc_;
  float x8e0_;
  float mMinGrabberGrabTime;
  float mMaxGrabberGrabTime;
  float x8ec_;
  float x8f0_;
  float mSpitForce;
  bool x8f8_;
  float x8fc_;
  float mMaxGrabberGrabRange;
  float mMinGrabberGrabRange;
  bool x908_;
  bool x909_;
  int x90c_;
  int x910_;
  CVector3f x914_;
  CQuaternion x920_;
  rstl::single_ptr< TLockedToken< CGenDescription > > mTendrilEffect;
  rstl::single_ptr< CSporbTendrilGen > mTendrilGen;
  ushort mGrabberFireSfx;
  ushort mGrabberFlightSfx;
  ushort mGrabberHitPlayerSfx;
  ushort mGrabberHitWorldSfx;
  ushort mGrabberRetractSfx;
  ushort mGrabberRetractMissedPlayerSfx;
  ushort mMorphballSpitSfx;
  ushort mGrabberExplosionSfx;
  ushort mBallEscapeSfx;
  ushort mNeedleTelegraphSfx;
  ushort mGrabberTelegraphSfx;
  bool x94e_;
  bool x94f_;
  CVector3f x950_;
  float x95c_;
  CVector3f x960_;
  float x96c_;
  float x970_;
  float x974_;
  float x978_;
  CDamageVulnerability x97c_;
  bool mIsPowerBombGuardian;
  CProjectileInfo mProjectileInfo;
  float mMaxPowerBombProjectileHeight;
  float mPowerBombProjectileFuseTime;
  rstl::vector< TUniqueId > x9e0_;
  ushort mPowerBombProjectileSfx;
  float x9f4_;
  float x9f8_;
  float mPowerBombProjectileStartDamageTime;
  float mPowerBombProjectileEndDamageTime;
  uchar mStage;
  float xa08_;
  float xa0c_;
  bool xa10_;
  bool xa11_;
  float mPowerBombProjectileDamageWaitTime;
  uchar xa18_;
  StageList mStages;
  rstl::reserved_vector< SShot, 4 > mShots;
  CVector3f xa50_;
  float xa5c_;
  float xa60_;
  int xa64_;
  CVector3f xa68_;
  bool xa74_24_ : 1;
  bool xa74_25_ : 1;
  bool xa74_26_ : 1;
};
CHECK_SIZEOF(CSporbBase, 0xa78)

#endif // _CSPORBBASE
