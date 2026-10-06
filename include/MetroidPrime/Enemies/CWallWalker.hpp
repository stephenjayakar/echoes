#ifndef _CWALLWALKER
#define _CWALLWALKER

#include "types.h"

#include "Collision/CCollidableSphere.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "WorldFormat/CCollisionSurface.hpp"

// Echoes keeps CWallWalker in its own REL (WallCrawler); Parasite, WallWalker and others link
// against it. Layout recovered from the WallCrawler constructor; names follow Prime where the
// usage lines up.
class CWallWalker : public CPatterned {
public:
  enum EType {
    kWT_Parasite = 0,
    kWT_Oculus = 1,
    kWT_Geemer = 2,
    kWT_IceZoomer = 3,
    kWT_Seedling = 4,
  };

  CWallWalker(EPatternedAI character, TUniqueId uid, const rstl::string& name,
              EFlavorType flavor, const CEntityInfo& info, const CTransform4f& xf,
              const CModelData& modelData, const CPatternedInfo& patternedInfo,
              EMovementType movement, EColliderType collider, EBodyType body,
              const CActorParameters& params, float sphereRadius, float collisionCloseMargin,
              float alignAngVel, float advanceWpRadius, float playerObstructionMinDist, float f6,
              float f7, float f8, EType walkerType, bool disableMove, float f9, float f10,
              float f11);

  // CEntity
  ~CWallWalker() override;
  CEntity* TypesMatch(int typeId) const override;
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void Render(const CStateManager& mgr) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override;

  // CWallWalker
  // Unidentified virtual added in Echoes (WallCrawler fn_83_D8); it forwards a struct-returning
  // call to another object. Name and signature are placeholders.
  virtual void UnknownWallWalkerVirtual();

  static CVector3f ProjectVectorToPlane(const CVector3f& vec, const CVector3f& planeDir);

protected:
  void OrientToSurfaceNormal(const CVector3f& normal, float clampAngle);
  void AlignToFloor(CStateManager& mgr, float radius, const CVector3f& newPos, float dt);
  void GotoNextWaypoint(CStateManager& mgr);

  CCollisionSurface mAlignNormal;
  CCollidableSphere mColSphere;
  float mCollisionCloseMargin;
  float mAlignAngVel;
  float mTumbleAngle;
  float mPatrolPauseRemTime;
  float mAdvanceWpRadius;
  float mPlayerObstructionMinDist;
  float mBendingHackWeight;
  EType mWalkerType;
  short mThinkCounter;
  int x834_;
  CVector3f x838_;
  float x844_;
  float x848_;
  float x84c_;
  float x850_;
  float x854_;
  float x858_;
  float x85c_;
  bool mAlignToFloor : 1;
  bool mHasAlignSurface : 1;
  bool mPlayerObstructed : 1;
  bool mDisableMove : 1;
  bool mAddBendingWeight : 1;
  bool mApplyBendingHack : 1;
  bool x860_30_ : 1;
  bool x860_31_ : 1;
  bool x861_24_ : 1;
};
CHECK_SIZEOF(CWallWalker, 0x868)

#endif // _CWALLWALKER
