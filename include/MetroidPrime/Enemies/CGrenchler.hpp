#ifndef _CGRENCHLER
#define _CGRENCHLER

#include "types.h"

#include "MetroidPrime/CBoneTracking.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CSurfaceAlignmentHelper.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/Structs/SLdrAudioPlaybackParms.hpp"

#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "Kyoto/TToken.hpp"

#include "rstl/auto_ptr.hpp"
#include "rstl/optional_object.hpp"
#include "rstl/rc_ptr.hpp"
#include "rstl/reserved_vector.hpp"
#include "rstl/single_ptr.hpp"

class CCollisionActorManager;
class CElectricDescription;
class CElementGen;
class CGenDescription;
class CScannableObjectInfo;
class CStateMachine;
class CWeaponDescription;

// Original class name from the Wii SEL exports (TypesMatch__10CGrenchlerCFi).
class CGrenchler : public CPatterned {
public:
  static const CVector3f skAimOffset; // Guessed name

  CGrenchler(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
             const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
             CAssetId stateMachine, float tailDestroyedHealth, float minTimeBetweenCharges,
             float f3, float chargeAttackMinRange, float chargeAttackMaxRange,
             float biteAttackMinRange, float biteAttackMaxRange, float biteAttackMinPause,
             bool isGrappleGuardian, bool hasHealthBar, const CDamageVulnerability& vulnerability,
             int tail0, int tail1, int tail2, int tail3, CAssetId taillessModel,
             CAssetId taillessSkinRules, int tailDark0, int tailDark1, int tailDark2,
             int tailDark3, CAssetId taillessModelDark, CAssetId taillessSkinRulesDark,
             ushort tailHitSound, ushort tailDestroyedSound, float biteAttackMaxPause,
             float biteAttackDamageRadius, const CDamageInfo& biteDamage,
             const CDamageInfo& beamDamage, float beamAttackMinRange, float beamAttackMaxRange,
             float beamAttackMinPause, float beamAttackMaxPause, float beamAttackMaxAngle,
             const SLdrAudioPlaybackParms& beamAttackSound, const CDamageInfo& burstDamage,
             CAssetId burstProjectile, float burstAttackMinRange, float burstAttackMaxRange,
             float burstAttackMinPause, float burstAttackMaxPause, float burstAttackDamageRadius,
             CAssetId surfaceRingsEffect, CAssetId shallowWaterRing, CAssetId shallowWaterSplash,
             CAssetId part, CAssetId grappleSwoosh, CAssetId grappleBeamPart,
             CAssetId grappleHitFx, const CDamageInfo& grappleDamage,
             const SLdrAudioPlaybackParms& grappleBeamSound, CAssetId beamEffect, int unknown,
             float unknown1, float unknown2, float unknown3, CAssetId grappleVisorEffect,
             const CDamageInfo& damageInfo, CAssetId part2,
             const SLdrAudioPlaybackParms& audioPlaybackParms, CAssetId grappleGuardianEyeGlow,
             CAssetId alternateScannableInfo, const CActorParameters& actParms);

  // CEntity
  ~CGrenchler() override;
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
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;
  CVector3f GetScanObjectIndicatorPosition(const CStateManager& mgr) const override;
  void FluidFXThink(EFluidState state, CScriptWater& water, CStateManager& mgr) override;
  void OnScanStateChange(EScanState state, CStateManager& mgr) override;
  CAABox GetSortingBounds(const CStateManager& mgr) const override;
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                       float dt) override;
  CScannableObjectInfo* GetScannableObjectInfo() const override;

  // CAi
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  void TakeDamage(const CVector3f& direction, float magnitude) override;

  // CPatterned
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  CDamageInfo GetContactDamage() const override;
  bool CanBeUnPossessed(CStateManager& mgr) const override;
  void IssueDeathBodyCommand(CStateManager& mgr, const CVector3f& direction) override;
  float GetFadeOnDeathTime() const override;

  // Transition functions
  virtual bool AbortCharge(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Alerted(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Attacked(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool AttackPatternOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BeamBadAngle(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BeamHitPlayer(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BeamHitSticky(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BeamHitWall(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Bored(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool BreakGrappleLoop(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanBeamAttack(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanBite(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanBurstAttack(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CancelManeuvering(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanCharge(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CanGrapple(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ChargeFinished(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ClearLineOfFire(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ClearPathToPlayer(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool CrystalDamaged(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool EmergedFromWater(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool FacingJumpEnd(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool FacingPlayer(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ForceGrapple(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Frustrated(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool GrapplingMorphball(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool HasValidJumpTarget(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InBeamRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InBiteRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InBurstRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InChargeRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool JumpLanded(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool JustBiteAttacked(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool JustBeamAttacked(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool JustBurstAttacked(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool JustHit(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ManeuverDone(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PauseOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PlayerBehindMe(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PlayerStuck(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PlayerSubmerged(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool PlayYellowHitReact(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ReturnToPatrol(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldBackstep(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldSlide(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool ShouldTurn(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool SlideOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool SlideStop(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool StopStruggling(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool StruggleOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Stuck(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Submerged(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TailIntact(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TookDamage(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TookKnockback(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool TooMuchTurning(CStateManager& mgr, const CTriggerData& data) const;

  // State functions
  virtual void Backstep(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void BeamAttack(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void BiteAttack(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void BurstAttack(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Charge(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void ChargeFailed(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Dead(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleAbort(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleBite(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleBreak(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleLoop(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrapplePull(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleSlide(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleSlideBonk(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void GrappleStruggle(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Jump(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Lurk(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Maneuver(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void MorphballBite(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Null(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Patrol(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void PauseBetweenBeams(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void PauseBetweenBites(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Pursue(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void ShakeOff(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void StopBeamAttack(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Taunt(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void Turn(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void TurnToJumpEnd(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void WalkTowardPlayer(CStateManager& mgr, EStateMsg msg, float arg);
  virtual void YellowHitReact(CStateManager& mgr, EStateMsg msg, float arg);

  // Code functions
  virtual void PickJumpTarget(CStateManager& mgr, float arg);
  virtual void SetLastActionAsBeam(CStateManager& mgr, float arg);

  void RegisterStateFunctions(CStateManager& mgr);
  CVector3f GetPlayerPosition(const CStateManager& mgr) const;
  bool InPlayerRange(const CStateManager& mgr, float minRange, float maxRange) const;
  bool IsFacing(const CVector3f& pos, float angle) const;
  bool IsFacingPlayer(const CStateManager& mgr, float angle) const;

private:
  // Guessed names for all helper structs below; layouts from the inlined constructors.
  struct SJumpData {
    SJumpData()
    : x4_(CVector3f::Zero())
    , x10_(CVector3f::Zero())
    , x20_(CDamageVulnerability::ReflectVulnerabilty())
    , x50_(-1000.f) {
      Reset();
    }
    void Reset();

    float x0_;
    CVector3f x4_;
    CVector3f x10_;
    bool x1c_24_ : 1;
    bool x1c_25_ : 1;
    bool x1c_26_ : 1;
    bool x1c_27_ : 1;
    bool x1c_28_ : 1;
    bool x1c_29_ : 1;
    CDamageVulnerability x20_;
    float x50_;
  };

  struct SAttackParms {
    SAttackParms(const CDamageInfo& damage, float minRange, float maxRange, float minPause,
                 float maxPause)
    : mDamage(damage)
    , mMinRange(minRange)
    , mMaxRange(maxRange)
    , mMinPause(minPause)
    , mMaxPause(maxPause) {}

    CDamageInfo mDamage;
    float mMinRange;
    float mMaxRange;
    float mMinPause;
    float mMaxPause;
  };

  struct SBiteAttack {
    SBiteAttack(const CDamageInfo& damage, float minRange, float maxRange, float minPause,
                float maxPause, float damageRadius)
    : x0_(false)
    , mDamage(damage)
    , mMinRange(minRange)
    , mMaxRange(maxRange)
    , mMinPause(minPause)
    , mMaxPause(maxPause)
    , x30_(CTransform4f::Identity())
    , mDamageRadius(damageRadius)
    , x64_(-1000.f) {}

    bool x0_;
    CDamageInfo mDamage;
    float mMinRange;
    float mMaxRange;
    float mMinPause;
    float mMaxPause;
    CTransform4f x30_;
    float mDamageRadius;
    float x64_;
  };

  struct SBeamAttack : SAttackParms {
    SBeamAttack(const CDamageInfo& damage, const SLdrAudioPlaybackParms& sound, float minRange,
                float maxRange, float minPause, float maxPause, float maxAngle)
    : SAttackParms(damage, minRange, maxRange, minPause, maxPause)
    , x2c_(false)
    , x30_(damage)
    , x4c_(-1000.f)
    , x50_(CVector3f::Zero())
    , mMaxAngle(maxAngle)
    , mSound(sound)
    , x78_(0)
    , x7c_(CTransform4f::Identity()) {}

    bool x2c_;
    CDamageInfo x30_;
    float x4c_;
    CVector3f x50_;
    float mMaxAngle;
    SLdrAudioPlaybackParms mSound;
    int x78_;
    CTransform4f x7c_;
  };

  struct SBurstAttack : SAttackParms {
    SBurstAttack(CAssetId projectile, const CDamageInfo& damage, float minRange, float maxRange,
                 float minPause, float maxPause, float damageRadius)
    : SAttackParms(damage, minRange, maxRange, minPause, maxPause)
    , x2c_(false)
    , mProjectile(projectile)
    , x34_(0.f)
    , mDamageRadius(damageRadius)
    , x3c_(-1000.f) {}

    bool x2c_;
    CAssetId mProjectile;
    float x34_;
    float mDamageRadius;
    float x3c_;
  };

  struct SChargeData {
    SChargeData(float minTimeBetweenCharges, float f2, float minRange, float maxRange)
    : x0_(CVector3f::Zero())
    , xc_(CVector3f::Zero())
    , mMinRange(minRange)
    , mMaxRange(maxRange)
    , x24_(-1000.f)
    , x28_(1.f)
    , mMinTimeBetweenCharges(minTimeBetweenCharges)
    , x30_(f2) {
      xc_ = CVector3f::Zero();
      x0_ = xc_;
      x20_ = 0.f;
    }

    CVector3f x0_;
    CVector3f xc_;
    float mMinRange;
    float mMaxRange;
    float x20_;
    float x24_;
    float x28_;
    float mMinTimeBetweenCharges;
    float x30_;
  };

  struct SParticleRef {
    SParticleRef(CAssetId id)
    : mId(id)
    , mDesc(id != kInvalidAssetId ? rstl::optional_object< TToken< CGenDescription > >(
                                        gpSimplePool->GetObj(SObjectTag('PART', id)))
                                  : rstl::optional_object_null()) {}

    CAssetId mId;
    rstl::optional_object< TToken< CGenDescription > > mDesc;
  };

  struct SGrappleBeam {
    SGrappleBeam(CAssetId part, CAssetId electric, CAssetId weapon, CAssetId part2,
                 CAssetId part3)
    : x0_(part)
    , x4_(rs_new CElementGen(
          TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', part))),
          CElementGen::kMOT_Normal, CElementGen::kOSF_One))
    , xc_(electric)
    , x10_(gpSimplePool->GetObj(SObjectTag('ELSC', xc_)))
    , x18_(kInvalidUniqueId)
    , x1c_(gpSimplePool->GetObj(SObjectTag('WPSC', weapon)))
    , x24_(part2)
    , x28_(part2 != kInvalidAssetId
               ? rstl::optional_object< TLockedToken< CGenDescription > >(
                     TLockedToken< CGenDescription >(
                         gpSimplePool->GetObj(SObjectTag('PART', part2))))
               : rstl::optional_object_null())
    , x38_(kInvalidUniqueId)
    , x3c_(part3 == kInvalidAssetId
               ? nullptr
               : rs_new CElementGen(
                     TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', part3))),
                     CElementGen::kMOT_Normal, CElementGen::kOSF_One))
    , x44_(0.f) {
      x10_.Lock();
      x1c_.Lock();
    }

    CAssetId x0_;
    rstl::auto_ptr< CElementGen > x4_;
    CAssetId xc_;
    TToken< CElectricDescription > x10_;
    TUniqueId x18_;
    TToken< CWeaponDescription > x1c_;
    CAssetId x24_;
    rstl::optional_object< TLockedToken< CGenDescription > > x28_;
    TUniqueId x38_;
    rstl::auto_ptr< CElementGen > x3c_;
    float x44_;
  };

  struct SStruggleData {
    SStruggleData(int maxCount, CAssetId part, float f1)
    : xc_(maxCount)
    , x10_(f1)
    , x1c_(-1000.f)
    , x28_(-1)
    , x2c_(part)
    , x30_(kInvalidUniqueId)
    , x34_(part != kInvalidAssetId ? rstl::optional_object< TToken< CGenDescription > >(
                                         gpSimplePool->GetObj(SObjectTag('PART', part)))
                                   : rstl::optional_object_null()) {
      x0_24_ = x0_25_ = x0_26_ = false;
      x8_ = x14_ = x18_ = x20_ = x24_ = 0.f;
      x4_ = 0;
    }

    bool x0_24_ : 1;
    bool x0_25_ : 1;
    bool x0_26_ : 1;
    int x4_;
    float x8_;
    int xc_;
    float x10_;
    float x14_;
    float x18_;
    float x1c_;
    float x20_;
    float x24_;
    int x28_;
    CAssetId x2c_;
    TUniqueId x30_;
    rstl::optional_object< TToken< CGenDescription > > x34_;
  };

  struct SBeamHitData {
    SBeamHitData(CAssetId swoosh, CAssetId part, const CDamageInfo& damage,
                 const SLdrAudioPlaybackParms& sound)
    : x0_(CVector3f::Zero())
    , xc_(0.f)
    , x10_(swoosh)
    , x24_(nullptr)
    , x2c_(part)
    , x30_(nullptr)
    , x38_(CVector3f::Zero())
    , x44_(0.f)
    , x48_(0)
    , x4c_(CVector3f::Zero())
    , x58_(damage)
    , x78_(sound)
    , x90_(0) {}

    CVector3f x0_;
    float xc_;
    CAssetId x10_;
    rstl::optional_object< TCachedToken< CGenDescription > > x14_;
    rstl::auto_ptr< CElementGen > x24_;
    CAssetId x2c_;
    rstl::auto_ptr< CElementGen > x30_;
    CVector3f x38_;
    float x44_;
    int x48_;
    CVector3f x4c_;
    CDamageInfo x58_;
    int x74_;
    SLdrAudioPlaybackParms x78_;
    int x90_;
  };

  struct SDamageEffect {
    SDamageEffect(const CDamageInfo& damage, CAssetId part)
    : mDamage(damage)
    , mDesc(part != kInvalidAssetId ? rstl::optional_object< TToken< CGenDescription > >(
                                          gpSimplePool->GetObj(SObjectTag('PART', part)))
                                    : rstl::optional_object_null()) {}

    CDamageInfo mDamage;
    rstl::optional_object< TToken< CGenDescription > > mDesc;
  };

  struct SVectors {
    SVectors() : x0_(CVector3f::Zero()), xc_(CVector3f::Zero()), x18_(CVector3f::Zero()) {}

    CVector3f x0_;
    CVector3f xc_;
    CVector3f x18_;
  };

  struct SSurfaceEffect {
    SSurfaceEffect(CAssetId part)
    : x0_(part != kInvalidAssetId
              ? rs_new CElementGen(TToken< CGenDescription >(
                                       gpSimplePool->GetObj(SObjectTag('PART', part))),
                                   CElementGen::kMOT_Normal, CElementGen::kOSF_One)
              : nullptr)
    , x8_(CVector3f::Zero())
    , x14_(CVector3f::Zero())
    , x20_(0.f) {}

    rstl::auto_ptr< CElementGen > x0_;
    CVector3f x8_;
    CVector3f x14_;
    float x20_;
  };

  struct SStateMachineRef {
    SStateMachineRef(CAssetId id) : mToken(gpSimplePool->GetObj(SObjectTag('FSM2', id))) {}

    rstl::optional_object< CToken > mToken;
  };

  struct STailModel {
    STailModel(CAssetId model, CAssetId skinRules) : mModel(model), mSkinRules(skinRules) {}

    CAssetId mModel;
    CAssetId mSkinRules;
    rstl::optional_object< TCachedToken< CModel > > x8_;
  };

  struct SAttackHistory {
    SAttackHistory() : x14_(-1), x18_(-1) {
      x34_ = 160.f;
      x20_ = 100.f;
      x24_ = x1c_ = x28_ = x2c_ = x30_ = 0.f;
    }
    int GetLastAction() const { return mHistory.empty() ? x14_ : mHistory.back(); }

    rstl::reserved_vector< int, 4 > mHistory;
    int x14_;
    int x18_;
    float x1c_;
    float x20_;
    float x24_;
    float x28_;
    float x2c_;
    float x30_;
    float x34_;
  };

  int GetLastAction() const { return mAttackHistory.GetLastAction(); }

  CPathFindSearch mPathFindSearch;
  float x8ac_;
  CVector3f x8b0_;
  float x8bc_;
  CBoneTracking mBoneTracking;
  float x8fc_;
  float x900_;
  float x904_;
  int x908_;
  bool x90c_24_ : 1;
  bool x90c_25_ : 1;
  bool mIsGrappleGuardian : 1;
  bool mHasHealthBar : 1;
  bool x90c_28_ : 1;
  bool x90c_29_ : 1;
  bool x90c_30_ : 1;
  CSurfaceAlignmentHelper mSurfaceAlignment;
  rstl::single_ptr< TCachedToken< CScannableObjectInfo > > mAlternateScannableInfo;
  CDamageVulnerability mVulnerability;
  rstl::ncrc_ptr< CNonUniformVulnerability > mNonUniformVulnerability;
  rstl::optional_object< CAABox > mSortingBounds;
  SStateMachineRef mStateMachine2;
  STailModel mTailModel;
  STailModel mTailModelDark;
  bool x9fc_;
  float xa00_;
  TUniqueId xa04_;
  bool xa06_;
  SJumpData mJumpData;
  CCollisionActorManager* mCollisionActorManager;
  SAttackHistory mAttackHistory;
  SBiteAttack mBiteAttack;
  SBeamAttack mBeamAttack;
  SBurstAttack mBurstAttack;
  rstl::optional_object< CModelData > mTaillessModel;
  CTransform4f xc3c_;
  int xc6c_;
  float xc70_;
  float mTailHealth;
  float xc78_;
  ushort mTailHitSound;
  ushort mTailDestroyedSound;
  bool xc80_;
  SChargeData mChargeData;
  float xcb8_;
  bool xcbc_;
  float xcc0_;
  float xcc4_;
  int xcc8_;
  TUniqueId xccc_;
  CVector3f xcd0_;
  float xcdc_;
  float xce0_;
  bool xce4_;
  float xce8_;
  float xcec_;
  float xcf0_;
  bool xcf4_;
  SParticleRef xcf8_;
  SGrappleBeam mGrappleBeam;
  SStruggleData mStruggle;
  SBeamHitData mBeamHit;
  float xe24_;
  float xe28_;
  CVector3f xe2c_;
  int xe38_;
  int xe3c_;
  int xe40_;
  int xe44_;
  int xe48_;
  int xe4c_;
  int xe50_;
  int xe54_;
  float xe58_;
  float xe5c_;
  float xe60_;
  float xe64_;
  float xe68_;
  int xe6c_;
  CTransform4f xe70_;
  bool xea0_24_ : 1;
  bool xea0_25_ : 1;
  bool xea0_26_ : 1;
  float xea4_;
  bool xea8_24_ : 1;
  bool xea8_25_ : 1;
  int xeac_;
  SLdrAudioPlaybackParms xeb0_;
  int xec8_;
  SDamageEffect xecc_;
  SVectors xef4_;
  int xf18_;
  float xf1c_;
  TUniqueId xf20_;
  SSurfaceEffect xf24_;
};
CHECK_SIZEOF(CGrenchler, 0xf48)

#endif // _CGRENCHLER
