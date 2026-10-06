#ifndef _CSPACEPIRATE
#define _CSPACEPIRATE

#include "MetroidPrime/Enemies/CPatterned.hpp"

class CSpacePirate : public CPatterned {
public:
  // CEntity
  CEntity* TypesMatch(int typeId) const override;

  bool AttachActorToPirate(TUniqueId id);
  void DetachActorFromPirate();
  TUniqueId GetAttachedActor() const { return mAttachedActor; }
  bool AllEnergyDrained() const { return mAllEnergyDrained; }

private:
  uchar x7c0_[0x138];
  bool x8f8_24_ : 1;
  bool x8f8_25_ : 1;
  bool x8f8_26_ : 1;
  bool x8f8_27_ : 1;
  bool x8f8_28_ : 1;
  bool x8f8_29_ : 1;
  bool mAllEnergyDrained : 1; // Guessed Prime name; read by the Metroid energy drain.
  bool x8f8_31_ : 1;
  uchar x8f9_[0x18b];
  TUniqueId mAttachedActor; // Guessed member name.
  uchar xa86_[0xaa];
  void* xb30_;
  uchar xb34_[0x13c];
};
CHECK_SIZEOF(CSpacePirate, 0xc70)

#endif
