#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

static void SetFuncPtrs() {
  static SDigitalGuardian_FuncPtrs funcPtrs;
  SetSDigitalGuardian_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSDigitalGuardian_FuncPtrs(nullptr); }
