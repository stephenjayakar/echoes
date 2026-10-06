#ifndef _CBABYMETROID
#define _CBABYMETROID

#include "MetroidPrime/Enemies/CMetroid.hpp"

// Original class name from the Wii SEL exports (TypesMatch__12CBabyMetroidCFi). Layout past the
// CMetroid base is not recovered yet.
class CBabyMetroid : public CMetroid {
public:
  CBabyMetroid(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
               const CTransform4f& xf, const CModelData& mData, const CPatternedInfo& pInfo,
               const CActorParameters& aParms, const CMetroidData& metroidData);

private:
  uchar xa48_[0x88];
};
CHECK_SIZEOF(CBabyMetroid, 0xAD0)

#endif // _CBABYMETROID
