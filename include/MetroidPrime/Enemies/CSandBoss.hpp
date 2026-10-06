#ifndef _CSANDBOSS
#define _CSANDBOSS

#include "Kyoto/Math/CQuaternion.hpp"
#include "MetroidPrime/CBoneTracking.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSandBoss.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"
#include "rstl/vector.hpp"

class CCollisionActorManager;
class CScannableObjectInfo;
class CScriptSafeZone;

// Guessed name; the charge beam fired by the sand boss ("SandBossChargeBeam").
class CSandBossChargeBeam : public CPlasmaProjectile {
public:
  CSandBossChargeBeam(const TToken< CWeaponDescription >& description, const CBeamInfo& beamInfo,
                      TUniqueId uid, TAreaId areaId, TUniqueId owner, const CPlane& plane);
  ~CSandBossChargeBeam() override;

  CRayCastResult RayCollisionCheckWithWorld(TUniqueId& idOut, const CVector3f& start,
                                            const CVector3f& end, float magnitude,
                                            rstl::reserved_vector< TUniqueId, 1024 >& nearList,
                                            CStateManager& mgr,
                                            EStaticGeometryTest staticTest) override;

private:
  CPlane mGroundPlane; // Guessed name.
};

// Original class name from the Wii SEL exports (TypesMatch__9CSandBossCFi, TCastToPtr<9CSandBoss>).
class CSandBoss : public CPatterned {
public:
  CSandBoss(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
            const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
            const CActorParameters& aParms, const SLdrSandBossData& data);
  ~CSandBoss() override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;
  const CDamageVulnerability* GetDamageVulnerability() const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  CScannableObjectInfo* GetScannableObjectInfo() const override;

  // CAi
  bool IsListening() const override { return true; }

  // CPatterned
  void RenderSystemsToBeDrawnLast(const CStateManager& mgr, uint mask,
                                  uint target) const override;
  CProjectileInfo* ProjectileInfo() override;
  void SetupStateMachine(CStateManager& mgr) override;
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  // Triggers
  bool StateOver(CStateManager& mgr, const CTriggerData& data) const;
  bool IsUnderGround(CStateManager& mgr, const CTriggerData& data) const;
  bool IsRecovered(CStateManager& mgr, const CTriggerData& data) const;
  bool CanStartNewRound(CStateManager& mgr, const CTriggerData& data) const;
  bool BeginStampede(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundStampedePoint(CStateManager& mgr, const CTriggerData& data) const;
  bool StampedeOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldRepeaterAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool FoundSafeZoneTarget(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldTripleCharge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDoubleCharge(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSnapJaws(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldDestroySphere(CStateManager& mgr, const CTriggerData& data) const;
  bool IsFacingPlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool IsBallInSuckRange(CStateManager& mgr, const CTriggerData& data) const;
  bool IsLastStage(CStateManager& mgr, const CTriggerData& data) const;
  bool HeadArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const;
  bool ArmorDestroyed(CStateManager& mgr, const CTriggerData& data) const;
  bool SyncStampede(CStateManager& mgr, const CTriggerData& data) const;
  bool SyncAttachToSphere(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Recover(CStateManager& mgr, EStateMsg msg, float dt);
  void UnderGround(CStateManager& mgr, EStateMsg msg, float dt);
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);
  void Deactivate(CStateManager& mgr, EStateMsg msg, float dt);
  void AttachToSphere(CStateManager& mgr, EStateMsg msg, float dt);
  void RotateToPlayer(CStateManager& mgr, EStateMsg msg, float dt);
  void SuckAir(CStateManager& mgr, EStateMsg msg, float dt);
  void SuckBall(CStateManager& mgr, EStateMsg msg, float dt);
  void BallInMouth(CStateManager& mgr, EStateMsg msg, float dt);
  void DestroySphere(CStateManager& mgr, EStateMsg msg, float dt);
  void JumpOffSphere(CStateManager& mgr, EStateMsg msg, float dt);
  void SpitOutBall(CStateManager& mgr, EStateMsg msg, float dt);
  void DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void RepeaterAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void DoubleCharge(CStateManager& mgr, EStateMsg msg, float dt);
  void TripleCharge(CStateManager& mgr, EStateMsg msg, float dt);
  void SnapJaws(CStateManager& mgr, EStateMsg msg, float dt);
  void SelectStampedePoint(CStateManager& mgr, EStateMsg msg, float dt);
  void SeekStampedePoint(CStateManager& mgr, EStateMsg msg, float dt);
  void Stampede(CStateManager& mgr, EStateMsg msg, float dt);

  // Code functions
  void SetTripleChargeArmor(CStateManager& mgr, float dt);
  void SetDoubleChargeArmor(CStateManager& mgr, float dt);
  void SetStampedeArmor(CStateManager& mgr, float dt);
  void SetAttachedArmor(CStateManager& mgr, float dt);
  void SetSuckAirArmor(CStateManager& mgr, float dt);
  void RemovePlayerCollision(CStateManager& mgr, float dt);
  void RestorePlayerCollision(CStateManager& mgr, float dt);
  void SelectSafeZoneTarget(CStateManager& mgr, float dt);
  void WaitStampede(CStateManager& mgr, float dt);
  void WaitAttachToSphere(CStateManager& mgr, float dt);
  void LockSphere(CStateManager& mgr, float dt);
  void UnLockSphere(CStateManager& mgr, float dt);
  void AllowSyncAttacks(CStateManager& mgr, float dt);
  void PreventSyncAttacks(CStateManager& mgr, float dt);
  void Deactivate(CStateManager& mgr, float dt);
  void EndStampede(CStateManager& mgr, float dt);
  void PlayHeadArmorExplosion(CStateManager& mgr, float dt);
  void ResetHeadArmorHP(CStateManager& mgr, float dt);
  void ResetAttackTimes(CStateManager& mgr, float dt);
  void SetCinematicStateUnderGround(CStateManager& mgr, float dt);
  void SetCinematicStateStampede(CStateManager& mgr, float dt);
  void SetCinematicStateNormal(CStateManager& mgr, float dt);
  void SetCinematicStateStunned(CStateManager& mgr, float dt);
  void SetCinematicStateChoking(CStateManager& mgr, float dt);
  void SetCinematicStateExitSphere(CStateManager& mgr, float dt);

  // Guessed names.
  int GetStage() const;
  void SetupArmorModels();
  void SetupCollisionActors(CStateManager& mgr);
  void FindOtherBosses(CStateManager& mgr);
  void OnCollisionActorHit(CStateManager& mgr, TUniqueId id);
  void OnCollisionActorDamage(CStateManager& mgr, TUniqueId id);
  void UpdateArmorColor(float dt, CStateManager& mgr);
  void UpdateStampede(CStateManager& mgr, float dt);
  void UpdateCinematicState(CStateManager& mgr);
  void RenderArmor(const CStateManager& mgr, const CTransform4f& xf, const CModelFlags& flags,
                   const CModelFlags& headFlags) const;

private:
  // Guessed names; per-piece armor state.
  enum EArmorState {
    kArmor_Attached,
    kArmor_Stampede,
    kArmor_Destroyed,
  };

  // Guessed name; cinematic state values reported to the other bosses.
  enum ECinematicState {
    kCS_Normal,
    kCS_UnderGround,
    kCS_Stampede,
    kCS_Stunned,
    kCS_Choking,
    kCS_ExitSphere,
  };

  SLdrSandBossData mData;
  rstl::single_ptr< CCollisionActorManager > mCollisionActorManager;
  CBoneTracking mBoneTracking;
  rstl::reserved_vector< TUniqueId, 3 > mOtherBosses; // Guessed name.
  int xde4_;
  float xde8_;
  rstl::single_ptr< TLockedToken< CScannableObjectInfo > > mScanInfo; // Guessed name.
  int xdf0_;
  int xdf4_;
  int xdf8_;
  int xdfc_;
  CProjectileInfo mDarkBeamInfo;   // Guessed name.
  CProjectileInfo mChargeBeamInfo; // Guessed name.
  rstl::optional_object< TLockedToken< CGenDescription > > mHeadArmorExplosion;     // Guessed name.
  rstl::optional_object< TLockedToken< CGenDescription > > mStampedeArmorExplosion; // Guessed name.
  rstl::optional_object< TLockedToken< CGenDescription > > mStampedeSandFountain;   // Guessed name.
  TUniqueId xe80_;
  TUniqueId xe82_;
  TUniqueId xe84_;
  TUniqueId xe86_;
  TUniqueId xe88_;
  TUniqueId xe8a_;
  TUniqueId xe8c_;
  TUniqueId xe8e_;
  TUniqueId xe90_;
  rstl::reserved_vector< CVector3f, 8 > xe94_;
  TUniqueId xef8_;
  rstl::reserved_vector< CSegId, 8 > mSpineSegIds; // Guessed name.
  CSegId mHeadSegId;                               // Guessed name.
  CColor xf0c_;
  float xf10_;
  float xf14_;
  float xf18_;
  float xf1c_;
  float xf20_;
  float xf24_;
  float xf28_;
  float xf2c_;
  float xf30_;
  float xf34_;
  float xf38_;
  int xf3c_;
  float xf40_;
  int xf44_;
  int xf48_;
  rstl::reserved_vector< rstl::optional_object< CModelData >, 8 > mAttachedArmorModels; // Guessed name.
  rstl::reserved_vector< rstl::optional_object< CModelData >, 8 > mStampedeArmorModels; // Guessed name.
  rstl::reserved_vector< EArmorState, 8 > mArmorStates;                                   // Guessed name.
  rstl::vector< int > x1478_;
  TLockedToken< CSkinnedModel > mNormalSkinnedModel;   // Guessed name.
  TLockedToken< CSkinnedModel > mTailArmorSkinnedModel; // Guessed name.
  CTransform4f x14a0_;
  CQuaternion x14d0_;
  CDamageInfo mSnapJawDamage;   // Guessed name.
  CDamageInfo mSpitOutDamage;   // Guessed name.
  CDamageInfo mStampedeDamage;  // Guessed name.
  CDamageInfo mDoubleChargeDamage; // Guessed name.
  CDamageInfo mTripleChargeDamage; // Guessed name.
  CDamageVulnerability mDamageVulnerability;   // Guessed name.
  CDamageVulnerability mStampedeVulnerability; // Guessed name.
  CDamageVulnerability mSuckAirVulnerability;  // Guessed name.
  CDamageVulnerability x15fc_;
  CDamageVulnerability x162c_;
  bool x165c_24_ : 1;
  bool x165c_25_ : 1;
  bool x165c_26_ : 1;
  bool x165c_27_ : 1;
  bool x165c_28_ : 1;
  bool x165c_29_ : 1;
  bool x165c_30_ : 1;
  bool x165c_31_ : 1;
  bool x165d_24_ : 1;
  bool x165d_25_ : 1;
  bool x165d_26_ : 1;
  bool x165d_27_ : 1;
  bool x165d_28_ : 1;
  bool x165d_29_ : 1;
  bool x165d_30_ : 1;
  bool x165d_31_ : 1;
  bool x165e_24_ : 1;
  bool x165e_25_ : 1;
  bool x165e_26_ : 1;
  bool x165e_27_ : 1;
  bool x165e_28_ : 1;
};
CHECK_SIZEOF(CSandBoss, 0x1660)

#endif // _CSANDBOSS
