#include "MetroidPrime/ScriptObjects/CScriptAreaDamage.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrAreaDamage.hpp"

#include "rstl/algorithm.hpp"

CScriptAreaDamage::CScriptAreaDamage(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CDamageInfo& damage,
                                     float pulseTime, float graceTime)
: CEntity(uid, info, name, 0)
, mPulseTime(pulseTime)
, mDamage(damage)
, mExcludedPlayers()
, mPlayerGraceTimers()
, mGraceTime(graceTime)
, mPulseAccumulator(0.f) {}

namespace {
// Guessed name. Matches list entries whose key is the given player ID.
class CPlayerIdMatcher {
public:
  CPlayerIdMatcher(const TUniqueId& uid) : mUid(uid) {}
  template < typename T >
  bool operator()(const T& entry) const {
    return entry.first == mUid;
  }

private:
  TUniqueId mUid;
};
} // namespace

void CScriptAreaDamage::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CEntity::AcceptScriptMsg(mgr, msg);
  if (!GetActive()) {
    return;
  }

  switch (msg.GetMessage()) {
  case kSM_Decrement: {
    if (msg.GetOriginator() == kInvalidUniqueId) {
      break;
    }

    rstl::list< TExclusion >::iterator exclusion = rstl::find_if(
        mExcludedPlayers.begin(), mExcludedPlayers.end(), CPlayerIdMatcher(msg.GetOriginator()));
    if (exclusion == mExcludedPlayers.end()) {
      mExcludedPlayers.push_back(TExclusion(msg.GetOriginator(), 0));
    } else {
      ++exclusion->second;
    }

    rstl::list< TGraceTimer >::iterator timer =
        rstl::find_if(mPlayerGraceTimers.begin(), mPlayerGraceTimers.end(),
                      CPlayerIdMatcher(msg.GetOriginator()));
    if (timer != mPlayerGraceTimers.end()) {
      mPlayerGraceTimers.erase(timer);
    }
    break;
  }
  case kSM_Increment: {
    if (msg.GetOriginator() == kInvalidUniqueId) {
      break;
    }

    rstl::list< TExclusion >::iterator exclusion = rstl::find_if(
        mExcludedPlayers.begin(), mExcludedPlayers.end(), CPlayerIdMatcher(msg.GetOriginator()));
    if (exclusion != mExcludedPlayers.end() && exclusion->second-- == 0) {
      mExcludedPlayers.erase(exclusion);
    }

    rstl::list< TGraceTimer >::iterator timer =
        rstl::find_if(mPlayerGraceTimers.begin(), mPlayerGraceTimers.end(),
                      CPlayerIdMatcher(msg.GetOriginator()));
    if (timer != mPlayerGraceTimers.end()) {
      mPlayerGraceTimers.erase(timer);
    }
    break;
  }
  default:
    break;
  }
}

void CScriptAreaDamage::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }

  mPulseAccumulator += dt;
  const bool continuous = mPulseTime == 0.f;
  const float pulseTime = continuous ? dt : mPulseTime;
  while (mPulseAccumulator >= pulseTime) {
    mPulseAccumulator -= pulseTime;
    for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
      const CPlayer* player = mgr.GetPlayer(i);
      const TUniqueId uid = player->GetUniqueId();
      rstl::list< TExclusion >::iterator exclusion = mExcludedPlayers.begin();
      while (exclusion != mExcludedPlayers.end() && exclusion->first != uid) {
        ++exclusion;
      }
      if (exclusion != mExcludedPlayers.end() ||
          player->GetCurrentAreaId() != GetCurrentAreaId()) {
        continue;
      }
      if (!player->GetDamageVulnerability()->WeaponHits(mDamage.GetWeaponMode(), 0)) {
        continue;
      }

      rstl::list< TGraceTimer >::iterator timer = mPlayerGraceTimers.begin();
      while (timer != mPlayerGraceTimers.end() && timer->first != uid) {
        ++timer;
      }
      if (timer == mPlayerGraceTimers.end()) {
        timer = mPlayerGraceTimers.insert(mPlayerGraceTimers.end(), TGraceTimer(uid, 0.f));
      }
      timer->second += pulseTime;
      if (timer->second > mGraceTime) {
        const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
            CMaterialList(kMT_Unknown59), CMaterialList());
        mgr.ApplyDamage(GetUniqueId(), uid, GetUniqueId(),
                        continuous ? CDamageInfo(mDamage, dt) : mDamage, filter,
                        CVector3f::Zero());
        SendScriptMsgs(kSS_Damage, mgr, uid, kSM_None);
      }
    }
  }
}

CEntity* LoadAreaDamage(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrAreaDamage sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrAreaDamage.inc"

  return rs_new CScriptAreaDamage(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToDamageInfo(sldrThis.damage), sldrThis.pulseTime,
                                  sldrThis.graceTime);
}

CScriptAreaDamage::~CScriptAreaDamage() {}
