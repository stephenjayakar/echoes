#include "MetroidPrime/ScriptObjects/CScriptFrontEndDataNetwork.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Input/CFinalInput.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CSaveGameScreen.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrFrontEndDataNetwork.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptColorModulate.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlatform.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Tweaks/CTweakGui.hpp"
#include "REL/REL_Setup.h"

// The ScriptGui REL entity (native type 11) that forwards a controller index.
struct SGuiControllerSource {
  uchar x0_pad[0x38];
  int mController;
};
extern "C" SGuiControllerSource* fn_8009A6E4(CEntity* entity);

static float sMaxSpin = 1080.f;
static float sSpinAccel = 120.f;

SDataNetworkNode::SDataNetworkNode(TUniqueId id, int index, int parent, bool isProxy,
                                   bool parentIsProxy)
: mId(id)
, mIndex(index)
, mParent(parent)
, mSelectedChild(-1)
, mOffset(CVector3f::Zero())
, mPos(CVector3f::Zero())
, x38(CVector3f::Zero())
, mVelocity(CVector3f::Zero())
, mRenderPos(CVector3f::Zero())
, x5c(1.f)
, x60(0.f)
, x64(0.f)
, mIsProxy(isProxy)
, mParentIsProxy(parentIsProxy) {
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
void SDataNetworkNode::SetSelectedChild(int v) { mSelectedChild = v; }
int SDataNetworkNode::GetSelectedChild() const { return mSelectedChild; }
void SDataNetworkNode::SetParent(int v) { mParent = v; }
void SDataNetworkNode::SetVelocity(const CVector3f& v) { mVelocity = v; }
const CVector3f& SDataNetworkNode::GetVelocity() const { return mVelocity; }
void SDataNetworkNode::SetX38(const CVector3f& v) { x38 = v; }
const CVector3f& SDataNetworkNode::GetX38() const { return x38; }
void SDataNetworkNode::SetRenderPos(const CVector3f& v) { mRenderPos = v; }
const CVector3f& SDataNetworkNode::GetRenderPos() const { return mRenderPos; }
void SDataNetworkNode::SetPos(const CVector3f& v) { mPos = v; }
const CVector3f& SDataNetworkNode::GetPos() const { return mPos; }
void SDataNetworkNode::SetOffset(const CVector3f& v) { mOffset = v; }
const CVector3f& SDataNetworkNode::GetOffset() const { return mOffset; }
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
, mPlatformId(kInvalidUniqueId)
, mPrevIndex(0)
, mCurIndex(0)
, mSpin(CVector2f::Zero())
, mSpinAccel(CVector2f::Zero())
, mOrientation(CQuaternion::NoRotation())
, mTransitionState(0)
, mTransitionForward(1)
, mTransitionT(0.f)
, mTransitionDuration(1.f)
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
, mController(0)
, mActiveController(0)
, mRotationSound(rotationSound)
, mRotationSoundVolume(rotationSoundVolume) {
  if (mIsRoot) {
    mControllers.reserve(4);
  }
}

void CScriptFrontEndDataNetwork::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Lock:
    SetLocked(true, mgr);
    break;
  case kSM_Unlock:
    SetLocked(false, mgr);
    break;
  case kSM_Open:
    if (CScriptFrontEndDataNetwork* root =
            TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(mRootId))) {
      root->OpenNode(GetUniqueId(), mgr);
    }
    break;
  case kSM_Close:
    if (mIsRoot) {
      CloseNode(mgr);
    } else if (CScriptFrontEndDataNetwork* root =
                   TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(mRootId))) {
      root->CloseNode(mgr);
    }
    break;
  case kSM_AreaLoaded: {
    rstl::vector< TUniqueId > ids(FindConnectedObjects(mgr, kSS_Connect, kSM_Attach));
    for (rstl::vector< TUniqueId >::iterator it = ids.begin(); it != ids.end(); ++it) {
      if (TCastToConstPtr< CScriptPlatform >(mgr.GetObjectById(*it))) {
        mPlatformId = *it;
        break;
      }
    }
    if (mIsRoot) {
      BuildNetwork(mgr);
      return;
    }
    break;
  }
  case kSM_Increment:
    if (!GetActive()) {
      mgr.SendScriptMsg(this, GetUniqueId(), kSM_Activate);
      CScriptColorModulate::FadeInHelper(mgr, GetUniqueId(), 0.75f);
    }
    break;
  case kSM_Decrement:
    CScriptColorModulate::FadeOutHelper(mgr, GetUniqueId(), 0.75f);
    break;
  case kSM_Follow:
    if (mIsRoot) {
      if (SGuiControllerSource* source =
              fn_8009A6E4(const_cast< CEntity* >(mgr.GetObjectById(msg.GetSenderId())))) {
        AddController(source->mController);
      }
    }
    break;
  case kSM_Escape:
    ClearControllers();
    break;
  case kSM_Reset:
    ResetTransition();
    break;
  case kSM_InternalMessage02:
    if (CScriptFrontEndDataNetwork* root =
            TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(mRootId))) {
      root->FaceNode(GetUniqueId(), mgr, false);
    }
    break;
  case kSM_InternalMessage03:
    if (CScriptFrontEndDataNetwork* root =
            TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(mRootId))) {
      root->FaceNode(GetUniqueId(), mgr, true);
    }
    break;
  default:
    break;
  }
  CActor::AcceptScriptMsg(mgr, msg);
  if (GetActive()) {
  }
}

void CScriptFrontEndDataNetwork::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (!GetActive() || !mIsRoot) {
    return;
  }

  x1a8 = rstl::max_val(x1a8 - 3.f * dt, 0.f);
  UpdateTransition(mgr, dt);
  mSpin += mSpinAccel * dt;
  mSpin *= 0.97f;
  if (mSpin.MagSquared() > 1166400.f) {
    mSpin = mSpin.AsNormalized() * sMaxSpin;
  }
  const CVector2f spin = mSpin * dt;
  const CQuaternion pitch = CQuaternion::XRotation(CRelAngle::FromDegrees(spin.GetY()));
  const CQuaternion yaw = CQuaternion::ZRotation(CRelAngle::FromDegrees(spin.GetX()));
  mOrientation = mOrientation * yaw * pitch;

  if (mTransitionState == 0 && !mIsLocked) {
    SDataNetworkNode& node = mNodes[mCurIndex];
    const CTransform4f xf(mOrientation.BuildTransform4f());
    const CVector3f target = (-1.f * node.GetNetwork(mgr)->mConnectionRadius) * xf.GetForward();
    float best = 10000.f;
    if (!node.GetNetwork(mgr)->x2d3) {
      const int prevSelected = node.GetSelectedChild();
      const CVector3f& pos = node.GetPos();
      for (int i = 0; i < node.mChildren.size(); ++i) {
        SDataNetworkNode& child = mNodes[node.mChildren[i]];
        const CScriptFrontEndDataNetwork* net = child.GetNetwork(mgr);
        if (net->GetActive() && net->mCanBeSelected) {
          const float distSq = ((child.GetPos() - pos) - target).MagSquared();
          const float score = (prevSelected == i ? 1.f : 1.5f) * distSq;
          if (score < best) {
            node.SetSelectedChild(i);
            best = score;
          }
        }
      }
      if (prevSelected != node.GetSelectedChild()) {
        x1a8 = 1.f;
        if (node.GetSelectedChild() != -1) {
          SendScriptMsgs(kSS_Modify, mgr);
        }
        if (prevSelected != -1) {
          mNodes[node.mChildren[prevSelected]].GetNetwork(mgr)->SendScriptMsgs(kSS_Zero, mgr);
        }
        mNodes[node.mChildren[node.GetSelectedChild()]].GetNetwork(mgr)->SendScriptMsgs(
            kSS_MaxReached, mgr);
      }
    }
    node.GetNetwork(mgr)->SendScriptMsgs(kSS_Inside, mgr);
  }

  const CSaveGameScreen* saveScreen = mgr.mSaveGameScreen.get();
  if (saveScreen == nullptr || saveScreen->GetUIType() == CSaveGameScreen::kUIT_SaveReady) {
    if (mControllers.size() == 0) {
      const CFinalInput& input = mgr.mFinalInputs[mController];
      HandleRotation(input, mgr);
      HandleButtons(input, mgr);
      HandleStick(input, mgr);
    } else {
      const CFinalInput& input = mgr.mFinalInputs[mControllers[mActiveController]];
      bool handled = HandleRotation(input, mgr);
      handled |= HandleButtons(input, mgr);
      handled |= HandleStick(input, mgr);
      if (!handled) {
        for (int i = 0; i < mControllers.size(); ++i) {
          if (i != mActiveController) {
            const CFinalInput& other = mgr.mFinalInputs[mControllers[i]];
            bool otherHandled = HandleRotation(other, mgr);
            otherHandled |= HandleButtons(other, mgr);
            otherHandled |= HandleStick(other, mgr);
            if (otherHandled) {
              mActiveController = i;
              break;
            }
          }
        }
      }
    }
  }
  CSfxManager::SfxVolume(mRotationSfx, mRotationSoundVolume);
}

void CScriptFrontEndDataNetwork::AddToRenderer(const CStateManager& mgr) const {
  EnsureRendered(mgr);
}

bool CScriptFrontEndDataNetwork::CanRenderUnsorted(const CStateManager&) const { return false; }

void CScriptFrontEndDataNetwork::SetRootId(TUniqueId id) { mRootId = id; }

TUniqueId CScriptFrontEndDataNetwork::GetPlatformId() const { return mPlatformId; }

void CScriptFrontEndDataNetwork::BuildNetwork(CStateManager& mgr) {
  mNodes.reserve(64);
  mNodes.push_back_unsafe(SDataNetworkNode(GetUniqueId(), 0, -1, false, false));
  for (int i = 0; i < mNodes.size(); ++i) {
    rstl::vector< TUniqueId > ids(
        mgr.GetObjectById(mNodes[i].mId)->FindConnectedObjects(mgr, kSS_Connect, kSM_Attach));
    for (rstl::vector< TUniqueId >::iterator it = ids.begin(); it != ids.end(); ++it) {
      if (CScriptFrontEndDataNetwork* net =
              TCastToPtr< CScriptFrontEndDataNetwork >(mgr.ObjectById(*it))) {
        net->SetRootId(GetUniqueId());
        mNodes[i].AddChild(AddNode(mgr, *it, i));
      }
    }
  }
  LayoutChildren(0, mgr);
}

int CScriptFrontEndDataNetwork::AddNode(CStateManager& mgr, TUniqueId id, int parent) {
  const int count = mNodes.size();
  for (int i = 0; i < count; ++i) {
    if (id == mNodes[i].mId) {
      return i;
    }
  }
  const CScriptFrontEndDataNetwork* net =
      TCastToConstPtr< CScriptFrontEndDataNetwork >(mgr.GetObjectById(id));
  const CScriptFrontEndDataNetwork* parentNet = mNodes[parent].GetNetwork(mgr);
  mNodes.push_back_unsafe(
      SDataNetworkNode(id, count, parent, net->mIsProxy, parentNet->mIsProxy));
  return count;
}

void CScriptFrontEndDataNetwork::LayoutChildren(int idx, CStateManager& mgr) {
  SDataNetworkNode& node = mNodes[idx];
  const CScriptFrontEndDataNetwork* net = node.GetNetwork(mgr);
  for (rstl::vector< int >::iterator it = node.mChildren.begin(); it != node.mChildren.end();
       ++it) {
    const int childIdx = *it;
    SDataNetworkNode& child = mNodes[childIdx];
    const CVector3f offset = child.GetNetwork(mgr)->GetTranslation() - net->GetTranslation();
    child.SetOffset(offset);
    CVector3f dir = offset;
    if (dir.CanBeNormalized()) {
      dir = net->mConnectionRadius * dir.AsNormalized();
    }
    child.SetPos((node.GetPos() + dir) - mNodes[0].GetPos());
    child.SetRenderPos(child.GetPos());
    LayoutChildren(childIdx, mgr);
  }
}

void CScriptFrontEndDataNetwork::ResetTransition() {
  if (mCurIndex != 0) {
    mNodes[mCurIndex].SetSelectedChild(-1);
  }
  mPrevIndex = 0;
  mCurIndex = 0;
  mNodes[0].SetSelectedChild(-1);
  mSpin = CVector2f::Zero();
  mSpinAccel = CVector2f::Zero();
  mTransitionState = 3;
  mTransitionT = 1.f;
  mTransitionDuration = mExpandTime;
}

void CScriptFrontEndDataNetwork::SimulateChildren(CStateManager& mgr, int idx, float dt) {
  SDataNetworkNode& node = mNodes[idx];
  const CVector3f& center = node.GetPos();
  const float radius = node.GetNetwork(mgr)->mConnectionRadius;
  const float maxAttractStep = 1.83f * radius;
  const float maxFlockStep = 0.83f * radius;
  for (int i = 0; i < node.mChildren.size(); ++i) {
    const int childIdx = node.mChildren[i];
    SDataNetworkNode& child = mNodes[childIdx];
    CVector3f pos = child.GetPos();
    if (node.GetSelectedChild() != -1 &&
        childIdx == node.mChildren[node.GetSelectedChild()] &&
        node.GetNetwork(mgr)->x2ce) {
      child.SetX38(CVector3f::Zero());
      child.SetVelocity(CVector3f::Zero());
    } else {
      CVector3f accel = CVector3f::Zero();
      const bool attract = child.GetNetwork(mgr)->x2cd;
      if (attract) {
        CVector3f target = child.GetOffset();
        target = mOrientation.BuildTransform4f().Rotate(target);
        target += node.GetPos();
        accel += GetAttraction(child, target);
      } else {
        accel += GetSeparation(mgr, node, childIdx);
        accel += GetCohesion(mgr, node, childIdx);
      }
      const CVector3f velocity = child.GetVelocity() + accel;
      CVector3f step = attract ? accel : child.GetX38() + 3.f * (dt * velocity);
      if (step.CanBeNormalized()) {
        const float len = step.Magnitude();
        const float& maxStep = attract ? maxAttractStep : maxFlockStep;
        const float clamped = 0.1f > len ? 0.1f : (maxStep < len ? maxStep : len);
        step = clamped * ((1.f / len) * step);
      }
      pos += dt * step;
      child.SetX38(step);
      child.SetVelocity(velocity);
    }
    const CVector3f delta = pos - center;
    if (delta.CanBeNormalized()) {
      const CVector3f dir = delta.AsNormalized();
      child.SetPos(child.GetNetwork(mgr)->x2cd ? pos : center + radius * dir);
    }
  }
}

void CScriptFrontEndDataNetwork::UpdateRenderPositions(CStateManager& mgr, int idx) {
  const bool forward = mTransitionForward == 1;
  int curIdx = mCurIndex;
  if (mCurIndex > 0 && mNodes[mCurIndex].mParentIsProxy) {
    curIdx = mNodes[mCurIndex].mParent;
  }
  int prevIdx = mPrevIndex;
  if (mPrevIndex > 0 && mNodes[mPrevIndex].mParentIsProxy) {
    prevIdx = mNodes[mPrevIndex].mParent;
  }
  SDataNetworkNode& node = mNodes[idx];
  const CVector3f& pos = node.GetPos();
  node.SetRenderPos(pos);
  if (mTransitionState == 3 && idx > 0) {
    mNodes[node.mParent].SetRenderPos(pos);
  }
  for (int i = 0; i < node.mChildren.size(); ++i) {
    const int childIdx = node.mChildren[i];
    SDataNetworkNode& child = mNodes[childIdx];
    CScriptFrontEndDataNetwork* net = node.GetNetwork(mgr);
    const float invT = 1.f - mTransitionT;
    const float radius = net->mConnectionRadius;
    float scale = 1.f;
    switch (mTransitionState) {
    case 1:
      if (childIdx != curIdx) {
        const CMayaSpline& spline = net->mShrinkSpline;
        scale = spline.GetKnots().size() == 0 ? mTransitionT : spline.EvaluateAt(invT);
      }
      break;
    case 2:
      if (forward) {
        if (childIdx == curIdx) {
          const CMayaSpline& spline = net->mMoveInSpline;
          scale =
              spline.GetKnots().size() == 0 ? mTransitionT : spline.EvaluateAt(mTransitionT);
        }
      } else if (childIdx == prevIdx) {
        const CMayaSpline& spline = net->mMoveSpline;
        scale = spline.GetKnots().size() == 0 ? invT : spline.EvaluateAt(invT);
      }
      break;
    case 3:
      if (childIdx != prevIdx && childIdx != curIdx) {
        const CMayaSpline& spline = net->mExpandSpline;
        scale = spline.GetKnots().size() == 0 ? 1.f - mTransitionT : spline.EvaluateAt(invT);
      }
      break;
    }
    const CVector3f delta = child.GetPos() - pos;
    if (delta.CanBeNormalized()) {
      const CVector3f dir = delta.AsNormalized();
      if (child.GetNetwork(mgr)->x2cd) {
        child.SetRenderPos(pos + scale * delta);
      } else {
        child.SetRenderPos(pos + scale * (radius * dir));
      }
    }
  }
}

void CScriptFrontEndDataNetwork::SetLocked(bool locked, CStateManager& mgr) {
  mIsLocked = locked;
  if (mIsLocked) {
    SendScriptMsgs(kSS_Locked, mgr);
    mSpin = CVector2f::Zero();
    mSpinAccel = CVector2f::Zero();
  } else {
    SendScriptMsgs(kSS_Unlocked, mgr);
  }
}

void CScriptFrontEndDataNetwork::OpenNode(TUniqueId id, CStateManager& mgr) {
  int idx = 0;
  for (int i = 0; i < mNodes.size(); ++i, ++idx) {
    SDataNetworkNode* node = &mNodes[i];
    if (node->mId == id) {
      SDataNetworkNode* target = node;
      if (node->GetNetwork(const_cast< const CStateManager& >(mgr))->mIsProxy) {
        node->GetNetwork(mgr)->SendScriptMsgs(kSS_Entered, mgr);
        idx = node->mChildren[0];
        target = &mNodes[idx];
        target->SetParent(node->mIndex);
        target->SetPos(node->GetPos());
        target->SetRenderPos(node->GetRenderPos());
        LayoutChildren(idx, mgr);
      }
      target->GetNetwork(mgr)->SendScriptMsgs(kSS_Entered, mgr);
      SetSelection(mgr, idx, !GetActive());
      return;
    }
  }
}

void CScriptFrontEndDataNetwork::CloseNode(CStateManager& mgr) {
  SDataNetworkNode& node = mNodes[mCurIndex];
  node.GetNetwork(mgr)->SendScriptMsgs(kSS_PressB, mgr);
  int parent = node.mParent;
  if (parent != -1) {
    node.SetSelectedChild(-1);
    SDataNetworkNode* parentNode = &mNodes[parent];
    if (parentNode->GetNetwork(const_cast< const CStateManager& >(mgr))->mIsProxy) {
      parentNode->GetNetwork(mgr)->SendScriptMsgs(kSS_PressB, mgr);
      parent = parentNode->mParent;
      parentNode = &mNodes[parent];
    }
    parentNode->GetNetwork(mgr)->SendScriptMsgs(kSS_Entered, mgr);
    SetSelection(mgr, parent, false);
  }
}

void CScriptFrontEndDataNetwork::FaceNode(TUniqueId id, const CStateManager& mgr,
                                          bool onlyWhenInactive) {
  if (TCastToConstPtr< CScriptFrontEndDataNetwork >(mgr.GetObjectById(id))) {
    for (int i = 1; i < mNodes.size(); ++i) {
      SDataNetworkNode& node = mNodes[i];
      if (node.mId == id) {
        SDataNetworkNode& parent = mNodes[node.mParent];
        if (onlyWhenInactive && GetActive()) {
          return;
        }
        const CQuaternion target = CQuaternion::FromMatrix(
            CTransform4f::LookAt(node.GetPos(), parent.GetPos(), CVector3f::Up()));
        const float angle = mOrientation.AngleFrom(target).AsDegrees();
        if (angle > 20.f) {
          mOrientation = CQuaternion::Slerp(mOrientation, target, (angle - 20.f) / angle);
        }
        return;
      }
    }
  }
}

void CScriptFrontEndDataNetwork::AddController(int controller) {
  for (int i = 0; i < mControllers.size(); ++i) {
    if (controller == mControllers[i]) {
      return;
    }
  }
  mControllers.push_back_unsafe(controller);
}

void CScriptFrontEndDataNetwork::ClearControllers() {
  mControllers.clear();
  mActiveController = 0;
}

void CScriptFrontEndDataNetwork::Render(const CStateManager& mgr) const {
  if (!mIsRoot) {
    return;
  }
  CVector3f pos = mNodes[mCurIndex].GetRenderPos();
  const CTransform4f rot(mOrientation.BuildTransform4f());
  CGraphics::SetCullMode(kCM_None);
  gpRender->SetModelMatrix(CTransform4f::Identity());
  gpRender->SetDepthReadWrite(false, false);
  gpRender->SetBlendMode_Replace();
  gpRender->PrimColor(CColor::White());
  const int prevIdx = mPrevIndex;
  const SDataNetworkNode& prev = mNodes[prevIdx];
  switch (mTransitionState) {
  case 0:
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, 1.f - mTransitionT, mCurIndex);
    break;
  case 1:
    pos = prev.GetRenderPos();
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, mTransitionT, mPrevIndex);
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, 1.f - mTransitionT, mCurIndex);
    break;
  case 2:
    if (mTransitionForward == 1) {
      pos = prev.GetRenderPos();
    }
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, mTransitionT, mPrevIndex);
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, 1.f - mTransitionT, mCurIndex);
    break;
  case 3:
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, mTransitionT, prevIdx);
    RenderNode(mgr, CTransform4f::Translate(pos) * rot, 1.f - mTransitionT, mCurIndex);
    break;
  }
  CGraphics::SetCullMode(kCM_Front);
}

bool CScriptFrontEndDataNetwork::HandleRotation(const CFinalInput& input, CStateManager& mgr) {
  const float scale = 100.f * input.DeltaTime();
  bool handled = false;
  float x = scale * (-input.GetAnalogLeftX() * gpTweakGui->GetLogBookRotationSpeed());
  float y = scale * (input.GetAnalogLeftY() * gpTweakGui->GetLogBookRotationSpeed());
  CScriptFrontEndDataNetwork* net = mNodes[mCurIndex].GetNetwork(mgr);
  if (net->x2d3) {
    y = x = 0.f;
  }
  if (mIsLocked || mTransitionState != 0) {
    y = x = 0.f;
  }
  if (CMath::AbsF(x) < 0.01f) {
    x = 0.f;
  }
  if (CMath::AbsF(y) < 0.01f) {
    y = 0.f;
  }
  if (close_enough(x, 0.f) && close_enough(y, 0.f)) {
    if (mRotationSfx) {
      CSfxManager::SfxStop(mRotationSfx);
      mRotationSfx = CSfxHandle();
    }
  } else {
    handled = true;
    if (!mRotationSfx) {
      mRotationSfx = CSfxManager::SfxStart(mRotationSound, mRotationSoundVolume, 0x3f,
                                           mgr.GetNextAreaId().Value(), true, true);
    }
  }
  if (!net->x2ce) {
    x = y = 0.f;
  }
  mSpinAccel = CVector2f(x, y) * sSpinAccel;
  return handled;
}

bool CScriptFrontEndDataNetwork::HandleButtons(const CFinalInput& input, CStateManager& mgr) {
  bool handled = false;
  SDataNetworkNode* node = &mNodes[mCurIndex];
  CScriptFrontEndDataNetwork* net = node->GetNetwork(mgr);
  if (mTransitionT == 0.f && !mIsLocked) {
    if (input.PA()) {
      handled = true;
      CScriptFrontEndDataNetwork* current = node->GetNetwork(mgr);
      if (node->mChildren.size() != 0 && node->GetSelectedChild() != -1) {
        node = &mNodes[node->mChildren[node->GetSelectedChild()]];
        if (node->GetNetwork(const_cast< const CStateManager& >(mgr))->mIsLocked) {
          node->GetNetwork(mgr)->SendScriptMsgs(kSS_ResistedDamage, mgr);
        } else {
          current->SendScriptMsgs(kSS_PressA, mgr);
          OpenNode(node->mId, mgr);
        }
      }
    } else if (input.PB()) {
      handled = true;
      if (net->mIsLocked) {
        net->SendScriptMsgs(kSS_ReflectedDamage, mgr);
      } else {
        CloseNode(mgr);
      }
    } else if (input.PX()) {
      handled = true;
      if (!net->mIsLocked) {
        net->SendScriptMsgs(kSS_PressX, mgr);
      }
    } else if (input.PY()) {
      handled = true;
      if (!net->mIsLocked) {
        net->SendScriptMsgs(kSS_PressY, mgr);
      }
    } else if (input.PZ()) {
      handled = true;
      if (!net->mIsLocked) {
        net->SendScriptMsgs(kSS_PressZ, mgr);
      }
    } else if (input.PStart()) {
      handled = true;
      if (!net->mIsLocked) {
        net->SendScriptMsgs(kSS_PressStart, mgr);
      }
    }
  }
  return handled;
}

bool CScriptFrontEndDataNetwork::HandleStick(const CFinalInput& input, CStateManager& mgr) {
  bool handled = false;
  SDataNetworkNode& node = mNodes[mCurIndex];
  CScriptFrontEndDataNetwork* net = node.GetNetwork(mgr);
  int prevSelected = node.GetSelectedChild();
  if (!mIsLocked && net->x2d3) {
    const float x = input.GetAnalogLeftX();
    const float y = input.GetAnalogLeftY();
    if (CMath::AbsF(x) < 0.3f && CMath::AbsF(y) < 0.3f) {
      if (prevSelected == -1) {
        node.SetSelectedChild(0);
        prevSelected = 0;
      }
    } else {
      CVector3f dir(x, 0.f, y);
      handled = true;
      dir = net->mConnectionRadius * dir.AsNormalized();
      dir = mOrientation.Transform(dir);
      const CVector3f& pos = node.GetPos();
      float best = FLT_MAX;
      int bestIdx = 0;
      for (int i = 0; i < node.mChildren.size(); ++i) {
        SDataNetworkNode& child = mNodes[node.mChildren[i]];
        if (!child.GetNetwork(const_cast< const CStateManager& >(mgr))->mIsLocked) {
          const CVector3f delta = (child.GetPos() - pos) - dir;
          const float distSq = delta.MagSquared();
          if (distSq < best) {
            bestIdx = i;
            best = distSq;
          }
        }
      }
      node.SetSelectedChild(bestIdx);
    }
  }
  if (prevSelected != node.GetSelectedChild() && node.GetSelectedChild() != -1) {
    SendScriptMsgs(kSS_Modify, mgr);
  }
  return handled;
}

void CScriptFrontEndDataNetwork::SetSelection(CStateManager& mgr, int index, bool immediate) {
  mPrevIndex = mCurIndex;
  mCurIndex = index;
  if (immediate) {
    mTransitionState = 0;
    mTransitionT = 0.f;
    mTransitionDuration = 1.f;
  } else {
    const SDataNetworkNode& node = mNodes[mPrevIndex];
    float time = node.GetNetwork(mgr)->mShrinkTime;
    if (mCurIndex < mPrevIndex) {
      mTransitionForward = 0;
    } else {
      mTransitionForward = 1;
    }
    mTransitionState = 1;
    mTransitionDuration = time;
    if (node.mChildren.size() == 0) {
      mTransitionT = 0.f;
      UpdateTransition(mgr, 0.f);
    } else {
      mTransitionT = 1.f;
    }
  }
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

CVector3f CScriptFrontEndDataNetwork::GetSeparation(const CStateManager& mgr,
                                                    const SDataNetworkNode& node,
                                                    int idx) const {
  CVector3f result = CVector3f::Zero();
  float best = 999999.f;
  const CVector3f& pos = mNodes[idx].GetPos();
  const int* children = node.mChildren.data();
  const int count = node.mChildren.size();
  for (int i = 0; i < count; ++i) {
    const int childIdx = children[i];
    if (idx != childIdx) {
      const float distSq = (mNodes[childIdx].GetPos() - pos).MagSquared();
      if (distSq < best) {
        best = distSq;
        result = mNodes[childIdx].GetPos();
      }
    }
  }
  const float radius = 1.53f * node.GetNetwork(mgr)->mConnectionRadius;
  result = GetFalloff(radius, 0.8f, pos, result);
  if (node.mParent != -1) {
    result += GetFalloff(radius, 0.8f, pos, mNodes[node.mParent].GetPos());
  }
  return result;
}

CVector3f CScriptFrontEndDataNetwork::GetCohesion(const CStateManager& mgr,
                                                  const SDataNetworkNode& node,
                                                  int idx) const {
  CVector3f sum = CVector3f::Zero();
  const CVector3f& pos = mNodes[idx].GetPos();
  int count = 0;
  const float maxDist = 6.667f * node.GetNetwork(mgr)->mConnectionRadius;
  const float maxDistSq = maxDist * maxDist;
  const int* children = node.mChildren.data();
  const int numChildren = node.mChildren.size();
  for (int i = 0; i < numChildren; ++i) {
    const int childIdx = children[i];
    if (idx != childIdx) {
      if ((pos - mNodes[childIdx].GetPos()).MagSquared() < maxDistSq) {
        ++count;
        sum += mNodes[childIdx].GetPos();
      }
    }
  }
  if (count > 0) {
    sum *= 1.f / count;
    const CVector3f delta = sum - pos;
    if (delta.CanBeNormalized()) {
      const float distSq = delta.MagSquared();
      float t;
      if (distSq < maxDistSq) {
        t = distSq / maxDistSq;
      } else {
        t = 1.f;
      }
      return 0.2f * (t * delta.AsNormalized());
    }
  }
  return CVector3f::Zero();
}

CVector3f CScriptFrontEndDataNetwork::GetAttraction(const SDataNetworkNode& node,
                                                    const CVector3f& pos) const {
  const CVector3f delta = pos - node.GetPos();
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
