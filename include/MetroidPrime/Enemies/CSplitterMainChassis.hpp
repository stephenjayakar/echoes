#ifndef _CSPLITTERMAINCHASSIS
#define _CSPLITTERMAINCHASSIS

#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CLineOfSightTracker.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterMainChassis.hpp"
#include "rstl/single_ptr.hpp"

class CCollisionActorManager;

// Runtime copy of the loader record with converted damage data (layout from the REL copy
// constructor).
struct CSplitterMainChassisData : public SLdrSplitterMainChassisData {
  CSplitterMainChassisData(const SLdrSplitterMainChassisData& data);

  CDamageInfo mLegStabDamage;
  CDamageInfo mSpinAttackDamage;
  CDamageVulnerability mSpinAttackVulnerability;
};
CHECK_SIZEOF(CSplitterMainChassisData, 0x3BC)

// Original class name from the Wii SEL exports (TypesMatch__20CSplitterMainChassisCFi,
// AutoDestruct__20CSplitterMainChassisFf). Member names are guessed.
class CSplitterMainChassis : public CPatterned {
public:
  // The Wii export corroborates this interface, not the DOL facade's original spelling.
  void AutoDestruct(float time);

  // Guessed names; used by the command module while docking.
  CVector3f GetDockPosition() const;
  bool Dock(CStateManager& mgr, TUniqueId commandModule);
  void CancelDockingRequest(const TUniqueId& commandModule);
  bool RequestDocking(const TUniqueId& commandModule);

  float GetHeadHealth() const { return xd28_; } // Guessed name
  bool IsDocked() const { return mDockedCommandModule != kInvalidUniqueId; } // Guessed name
  TUniqueId GetDockedCommandModule() const { return mDockedCommandModule; } // Guessed name
  TUniqueId GetTargetId() const { return xcc0_; }                          // Guessed name

private:
  CSplitterMainChassisData mData;
  CPathFindSearch mPathFindSearch;
  rstl::single_ptr< CCollisionActorManager > mCollisionActorManager;
  TUniqueId mDockedCommandModule;
  TUniqueId mDockingCommandModule;
  CLineOfSightTracker mLineOfSightTracker;
  float xcb0_;
  int xcb4_;
  float xcb8_;
  TUniqueId xcbc_;
  TUniqueId xcbe_;
  TUniqueId xcc0_;
  TUniqueId xcc2_;
  TUniqueId xcc4_;
  TUniqueId xcc6_;
  CSegId mDockLocator;
  CSegId xcc9_;
  CVector3f xccc_;
  CVector3f xcd8_;
  CVector3f xce4_;
  CVector3f xcf0_;
  int xcfc_;
  int mState;
  int xd04_;
  int xd08_;
  float xd0c_;
  float xd10_;
  float xd14_;
  float xd18_;
  float xd1c_;
  float xd20_;
  float xd24_;
  float xd28_;
  float xd2c_;
  float xd30_;
  float xd34_;
  float xd38_;
  float xd3c_;
  bool xd40_24_ : 1;
  bool xd40_25_ : 1;
  bool xd40_26_ : 1;
  bool xd40_27_ : 1;
  bool xd40_28_ : 1;
  bool xd40_29_ : 1;
  bool xd40_30_ : 1;
  bool xd40_31_ : 1;
  bool xd41_24_ : 1;
  bool xd41_25_ : 1;
  bool xd41_26_ : 1;
  bool xd41_27_ : 1;
  bool xd41_28_ : 1;
  bool xd41_29_ : 1;
  bool xd41_30_ : 1;
  bool xd41_31_ : 1;
};
CHECK_SIZEOF(CSplitterMainChassis, 0xD48)

#endif // _CSPLITTERMAINCHASSIS
