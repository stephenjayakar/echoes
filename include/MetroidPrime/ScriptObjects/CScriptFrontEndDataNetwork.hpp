#ifndef _CSCRIPTFRONTENDDATANETWORK
#define _CSCRIPTFRONTENDDATANETWORK

#include "Kyoto/Audio/CSfxHandle.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CMayaSpline.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CVector2f.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "MetroidPrime/CActor.hpp"
#include "rstl/vector.hpp"

class CScriptFrontEndDataNetwork;

// Guessed name. One record per node of the network, owned by the root node.
struct SDataNetworkNode {
  SDataNetworkNode(TUniqueId id, int parent, int depth, bool flagA, bool flagB);

  CScriptFrontEndDataNetwork* GetNetwork(CStateManager& mgr);
  const CScriptFrontEndDataNetwork* GetNetwork(const CStateManager& mgr) const;

  void SetX64(float v);
  void SetX60(float v);
  void SetX5C(float v);
  void SetX1C(int v);
  int GetX1C() const;
  void SetDepth(int v);
  void SetX44(const CVector3f& v);
  const CVector3f& GetX44() const;
  void SetX38(const CVector3f& v);
  const CVector3f& GetX38() const;
  void SetX50(const CVector3f& v);
  const CVector3f& GetX50() const;
  void SetX2C(const CVector3f& v);
  const CVector3f& GetX2C() const;
  void SetX20(const CVector3f& v);
  const CVector3f& GetX20() const;
  void AddChild(int idx);

  TUniqueId mId;
  int mParent;
  int mDepth;
  rstl::vector< int > mChildren;
  int x1c;
  CVector3f x20;
  CVector3f x2c;
  CVector3f x38;
  CVector3f x44;
  CVector3f x50;
  float x5c;
  float x60;
  float x64;
  bool x68_24 : 1;
  bool x68_25 : 1;
};
CHECK_SIZEOF(SDataNetworkNode, 0x6c)

class CScriptFrontEndDataNetwork : public CActor {
public:
  CScriptFrontEndDataNetwork(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                             const CTransform4f& xf, const CMayaSpline& shrinkSpline,
                             const CMayaSpline& moveSpline, const CMayaSpline& expandSpline,
                             const CMayaSpline& moveInSpline, const bool isRoot, const bool b2,
                             const bool b3, const bool isProxy, const bool canBeSelected,
                             const bool isLocked, const bool b7, const bool b8,
                             CAssetId hotDotTexture, CAssetId hotDotHaloTexture,
                             CAssetId hotDotAButtonTexture, const CColor& selectedColor,
                             const CColor& unselectedMinColor, const CColor& unselectedMaxColor,
                             const CColor& disabledColor, TSfxId rotationSound,
                             int rotationSoundVolume, float shrinkTime, float moveTime,
                             float expandTime, float moveInTime, float connectionRadius);

  // CEntity
  ~CScriptFrontEndDataNetwork() override;
  CEntity* TypesMatch(int typeId) const override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg&) override;

  // CActor
  void AddToRenderer(const CStateManager&) const override;
  void Render(const CStateManager&) const override;
  bool CanRenderUnsorted(const CStateManager&) const override;

  TUniqueId GetX15A() const;
  void SetRootId(TUniqueId id);
  void ClearX2F8();
  void AddX2F8(int v);
  void ResetTransition();
  void SetSelection(CStateManager& mgr, int index, bool immediate);
  void UpdateTransition(CStateManager& mgr, float dt);

  CVector3f GetAttraction(const SDataNetworkNode& node, const CVector3f& pos) const;
  CVector3f GetFalloff(float radius, float strength, const CVector3f& a,
                       const CVector3f& b) const;

private:
  TUniqueId mRootId;
  TUniqueId x15a;
  rstl::vector< SDataNetworkNode > mNodes;
  int mPrevIndex;
  int mCurIndex;
  CVector2f x174;
  CVector2f x17c;
  CQuaternion x184;
  int mTransitionState;
  int x198;
  float x19c;
  float x1a0;
  int x1a4;
  int x1a8;
  CMayaSpline mShrinkSpline;
  float mShrinkTime;
  CMayaSpline mMoveSpline;
  float mMoveTime;
  CMayaSpline mExpandSpline;
  float mExpandTime;
  CMayaSpline mMoveInSpline;
  float mMoveInTime;
  bool mIsRoot;
  bool x2cd;
  bool x2ce;
  bool mIsProxy;
  bool mCanBeSelected;
  bool mIsLocked;
  bool x2d2;
  bool x2d3;
  float mConnectionRadius;
  CAssetId mHotDotTexture;
  CAssetId mHotDotHaloTexture;
  CAssetId mHotDotAButtonTexture;
  CColor mSelectedColor;
  CColor mUnselectedMinColor;
  CColor mUnselectedMaxColor;
  CColor mDisabledColor;
  int x2f4;
  rstl::vector< int > x2f8;
  int x308;
  CSfxHandle x30c;
  TSfxId mRotationSound;
  int mRotationSoundVolume;
};
CHECK_SIZEOF(CScriptFrontEndDataNetwork, 0x318)

#endif // _CSCRIPTFRONTENDDATANETWORK
