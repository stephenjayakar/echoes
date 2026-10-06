#ifndef _CSPORBTOP
#define _CSPORBTOP

#include "Collision/CCollidableSphere.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"

// Original class name from the Wii SEL exports (TypesMatch__9CSporbTopCFi, TCastToPtr<9CSporbTop>).
// The top (head) of the Sporb plant; it follows the base's locator and hosts the grabbed ball.
class CSporbTop : public CPatterned {
public:
  CSporbTop(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
            const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
            const CActorParameters& aParms);

  // CEntity
  ~CSporbTop() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  CVector3f GetOrbitPosition(const CStateManager& mgr) const override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override;

  // CAi
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;

  // CPatterned
  void Freeze(CStateManager& mgr, const CVector3f& position, CUnitVector3f direction,
              float duration, float intoFreezeDuration) override;
  void SetupStateMachine(CStateManager& mgr) override;
  bool IsScanVisorSelfRender() const override { return true; }
  CAABox GetScanVisorRenderBounds(const CStateManager& mgr) const override;
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  CAABox GetModelBounds() const; // Guessed name.
  void StartFlinch(CStateManager& mgr); // Guessed name.
  void OnBaseEvent(); // Guessed name; empty in this build.

  // Triggers
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldPatrol(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldAttack(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldFire(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldReload(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldClose(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSpit(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Sleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Patrol(CStateManager& mgr, EStateMsg msg, float dt);
  void Attack(CStateManager& mgr, EStateMsg msg, float dt);
  void WakeUp(CStateManager& mgr, EStateMsg msg, float dt);
  void GoToSleep(CStateManager& mgr, EStateMsg msg, float dt);
  void Fire(CStateManager& mgr, EStateMsg msg, float dt);
  void Reload(CStateManager& mgr, EStateMsg msg, float dt);
  void Close(CStateManager& mgr, EStateMsg msg, float dt);
  void Spit(CStateManager& mgr, EStateMsg msg, float dt);
  void Flinch(CStateManager& mgr, EStateMsg msg, float dt);

  TUniqueId GetBaseId() const { return mBaseId; }
  void SetBaseId(TUniqueId id) { mBaseId = id; }
  TUniqueId GetProjectileId() const { return mProjectileId; }
  void SetProjectileId(TUniqueId id) { mProjectileId = id; }
  int GetState() const { return mState; }
  void SetState(int state) { mState = state; }

private:
  int mState; // Guessed name; set by the base, read by the triggers.
  CVector3f mOrbitPosition;
  float mFreezeDuration;
  CCollidableSphere mCollisionPrimitive;
  int mGenerateType;     // Guessed name; generate type used by the Fire state.
  TUniqueId mBaseId;       // Guessed name.
  TUniqueId mProjectileId; // Guessed name.
  bool x800_24_ : 1;
};
CHECK_SIZEOF(CSporbTop, 0x808)

#endif // _CSPORBTOP
