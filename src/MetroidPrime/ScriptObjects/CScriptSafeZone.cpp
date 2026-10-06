#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"

#include "MetroidPrime/ScriptLoaderRel.hpp"

CEntity* REL_LoadSafeZone(CStateManager& mgr, CInputStream& input, CEntityInfo& info);
CEntity* REL_LoadSafeZoneCrystal(CStateManager& mgr, CInputStream& input, CEntityInfo& info);

SSafeZone_FuncPtrs REL_loader_SafeZone;

void SetRelLoaderFunctionToLoader() {
  REL_loader_SafeZone.mLoadSafeZone = REL_LoadSafeZone;
  REL_loader_SafeZone.mLoadSafeZoneCrystal = REL_LoadSafeZoneCrystal;
  REL_loader_SafeZone.mApplyRenderEffect =
      static_cast< void (CEntity::*)(CStateManager&) >(&CScriptSafeZone::ApplyRenderEffect);
  SetSSafeZone_FuncPtrs(&REL_loader_SafeZone);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSSafeZone_FuncPtrs(nullptr); }
