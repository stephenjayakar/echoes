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
  // Guessed element type: the PirateRagDoll REL fills it from CScriptAIWaypoint locator indices
  // with a placement-constructed 4-byte value that rstl does not treat as trivially destructible.
  struct SWaypointLocator {
    int mIndex;
  };

  rstl::reserved_vector< TUniqueId, 6 > mWaypoints;
  rstl::reserved_vector< SWaypointLocator, 6 > mWaypointLocators;
  rstl::reserved_vector< bool, 6 > mWaypointActive;
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
