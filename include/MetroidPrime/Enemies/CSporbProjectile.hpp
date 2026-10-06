#ifndef _CSPORBPROJECTILE
#define _CSPORBPROJECTILE

#include "Collision/CCollidableSphere.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"

// Original class name from the Wii SEL exports (TypesMatch__16CSporbProjectileCFi,
// TCastToPtr<16CSporbProjectile>). The pod that swallows and spits the morph ball.
class CSporbProjectile : public CPatterned {
  friend class CSporbBase;

public:
  CSporbProjectile(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                   const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
                   const CActorParameters& aParms, CAssetId ballSpitEffect,
                   CAssetId ballEscapeEffect);

  // CEntity
  ~CSporbProjectile() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override { return &mCollisionPrimitive; }
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  // CPatterned
  void SetupStateMachine(CStateManager& mgr) override;
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  void CreateEscapeEffect(CStateManager& mgr); // Guessed name.
  void CreateSpitEffect(CStateManager& mgr);   // Guessed name.

  // Triggers
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFire(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldReload(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldLaunch(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldClose(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldOpen(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSpit(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Sleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  void WakeUp(CStateManager& mgr, EStateMsg msg, float dt);
  void GoToSleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Fire(CStateManager& mgr, EStateMsg msg, float dt);
  void Reload(CStateManager& mgr, EStateMsg msg, float dt);
  void Launch(CStateManager& mgr, EStateMsg msg, float dt);
  void Close(CStateManager& mgr, EStateMsg msg, float dt);
  void Open(CStateManager& mgr, EStateMsg msg, float dt);
  void Spit(CStateManager& mgr, EStateMsg msg, float dt);

  int GetState() const { return mState; }
  void SetState(int state) { mState = state; }
  TUniqueId GetTopId() const { return mTopId; }
  void SetTopId(TUniqueId id) { mTopId = id; }

private:
  int mState;        // Guessed name; set by the base, read by the triggers.
  int mCaptureState; // Guessed name; 0 idle, 1 hit the world, 2 holding the ball.
  CCollidableSphere mCollisionPrimitive;
  float mCaptureRadius; // Guessed name; twice the collision radius.
  bool mHoldingBall;    // Guessed name.
  bool x7ed_;
  CAssetId mBallSpitEffect;
  CAssetId mBallEscapeEffect;
  uchar mEffectCount;
  TUniqueId mTopId;
};
CHECK_SIZEOF(CSporbProjectile, 0x800)

#endif // _CSPORBPROJECTILE
