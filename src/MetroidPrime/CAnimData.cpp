#include "MetroidPrime/CAnimData.hpp"

#include "Kyoto/Animation/CAnimMathUtils.hpp"
#include "Kyoto/Animation/CAnimSysContext.hpp"
#include "Kyoto/Animation/CAnimTreeBlend.hpp"
#include "Kyoto/Animation/CAnimTreeNode.hpp"
#include "Kyoto/Animation/CAnimationManager.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "Kyoto/Animation/CJointData_LinearStorage.hpp"
#include "Kyoto/Animation/CPrimitive.hpp"
#include "Kyoto/Animation/CTransitionManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Graphics/CModel.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "rstl/algorithm.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Factories/CCharacterFactory.hpp"

typedef rstl::reserved_vector< rstl::pair< uint, CAdditiveAnimPlayback >, 8 > TAdditiveAnims;

rstl::reserved_vector< CBoolPOINode, 8 > CAnimData::mBoolPOINodes;
rstl::reserved_vector< CInt32POINode, 16 > CAnimData::mInt32POINodes;
rstl::reserved_vector< CParticlePOINode, 64 > CAnimData::mParticlePOINodes;
rstl::reserved_vector< CSoundPOINode, 48 > CAnimData::mSoundPOINodes;
static rstl::reserved_vector< CInt32POINode, 16 > sInt32TransientCache;
static CInt32POINode* sInt32TransientCacheData;
static int sPOICacheReferenceCount;

CAnimData::CAnimData(
    CAssetId selfId, const CCharacterInfo& charInfo, int defaultAnim, int charIdx, bool loop,
    const TLockedToken< CCharLayoutInfo >& layoutData, const TToken< CSkinnedModel >& modelData,
    const rstl::optional_object< TLockedToken< CSkinnedModel > >& iceModelData,
    const rstl::optional_object< TLockedToken< CSpatialPrimitive > >& spatialPrimitive,
    const rstl::ncrc_ptr< CAnimSysContext >& animCtx,
    const rstl::rc_ptr< CAnimationManager >& animMgr,
    const rstl::rc_ptr< CTransitionManager >& transMgr,
    const TLockedToken< CCharacterFactory >& charFactory, bool animatedScale)
: mCharFactory(charFactory)
, mCharInfo(charInfo)
, mLayoutData(layoutData)
, mModelData(modelData)
, mIceModelData(iceModelData)
, mSpatialPrimitive(spatialPrimitive)
, mXrayModel(nullptr)
, mInfraModel(nullptr)
, mAnimCtx(animCtx)
, mAnimMgr(animMgr)
, mAnimDir(kAD_Forward)
, mAabb(CAABox::MakeMaxInvertedBox())
, mParticleDB()
, mSelfId(selfId)
, mAlignPos(CVector3f::Zero())
, mAlignRot(CQuaternion::NoRotation())
, mAnimRoot()
, mTransMgr(transMgr)
, mSpeedScale(1.f)
, mCharIdx(charIdx)
, mCurrentAnim(defaultAnim)
, mPassedBoolCount(0)
, mPassedIntCount(0)
, mPassedParticleCount(0)
, mPassedSoundCount(0)
, mParticleLightIdx(0)
, x2a8_(8)
, mAnimating(false)
, mLoop(loop)
, mAligningPos(false)
, x2ac_27_(false)
, x2ac_28_(false)
, mAnimationJustStarted(false)
, mPoseBuilt(false)
, mAnimatedScale(animatedScale)
, mUniformScale(false)
, x2ad_25_(true)
, mPose(layoutData->GetBodyPartSegIds().GetCount(), animatedScale ? 1 : 0, 0)
, mPoseBuilder(CLayoutDescription(layoutData), animatedScale)
, mJointData()
, mPlaybackParms(-1, -1, 1.f, true)
, mAdditiveAnims()
, mCachedBoundsAnimId(-1)
, mCachedAnimBounds(CAABox::MakeMaxInvertedBox()) {
  if (sPOICacheReferenceCount == 0) {
    mBoolPOINodes.resize(8, CBoolPOINode(0xffffffff, kPT_EmptyBool, CCharAnimTime(0.f), -1, false,
                                         1.f, -1, 0, false));
    mInt32POINodes.resize(16, CInt32POINode(0xffffffff, kPT_EmptyInt32, CCharAnimTime(0.f), -1,
                                            false, 1.f, -1, 0, 0, rstl::string_l("root")));
    mParticlePOINodes.resize(64, CParticlePOINode(0xffffffff, kPT_Particle, CCharAnimTime(0.f), -1,
                                                  false, 1.f, -1, 0,
                                                  CParticleData(0, SObjectTag(0, 0), CSegId(0), 1.f,
                                                                CParticleData::kPM_Initial)));
    mSoundPOINodes.resize(48, CSoundPOINode(0xffffffff, kPT_Sound, CCharAnimTime(0.f), -1, false,
                                            1.f, -1, 0, 0, 0.f, 0.f, CSegId(0), 0, 0, 0.f));
  }
  ++sPOICacheReferenceCount;

  mAabb = mModelData->GetModel()->GetAABB();
  mParticleDB.CacheParticleDesc(charInfo.GetParticleResData());
  // TODO: Build mAnimRoot from the character-mapped defaultAnim with no special orders.
}

CAnimData::~CAnimData() {
  if (--sPOICacheReferenceCount == 0) {
    mBoolPOINodes.clear();
    mInt32POINodes.clear();
    mParticlePOINodes.clear();
    mSoundPOINodes.clear();
  }
}

CAABox CAnimData::GetBoundingBox() const {
  const rstl::vector< rstl::pair< uint, CAABox > >& bounds = mCharInfo.GetAnimBoundsById();
  if (bounds.size() > 0) {
    const CAnimTreeEffectiveContribution contrib = mAnimRoot->GetContributionOfHighestInfluence();
    const uint animId = contrib.GetAnimDatabaseIndex();
    if (animId != mCachedBoundsAnimId) {
      rstl::vector< rstl::pair< uint, CAABox > >::const_iterator it =
          rstl::find_by_key(bounds, contrib.GetAnimDatabaseIndex());
      if (it == bounds.end()) {
        mCachedAnimBounds = mAabb;
      } else {
        mCachedAnimBounds = it->second;
      }
    }
    return mCachedAnimBounds;
  }
  return mAabb;
}

CAABox CAnimData::GetBoundingBox(const CTransform4f& xf) const {
  return GetBoundingBox().GetTransformedAABox(xf);
}

CAABox CAnimData::CalcBoundingBoxFromModelVerts() const {
  // TODO: Accumulate the model vertices after applying the reference pose.
  return mAabb;
}

void CAnimData::ResetPOILists() {
  mPassedBoolCount = 0;
  mPassedIntCount = 0;
  mPassedParticleCount = 0;
  mPassedSoundCount = 0;
}

float CAnimData::GetAverageVelocity(int anim) const {
  // TODO: Weight primitive velocities by their animation durations.
  return 0.f;
}

// Guessed name.
void CAnimData::CollectAnimationResources(rstl::vector< SObjectTag >& tagsOut) const {
  rstl::set< SObjectTag > tags;
  rstl::vector< uint >::const_iterator it = mCharInfo.GetAnimationIndexList().begin();
  for (; it != mCharInfo.GetAnimationIndexList().end(); ++it) {
    const rstl::rc_ptr< IMetaAnim > anim = mAnimMgr->GetMetaAnimation(*it);
    rstl::set< CPrimitive > primitives;
    anim->GetUniquePrimitives(primitives);
    rstl::set< CPrimitive >::const_iterator begin = primitives.begin();
    rstl::set< CPrimitive >::const_iterator end = primitives.end();
    rstl::set< CPrimitive >::const_iterator prim = begin;
    for (; prim != primitives.end(); ++prim) {
      tags.insert(SObjectTag('ANIM', prim->GetAnimResId()));
    }
  }
  tagsOut.reserve(tagsOut.size() + tags.size());
  tagsOut.insert(tagsOut.end(), tags.begin(), tags.end());
}

// Guessed name.
void CAnimData::CollectAnimationTokens(rstl::vector< CToken >& tokensOut, bool lock) const {
  rstl::vector< SObjectTag > tags;
  CollectAnimationResources(tags);
  if (tags.size() == 0) {
    return;
  }
  tokensOut.reserve(tags.size() + tokensOut.size());
  for (int i = 0; i < tags.size(); ++i) {
    CToken token = gpSimplePool->GetObj(tags[i]);
    if (lock) {
      token.Lock();
    }
    tokensOut.push_back_unsafe(token);
  }
}

void CAnimData::AdvanceParticles(const CTransform4f& xf, float dt, const CVector3f& scale,
                                 CStateManager* mgr) {
  mParticleDB.Update(dt, *this, **mLayoutData, xf, scale, mgr);
}

void CAnimData::DrawSkinnedModel(const CSkinnedModel& model, const CModelFlags& flags) const {
  // TODO: Set lighting/debug render state and draw with the linear pose.
}

void CAnimData::InitializeCache() {
  sInt32TransientCache.clear();
  sInt32TransientCache.resize(16, CInt32POINode(0xffffffff, kPT_EmptyInt32, CCharAnimTime(0.f), -1,
                                                false, 1.f, -1, 0, 0, rstl::string_l("root")));
  sInt32TransientCacheData = sInt32TransientCache.data();
}

void CAnimData::FreeCache() {
  sInt32TransientCache.clear();
  sInt32TransientCacheData = nullptr;
}

void CAnimData::SetSkinnedModel(const TLockedToken< CSkinnedModel >& model) {
  mModelData = model;
  mAabb = mModelData->GetModel()->GetAABB();
}

void CAnimData::SetXRayModel(const TLockedToken< CModel >& model,
                             const TLockedToken< CSkinRules >& skin) {
  mXrayModel =
      rstl::rc_ptr< CSkinnedModel >(rs_new CSkinnedModel(model, skin, mModelData->GetLayoutInfo()));
}

void CAnimData::SetInfraModel(const TLockedToken< CModel >& model,
                              const TLockedToken< CSkinRules >& skin) {
  mInfraModel =
      rstl::rc_ptr< CSkinnedModel >(rs_new CSkinnedModel(model, skin, mModelData->GetLayoutInfo()));
}

void CAnimData::AdvanceAnim(CCharAnimTime& time, CVector3f& offset, CQuaternion& rotation) {
  // TODO: Advance/simplify the root and apply the resulting position and rotation deltas.
}

CAdvancementDeltas CAnimData::AdvanceIgnoreParticles(float dt, CRandom16& random,
                                                     bool advanceTree) {
  bool suspendEffects;
  return DoAdvance(dt, suspendEffects, random, advanceTree);
}

CAdvancementDeltas CAnimData::Advance(float dt, float minParticleWeight, const CVector3f& scale,
                                      CStateManager* mgr, CRandom16& random, TAreaId areaId,
                                      bool advanceTree) {
  bool suspendParticles;
  const CAdvancementDeltas deltas = DoAdvance(dt, suspendParticles, random, advanceTree);
  if (suspendParticles) {
    mParticleDB.SuspendAllActiveEffects(mgr);
  }
  const int count = mPassedParticleCount;
  for (int i = 0; i < count; ++i) {
    const CParticlePOINode& node = mParticlePOINodes[i];
    if (node.GetCharacterIndex() == -1 || mCharIdx == node.GetCharacterIndex()) {
      if (node.GetMaximumDistance() > minParticleWeight ||
          (mgr != nullptr && !mgr->IsMultiplayer() &&
           mgr->GetCameraManager(0)->IsInCinematicCamera())) {
        mParticleDB.AddParticleEffect(node.GetNameHash(), node.GetFlags(),
                                      node.GetParticleData(), scale, mgr, areaId, false,
                                      mParticleLightIdx);
      }
    }
  }
  return deltas;
}

CAdvancementDeltas CAnimData::DoAdvance(float dt, bool& suspendEffects, CRandom16& random,
                                        bool advanceTree) {
  // TODO: Advance the animation tree, process POIs and combine additive deltas.
  return CAdvancementDeltas(CVector3f::Zero(), CQuaternion::NoRotation());
}

// Guessed name.
rstl::ncrc_ptr< CAnimTreeNode >
CAnimData::BuildAnimationTree(const CAnimPlaybackParms& parms) const {
  const int animB = parms.GetSecondAnimationId();
  const uint animA = mCharInfo.GetAnimationIndexList()[parms.GetAnimationId()];
  const float blendWeight = parms.GetBlendWeight();
  if (animB != -1) {
    const uint animBIdx = mCharInfo.GetAnimationIndexList()[animB];
    const rstl::ncrc_ptr< CAnimTreeNode > treeA =
        GetAnimationManager()->GetAnimationTree(animA, CMetaAnimTreeBuildOrders::NoSpecialOrders());
    const rstl::ncrc_ptr< CAnimTreeNode > treeB = GetAnimationManager()->GetAnimationTree(
        animBIdx, CMetaAnimTreeBuildOrders::NoSpecialOrders());
    return rstl::ncrc_ptr< CAnimTreeNode >(
        rs_new CAnimTreeBlend(false, treeA, treeB, blendWeight,
                              CAnimTreeBlend::CreatePrimitiveName(treeA, treeB, blendWeight)));
  }
  return GetAnimationManager()->GetAnimationTree(animA, CMetaAnimTreeBuildOrders::NoSpecialOrders());
}

// Guessed name.
rstl::ncrc_ptr< CAnimTreeNode >
CAnimData::BuildTransitionTree(const CAnimPlaybackParms& parms) const {
  // TODO: Build a transition from the current root to BuildAnimationTree(parms).
  return rstl::ncrc_ptr< CAnimTreeNode >();
}

void CAnimData::SetAnimation(const CAnimPlaybackParms& parms, bool noTrans) {
  const uint numChildren = mAnimRoot->VGetNumChildren();
  if (parms.GetAnimationId() == mPlaybackParms.GetAnimationId() ||
      (parms.GetSecondAnimationId() == mPlaybackParms.GetSecondAnimationId() &&
       parms.GetSecondAnimationId() != -1) ||
      (parms.GetBlendWeight() == mPlaybackParms.GetBlendWeight() &&
       parms.GetBlendWeight() != 1.f)) {
    if (mAnimationJustStarted) {
      return;
    }
  }
  if (numChildren >= x2a8_) {
    return;
  }
  ResetPOILists();
  mSpeedScale = 1.f;
  mPlaybackParms.SetAnimationId(parms.GetAnimationId());
  mPlaybackParms.SetSecondAnimationId(parms.GetSecondAnimationId());
  mPlaybackParms.SetBlendWeight(parms.GetBlendWeight());
  mCurrentAnim = parms.GetAnimationId();
  const bool animating = parms.GetAnimating();
  const rstl::ncrc_ptr< CAnimTreeNode > tree = BuildAnimationTree(parms);
  if (!noTrans) {
    mAnimRoot = mTransMgr->GetTransitionTree(mAnimRoot, tree);
  } else {
    mAnimRoot = tree;
  }
  mAnimating = animating;
  CalcPlaybackAlignmentParms(parms, tree);
  ResetPOILists();
  mAnimationJustStarted = true;
}

void CAnimData::GetAnimationPrimitives(const CAnimPlaybackParms& parms,
                                       rstl::set< CPrimitive >& primsOut) const {
  const uint animA = mCharInfo.GetAnimationIndexList()[parms.GetAnimationId()];
  const int animB = parms.GetSecondAnimationId();
  GetAnimationManager()->GetMetaAnimation(animA)->GetUniquePrimitives(primsOut);
  if (animB != -1) {
    const uint animBIdx = mCharInfo.GetAnimationIndexList()[animB];
    GetAnimationManager()->GetMetaAnimation(animBIdx)->GetUniquePrimitives(primsOut);
  }
}

void CAnimData::BuildPoseIfNecessary() const {
  if (!mPoseBuilt) {
    RecalcPoseBuilder(nullptr);
    mPoseBuilt = true;
  }
}

void CAnimData::BuildPose() const { BuildPoseIfNecessary(); }

void CAnimData::PreRender() { BuildPoseIfNecessary(); }

void CAnimData::SetupRender() const { BuildPoseIfNecessary(); }

void CAnimData::Render(const CSkinnedModel& model, const CModelFlags& flags) const {
  SetupRender();
  DrawSkinnedModel(model, flags);
}

void CAnimData::RenderAuxiliary(const CFrustumPlanes& planes) const {
  mParticleDB.AddToRendererClipped(planes);
}

void CAnimData::RecalcPoseBuilder(const CCharAnimTime* time) const {
  CAnimMathUtils::sUseFastSlerp = x2ad_25_;
  const CCharLayoutInfo& layout = **mLayoutData;
  rstl::optional_object< CJointData_LinearStorage > localStorage;
  CJointData_LinearStorage* storage = mJointData.get();
  if (storage == nullptr) {
    storage = new (localStorage.prepare_emplace()) CJointData_LinearStorage(
        layout.GetBodyPartSegIds().GetCount(), CJointData_LinearStorage::kAF_Pool);
  } else {
    storage->ResetFlags();
  }
  if (mAnimatedScale) {
    storage->SetHasScales(true);
  }
  if (time == nullptr) {
    mAnimRoot->VGetSegData(layout, *storage);
  } else {
    mAnimRoot->VGetSegData(layout, *storage, *time);
  }
  AddAdditiveSegData(*storage);
  mPose.BuildPose(layout, *storage);
}

rstl::ncrc_ptr< CAnimSysContext > CAnimData::GetAnimSysContext() const { return mAnimCtx; }

float CAnimData::GetAnimationDuration(int anim) const {
  // TODO: Query the selected animation tree's steady-state duration.
  return 0.f;
}

float CAnimData::GetAnimTimeRemaining(const rstl::string& name) const {
  float remaining = mAnimRoot->VGetTimeRemaining().GetSeconds();
  if (mSpeedScale > 0.f) {
    remaining /= mSpeedScale;
  }
  return remaining;
}

bool CAnimData::IsAnimTimeRemaining(float tolerance, const rstl::string& name) const {
  if (mAnimRoot.GetPtr() != nullptr) {
    return !close_enough(mAnimRoot->VGetTimeRemaining().GetSeconds(), 0.f, tolerance);
  }
  return false;
}

CSegId CAnimData::GetLocatorSegId(const rstl::string& name) const {
  return mLayoutData->GetSegIdFromString(name);
}

CTransform4f CAnimData::GetLocatorTransform(const rstl::string& name,
                                            const CCharAnimTime* time) const {
  CSegId id = mLayoutData->GetSegIdFromString(name);
  return GetLocatorTransform(id, time);
}

CTransform4f CAnimData::GetLocatorTransform(CSegId id, const CCharAnimTime* time) const {
  if (id != CSegId::Invalid()) {
    if (time != nullptr || !mPoseBuilt) {
      RecalcPoseBuilder(time);
      mPoseBuilt = time == nullptr;
    }
    return CTransform4f(mPose.GetRotation(id), mPose.GetOffset(id));
  }
  return CTransform4f::Identity();
}

void CAnimData::CalcPlaybackAlignmentParms(const CAnimPlaybackParms& parms,
                                           const rstl::ncrc_ptr< CAnimTreeNode >& tree) {
  // TODO: Recover alignment events and the locator-relative position/rotation adjustments.
}

void CAnimData::SetRandomPlaybackRate(CRandom16& random) {
  for (int i = 0; i < mPassedIntCount; ++i) {
    const CInt32POINode& poi = mInt32POINodes[i];
    if (poi.GetPoiType() == kPT_RandRate) {
      const int range = poi.GetValue();
      const float rate = (random.Next() % range) / 100.f;
      if ((random.Next() % 100) < 50) {
        mSpeedScale = 1.f + rate;
      } else {
        mSpeedScale = 1.f - rate;
      }
      break;
    }
  }
}

void CAnimData::SetPlaybackRate(float rate) { mSpeedScale = rate; }

void CAnimData::MultiplyPlaybackRate(float scale) { mSpeedScale *= scale; }

CCharAnimTime CAnimData::GetTimeOfUserEventForAnimation(int anim, EUserEventType type) const {
  const uint animIdx = mCharInfo.GetAnimationIndexList()[anim];
  const rstl::ncrc_ptr< CAnimTreeNode > tree =
      GetAnimationManager()->GetAnimationTree(animIdx, CMetaAnimTreeBuildOrders::NoSpecialOrders());
  return GetTimeOfUserEvent(type, CCharAnimTime(GetAnimationDuration(anim)), tree);
}

CCharAnimTime CAnimData::GetTimeOfUserEvent(EUserEventType type, const CCharAnimTime& time) const {
  return GetTimeOfUserEvent(type, time, mAnimRoot);
}

CCharAnimTime CAnimData::GetTimeOfUserEvent(EUserEventType type, const CCharAnimTime& time,
                                            const rstl::ncrc_ptr< CAnimTreeNode >& tree) const {
  const int count = tree->GetInt32POIList(time, sInt32TransientCacheData, 16, 0, 64);
  for (int i = 0; i < count; ++i) {
    const CInt32POINode& node = sInt32TransientCacheData[i];
    if (node.GetPoiType() == kPT_UserEvent && node.GetValue() == type) {
      const CCharAnimTime result = node.GetTime();
      for (; i < count; ++i) {
        sInt32TransientCacheData[i] =
            CInt32POINode(0xffffffff, kPT_EmptyInt32, CCharAnimTime(0.f), -1, false, 1.f, -1, 0, 0,
                          rstl::string_l("root"));
      }
      return result;
    }
    sInt32TransientCacheData[i] = CInt32POINode(0xffffffff, kPT_EmptyInt32, CCharAnimTime(0.f),
                                                -1, false, 1.f, -1, 0, 0, rstl::string_l("root"));
  }
  return CCharAnimTime::Infinity();
}

// Guessed name.
int CAnimData::CountUserEvents(EUserEventType type, const CCharAnimTime& time,
                               const rstl::ncrc_ptr< CAnimTreeNode >& tree) const {
  const int count = tree->GetInt32POIList(time, sInt32TransientCacheData, 16, 0, 64);
  int result = 0;
  for (int i = 0; i < count; ++i) {
    if (sInt32TransientCacheData[i].GetPoiType() == kPT_UserEvent &&
        type == sInt32TransientCacheData[i].GetValue()) {
      ++result;
    }
    sInt32TransientCacheData[i] = CInt32POINode(0xffffffff, kPT_EmptyInt32, CCharAnimTime(0.f), -1, false, 1.f, -1, 0, 0,
                         rstl::string_l("root"));
  }
  return result;
}

rstl::rc_ptr< CAnimationManager > CAnimData::GetAnimationManager() const { return mAnimMgr; }

// Guessed name.
int CAnimData::CountUserEventsForAnimation(int anim, EUserEventType type) const {
  const uint animIdx = mCharInfo.GetAnimationIndexList()[anim];
  const rstl::ncrc_ptr< CAnimTreeNode > tree =
      GetAnimationManager()->GetAnimationTree(animIdx, CMetaAnimTreeBuildOrders::NoSpecialOrders());
  return CountUserEvents(type, CCharAnimTime(GetAnimationDuration(anim)), tree);
}

void CAnimData::Touch(const CSkinnedModel& model, int shaderIdx) {
  model.GetModel()->Touch(shaderIdx);
}

void CAnimData::InitializeEffects(CStateManager& mgr, TAreaId areaId, const CVector3f& scale) {
  const uint effectCount = mCharInfo.GetEffects().size();
  for (uint i = 0; i < effectCount; ++i) {
    const rstl::pair< rstl::string, rstl::vector< CEffectComponent > > effect =
        mCharInfo.GetEffects()[i];
    const uint componentCount = effect.second.size();
    for (uint j = 0; j < componentCount; ++j) {
      const CEffectComponent& component = effect.second[j];
      mParticleDB.CacheParticleDesc(component.GetParticleTag());
      mParticleDB.AddParticleEffect(
          component.GetComponentNameHash(), component.GetFlags(),
          CParticleData(0, component.GetParticleTag(), component.GetSegmentId(),
                        component.GetScale(), component.GetParentedMode()),
          scale, &mgr, areaId, true, mParticleLightIdx);
      mParticleDB.SetParticleEffectState(component.GetComponentNameHash(), false, &mgr);
    }
  }
}

CParticleGenInfo* CAnimData::GetFirstParticleEffect(const rstl::string& name) {
  const CCharacterInfo::TEffectList& effects = mCharInfo.GetEffects();
  CCharacterInfo::TEffectList::const_iterator it = rstl::find_by_key(effects, name);
  if (it != effects.end() && !it->second.empty()) {
    return mParticleDB.GetParticleEffect(it->second[0].GetComponentNameHash());
  }
  return nullptr;
}

void CAnimData::SetEffectState(const rstl::string& name, bool active, CStateManager& mgr) {
  const CCharacterInfo::TEffectList effects = mCharInfo.GetEffects();
  CCharacterInfo::TEffectList::const_iterator it = rstl::find_by_key(effects, name);
  if (it != effects.end()) {
    rstl::vector< CEffectComponent >::const_iterator end = it->second.end();
    rstl::vector< CEffectComponent >::const_iterator comp = it->second.begin();
    for (; comp != end; ++comp) {
      mParticleDB.SetParticleEffectState(comp->GetComponentNameHash(), active, &mgr);
    }
  }
}

void CAnimData::SetEffectComponentExternalParam(const rstl::string& name, int index, float value) {
  const CCharacterInfo::TEffectList effects = mCharInfo.GetEffects();
  CCharacterInfo::TEffectList::const_iterator it = rstl::find_by_key(effects, name);
  if (it != effects.end()) {
    rstl::vector< CEffectComponent >::const_iterator comp = it->second.begin();
    if (comp != it->second.end()) {
      mParticleDB.SetParticleExternalParam(comp->GetComponentNameHash(), index, value);
    }
  }
}

void CAnimData::SetPhase(float phase) { mAnimRoot->VSetPhase(phase); }

void CAnimData::SetKeepJSPose(bool keep) {
  if (keep) {
    if (mJointData.null()) {
      mJointData = rstl::auto_ptr< CJointData_LinearStorage >(rs_new CJointData_LinearStorage(
          mLayoutData->GetBodyPartSegIds().GetCount(), CJointData_LinearStorage::kAF_Heap));
      mJointData->SetZeroRotation();
      mJointData->ResetScales();
      mJointData->SetReferenceOffsets(**mLayoutData);
    }
  } else {
    mJointData = rstl::auto_ptr< CJointData_LinearStorage >();
  }
}

// Guessed name.
void CAnimData::SetAnimationTreeLimit(int limit) { x2a8_ = limit; }

rstl::rc_ptr< CAnimationManager > CAnimData::GetAnimationManager() { return mAnimMgr; }

void CAnimData::AddAdditiveAnimation(uint idx, float weight, bool active, bool fadeOut) {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  TAdditiveAnims::iterator it = mAdditiveAnims.begin();
  for (; it != mAdditiveAnims.end(); ++it) {
    if (anim == it->first) {
      break;
    }
  }
  if (it != mAdditiveAnims.end()) {
    it->second.SetLoop(active);
    CAdditiveAnimPlayback& playback = it->second;
    playback.SetWeight(weight);
    playback.SetFadeOutWhenAnimOver(!playback.IsLoop() && fadeOut);
  } else {
    const rstl::ncrc_ptr< CAnimTreeNode > node =
        GetAnimationManager()->GetAnimationTree(anim, CMetaAnimTreeBuildOrders::NoSpecialOrders());
    const rstl::vector< rstl::pair< uint, CAdditiveAnimationInfo > >& infos =
        mCharFactory->GetAdditiveAnimInfoList();
    rstl::vector< rstl::pair< uint, CAdditiveAnimationInfo > >::const_iterator infoIt =
        rstl::binary_find(infos.begin(), infos.end(), anim,
                          rstl::pair_sorter_finder< rstl::pair< uint, CAdditiveAnimationInfo >,
                                                    rstl::less< uint > >(rstl::less< uint >()));
    const CAdditiveAnimationInfo info =
        infoIt != infos.end() ? infoIt->second : mCharFactory->GetDefaultAdditiveAnimInfo();
    const CAdditiveAnimPlayback playback(node, weight, active, info, fadeOut);
    mAdditiveAnims.push_back(rstl::pair< uint, CAdditiveAnimPlayback >(anim, playback));
  }
}

void CAnimData::DelAdditiveAnimation(uint idx) {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  TAdditiveAnims::iterator it = mAdditiveAnims.begin();
  for (; it != mAdditiveAnims.end(); ++it) {
    if (anim == it->first) {
      break;
    }
  }
  if (it != mAdditiveAnims.end()) {
    CAdditiveAnimPlayback& playback = it->second;
    const CAdditiveAnimPlayback::EPlaybackPhase phase = playback.GetFadingMode();
    if (phase != CAdditiveAnimPlayback::kPP_FadingOut &&
        phase != CAdditiveAnimPlayback::kPP_FadedOut) {
      playback.FadeOut();
    }
  }
}

void CAnimData::DelAdditiveAnimationImmediately(uint idx) {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  for (TAdditiveAnims::iterator it = mAdditiveAnims.begin(); it != mAdditiveAnims.end(); ++it) {
    if (anim == it->first) {
      mAdditiveAnims.erase(it);
      return;
    }
  }
}

float CAnimData::GetAdditiveAnimationWeight(uint idx) {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  for (TAdditiveAnims::const_iterator it = mAdditiveAnims.begin(); it != mAdditiveAnims.end();
       ++it) {
    if (anim == it->first) {
      return it->second.GetWeight();
    }
  }
  return 0.f;
}

bool CAnimData::IsAdditiveAnimationActive(uint idx) const {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  for (TAdditiveAnims::const_iterator it = mAdditiveAnims.begin(); it != mAdditiveAnims.end();
       ++it) {
    if (anim == it->first) {
      return true;
    }
  }
  return false;
}

rstl::rc_ptr< CAnimTreeNode > CAnimData::GetAdditiveAnimationTree(uint idx) const {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  TAdditiveAnims::const_iterator it = mAdditiveAnims.begin();
  for (; it != mAdditiveAnims.end(); ++it) {
    if (anim == it->first) {
      break;
    }
  }
  if (it == mAdditiveAnims.end()) {
    return rstl::rc_ptr< CAnimTreeNode >(nullptr);
  }
  return it->second.GetAnimationTree();
}

const rstl::ncrc_ptr< CAnimTreeNode >& CAnimData::GetAnimationTree() const { return mAnimRoot; }

bool CAnimData::IsAdditiveAnimation(uint idx) const {
  const uint anim = mCharInfo.GetAnimationIndexList()[idx];
  const rstl::vector< rstl::pair< uint, CAdditiveAnimationInfo > >& infos =
      mCharFactory->GetAdditiveAnimInfoList();
  rstl::vector< rstl::pair< uint, CAdditiveAnimationInfo > >::const_iterator it =
      rstl::binary_find(infos.begin(), infos.end(), anim,
                        rstl::pair_sorter_finder< rstl::pair< uint, CAdditiveAnimationInfo >,
                                                  rstl::less< uint > >(rstl::less< uint >()));
  return it != infos.end();
}

SAdvancementResults CAnimData::AdvanceAdditiveAnim(rstl::rc_ptr< CAnimTreeNode >& tree,
                                                   CCharAnimTime time) {
  const SAdvancementResults results = tree->VAdvanceView(time);
  const rstl::optional_object< rstl::ownership_transfer< IAnimReader > > simplified =
      tree->Simplified();
  if (simplified.valid()) {
    tree = Cast(simplified.data());
  }
  return results;
}

CAdvancementDeltas CAnimData::UpdateAdditiveAnims(float dt) {
  TAdditiveAnims::iterator it = mAdditiveAnims.begin();
  while (it != mAdditiveAnims.end()) {
    CAdditiveAnimPlayback& playback = it->second;
    playback.Update(dt);
    const CCharAnimTime remaining = playback.GetAnimationTree()->VGetTimeRemaining();
    const CAdditiveAnimPlayback::EPlaybackPhase phase = playback.GetFadingMode();
    if (close_enough(remaining.GetSeconds(), 0.f) && playback.IsFadeOutWhenAnimOver() &&
        phase != CAdditiveAnimPlayback::kPP_FadedOut &&
        phase != CAdditiveAnimPlayback::kPP_FadingOut) {
      playback.FadeOut();
    }
    if (phase == CAdditiveAnimPlayback::kPP_FadedOut) {
      it = mAdditiveAnims.erase(it);
    } else {
      ++it;
    }
  }
  return AdvanceAdditiveAnims(dt);
}

CAdvancementDeltas CAnimData::AdvanceAdditiveAnims(float dt) {
  // TODO: Advance active additive trees and accumulate their motion.
  return CAdvancementDeltas(CVector3f::Zero(), CQuaternion::NoRotation());
}

void CAnimData::AddAdditiveSegData(CJointData_LinearStorage& data) const {
  const uint count = mAdditiveAnims.size();
  const CCharLayoutInfo& layout = **mLayoutData;
  for (uint i = 0; i < count; ++i) {
    const CAdditiveAnimPlayback& playback = mAdditiveAnims[i].second;
    const float weight = playback.GetWeight();
    if (!close_enough(weight, 0.f)) {
      CJointData_LinearStorage additive(layout.GetNumSegments(), CJointData_LinearStorage::kAF_Pool);
      additive.SetUseZeroOffsets(true);
      if (data.HasScales()) {
        additive.SetHasScales(true);
      }
      playback.GetAnimationTree()->VGetSegData(layout, additive);
      data.Add(additive, weight);
    }
  }
}

// Guessed name.
int CAnimData::FindBestAnimation(const CPASAnimParmData& parms) const {
  return GetPASDatabase().FindBestAnimation(parms, -1).second;
}

// Guessed name.
void CAnimData::SetModelScale(const CVector3f& scale) {
  mUniformScale =
      close_enough(scale.GetX(), scale.GetY()) && close_enough(scale.GetX(), scale.GetZ());
  mPose.SetUniformScale(mUniformScale);
}

void CAnimData::AddAnimatedScale() {
  mAnimatedScale = true;
  mPose.AllocateScale();
}
