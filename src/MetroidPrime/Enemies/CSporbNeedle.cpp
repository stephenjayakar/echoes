#include "MetroidPrime/Enemies/CSporbNeedle.hpp"
#include "MetroidPrime/Enemies/CSporbPowerBomb.hpp"

#include "Collision/CCollisionInfoList.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/Cameras/CGameCamera.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSporbNeedle.hpp"
#include "MetroidPrime/TCastTo.hpp"

CSporbPowerBomb::CSporbPowerBomb(bool active, const TToken< CWeaponDescription >& description,
                                 EWeaponType type, const CTransform4f& xf,
                                 EMaterialTypes excludeMaterial, const CDamageInfo& damage,
                                 TUniqueId uid, TAreaId areaId, TUniqueId owner,
                                 TUniqueId homingTarget, uint attribs, bool underwater,
                                 const CVector3f& scale, const CImpactVisorEffect& visorEffect,
                                 bool unused, bool playImpactSound, float fuseTime,
                                 float startDamageTime, float endDamageTime, float damageWaitTime)
: CEnergyProjectile(active, description, type, xf, excludeMaterial, damage, uid, areaId, owner,
                    homingTarget, attribs, underwater, scale, visorEffect, unused,
                    playImpactSound, false, 1.f, 4.f, 4.f)
, mFuseTime(fuseTime)
, mFuseTimer(0.f)
, mDamageTimer(0.f)
, mDamageRadius(0.f)
, mDamageRadiusRate(damage.GetRadius() / (endDamageTime - startDamageTime))
, mStartDamageTime(startDamageTime)
, mEndDamageTime(endDamageTime)
, mExplosionNormal(CVector3f::Up())
, mLandState(kLS_None)
, mDamageWaitTime(damageWaitTime)
, mDamageCooldown(0.f) {}

void CSporbPowerBomb::Think(float dt, CStateManager& mgr) {
  mDamageCooldown = rstl::max_val(0.f, mDamageCooldown - dt);
  if ((mLandState == kLS_HitActor || mLandState == kLS_HitWorld) && mFuseTimer >= mFuseTime) {
    if (mDamageTimer >= mStartDamageTime && mDamageTimer <= mEndDamageTime) {
      CDamageInfo& damage = mOrigDamageInfo;
      SetExplodePending(true);
      damage.SetRadius(mDamageRadius);
      CMaterialFilter filter = GetFilter();
      filter.ExcludeList().Remove(kMT_Character);
      const CVector3f center = GetTranslation();
      const CAABox bounds(center + CVector3f(-mDamageRadius, -mDamageRadius, -mDamageRadius),
                          center + CVector3f(mDamageRadius, mDamageRadius, mDamageRadius));
      rstl::reserved_vector< TUniqueId, 1024 > nearList;
      mgr.BuildNearList(nearList, bounds, filter, this);
      for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
           it != nearList.end(); ++it) {
        const TUniqueId id = *it;
        CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(id));
        const CVector3f pos = GetTranslation();
        if (player != nullptr) {
          const CVector3f delta = player->GetTranslation() - pos;
          if (delta.MagSquared() < mDamageRadius * mDamageRadius) {
            if (mDamageCooldown <= 0.f) {
              if (mgr.TestRayDamage(pos, *player, nearList)) {
                mgr.ApplyRadiusDamage(*this, pos, *player, GetUniqueId(), damage);
                mDamageCooldown = mDamageWaitTime;
              }
            } else if (mgr.TestRayDamage(pos, *player, nearList)) {
              CDamageInfo info = damage;
              info.SetDamage(0.f);
              info.SetRadiusDamage(0.f);
              mgr.ApplyRadiusDamage(*this, pos, *player, GetUniqueId(), info);
            }
          }
        } else if (CActor* actor = TCastToPtr< CActor >(mgr.ObjectById(id))) {
          const CVector3f delta = actor->GetTranslation() - pos;
          if (delta.MagSquared() < mDamageRadius * mDamageRadius &&
              mgr.TestRayDamage(pos, *actor, nearList)) {
            mgr.ApplyRadiusDamage(*this, pos, *actor, GetUniqueId(), damage);
          }
        }
      }
      mDamageRadius += mDamageRadiusRate * dt;
    }
    mDamageTimer += dt;
  } else if (mLandState == kLS_HitWorld) {
    mFuseTimer += dt;
  }
  CEnergyProjectile::Think(dt, mgr);
}

void CSporbPowerBomb::ResolveCollisionWithWorld(const CRayCastResult& result,
                                                CStateManager& mgr) {
  mLandState = kLS_HitWorld;
  mExplosionNormal = result.GetPlane().GetNormal();
  Projectile().SetVelocity(CVector3f::Zero());
  Projectile().SetGravity(CVector3f::Zero());
}

void CSporbPowerBomb::ResolveCollisionWithActor(const CRayCastResult& result, CActor& actor,
                                                CStateManager& mgr) {
  mLandState = kLS_HitActor;
  mFuseTimer = mFuseTime;
  mExplosionNormal = result.GetPlane().GetNormal();
  Projectile().SetVelocity(CVector3f::Zero());
  Projectile().SetGravity(CVector3f::Zero());
}

CRayCastResult CSporbPowerBomb::RayCollisionCheckWithWorld(
    TUniqueId& idOut, const CVector3f& start, const CVector3f& end, float magnitude,
    rstl::reserved_vector< TUniqueId, 1024 >& nearList, CStateManager& mgr,
    EStaticGeometryTest staticTest) {
  return CGameProjectile::RayCollisionCheckWithWorld(idOut, start, end, magnitude, nearList, mgr,
                                                     kSGT_CollisionGeometry);
}

void CSporbPowerBomb::Render(const CStateManager& mgr) const { CEnergyProjectile::Render(mgr); }

rstl::optional_object< TLockedToken< CGenDescription > >
CSporbPowerBomb::GetImpactParticle(CStateManager& mgr) {
  return rstl::optional_object_null();
}

CVector3f CSporbPowerBomb::GetExplosionNormal() const { return mExplosionNormal; }

CSporbPowerBomb::~CSporbPowerBomb() {}

static CElementGen* CreateElementGen(CAssetId id) {
  if (id != kInvalidAssetId) {
    TLockedToken< CGenDescription > desc(gpSimplePool->GetObj(SObjectTag('PART', id)));
    return rs_new CElementGen(desc, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
  }
  return nullptr;
}

CSporbNeedle::CSporbNeedle(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& mData,
                           const CActorParameters& aParms, TUniqueId owner, float speed,
                           float mass, float fuseTime, CAssetId explosionEffect,
                           CAssetId trailEffect, ushort launchSfx, ushort flightSfx,
                           ushort hitPlayerSfx, ushort collisionSfx, ushort explosionSfx,
                           const CDamageInfo& damage)
: CPhysicsActor(uid, name, info, 0, xf, mData, CMaterialList(kMT_Unknown59),
                !mData.IsNull() ? mData.GetBounds()
                                : CAABox(CVector3f(-0.5f, -0.5f, -0.5f), CVector3f(0.5f, 0.5f, 0.5f)),
                SMoverData(mass), aParms, CPhysicsActor::skDefaultStepData)
, mOwnerId(owner)
, mFuseTime(fuseTime)
, mFuseTimer(0.f)
, mExplosionGen(CreateElementGen(explosionEffect))
, mTrailGen(CreateElementGen(trailEffect))
, mLaunchSfx(launchSfx)
, mFlightSfx(flightSfx)
, mHitPlayerSfx(hitPlayerSfx)
, mCollisionSfx(collisionSfx)
, mExplosionSfx(explosionSfx)
, mDamage(damage)
, mFlightSoundDelay(0)
, mCollisionPrimitive(CSphere(CVector3f::Zero(), 0.2f), GetMaterialList())
, mExploded(false)
, mStuck(false)
, mLaunchSoundPlayed(false) {
  SetVelocityWR(speed * xf.GetForward());
  SetMomentumWR(CVector3f::Zero());
  mExplosionGen->SetParticleEmission(false);
  if (!mTrailGen.null()) {
    mTrailGen->SetParticleEmission(true);
  }
  const CMaterialList include(kMT_Unknown59, kMT_Player);
  const CMaterialList exclude(kMT_Projectile, kMT_Character);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(include, exclude));
}

CSporbNeedle::~CSporbNeedle() {}

void CSporbNeedle::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Create:
    break;
  case kSM_AreaLoaded:
    AddMaterial(kMT_Projectile, mgr);
    break;
  }
  CActor::AcceptScriptMsg(mgr, msg);
}

void CSporbNeedle::PreThink(float dt, CStateManager& mgr) { CEntity::PreThink(dt, mgr); }

inline void CSporbNeedle::PlaySound(CStateManager& mgr, ushort sfx, uint flags) {
  const int areaId = mgr.GetNextAreaId().Value();
  ProcessSoundEvent(flags | sfx, 1.f, 0, 0.1f, 50.f, CSegId(0), 0, 0, 0.f, 0x14, 0x7f,
                    GetCameraDistanceSquared(mgr), GetTranslation(), areaId, mgr, true);
}

void CSporbNeedle::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (!mLaunchSoundPlayed) {
    PlaySound(mgr, mLaunchSfx);
    mLaunchSoundPlayed = true;
  }
  if (mFlightSoundDelay != 0) {
    PlaySound(mgr, mFlightSfx, 0xa0000000);
  } else {
    ++mFlightSoundDelay;
  }
  UpdateParticles(mgr, dt);
  if (mExplosionGen->IsSystemDeletable()) {
    mgr.DeleteObjectRequest(GetUniqueId());
  }
  if (mStuck) {
    Stop();
  }
}

void CSporbNeedle::Touch(CActor& actor, CStateManager& mgr) { CActor::Touch(actor, mgr); }

void CSporbNeedle::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                CStateManager& mgr) {
  static const CMaterialList skWorldTypes(kMT_Unknown59, kMT_Ceiling, kMT_Wall, kMT_Floor);
  bool hitObject = false;
  if (id != mOwnerId) {
    const CEntity* entity = mgr.GetObjectById(id);
    const CSporbNeedle* needle = TCastToConstPtr< CSporbNeedle >(entity);
    if (entity != nullptr && needle == nullptr) {
      hitObject = true;
    }
  }
  if (hitObject) {
    Explode(mgr, id);
    StopLoopedSounds();
    PlaySound(mgr, mHitPlayerSfx);
  } else {
    for (int i = 0; i < list.GetCount(); ++i) {
      if (list[i].GetMaterialLeft().SharesMaterials(skWorldTypes)) {
        mStuck = true;
        Stop();
        StopLoopedSounds();
        PlaySound(mgr, mCollisionSfx);
        break;
      }
    }
  }
  CPhysicsActor::CollidedWith(id, list, mgr);
}

void CSporbNeedle::Render(const CStateManager& mgr) const {
  if (!mExploded && HasModelData()) {
    GetModelData()->Render(mgr, GetTransform(), nullptr, CModelFlags::Normal());
  }
}

void CSporbNeedle::AddToRenderer(const CStateManager& mgr) const {
  CActor::AddToRenderer(mgr);
  if (mExploded) {
    if (!mExplosionGen.null()) {
      gpRender->AddParticleGen(*mExplosionGen);
    }
  } else if (!mTrailGen.null()) {
    gpRender->AddParticleGen(*mTrailGen);
  }
}

void CSporbNeedle::UpdateParticles(CStateManager& mgr, float dt) {
  if (GetActive()) {
    const CTransform4f rotation = GetTransform().GetRotation();
    const CVector3f translation = GetTranslation();
    const CVector3f scale =
        HasModelData() ? GetModelData()->GetScale() : CVector3f(1.f, 1.f, 1.f);
    if (mExploded) {
      Stop();
      mExplosionGen->SetOrientation(rotation);
      mExplosionGen->SetGlobalTranslation(translation);
      mExplosionGen->SetGlobalScale(scale);
      mExplosionGen->Update(dt);
    } else if (!mTrailGen.null()) {
      mTrailGen->SetOrientation(rotation);
      mTrailGen->SetTranslation(translation);
      mTrailGen->SetGlobalScale(scale);
      mTrailGen->Update(dt);
    }
    UpdateFuse(mgr, dt);
  }
}

void CSporbNeedle::Explode(CStateManager& mgr, TUniqueId hitId) {
  if (!mExploded) {
    RemoveMaterial(kMT_Unknown59, mgr);
    mExploded = true;
    CSfxManager::AddEmitter(mExplosionSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                            false);
    mExplosionGen->SetParticleEmission(true);
    if (!mTrailGen.null()) {
      mTrailGen->SetParticleEmission(false);
    }

    bool isOwner = hitId == mOwnerId;
    if (const CCollisionActor* colAct =
            TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(hitId))) {
      isOwner = colAct->GetOwnerId() == mOwnerId;
    }
    if (hitId != kInvalidUniqueId && !isOwner) {
      mgr.ApplyDamage(GetUniqueId(), hitId, GetUniqueId(), mDamage,
                      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59), CMaterialList()),
                      CVector3f::Zero());
    }

    if (mDamage.GetRadius() > 0.f && hitId == kInvalidUniqueId) {
      const CVector3f center = GetTranslation();
      const CAABox bounds(center - CVector3f(mDamage.GetRadius(), mDamage.GetRadius(),
                                             mDamage.GetRadius()),
                          center + CVector3f(mDamage.GetRadius(), mDamage.GetRadius(),
                                             mDamage.GetRadius()));
      const CMaterialFilter filter =
          CMaterialFilter::MakeInclude(CMaterialList(kMT_Character, kMT_Player));
      rstl::reserved_vector< TUniqueId, 1024 > nearList;
      mgr.BuildNearList(nearList, bounds, filter, nullptr);
      for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
           it != nearList.end(); ++it) {
        bool skip = *it == mOwnerId;
        if (const CCollisionActor* colAct =
                TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(*it))) {
          skip = colAct->GetOwnerId() == mOwnerId;
        }
        if (!skip) {
          if (CEntity* entity = mgr.ObjectById(*it)) {
            const CActor* actor = static_cast< const CActor* >(entity);
            const CVector3f delta = actor->GetTranslation() - GetTranslation();
            const float distance = delta.Magnitude();
            if (distance < mDamage.GetRadius()) {
              const float falloff = (mDamage.GetRadius() - distance) / mDamage.GetRadius();
              const CDamageInfo damage(mDamage.GetWeaponMode(), falloff * mDamage.GetDamage(),
                                       mDamage.GetRadius(), falloff * mDamage.GetKnockBackPower(),
                                       false, false);
              mgr.ApplyDamage(GetUniqueId(), *it, GetUniqueId(), damage,
                              CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Unknown59),
                                                                  CMaterialList()),
                              CVector3f::Zero());
            }
          }
        }
      }
    }
  }
}

void CSporbNeedle::UpdateFuse(CStateManager& mgr, float dt) {
  if (!mExploded && mStuck) {
    mFuseTimer += dt;
    if (mFuseTimer >= mFuseTime) {
      Explode(mgr, kInvalidUniqueId);
      PlaySound(mgr, mExplosionSfx);
    }
  }
}

float CSporbNeedle::GetCameraDistanceSquared(CStateManager& mgr) const {
  float distanceSquared = 3.4028235e38f;
  const CVector3f position = GetTranslation();
  for (int i = 0; i < uint(mgr.GetNumPlayers()); ++i) {
    const CGameCamera* camera = mgr.GetCameraManager(i)->GetCurrentCamera(mgr, true);
    const CVector3f delta = camera->GetTranslation() - position;
    const float cameraDistanceSquared = delta.MagSquared();
    if (cameraDistanceSquared < distanceSquared) {
      distanceSquared = cameraDistanceSquared;
    }
  }
  return distanceSquared;
}

const CCollisionPrimitive* CSporbNeedle::GetCollisionPrimitive() const {
  return &mCollisionPrimitive;
}

rstl::optional_object< CAABox > CSporbNeedle::GetTouchBounds() const {
  return GetCollisionPrimitive()->CalculateAABox(GetPrimitiveTransform());
}

CEntity* REL_LoadSporbNeedle(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSporbNeedle sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSporbNeedle.inc"

  if (sldrThis.model == kInvalidAssetId) {
    return nullptr;
  }
  return rs_new CSporbNeedle(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      CModelData(CStaticRes(sldrThis.model, sldrThis.editorProperties.transform.scale)),
      LdrToActorParameters(sldrThis.actorInformation), kInvalidUniqueId, sldrThis.initialSpeed,
      sldrThis.mass, sldrThis.fuseTime, sldrThis.explosionEffect, sldrThis.trailEffect,
      sldrThis.launchSound, sldrThis.flightSound, sldrThis.hitPlayerSound,
      sldrThis.collisionSound, sldrThis.explosionSound, LdrToDamageInfo(sldrThis.attackDamage));
}
