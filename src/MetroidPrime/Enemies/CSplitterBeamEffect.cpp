#include "MetroidPrime/Enemies/CSplitterBeamEffect.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Graphics/CGX.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Graphics/CModel.hpp"
#include "Kyoto/Graphics/CTexture.hpp"
#include "Kyoto/Math/CFrustumPlanes.hpp"
#include "Kyoto/PVS/CPVSVisOctree.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "WorldFormat/CPVSAreaSet.hpp"

#include <dolphin/gx.h>
#include <dolphin/os/OSCache.h>

static const CAbsAngle skMinFov = CAbsAngle::FromDegrees(1.f);

// 8x4 I8 ramp used to fade the beam out along its length.
static const uchar skFadeRamp[32] ATTRIBUTE_ALIGN(32) = {
    0xFF, 0xDB, 0xB7, 0x93, 0x6F, 0x4B, 0x27, 0x00, 0xFF, 0xDB, 0xB7,
    0x93, 0x6F, 0x4B, 0x27, 0x00, 0xFF, 0xDB, 0xB7, 0x93, 0x6F, 0x4B,
    0x27, 0x00, 0xFF, 0xDB, 0xB7, 0x93, 0x6F, 0x4B, 0x27, 0x00,
};

CSplitterBeamEffect::CSplitterBeamEffect(TUniqueId uid, const CEntityInfo& info,
                                         const rstl::string& name, const CTransform4f& xf,
                                         int resolution, const CAbsAngle& maxFov, float range,
                                         const CColor& innerColor0, const CColor& innerColor1,
                                         const CColor& outerColor0, const CColor& outerColor1,
                                         float scrollLength, float fadeLength, float expandRate)
: CActor(uid, name, info, 0, xf, CModelData::CModelDataNull(), CMaterialList(kMT_NoStepLogic),
         CActorParameters::None(), kInvalidUniqueId)
, mMaxFov(maxFov)
, mRange(range)
, mInnerColor0(innerColor0)
, mInnerColor1(innerColor1)
, mOuterColor0(outerColor0)
, mOuterColor1(outerColor1)
, mScrollLength(scrollLength)
, mFadeLength(fadeLength)
, mExpandRate(expandRate)
, mDepthTexture(rs_new CTexture(kTF_IA8, 4, resolution, 1))
, mFov(skMinFov)
, mPreRenderCount(0)
, mExpansion(0.f)
, mRandom(99)
, mOuterFlicker(0.f)
, mInnerFlicker(0.f)
, mExpanding(true) {}

CSplitterBeamEffect::~CSplitterBeamEffect() { mDepthTexture->ScheduleDeletion(); }

void CSplitterBeamEffect::PreRenderAllViewports(CStateManager& mgr) {
  const CTransform4f& xf = GetTransform();
  const CVector3f pos = GetTranslation();
  CAABox bounds(pos, pos);
  float tanHalfFov = tan(0.5f * mFov.AsRadians());
  const float halfWidth = mRange * tanHalfFov;
  bounds.AccumulateBounds(xf * CVector3f(0.f, mRange, halfWidth));
  bounds.AccumulateBounds(xf * CVector3f(0.f, mRange, -halfWidth));
  SetOtherBounds(bounds);
  SetRenderBounds(bounds);
  if (GetRenderBoundsDirty()) {
    UpdatePortalSystemState(mgr);
    SetRenderBoundsDirty(false);
  }
}

void CSplitterBeamEffect::PreRender(CStateManager& mgr) {
  ++mPreRenderCount;
  const CTransform4f& xf = GetTransform();
  const CGraphics::CProjectionState oldProjection = CGraphics::GetProjectionState();
  const CTransform4f oldView = CGraphics::GetViewMatrix();
  const CViewport oldViewport = CGraphics::GetViewport();
  const float oldNear = CGraphics::GetDepthNear();
  const float oldFar = CGraphics::GetDepthFar();
  bool useVideoFilter = CGraphics::GetUseVideoFilter();
  CGraphics::SetUseVideoFilter(false);
  gpRender->SetViewport(0, CGraphics::GetRenderMode().efbHeight - mDepthTexture->GetHeight(),
                        mDepthTexture->GetWidth(), mDepthTexture->GetHeight());
  CGraphics::SetDepthRange(0.f, 1.f);
  gpRender->SetWorldViewpoint(xf);
  CGraphics::SetModelMatrix(CTransform4f::Identity());
  const CVector3f extent = mRange * CVector3f::One();
  const CAABox nearBounds(xf.GetTranslation() - extent, xf.GetTranslation() + extent);
  CGX::SetNumChans(0);
  CGraphics::DisableAllLights();

  CPVSVisSet visSet(kVSS_OutOfBounds);
  const CGameArea* area = mgr.GetWorld()->GetArea(GetCurrentAreaId());
  const CPVSAreaSet* pvs = area->GetPostConstructed()->mPvs.get();
  if (pvs != nullptr && gkPVSEnabled == 1) {
    const CPVSVisOctree& octree = pvs->GetVisOctree();
    const CVector3f localPoint = area->GetPostConstructed()->mInverseTransform * GetTranslation();
    CPVSVisSet set = octree.GetVisSet(localPoint);
    if (set.GetState() == kVSS_NodeFound) {
      visSet = set;
    }
  }

  const float aspect = CGraphics::GetPixelAspectRatio() *
                       (static_cast< float >(mDepthTexture->GetWidth()) /
                        static_cast< float >(mDepthTexture->GetHeight()));
  const CFrustumPlanes frustum(xf, mFov.AsRadians(), aspect, 0.2f, true, mRange);
  gpRender->SetPerspective(mFov.AsDegrees(), aspect, 0.2f, mRange);
  CGX::SetNumTevStages(1);
  CGX::SetNumTexGens(1);
  CGX::SetNumIndStages(0);
  CGX::SetTevDirect(GX_TEVSTAGE0);
  CGX::SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ONE);
  CGX::SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
  CGX::SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_8_8);
  CGX::SetStandardTevColorAlphaOp(GX_TEVSTAGE0);
  CGX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
  CGX::SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, GX_IDENTITY, false, GX_PTIDENTITY);
  CGX::SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
  CGX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
  CGX::SetZMode(true, GX_LEQUAL, true);
  CCubeRenderer::That()->DrawVisibleAreaGeometry(GetCurrentAreaId().Value(), visSet, frustum,
                                                 GetOtherBounds());

  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, nearBounds, CMaterialFilter::GetPassEverything(), nullptr);
  for (AUTO(it, nearList.begin()); it != nearList.end(); ++it) {
    const CActor* actor = static_cast< const CActor* >(mgr.GetObjectById(*it));
    if (actor && actor->CanDrawStatic() && actor->GetTakesProjectedShadow()) {
      const CAABox& actorBounds = actor->GetOtherBounds();
      if (frustum.BoxInFrustumPlanes(actorBounds) && nearBounds.DoBoundsOverlap(actorBounds)) {
        const CModelData* modelData = actor->GetModelData();
        const CTransform4f modelXf =
            actor->GetTransform() * CTransform4f::Scale(modelData->GetScale());
        gpRender->SetModelMatrix(modelXf);
        modelData->PickStaticModel(CModelData::kWM_Normal)->DolphinDrawFlat(CModel::kDF_All);
      }
    }
  }

  GXSetTexCopySrc(0, 0, mDepthTexture->GetWidth(), mDepthTexture->GetHeight());
  GXSetTexCopyDst(mDepthTexture->GetWidth(), mDepthTexture->GetHeight(), GX_TF_Z16, false);
  mDepthTexture->SetFlag1(true);
  GXCopyTex(mDepthTexture->GetBitMapData(0), true);
  mDepthTexture->UnLock();
  GXPixModeSync();
  CGraphics::SetDepthRange(oldNear, oldFar);
  CGraphics::SetUseVideoFilter(useVideoFilter);
  CGraphics::SetProjectionState(oldProjection);
  gpRender->SetWorldViewpoint(oldView);
  gpRender->SetViewport(oldViewport.mLeft, oldViewport.mTop, oldViewport.mWidth,
                        oldViewport.mHeight);
  SetPreRenderClipped(!mgr.IsActorVisible(*this));
  mOuterFlicker = mRandom.Float();
  mInnerFlicker = mRandom.Float();
}

void CSplitterBeamEffect::AddToRenderer(const CStateManager& mgr) const {
  if (!GetPreRenderClipped()) {
    EnsureRendered(mgr);
  }
}

void CSplitterBeamEffect::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (GetActive()) {
    if (mExpanding) {
      mExpansion = rstl::min_val(dt * mExpandRate + mExpansion, 1.f);
    } else {
      mExpansion = rstl::max_val(0.f, mExpansion - dt * mExpandRate);
    }
    mFov = CAbsAngle::FromRadians((mMaxFov.AsRadians() - skMinFov.AsRadians()) * mExpansion +
                                  skMinFov.AsRadians());
  }
}

void CSplitterBeamEffect::Render(const CStateManager& mgr) const {
  if (mPreRenderCount < 2) {
    return;
  }

  CGraphics::SetModelMatrix(GetTransform());
  const float height = mDepthTexture->GetHeight();
  float tanHalfFov = tan(0.5f * mFov.AsRadians());
  const float step = (2.f * tanHalfFov) / (height - 1.f);
  CGX::SetTevKColor(GX_KCOLOR0,
                    CColor::Lerp(mInnerColor0, mInnerColor1, mInnerFlicker).GetGXColor());
  CGX::SetTevKColor(GX_KCOLOR1,
                    CColor::Lerp(mOuterColor0, mOuterColor1, mOuterFlicker).GetGXColor());
  CGX::SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_ZERO);
  CGX::SetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_KONST);
  CGX::SetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO);
  CGX::SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
  CGX::SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
  CGX::SetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
  CGX::SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
  CGX::SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_8_8);
  CGX::SetTevKColorSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K1);
  CGX::SetStandardTevColorAlphaOp(GX_TEVSTAGE0);
  CGX::SetStandardTevColorAlphaOp(GX_TEVSTAGE1);
  CGX::SetStandardTevColorAlphaOp(GX_TEVSTAGE2);
  CGX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
  CGX::SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD2, GX_TEXMAP1, GX_COLOR_NULL);
  CGX::SetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
  CGX::SetNumTevStages(3);
  CGX::SetNumChans(0);

  float fadeMtx[2][4] = {{0.f, 0.f, 0.f, 0.f}, {0.f, 0.f, 0.f, 0.f}};
  fadeMtx[0][1] = 1.f / mFadeLength;
  fadeMtx[0][3] = 1.f + -mRange / mFadeLength;
  const float seconds = CGraphics::GetSecondsMod900();
  const float scrollScale = 1.f / mScrollLength;
  float scrollMtx0[2][4] = {{0.f, 0.f, 0.f, 0.f}, {0.f, 0.f, 0.f, 0.f}};
  scrollMtx0[0][1] = scrollScale;
  scrollMtx0[0][3] = seconds;
  scrollMtx0[1][2] = scrollScale;
  scrollMtx0[1][3] = 0.5f * seconds;
  float scrollMtx1[2][4] = {{0.f, 0.f, 0.f, 0.f}, {0.f, 0.f, 0.f, 0.f}};
  scrollMtx1[0][2] = 1.4f * scrollScale;
  scrollMtx1[0][3] = 0.3f * seconds;
  scrollMtx1[1][1] = 1.5f * scrollScale;
  scrollMtx1[1][3] = 1.15f * seconds;
  GXLoadTexMtxImm(fadeMtx, GX_TEXMTX0, GX_MTX2x4);
  GXLoadTexMtxImm(scrollMtx0, GX_TEXMTX1, GX_MTX2x4);
  GXLoadTexMtxImm(scrollMtx1, GX_TEXMTX2, GX_MTX2x4);
  CGX::SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX0, false, GX_PTIDENTITY);
  CGX::SetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX1, false, GX_PTIDENTITY);
  CGX::SetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX2, false, GX_PTIDENTITY);
  CGX::SetNumTexGens(3);

  CTexture::InvalidateTexmap(GX_TEXMAP0);
  GXTexObj rampObj;
  GXInitTexObj(&rampObj, const_cast< uchar* >(skFadeRamp), 8, 4, GX_TF_I8, GX_CLAMP, GX_CLAMP,
               false);
  GXInitTexObjLOD(&rampObj, GX_LINEAR, GX_LINEAR, 0.f, 0.f, 0.f, true, false, GX_ANISO_1);
  GXLoadTexObj(&rampObj, GX_TEXMAP0);
  CCubeRenderer::That()->GetDarkWorldCloud().Load(GX_TEXMAP1, CTexture::kCM_Mirror);
  CGraphics::SetBlendMode(kBM_Blend, kBF_One, kBF_One, kLO_Clear);
  CGraphics::SetAlphaCompare(kAF_Always, 0, kAO_Or, kAF_Always, 0);
  gpRender->SetDepthReadWrite(true, false);
  CGraphics::SetCullMode(kCM_None);
  CGX::ResetVtxDescv();
  CGX::SetVtxDesc(GX_VA_POS, GX_DIRECT);
  CGX::Begin(GX_TRIANGLEFAN, GX_VTXFMT0, mDepthTexture->GetHeight() + 1);
  GXPosition3f32(0.f, 0.f, 0.f);
  const void* depth = mDepthTexture->GetConstBitMapData(0);
  const float range = mRange;
  const short width = mDepthTexture->GetWidth();
  for (int i = 0; i < mDepthTexture->GetHeight(); ++i) {
    const ushort z = __lhbrx(const_cast< void* >(depth), (width / 2 + i * width) * 2);
    const float dist = (0.2f * -range) / ((z / 65535.f) * (range - 0.2f) - range);
    GXPosition3f32(0.f, dist, tanHalfFov * dist);
    tanHalfFov -= step;
  }
  CGX::End();
  DCInvalidateRange(const_cast< void* >(depth),
                    (mDepthTexture->GetWidth() * mDepthTexture->GetHeight() * 2 + 31) & ~31);
  CGraphics::SetCullMode(kCM_Front);
}
