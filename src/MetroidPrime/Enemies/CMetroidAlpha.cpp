#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "REL/REL_Setup.h"

// TODO: class header/implementation. Drafts: python <mwcc-permuter>/scripts/m2c_draft.py fn_<id>_<off> --rel Metroid
// Order matters: functions are emitted in REVERSE source order, so RELMain/RELExit (start of .text) go last.

static void SetFuncPtrs() {
  static SMetroid_FuncPtrs funcPtrs;
  // TODO: funcPtrs.<member> = &Load...;  (record layout: include/MetroidPrime/ScriptLoaderRel.hpp)
  SetSMetroid_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSMetroid_FuncPtrs(nullptr); }
