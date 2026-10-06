#ifndef _CSPLITTERBEAMEFFECT
#define _CSPLITTERBEAMEFFECT

#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Math/CAbsAngle.hpp"
#include "MetroidPrime/CActor.hpp"
#include "rstl/single_ptr.hpp"

class CTexture;

// Original class name and constructor signature from the Wii SEL exports. The beam renders the
// scene depth from its own viewpoint into a small texture and draws a fan clipped to that depth.
class CSplitterBeamEffect : public CActor {
public:
  CSplitterBeamEffect(TUniqueId uid, const CEntityInfo& info, const rstl::string& name,
                      const CTransform4f& xf, int resolution, const CAbsAngle& maxFov,
                      float range, const CColor& innerColor0, const CColor& innerColor1,
                      const CColor& outerColor0, const CColor& outerColor1, float scrollLength,
                      float fadeLength, float expandRate);
  ~CSplitterBeamEffect() override;

  // CEntity
  void Think(float dt, CStateManager& mgr) override;

  // CActor
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;

  // Guessed names.
  void SetExpanding(bool expanding) { mExpanding = expanding; }
  const CAbsAngle& GetFov() const { return mFov; }

private:
  CAbsAngle mMaxFov;
  float mRange;
  CColor mInnerColor0;
  CColor mInnerColor1;
  CColor mOuterColor0;
  CColor mOuterColor1;
  float mScrollLength;
  float mFadeLength;
  float mExpandRate;
  rstl::single_ptr< CTexture > mDepthTexture;
  CAbsAngle mFov;
  int mPreRenderCount;
  float mExpansion;
  CRandom16 mRandom;
  float mOuterFlicker;
  float mInnerFlicker;
  bool mExpanding;
};
CHECK_SIZEOF(CSplitterBeamEffect, 0x1A0)

#endif // _CSPLITTERBEAMEFFECT
