#ifndef _CBLOGG
#define _CBLOGG

#include "types.h"

#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CLineOfSightTracker.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

#include "rstl/rc_ptr.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"
#include "rstl/vector.hpp"

class CBlogg;
class CCollisionActorManager;
struct SLdrBloggStruct;

// Guessed name. Loader-built bundle of one SLdrBloggStruct (attack count range and three weights).
struct SBloggAttackCount {
  SBloggAttackCount(const SLdrBloggStruct& data);

  uchar mMin;
  uchar mMax;
  float x4_;
  float x8_;
  float xc_;
};

// Original class name from the Wii SEL exports (TypesMatch__6CBloggCFi).
class CBlogg : public CPatterned {
public:
  CBlogg(TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
         const CModelData& mData, const CPatternedInfo& pInfo, const CActorParameters& actParms,
         float minAttackAngle, float maxAttackAngle, float minProjectileDelay,
         float maxProjectileDelay, uchar unknown0xa19d5f62, CAssetId projectile,
         const CDamageInfo& projectileDamage, float bodyDamageMultiplier,
         float mouthDamageMultiplier, float mouthDamageAngle,
         const CDamageVulnerability& armorVulnerability, float chargeDamageRadius,
         float chargeDamage, float biteDamage, float ballSpitDamage, float fishAttractionRadius,
         float fishAttractionPriority, float aggressiveness, float unknown0x479ccc37,
         float unknown0x689a803f, float unknown0x800a2b0d, float chargeTurnSpeed,
         float chargeSpeedMultiplier, float maxMeleeRange, float maxBallDetectionRange,
         float maxPlayerPursuitTime, float maxBallPursuitTime, ushort mouthOpenSound,
         float minDelayBetweenMeleeAttacks, float maxCollisionTime, bool isMegaBlogg,
         float projectileBlurRadius, float projectileBlurTime,
         const CDamageVulnerability& ingPossessedArmorVulnerability,
         const SBloggAttackCount& attackCount1, const SBloggAttackCount& attackCount2,
         const SBloggAttackCount& attackCount3);

  // CEntity
  ~CBlogg() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;

  // CPhysicsActor
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  // CAi
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void TakeDamage(const CVector3f& direction, float magnitude) override;

  // CPatterned
  CProjectileInfo* ProjectileInfo() override { return &mProjectileInfo; }
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  CDamageInfo GetContactDamage() const override;
  void SetupStateMachine(CStateManager& mgr) override;
  void SetIngPossessed(bool possessed, CStateManager& mgr) override;
  void SetIngPossessed(bool possessed, float duration, CStateManager& mgr) override;
  CVector3f GetIngSnatchingNormal(float t) const override;
  CVector3f GetIngSnatchingPoint(float t) const override;

  // Triggers
  bool ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const;
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldPrepareToAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool InAttackPosition(CStateManager& mgr, const CTriggerData& data) const;
  bool InValidPosition(CStateManager& mgr, const CTriggerData& data) const;
  bool IsFacingPlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool IsPlayerStunned(CStateManager& mgr, const CTriggerData& data) const;
  bool ProjectileAttackDelay(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldCharge(CStateManager& mgr, const CTriggerData& data) const;
  bool IsChargeOver(CStateManager& mgr, const CTriggerData& data) const;
  bool CanMeleeAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool CanRangedAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool CanTaunt(CStateManager& mgr, const CTriggerData& data) const;
  bool InProjectileRange(CStateManager& mgr, const CTriggerData& data) const;
  bool CollidedWithWall(CStateManager& mgr, const CTriggerData& data) const;
  bool CanBitePlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool InMeleeRange(CStateManager& mgr, const CTriggerData& data) const;
  bool InBiteRange(CStateManager& mgr, const CTriggerData& data) const;
  bool CantMoveToPlayer(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldEndPursuit(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldEndBallPursuit(CStateManager& mgr, const CTriggerData& data) const;
  bool PlayerInBallMode(CStateManager& mgr, const CTriggerData& data) const;
  bool DetectBall(CStateManager& mgr, const CTriggerData& data) const;
  bool CanGrabBall(CStateManager& mgr, const CTriggerData& data) const;
  bool BallGrabbed(CStateManager& mgr, const CTriggerData& data) const;
  bool IsPlayerReachable(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAbortBallGrab(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void MoveToAttackPosition(CStateManager& mgr, EStateMsg msg, float dt);
  void MoveToValidPosition(CStateManager& mgr, EStateMsg msg, float dt);
  void FacePlayer(CStateManager& mgr, EStateMsg msg, float dt);
  void ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void ChargeTelegraph(CStateManager& mgr, EStateMsg msg, float dt);
  void ChargeAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt);
  void Stunned(CStateManager& mgr, EStateMsg msg, float dt);
  void Taunt(CStateManager& mgr, EStateMsg msg, float dt);
  void MoveToPlayer(CStateManager& mgr, EStateMsg msg, float dt);
  void GrabBall(CStateManager& mgr, EStateMsg msg, float dt);
  void Thrash(CStateManager& mgr, EStateMsg msg, float dt);
  void SpitBall(CStateManager& mgr, EStateMsg msg, float dt);
  void Dead(CStateManager& mgr, EStateMsg msg, float dt);

  // Code functions
  void ComputeAttackPositions(CStateManager& mgr, float arg);
  void ComputeTauntProbability(CStateManager& mgr, float arg);
  void EndMeleePursuit(CStateManager& mgr, float arg);

  bool IsInMouthAngle(const CVector3f& direction) const;
  bool IsMegaBlogg() const { return mIsMegaBlogg; }
  int GetIsGrabbingBall() const { return x990_; }
  const CDamageVulnerability& GetIngPossessedArmorVulnerability() const {
    return mIngPossessedArmorVulnerability;
  }

private:
  int x7c0_;
  int x7c4_;
  int x7c8_;
  int x7cc_;
  int x7d0_;
  int x7d4_;
  int x7d8_;
  float x7dc_;
  float x7e0_;
  float x7e4_;
  float x7e8_;
  CVector3f x7ec_;
  CVector3f x7f8_;
  CDamageInfo mContactDamageInfo;
  rstl::single_ptr< CCollisionActorManager > mCollisionActorManager;
  float mMinAttackAngle;
  float mMaxAttackAngle;
  float mMinAttackRange;
  float mMaxAttackRange;
  float x834_;
  float x838_;
  CVector3f x83c_;
  TUniqueId x848_;
  CPathFindSearch mPathFindSearch;
  CVector3f x938_;
  float mMinProjectileDelay;
  float mMaxProjectileDelay;
  float mProjectileDelay;
  uchar x950_;
  uchar x951_;
  CProjectileInfo mProjectileInfo;
  CVector3f x97c_;
  float x988_;
  float x98c_;
  int x990_;
  TUniqueId x994_;
  TUniqueId x996_;
  rstl::reserved_vector< CVector3f, 16 > mAttackPositions;
  float mBodyDamageMultiplier;
  float mMouthDamageMultiplier;
  CDamageVulnerability mArmorVulnerability;
  CDamageVulnerability mIngPossessedArmorVulnerability;
  TUniqueId xac4_;
  float mMouthDamageAngle;
  float mChargeDamageRadius;
  float mChargeDamage;
  float mChargeTurnSpeed;
  float mChargeSpeedMultiplier;
  float mBiteDamage;
  float mBallSpitDamage;
  float mMaxMeleeRange;
  float mMaxBallDetectionRange;
  float mMaxPlayerPursuitTime;
  float mMaxBallPursuitTime;
  float xaf4_;
  float xaf8_;
  float mFishAttractionRadius;
  float mFishAttractionPriority;
  float mAggressiveness;
  float xb08_;
  float xb0c_;
  float xb10_;
  int xb14_;
  bool xb18_;
  float xb1c_;
  float mMaxCollisionTime;
  float xb24_;
  float xb28_;
  float xb2c_;
  ushort mMouthOpenSound;
  float xb34_;
  rstl::vector< TUniqueId > mAiHints;
  float xb48_;
  float mMinDelayBetweenMeleeAttacks;
  rstl::ncrc_ptr< CNonUniformVulnerability > mMouthVulnerability;
  rstl::ncrc_ptr< CNonUniformVulnerability > mArmorNonUniformVulnerability;
  float mProjectileBlurRadius;
  float mProjectileBlurTime;
  float xb68_;
  CLineOfSightTracker mLineOfSight;
  uchar xbb0_;
  bool xbb1_;
  bool xbb2_;
  rstl::vector< SBloggAttackCount > mAttackCounts;
  bool xbc4_24_ : 1;
  bool xbc4_25_ : 1;
  bool xbc4_26_ : 1;
  bool xbc4_27_ : 1;
  bool xbc4_28_ : 1;
  bool xbc4_29_ : 1;
  bool xbc4_30_ : 1;
  bool mIsMegaBlogg : 1;
  bool xbc5_24_ : 1;
  bool xbc5_25_ : 1;
  bool xbc5_26_ : 1;
  bool xbc5_27_ : 1;
  bool xbc5_28_ : 1;
  bool xbc5_29_ : 1;
  bool xbc5_30_ : 1;
};
CHECK_SIZEOF(CBlogg, 0xbc8)

#endif // _CBLOGG
