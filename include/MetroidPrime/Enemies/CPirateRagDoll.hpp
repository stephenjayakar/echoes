#ifndef _CPIRATERAGDOLL
#define _CPIRATERAGDOLL

#include "types.h"

#include "MetroidPrime/CRagDoll.hpp"

#include "rstl/reserved_vector.hpp"

class CPatterned;

// Defined in the PirateRagDoll REL (module 50); only the parts used by its clients are declared.
class CPirateRagDoll : public CRagDoll {
public:
  CPirateRagDoll(CStateManager& mgr, CPatterned* pirate, ushort thudSfx, uint flags,
                 const float* limbScales, float gravity);
  ~CPirateRagDoll() {}

  void Prime(CStateManager& mgr, const CTransform4f& xf, CModelData& modelData) override;
  void Update(CStateManager& mgr, float dt, float waterTop) override;
  void PreRender(const CVector3f& pos, CModelData& modelData) override;

private:
  uchar xa0_[0x34];
  rstl::reserved_vector< int, 16 > xd4_;
  uchar x118_[4];
};
CHECK_SIZEOF(CPirateRagDoll, 0x11c)

#endif // _CPIRATERAGDOLL
