#include "MetroidPrime/ScriptObjects/CScriptFrontEndDataNetwork.hpp"

#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrFrontEndDataNetwork.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

SDataNetworkNode::SDataNetworkNode(TUniqueId id, int parent, int depth, bool flagA, bool flagB)
: mId(id)
, mParent(parent)
, mDepth(depth)
, x1c(-1)
, x20(CVector3f::Zero())
, x2c(CVector3f::Zero())
, x38(CVector3f::Zero())
, x44(CVector3f::Zero())
, x50(CVector3f::Zero())
, x5c(1.f)
, x60(0.f)
, x64(0.f)
, x68_24(flagA)
, x68_25(flagB) {
  mChildren.reserve(8);
}

CScriptFrontEndDataNetwork* SDataNetworkNode::GetNetwork(CStateManager& mgr) {
  return TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(mId));
}

const CScriptFrontEndDataNetwork* SDataNetworkNode::GetNetwork(const CStateManager& mgr) const {
  return TCastToConstPtr< CScriptFrontEndDataNetwork >(mgr.GetObjectById(mId));
}

void SDataNetworkNode::SetX64(float v) { x64 = v; }
void SDataNetworkNode::SetX60(float v) { x60 = v; }
void SDataNetworkNode::SetX5C(float v) { x5c = v; }
void SDataNetworkNode::SetX1C(int v) { x1c = v; }
int SDataNetworkNode::GetX1C() const { return x1c; }
void SDataNetworkNode::SetDepth(int v) { mDepth = v; }
void SDataNetworkNode::SetX44(const CVector3f& v) { x44 = v; }
const CVector3f& SDataNetworkNode::GetX44() const { return x44; }
void SDataNetworkNode::SetX38(const CVector3f& v) { x38 = v; }
const CVector3f& SDataNetworkNode::GetX38() const { return x38; }
void SDataNetworkNode::SetX50(const CVector3f& v) { x50 = v; }
const CVector3f& SDataNetworkNode::GetX50() const { return x50; }
void SDataNetworkNode::SetX2C(const CVector3f& v) { x2c = v; }
const CVector3f& SDataNetworkNode::GetX2C() const { return x2c; }
void SDataNetworkNode::SetX20(const CVector3f& v) { x20 = v; }
const CVector3f& SDataNetworkNode::GetX20() const { return x20; }
void SDataNetworkNode::AddChild(int idx) { mChildren.push_back_unsafe(idx); }

CScriptFrontEndDataNetwork::CScriptFrontEndDataNetwork(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CMayaSpline& shrinkSpline, const CMayaSpline& moveSpline,
    const CMayaSpline& expandSpline, const CMayaSpline& moveInSpline, const bool isRoot,
    const bool b2, const bool b3, const bool isProxy, const bool canBeSelected,
    const bool isLocked, const bool b7, const bool b8, CAssetId hotDotTexture,
    CAssetId hotDotHaloTexture, CAssetId hotDotAButtonTexture, const CColor& selectedColor,
    const CColor& unselectedMinColor, const CColor& unselectedMaxColor,
    const CColor& disabledColor, TSfxId rotationSound, int rotationSoundVolume,
    float shrinkTime, float moveTime, float expandTime, float moveInTime,
    float connectionRadius)
: CActor(uid, name, info, 0, xf, CModelData::CModelDataNull(), CMaterialList(),
         CActorParameters(), kInvalidUniqueId)
, mRootId(isRoot ? uid : kInvalidUniqueId)
, x15a(kInvalidUniqueId)
, mPrevIndex(0)
, mCurIndex(0)
, x174(CVector2f::Zero())
, x17c(CVector2f::Zero())
, x184(CQuaternion::NoRotation())
, mTransitionState(0)
, x198(1)
, x19c(0.f)
, x1a0(1.f)
, mShrinkSpline(shrinkSpline)
, mShrinkTime(shrinkTime)
, mMoveSpline(moveSpline)
, mMoveTime(moveTime)
, mExpandSpline(expandSpline)
, mExpandTime(expandTime)
, mMoveInSpline(moveInSpline)
, mMoveInTime(moveInTime)
, mIsRoot(isRoot)
, x2cd(b2)
, x2ce(b3)
, mIsProxy(isProxy)
, mCanBeSelected(canBeSelected)
, mIsLocked(isLocked)
, x2d2(b7)
, x2d3(b8)
, mConnectionRadius(connectionRadius)
, mHotDotTexture(hotDotTexture)
, mHotDotHaloTexture(hotDotHaloTexture)
, mHotDotAButtonTexture(hotDotAButtonTexture)
, mSelectedColor(selectedColor)
, mUnselectedMinColor(unselectedMinColor)
, mUnselectedMaxColor(unselectedMaxColor)
, mDisabledColor(disabledColor)
, x2f4(0)
, x308(0)
, mRotationSound(rotationSound)
, mRotationSoundVolume(rotationSoundVolume) {
  if (mIsRoot) {
    x2f8.reserve(4);
  }
}

void CScriptFrontEndDataNetwork::AddToRenderer(const CStateManager& mgr) const {
  EnsureRendered(mgr);
}

bool CScriptFrontEndDataNetwork::CanRenderUnsorted(const CStateManager&) const { return false; }

void CScriptFrontEndDataNetwork::SetRootId(TUniqueId id) { mRootId = id; }

TUniqueId CScriptFrontEndDataNetwork::GetX15A() const { return x15a; }

void CScriptFrontEndDataNetwork::ResetTransition() {
  if (mCurIndex != 0) {
    mNodes[mCurIndex].SetX1C(-1);
  }
  mPrevIndex = 0;
  mCurIndex = 0;
  mNodes[0].SetX1C(-1);
  x174 = CVector2f::Zero();
  x17c = CVector2f::Zero();
  mTransitionState = 3;
  x19c = 1.f;
  x1a0 = mExpandTime;
}

void CScriptFrontEndDataNetwork::SetSelection(CStateManager& mgr, int index, bool immediate) {
  mPrevIndex = mCurIndex;
  mCurIndex = index;
  if (immediate) {
    mTransitionState = 0;
    x19c = 0.f;
    x1a0 = 1.f;
  } else {
    const SDataNetworkNode& node = mNodes[mPrevIndex];
    float time = node.GetNetwork(mgr)->mShrinkTime;
    if (mCurIndex < mPrevIndex) {
      x198 = 0;
    } else {
      x198 = 1;
    }
    mTransitionState = 1;
    x1a0 = time;
    if (node.mChildren.size() == 0) {
      x19c = 0.f;
      UpdateTransition(mgr, 0.f);
    } else {
      x19c = 1.f;
    }
  }
}

void CScriptFrontEndDataNetwork::ClearX2F8() {
  x2f8.clear();
  x308 = 0;
}

void CScriptFrontEndDataNetwork::AddX2F8(int v) {
  for (int i = 0; i < x2f8.size(); ++i) {
    if (v == x2f8[i]) {
      return;
    }
  }
  x2f8.push_back_unsafe(v);
}

CVector3f CScriptFrontEndDataNetwork::GetFalloff(float radius, float strength, const CVector3f& a,
                                                 const CVector3f& b) const {
  const CVector3f delta = a - b;
  const float radSq = radius * radius;
  const float distSq = delta.MagSquared();
  if (distSq < radSq) {
    const float t = 1.f - distSq / radSq;
    if (delta.CanBeNormalized()) {
      return strength * (t * delta.AsNormalized());
    }
  }
  return CVector3f::Zero();
}

CVector3f CScriptFrontEndDataNetwork::GetAttraction(const SDataNetworkNode& node,
                                                    const CVector3f& pos) const {
  const CVector3f delta = pos - node.GetX2C();
  if (delta.CanBeNormalized()) {
    const float maxDist = 0.2f * mConnectionRadius;
    const float maxDistSq = maxDist * maxDist;
    const float distSq = delta.MagSquared();
    float t;
    if (distSq < maxDistSq) {
      t = distSq / maxDistSq;
    } else {
      t = 1.f;
    }
    return 10.f * (t * delta.AsNormalized());
  }
  return CVector3f::Zero();
}

CEntity* REL_LoadFrontEndDataNetwork(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrFrontEndDataNetwork sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrFrontEndDataNetwork.inc"

  return rs_new CScriptFrontEndDataNetwork(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      sldrThis.transitionShrinkSpline, sldrThis.transitionMoveSpline,
      sldrThis.transitionExpandSpline, sldrThis.transitionMoveInSpline, sldrThis.isRoot,
      sldrThis.unknown_0x77f59f4a, sldrThis.unknown_0x29c0cb7f, sldrThis.isProxy,
      sldrThis.canBeSelected, sldrThis.isLocked, sldrThis.unknown_0x8b8fa0fe,
      sldrThis.unknown_0xd0f2d612, sldrThis.hotDotTexture, sldrThis.hotDotHaloTexture,
      sldrThis.hotDotAButtonTexture, sldrThis.selectedColor, sldrThis.unselectedMinColor,
      sldrThis.unselectedMaxColor, sldrThis.disabledColor, sldrThis.rotationSound,
      sldrThis.rotationSoundVolume, sldrThis.transitionShrinkTime, sldrThis.transitionMoveTime,
      sldrThis.transitionExpandTime, sldrThis.transitionMoveInTime, sldrThis.connectionRadius);
}

static void SetFuncPtrs() {
  static SFrontEndDataNetwork_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &REL_LoadFrontEndDataNetwork;
  SetSFrontEndDataNetwork_FuncPtrs(&funcPtrs);
}

void RELMain() { SetFuncPtrs(); }

void RELExit() { SetSFrontEndDataNetwork_FuncPtrs(nullptr); }

CScriptFrontEndDataNetwork::~CScriptFrontEndDataNetwork() {}
