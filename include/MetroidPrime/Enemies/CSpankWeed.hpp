#ifndef _CSPANKWEED
#define _CSPANKWEED

#include "types.h"

#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"

#include "rstl/single_ptr.hpp"



// Original class name from the Wii SEL exports (TypesMatch__10CSpankWeedCFi). Prime 1 has the
// same class; the Echoes version builds its arm collision from the model's spatial primitive
// boxes and checks the detection ranges against every player.
class CSpankWeed : public CPatterned {
public:
  CSpankWeed(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
             const CTransform4f& xf, const CModelData& modelData,
             const CActorParameters& actorParams, const CPatternedInfo& patternedInfo,
             float maxDetectionRange, float maxHearingRange, float maxSightRange,
             float hideTime);

  // CEntity
  ~CSpankWeed() override {}
  void Think(float dt, CStateManager& mgr) override;
  CEntity* TypesMatch(int typeId) const override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void Render(const CStateManager& mgr) const override;
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;
  CVector3f GetAimPosition(const CStateManager& mgr, float dt) const override;

  // CAi
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;

  // CSpankWeed
  virtual bool InDetectionRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool InRange(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool HearPlayer(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Delay(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  virtual bool Attacked(CStateManager& mgr, const CTriggerData& data) const;
  virtual void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void FadeIn(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void FadeOut(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Lurk(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  virtual void Flinch(CStateManager& mgr, EStateMsg msg, float dt);

private:
  friend class CSpankWeedCollisionActor;

  bool IsPlayerInRange(CStateManager& mgr, float range) const; // Guessed name

  float mMaxDetectionRange;
  float mDetectionHeightRange;
  float mMaxHearingRange;
  float mMaxSightRange;
  float mHideTime;
  bool mCanKnockBack;
  float x7d8_;
  CVector3f mRetreatOrigin;
  TUniqueId x7e8_;
  rstl::single_ptr< CCollisionActorManager > mCollisionMgr;
  bool mIsHiding;
  CVector3f mLockonOffset;
  CVector3f mLockonTarget;
  int mState;
  int mPreviousState;
  int mAnimPhase;
};
CHECK_SIZEOF(CSpankWeed, 0x818)

#endif // _CSPANKWEED
