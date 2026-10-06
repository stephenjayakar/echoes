#ifndef _CDARKWORLDINFO
#define _CDARKWORLDINFO

#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "Kyoto/TToken.hpp"
#include "rstl/optional_object.hpp"

class CModel;
class CTexture;

// Guessed name. Shared dark-world volume parameters copied into transitions.
struct CDarkWorldInfo {
  // Meanings and identifier types remain unresolved; copied as separate halfwords.
  ushort x0_;
  ushort x2_;
  ushort x4_;
  ushort x6_;
  ushort x8_;
  float xc_;
  rstl::optional_object< TLockedToken< CModel > > x10_;
  float x20_;
  CVector2f mScroll1;
  CVector2f mScroll2;
  CVector2f mTexScale1;
  CVector2f mTexScale2;
  TLockedToken< CTexture > mEnvironment;
  TLockedToken< CTexture > mCloud1;
  TLockedToken< CTexture > mCloud2;
  CColor mColor;
  CColor mAdditiveColor;
};
CHECK_SIZEOF(CDarkWorldInfo, 0x70)

#endif // _CDARKWORLDINFO
