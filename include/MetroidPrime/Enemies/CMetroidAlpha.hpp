#ifndef _CMETROIDALPHA
#define _CMETROIDALPHA

#include "MetroidPrime/Enemies/CPatterned.hpp"

class CStateManager;

// REL-backed enemy; only the members used by other modules are recovered.
class CMetroidAlpha : public CPatterned {
public:
  // Guessed name. The DOL wrapper forwards this notification to the loaded module.
  void OnDockTouch(CStateManager& mgr);

  bool IsAttacking() const { return xa40_29_; } // Guessed name (CSpacePirate::ShouldDodge).

private:
  uchar x7c0_pad_[0xa40 - sizeof(CPatterned)];
  bool xa40_24_ : 1;
  bool xa40_25_ : 1;
  bool xa40_26_ : 1;
  bool xa40_27_ : 1;
  bool xa40_28_ : 1;
  bool xa40_29_ : 1;
};

#endif // _CMETROIDALPHA
