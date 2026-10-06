#include "MetroidPrime/Enemies/CGunTurretBase.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CGunTurretTop.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrGunTurretBase.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

const char* const CGunTurretBase::skConnectLocator = "connect_LCTR";

CEntity* LoadGunTurretBase(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  return nullptr;
}

SGunTurretBase_FuncPtrs REL_loader_GunTurret;

void SetRelLoaderFunctionToLoader() {
  REL_loader_GunTurret.mLoadBase = LoadGunTurretBase;
  REL_loader_GunTurret.mLoadTop = LoadGunTurretTop;
  SetSGunTurretBase_FuncPtrs(&REL_loader_GunTurret);
}

void RELMain() { SetRelLoaderFunctionToLoader(); }

void RELExit() { SetSGunTurretBase_FuncPtrs(nullptr); }
