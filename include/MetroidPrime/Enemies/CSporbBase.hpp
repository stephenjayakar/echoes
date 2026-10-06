#ifndef _CSPORBBASE
#define _CSPORBBASE

#include "MetroidPrime/Enemies/CPatterned.hpp"

// Original class name from the Wii SEL exports (TypesMatch__10CSporbBaseCFi, TCastToPtr<10CSporbBase>).
// The rooted Sporb body (Power Bomb Guardian when configured); owns the top, the grabber and the
// needles. Layout not recovered yet.
class CSporbBase : public CPatterned {
public:
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

  // CPatterned
  void ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                       const CModelFlags& flags) const override;

  CAABox GetModelBounds() const; // Guessed name.

  static const char* const skTopAttachLocator; // Guessed name.
};

#endif // _CSPORBBASE
