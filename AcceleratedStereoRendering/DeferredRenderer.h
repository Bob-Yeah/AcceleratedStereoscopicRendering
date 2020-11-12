/*
  Copyrighted(c) 2020, TH Köln.All rights reserved. Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met :

  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
  * Neither the name of TH Köln nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER
  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Authors: Niko Wissmann
 */

#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "StereoCameraController.h"

// Override all base texture with rainbow test texture to visualize Mip-Levels
#define _USERAINBOW 0
#define _RTR 0

#define NovelMethod 1

#define _DEBUG_LOG 1

using namespace Falcor;

class DeferredRenderer : public Renderer
{
public:

    // defines which render target is rendered next (left/right <=> 0/1)
    static uint32_t gStereoTarget;

    static float sIpd;

    static uint32_t gFrameIndex;
    void onLoad(SampleCallbacks* pSample, RenderContext* pRenderContext) override;
    void onFrameRender(SampleCallbacks* pSample, RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo) override;
    void onShutdown(SampleCallbacks* pSample) override;
    void onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height) override;
    bool onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent) override;
    void onDataReload(SampleCallbacks* pSample) override;
    void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override;

    void onClickResize();
    void resetFixedTime();

    float getZ0();

    bool sidebyside = true;

private:

    static const std::string skStartupScene;

    enum : uint32_t
    {
        RenderToScreen = 1,
        RenderToHMD
    } mRenderMode = RenderToScreen;

    enum : uint32_t
    {
        Both = 1,
        Left,
        Right
    } mOutputImage = Both;

    SampleCallbacks* mpSample;
    RenderGraph::SharedPtr mpGraph;

    StereoCameraController mCamController;
    HmdCameraController mHMDCamController;
    bool mbAltPressed = false;
    Fbo::SharedPtr mpHMDFbo;

    Fbo::SharedPtr mpLowFbo;
    float lowScaleFactor = 0.5; //0-1

    VRSystem* mpVrSystem;
    Texture::SharedPtr mpRainbowTex;

    uint32_t stereoCamIndex = 0;

    bool mUseFXAA = false;
    //std::string mLeftOutput = "ToneMapping_Left.dst";
    //debug
    std::string mLeftOutput = "Light.out";
    //std::string mLeftOutput = "GBuffer.posW";
    //std::string mRightOutput = "ToneMapping_Right.dst";
    //debug
#if NovelMethod
    std::string mRightOutput = "RightGather.out";
#else
    std::string mRightOutput = "Reprojection.out";
#endif
    bool mVRrunning = false;
    bool mUseReprojection = true;
    bool mCropOutput = false;
    bool mUseCameraPath = false;
    bool mShowRightDepth = false;

    void loadScene(SampleCallbacks* pSample, const std::string& filename);
    void updateValues();
    void initVR(Fbo* pTargetFbo);
    void applyCameraPathState();

    // Plain Stereo
    void renderToScreenSimple(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo);
    void renderToHMDSimple(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo);

    // Reprojection Pass
    void renderToScreenReprojected(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo);
    void renderToHMDReprojected(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo);

    std::string getSolutionDirectory();

    // Fixed update and helper vars for performance measurement
    void startMeasurement();
    void stopMeasurement();
    bool mUseFixedUpdate = false;
    float mFixedSpeed = 1.0f;
    double mFixedFrameTime = 0.0;
    int32_t mFrameCount = 0;
    bool mFixedRunning = true;
    int32_t mNumCyclesToRun = 5;
    int32_t mNumCycles = 0;
    bool mMeasurementRunning = false;
    int32_t mLightCount = 0;
};

