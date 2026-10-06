#ifndef _CCOMMANDOPIRATE
#define _CCOMMANDOPIRATE

#include "types.h"

#include "MetroidPrime/CBoneTracking.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CLineOfSightTracker.hpp"
#include "MetroidPrime/Enemies/CBouncyGrenade.hpp"
#include "MetroidPrime/Enemies/CBurstFire.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/Enemies/CPirateRagDoll.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "Kyoto/TToken.hpp"

#include "rstl/optional_object.hpp"
#include "rstl/single_ptr.hpp"

class CCollisionActorManager;
class CElementGen;
class CGenDescription;

// Guessed name: grenade parameters (SLdrUnknownStruct10).
class CCommandoGrenadeInfo {
public:
  CCommandoGrenadeInfo(const CBouncyGrenadeData& data, float empDuration, float minAttackInterval,
                       float postAttackPause, float attackChance, float minAttackDist,
                       float maxAttackDist, float minLaunchSpeed, float maxLaunchSpeed);

  CBouncyGrenadeData mData;
  float mEMPDuration;
  float mAttackChance;
  float mMinAttackInterval;
  float mPostAttackPause;
  float mMinAttackDist;
  float mMaxAttackDist;
  float mMinLaunchSpeed;
  float mMaxLaunchSpeed;
};
CHECK_SIZEOF(CCommandoGrenadeInfo, 0x70)

// Guessed name: shield parameters (SLdrCommandoShield).
class CCommandoShieldInfo {
public:
  CCommandoShieldInfo(const CDamageInfo& chargeDamage, const CDamageVulnerability& vulnerability,
                      float chargeMinAttackDist, float chargeMaxAttackDist, float chargeSpeed,
                      CAssetId explodeEffect, ushort sfxExplode, float x60, float x64,
                      float armShieldChance, float armShieldTime, float armShieldTimeVariation,
                      CAssetId armShieldExplodeEffect, CAssetId chargeEffect,
                      CAssetId armShieldEffect, ushort sfxTurnOn, ushort sfxTurnOff);

  CDamageInfo mChargeDamage;
  CDamageVulnerability mVulnerability;
  float mChargeMinAttackDist;
  float mChargeMaxAttackDist;
  float mChargeSpeed;
  CAssetId mExplodeEffect;
  ushort mSfxExplode;
  float x60_;
  float x64_;
  float mArmShieldChance;
  float mArmShieldTime;
  float mArmShieldTimeVariation;
  CAssetId mArmShieldExplodeEffect;
  CAssetId mChargeEffect;
  CAssetId mArmShieldEffect;
  ushort mSfxTurnOn;
  ushort mSfxTurnOff;
};
CHECK_SIZEOF(CCommandoShieldInfo, 0x84)

// Guessed name.
class CCommandoPirateData {
public:
  CCommandoPirateData(uint flags, ushort sfxImpact, ushort sfxHurled, ushort sfxDeath, ushort x6,
                      CAssetId x8, const CDamageInfo& bladeDamage, float aggressiveness,
                      float coverCheck, float searchRadius, float dodgeCheck, float hearingRadius,
                      float intraBurstShotTime, float intraBurstShotVariation, CAssetId projectile,
                      const CDamageInfo& projectileDamage, ushort sfxProjectile,
                      const CCommandoGrenadeInfo& grenadeInfo,
                      const CCommandoShieldInfo& shieldInfo);

  ushort mSfxImpact;
  ushort mSfxHurled;
  ushort mSfxDeath;
  ushort x6_;
  CAssetId x8_;
  CDamageInfo mBladeDamage;
  CAssetId mProjectile;
  CDamageInfo mProjectileDamage;
  ushort mSfxProjectile;
  float mAggressiveness;
  float mCoverCheck;
  float mSearchRadius;
  float mDodgeCheck;
  float mHearingRadius;
  float mIntraBurstShotTime;
  float mIntraBurstShotVariation;
  CCommandoGrenadeInfo mGrenadeInfo;
  CCommandoShieldInfo mShieldInfo;
  bool x15c_24_ : 1;
  bool x15c_25_ : 1;
  bool x15c_26_ : 1;
  bool x15c_27_ : 1;
  bool x15c_28_ : 1;
  bool x15c_29_ : 1;
  bool x15c_30_ : 1;
  bool x15c_31_ : 1;
  bool x15d_24_ : 1;
  bool x15d_25_ : 1;
  bool x15d_26_ : 1;
  bool x15d_27_ : 1;
};
CHECK_SIZEOF(CCommandoPirateData, 0x160)

class CCommandoPirate : public CPatterned {
public:
  CCommandoPirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                  const CTransform4f& xf, const CModelData& modelData,
                  const CActorParameters& actParms, const CPatternedInfo& pInfo,
                  const CCommandoPirateData& data);

  // CEntity
  ~CCommandoPirate() override {}
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
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
  CProjectileInfo* ProjectileInfo() override { return &mProjectileInfo; }
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  CDamageInfo GetContactDamage() const override;
  void SetupStateMachine(CStateManager& mgr) override;
  float GetGravityConstant() const override { return skGravityConstant; }

  // State functions
  void Lurk(CStateManager& mgr, EStateMsg msg, float arg);
  void Ambush(CStateManager& mgr, EStateMsg msg, float arg);
  void Alert(CStateManager& mgr, EStateMsg msg, float arg);
  void FaceTarget(CStateManager& mgr, EStateMsg msg, float arg);
  void SelectTarget(CStateManager& mgr, EStateMsg msg, float arg);
  void SetTargetDest(CStateManager& mgr, EStateMsg msg, float arg);
  void SetRetreatDest(CStateManager& mgr, EStateMsg msg, float arg);
  void SetJumpDest(CStateManager& mgr, EStateMsg msg, float arg);
  void SetPathMeshDest(CStateManager& mgr, EStateMsg msg, float arg);
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float arg);
  void EGrenadeAttack(CStateManager& mgr, EStateMsg msg, float arg);
  void PostEGrenadeAttack(CStateManager& mgr, EStateMsg msg, float arg);
  void JumpPointFind(CStateManager& mgr, EStateMsg msg, float arg);
  void JetBoost(CStateManager& mgr, EStateMsg msg, float arg);
  void PathFind(CStateManager& mgr, EStateMsg msg, float arg);
  void Patrol(CStateManager& mgr, EStateMsg msg, float arg);
  void FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float arg);
  void WarpIn(CStateManager& mgr, EStateMsg msg, float arg);
  void WarpOut(CStateManager& mgr, EStateMsg msg, float arg);
  void PostWarpOut(CStateManager& mgr, EStateMsg msg, float arg);
  void JumpBack(CStateManager& mgr, EStateMsg msg, float arg);
  void Dodge(CStateManager& mgr, EStateMsg msg, float arg);
  void ArmShield(CStateManager& mgr, EStateMsg msg, float arg);
  void ShieldCharge(CStateManager& mgr, EStateMsg msg, float arg);
  void ScriptedShieldCharge(CStateManager& mgr, EStateMsg msg, float arg);
  void RestoreOrientation(CStateManager& mgr, EStateMsg msg, float arg);
  void Crouch(CStateManager& mgr, EStateMsg msg, float arg);
  void WallHang(CStateManager& mgr, EStateMsg msg, float arg);
  void WallDetach(CStateManager& mgr, EStateMsg msg, float arg);
  void CoverFind(CStateManager& mgr, EStateMsg msg, float arg);
  void SetCoverDest(CStateManager& mgr, EStateMsg msg, float arg);
  void Cover(CStateManager& mgr, EStateMsg msg, float arg);
  void CoverAttack(CStateManager& mgr, EStateMsg msg, float arg);
  void BreakCover(CStateManager& mgr, EStateMsg msg, float arg);
  void GetUp(CStateManager& mgr, EStateMsg msg, float arg);
  void Dead(CStateManager& mgr, EStateMsg msg, float arg);

  // Transition functions
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFireEGrenade(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAmbush(CStateManager& mgr, const CTriggerData& data) const;
  bool BreakAmbush(CStateManager& mgr, const CTriggerData& data) const;
  bool HasLineOfSight(CStateManager& mgr, const CTriggerData& data) const;
  bool UnderFire(CStateManager& mgr, const CTriggerData& data) const;
  bool HeardShot(CStateManager& mgr, const CTriggerData& data) const;
  bool HasTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool HasNewTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool PathShagged(CStateManager& mgr, const CTriggerData& data) const;
  bool PathOver(CStateManager& mgr, const CTriggerData& data) const;
  bool IsOffPath(CStateManager& mgr, const CTriggerData& data) const;
  bool HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const;
  bool IsFacingTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const;
  bool TooClose(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDodge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldArmShield(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldShieldCharge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldBoost(CStateManager& mgr, const CTriggerData& data) const;
  bool AbortShieldCharge(CStateManager& mgr, const CTriggerData& data) const;
  bool IsAggressive(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundJumpPoint(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldCover(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundCover(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldCoverAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool CoverBlown(CStateManager& mgr, const CTriggerData& data) const;
  bool AbortSeekCover(CStateManager& mgr, const CTriggerData& data) const;

private:
  CCommandoPirateData mData;
  int x920_;
  int x924_;
  CPathFindSearch mPathFindSearch;
  CProjectileInfo mProjectileInfo;
  rstl::single_ptr< CCollisionActorManager > mShieldCollisionManager; // Guessed name.
  rstl::single_ptr< CCollisionActorManager > mBladeCollisionManager;  // Guessed name.
  CDamageVulnerability mShieldVulnerability;
  CBoneTracking mBoneTracking;
  CLineOfSightTracker mLineOfSightTracker;
  CBurstFire mBurstFire;
  int xb50_;
  int xb54_;
  int xb58_;
  float xb5c_;
  float xb60_;
  float xb64_;
  float xb68_;
  float xb6c_;
  float xb70_;
  float xb74_;
  float xb78_;
  float xb7c_;
  float xb80_;
  float xb84_;
  float xb88_;
  float xb8c_;
  float xb90_;
  CVector3f xb94_;
  CVector3f xba0_;
  CVector3f xbac_;
  CVector3f xbb8_;
  TUniqueId xbc4_;
  TUniqueId xbc6_;
  TUniqueId xbc8_;
  TUniqueId xbca_;
  TUniqueId xbcc_;
  TUniqueId xbce_;
  TUniqueId xbd0_;
  TUniqueId xbd2_;
  TUniqueId xbd4_;
  TUniqueId xbd6_;
  TUniqueId xbd8_;
  TUniqueId xbda_;
  int xbdc_;
  float xbe0_;
  float xbe4_;
  float xbe8_;
  float xbec_;
  CVector3f xbf0_;
  rstl::single_ptr< CPirateRagDoll > mRagDoll;
  float xc00_;
  int xc04_;
  rstl::optional_object< TLockedToken< CGenDescription > > mShieldExplodeEffect;
  rstl::optional_object< TLockedToken< CGenDescription > > mArmShieldExplodeEffect;
  rstl::optional_object< TLockedToken< CGenDescription > > xc28_;
  int xc38_;
  rstl::single_ptr< CElementGen > mArmShieldEffect;
  rstl::single_ptr< CElementGen > mShieldChargeEffect;
  float xc44_;
  float xc48_;
  int xc4c_;
  float xc50_;
  int xc54_;
  float xc58_;
  CSegId mHeadSeg;
  CSegId mLaunchSeg;
  CSegId mGunSeg;
  CSegId xc5f_;
  CSegId mRightWristSeg;
  CSegId mRightElbowSeg;
  CSegId mLeftWristSeg;
  bool xc63_24_ : 1;
  bool xc63_25_ : 1;
  bool xc63_26_ : 1;
  bool xc63_27_ : 1;
  bool xc63_28_ : 1;
  bool xc63_29_ : 1;
  bool xc63_30_ : 1;
  bool xc63_31_ : 1;
  bool xc64_24_ : 1;
  bool xc64_25_ : 1;
  bool xc64_26_ : 1;
  bool xc64_27_ : 1;
  bool xc64_28_ : 1;
  bool xc64_29_ : 1;
  bool xc64_30_ : 1;
  bool xc64_31_ : 1;
  bool xc65_24_ : 1;
  bool xc65_25_ : 1;
  bool xc65_26_ : 1;
  bool xc65_27_ : 1;
  bool xc65_28_ : 1;
  bool xc65_29_ : 1;
  bool xc65_30_ : 1;
  bool xc65_31_ : 1;
  bool xc66_24_ : 1;
  bool xc66_25_ : 1;
  bool xc66_26_ : 1;

  static const float skGravityConstant;
};
CHECK_SIZEOF(CCommandoPirate, 0xc68)

#endif // _CCOMMANDOPIRATE
