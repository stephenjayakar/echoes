#ifndef _CSPORBPOWERBOMB
#define _CSPORBPOWERBOMB

#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"

// Guessed class name: the Power Bomb Guardian's lobbed bomb. It has no TypesMatch override,
// so no Wii export names it. After its fuse it grows a damage sphere between two times.
class CSporbPowerBomb : public CEnergyProjectile {
public:
  CSporbPowerBomb(bool active, const TToken< CWeaponDescription >& description,
                  EWeaponType type, const CTransform4f& xf, EMaterialTypes excludeMaterial,
                  const CDamageInfo& damage, TUniqueId uid, TAreaId areaId, TUniqueId owner,
                  TUniqueId homingTarget, uint attribs, bool underwater, const CVector3f& scale,
                  const CImpactVisorEffect& visorEffect, bool unused, bool playImpactSound,
                  float fuseTime, float startDamageTime, float endDamageTime,
                  float damageWaitTime);

  // CEntity
  ~CSporbPowerBomb() override;
  void Think(float dt, CStateManager& mgr) override;

  // CActor
  void Render(const CStateManager& mgr) const override;

  // CGameProjectile
  CRayCastResult RayCollisionCheckWithWorld(TUniqueId& idOut, const CVector3f& start,
                                            const CVector3f& end, float magnitude,
                                            rstl::reserved_vector< TUniqueId, 1024 >& nearList,
                                            CStateManager& mgr,
                                            EStaticGeometryTest staticTest) override;
  void ResolveCollisionWithActor(const CRayCastResult& result, CActor& actor,
                                 CStateManager& mgr) override;

  // CEnergyProjectile
  void ResolveCollisionWithWorld(const CRayCastResult& result, CStateManager& mgr) override;
  rstl::optional_object< TLockedToken< CGenDescription > >
  GetImpactParticle(CStateManager& mgr) override;
  CVector3f GetExplosionNormal() const override;

private:
  enum ELandState {
    kLS_None = -1,
    kLS_HitActor,
    kLS_HitWorld,
  };

  float mFuseTime;
  float mFuseTimer;
  float mDamageTimer;
  float mDamageRadius;
  float mDamageRadiusRate;
  float mStartDamageTime;
  float mEndDamageTime;
  CVector3f mExplosionNormal;
  ELandState mLandState;
  float mDamageWaitTime;
  float mDamageCooldown;
};
CHECK_SIZEOF(CSporbPowerBomb, 0x5a0)

#endif // _CSPORBPOWERBOMB
