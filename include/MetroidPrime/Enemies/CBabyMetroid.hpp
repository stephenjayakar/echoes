#ifndef _CBABYMETROID
#define _CBABYMETROID

#include "MetroidPrime/Enemies/CMetroid.hpp"

// Original class name from the Wii SEL exports (TypesMatch__12CBabyMetroidCFi). Only the members
// used by the recovered functions are named; the rest of the layout is unresolved.
class CBabyMetroid : public CMetroid {
public:
  CBabyMetroid(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
               const CActorParameters& aParms, const CMetroidData& metroidData);

  // CActor
  void Render(const CStateManager& mgr) const override;

  // Triggers
  bool AnimOver(CStateManager& mgr, const CTriggerData& data) const;
  bool ShouldSeekEnergySource(CStateManager& mgr, const CTriggerData& data) const;
  bool AbsorbFinished(CStateManager& mgr, const CTriggerData& data) const;
  bool InEnergySourcePosition(CStateManager& mgr, const CTriggerData& data) const;

  // States
  void Generate(CStateManager& mgr, EStateMsg msg, float dt);

private:
  uint xa48_;
  float xa4c_; // Absorb duration (guessed role).
  float xa50_; // Absorb timer (guessed role).
  uchar xa54_[0x74];
  bool mShouldSeekEnergySource : 1;
  bool xac8_25_ : 1;
  bool xac8_26_ : 1;
};
CHECK_SIZEOF(CBabyMetroid, 0xAD0)

#endif // _CBABYMETROID
