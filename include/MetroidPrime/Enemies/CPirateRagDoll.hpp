#ifndef _CPIRATERAGDOLL
#define _CPIRATERAGDOLL

#include "MetroidPrime/CRagDoll.hpp"
#include "MetroidPrime/TGameTypes.hpp"

#include "Kyoto/Math/CVector3f.hpp"
#include "rstl/reserved_vector.hpp"

class CPatterned;

// Defined in the PirateRagDoll REL. Only the members the space pirate REL touches are named.
class CPirateRagDoll : public CRagDoll {
public:
  CPirateRagDoll(CStateManager& mgr, CPatterned* actor, ushort thudSfx, uint flags, float overTime,
                 float floatingGravity, const rstl::reserved_vector< float, 14 >& radii);
  ~CPirateRagDoll() {}

  CVector3f& TorsoImpulse() { return mTorsoImpulse; }
  void SetNoAiCollision(bool value) { x118_25_ = value; } // Guessed name.

private:
  CPatterned* mActor;
  ushort mThudSfx;
  float mSfxTimer;
  CVector3f mLastSFXPos;
  CVector3f mTorsoImpulse;
  rstl::reserved_vector< int, 3 > xc4_;
  rstl::reserved_vector< TUniqueId, 12 > mWaypoints;
  rstl::reserved_vector< int, 2 > xf0_;
  float xfc_;
  float x100_;
  float x104_;
  float x108_;
  float x10c_;
  uchar x110_;
  uchar x111_;
  float x114_;
  bool x118_24_ : 1;
  bool x118_25_ : 1;
};

#endif // _CPIRATERAGDOLL
