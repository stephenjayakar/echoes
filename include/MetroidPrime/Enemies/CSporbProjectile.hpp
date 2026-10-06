#ifndef _CSPORBPROJECTILE
#define _CSPORBPROJECTILE

#include "MetroidPrime/Enemies/CPatterned.hpp"

// Original class name from the Wii SEL exports (TypesMatch__16CSporbProjectileCFi,
// TCastToPtr<16CSporbProjectile>). The spat morph-ball shell. Layout not recovered yet.
class CSporbProjectile : public CPatterned {
public:
  // CEntity
  CEntity* TypesMatch(int typeId) const override;
};

#endif // _CSPORBPROJECTILE
