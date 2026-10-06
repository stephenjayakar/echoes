#ifndef _CSPORBNEEDLE
#define _CSPORBNEEDLE

#include "Collision/CCollidableSphere.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CPhysicsActor.hpp"
#include "rstl/single_ptr.hpp"

class CElementGen;

// Original class name from the Wii SEL exports (TypesMatch__12CSporbNeedleCFi,
// TCastToPtr<12CSporbNeedle>). Fuse-timed needle shot by the base.
class CSporbNeedle : public CPhysicsActor {
public:
  CSporbNeedle(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CActorParameters& aParms,
               TUniqueId owner, float speed, float mass, float fuseTime, CAssetId explosionEffect,
               CAssetId trailEffect, ushort launchSfx, ushort flightSfx, ushort hitPlayerSfx,
               ushort collisionSfx, ushort explosionSfx, const CDamageInfo& damage);

  // CEntity
  ~CSporbNeedle() override;
  CEntity* TypesMatch(int typeId) const override;
  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;

  // CActor
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  rstl::optional_object< CAABox > GetTouchBounds() const override;
  void Touch(CActor& actor, CStateManager& mgr) override;

  // CPhysicsActor
  const CCollisionPrimitive* GetCollisionPrimitive() const override;
  void CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                    CStateManager& mgr) override;

  void Explode(CStateManager& mgr, TUniqueId hitId); // Guessed name.

private:
  float GetCameraDistanceSquared(CStateManager& mgr) const; // Guessed name.
  void PlaySound(CStateManager& mgr, ushort sfx, uint flags = 0); // Guessed name.
  void UpdateFuse(CStateManager& mgr, float dt);             // Guessed name.
  void UpdateParticles(CStateManager& mgr, float dt);        // Guessed name.

  TUniqueId mOwnerId;
  float mFuseTime;
  float mFuseTimer;
  rstl::single_ptr< CElementGen > mExplosionGen;
  rstl::single_ptr< CElementGen > mTrailGen;
  ushort mLaunchSfx;
  ushort mFlightSfx;
  ushort mHitPlayerSfx;
  ushort mCollisionSfx;
  ushort mExplosionSfx;
  CDamageInfo mDamage;
  uchar mFlightSoundDelay; // Guessed name.
  CCollidableSphere mCollisionPrimitive;
  bool mExploded : 1;
  bool mStuck : 1;
  bool mLaunchSoundPlayed : 1;
};
CHECK_SIZEOF(CSporbNeedle, 0x338)

#endif // _CSPORBNEEDLE
