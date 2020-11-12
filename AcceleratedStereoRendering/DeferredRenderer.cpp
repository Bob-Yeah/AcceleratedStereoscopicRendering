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

#include "DeferredRenderer.h"
#include "RenderPasses/GBufferRaster.h"
#include "RenderPasses/DebugOutput.h"
#include "RenderPasses/Lighting.h"
#include "RenderPasses/Reprojection.h"
#include "RenderPasses/SimpleShadowPass.h"
#include "RenderPasses/WarpField.h"
#include "RenderPasses/BackwardWarping.h"

const std::string DeferredRenderer::skStartupScene = "Arcade/Arcade.fscene";
//const std::string DeferredRenderer::skStartupScene = "SimpleScene/simple.fscene";

const glm::vec4 skClearColor = vec4(1.0f, 0, 0, 1.f);
const bool initOpenVR = false; 

uint32_t DeferredRenderer::gStereoTarget = 0;
float DeferredRenderer::sIpd = 0.062;

void DeferredRenderer::onLoad(SampleCallbacks * pSample, RenderContext * pRenderContext)
{
    if (gpDevice->isFeatureSupported(Device::SupportedFeatures::Raytracing) == false)
    {
        logErrorAndExit("Device does not support raytracing!", true);
    }

    mpGraph = RenderGraph::create("Hybrid Stereo Renderer");

    // G-Buffer
    mpGraph->addPass(GBufferRaster::create(), "GBuffer");

    // Simple Shadow Pre-Pass
    SimpleShadowPass::SharedPtr pShadowPass = SimpleShadowPass::create();
    pShadowPass->mpMainRenderObject = this;
    mpGraph->addPass(pShadowPass, "SimpleShadowPass");

    // Main Lighting/Shading Pass
    Lighting::SharedPtr pLightPass = Lighting::create();
    pLightPass->mpLightCamera = pShadowPass->mpLightCamera;
    mpGraph->addPass(pLightPass, "Light");

    // Warp Field
    WarpField::SharedPtr pWarpFieldPass = WarpField::create();
    mpGraph->addPass(pWarpFieldPass, "WarpField");

    // Backward Warping
    BackwardWarping::SharedPtr pBackwardWarpingPass = BackwardWarping::create();
    mpGraph->addPass(pBackwardWarpingPass, "BackwardWarping");

    // Reprojection Pass (Raster and Ray Trace)
    Reprojection::SharedPtr pReprojPass = Reprojection::create();
    pReprojPass->mpMainRenderObject = this;
    pReprojPass->mpLightPass = pLightPass;
    mpGraph->addPass(pReprojPass, "Reprojection");

    // FXAA Pass Left
    FXAA::SharedPtr fxaaPassLeft = FXAA::create();
    mpGraph->addPass(fxaaPassLeft, "FXAA_Left");

    // FXAA Pass Right
    FXAA::SharedPtr fxaaPassRight = FXAA::create();
    mpGraph->addPass(fxaaPassRight, "FXAA_Right");

    // Tone Mapping
    ToneMapping::SharedPtr toneMapping = ToneMapping::create();
    mpGraph->addPass(toneMapping, "ToneMapping_Left");

    ToneMapping::SharedPtr toneMapping_right = ToneMapping::create();
    mpGraph->addPass(toneMapping_right, "ToneMapping_Right");

    // Former G-Buffer Debug Pass
    //DebugOutput::SharedPtr debugPass = DebugOutput::create();
    //debugPass->root = this;
    //mpGraph->addPass(debugPass, "DebugOutput");
    //mpGraph->addEdge("GBuffer.posW", "DebugOutput.posW");
    //mpGraph->addEdge("GBuffer.normW", "DebugOutput.normW");
    //mpGraph->addEdge("GBuffer.diffuseOpacity", "DebugOutput.diffuseOpacity");
    //mpGraph->addEdge("GBuffer.specRough", "DebugOutput.specRough");
    //mpGraph->addEdge("GBuffer.depthStencil", "DebugOutput.depth");

    // Links for Lighting Pass
    mpGraph->addEdge("GBuffer.posW", "Light.posW");
    mpGraph->addEdge("GBuffer.normW", "Light.normW");
    mpGraph->addEdge("GBuffer.diffuseOpacity", "Light.diffuseOpacity");
    mpGraph->addEdge("GBuffer.specRough", "Light.specRough");
    mpGraph->addEdge("SimpleShadowPass.depthStencil", "Light.shadowDepth");

    // Links for Reprojection Pass
    mpGraph->addEdge("GBuffer.depthStencil", "Reprojection.depth");
    mpGraph->addEdge("GBuffer.normW", "Reprojection.gbufferNormal");
    mpGraph->addEdge("GBuffer.posW", "Reprojection.gbufferPosition");
    mpGraph->addEdge("Light.out", "Reprojection.leftIn");
    mpGraph->addEdge("SimpleShadowPass.depthStencil", "Reprojection.shadowDepth");

    // FXAA Links (left and right)
    mpGraph->addEdge("Light.out", "FXAA_Left.src");
    mpGraph->addEdge("Reprojection.out", "FXAA_Right.src");

    // Tone Mapping
    mpGraph->addEdge("Light.out", "ToneMapping_Left.src");
    mpGraph->addEdge("Reprojection.out", "ToneMapping_Right.src");

    // Warp Field
    mpGraph->addEdge("GBuffer.depthStencil", "WarpField.depth");

    // Backward Warping
    mpGraph->addEdge("WarpField.WF", "BackwardWarping.Vs");
    mpGraph->addEdge("Light.out", "BackwardWarping.leftIn");

    //mpGraph->markOutput("DebugOutput.out");

    //mpGraph->markOutput("ToneMapping_Left.dst");
    mpGraph->markOutput("Light.out");


    if (mUseReprojection)
    {
        //mpGraph->markOutput("ToneMapping_Right.dst");
        mpGraph->markOutput("Reprojection.out");
    }

    if (mUseWarpField)
    {
        mpGraph->markOutput("BackwardWarping.out");
    }

    if (mUseFXAA)
    {
        mpGraph->markOutput("FXAA_Left.dst");
        mpGraph->markOutput("FXAA_Right.dst");
    }

    mpSample = pSample;
    mpGraph->onResize(pSample->getCurrentFbo().get());

    {
#if _USERAINBOW
        mpRainbowTex = createTextureFromFile(getSolutionDirectory() + "TestData\\rainbow_tex.dds", false, true, Resource::BindFlags::ShaderResource);
#endif // _USERAINBOW

        assert(mpGraph != nullptr);
        std::string filename;
        /*if (openFileDialog(Scene::kFileExtensionFilters, filename))
        {
            loadScene(pSample, filename);
        }*/
        loadScene(pSample, skStartupScene);
    }
}

void DeferredRenderer::onFrameRender(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo)
{
    if (mMeasurementRunning)
    {
        if (mFrameCount >= _PROFILING_LOG_BATCH_SIZE)
        {
            resetFixedTime();
            mNumCycles++;
        }
        if (mNumCycles > mNumCyclesToRun)
        {
            stopMeasurement();
        }
    }

    // Render mode switch - Screen or HMD
    switch (mRenderMode)
    {
    case DeferredRenderer::RenderToScreen:
        updateValues();
        pRenderContext->clearFbo(pTargetFbo.get(), glm::vec4(0), 1.0f, 0, FboAttachmentType::Color);
        if (mpGraph->getScene() != nullptr)
        {
            gStereoTarget = 0;
            mCamController.update();

            if (mUseFixedUpdate)
            {
                mpGraph->getScene()->update(mFixedFrameTime);
                if (mFixedRunning)
                {
                    mFrameCount++;
                }
                mFixedFrameTime = mFrameCount * (double)mFixedSpeed;
            }
            else
            {
                mpGraph->getScene()->update(pSample->getCurrentTime());
            }

            mpGraph->execute(pRenderContext);

            if (mUseReprojection)
            {
                renderToScreenReprojected(pSample, pRenderContext, pTargetFbo);
            }
            else
            {
                renderToScreenSimple(pSample, pRenderContext, pTargetFbo);
            }
        }
        break;
    case DeferredRenderer::RenderToHMD:
        if (mUseReprojection)
        {
            renderToHMDReprojected(pSample, pRenderContext, pTargetFbo);
        }
        else
        {
            renderToHMDSimple(pSample, pRenderContext, pTargetFbo);
        }
        break;
    default:
        break;
    }
}

void DeferredRenderer::renderToScreenSimple(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo)
{
    if (sidebyside)
    {
        switch (mOutputImage)
        {
        case DeferredRenderer::Both:
        {
            uvec4 rectSrc = uvec4(pSample->getCurrentFbo()->getWidth() / 4, 0, pSample->getCurrentFbo()->getWidth() * 0.75f, pSample->getCurrentFbo()->getHeight());
            uvec4 leftRectDst = uvec4(0, 0, pSample->getCurrentFbo()->getWidth() / 2, pSample->getCurrentFbo()->getHeight());
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0), mCropOutput ? rectSrc : glm::uvec4(-1), leftRectDst);

            gStereoTarget = 1;
            mpGraph->execute(pRenderContext);
            uvec4 rightRect = uvec4(pSample->getCurrentFbo()->getWidth() / 2, 0, pSample->getCurrentFbo()->getWidth() / 2 + pSample->getCurrentFbo()->getWidth() / 2, pSample->getCurrentFbo()->getHeight());
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0), mCropOutput ? rectSrc : glm::uvec4(-1), rightRect);
        }
        break;
        case DeferredRenderer::Left:
        {
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
            gStereoTarget = 1;
            mpGraph->execute(pRenderContext);
        }
        break;
        case DeferredRenderer::Right:
        {
            gStereoTarget = 1;
            mpGraph->execute(pRenderContext);
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
        }
        break;
        }
    }
    else
    {
        pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void DeferredRenderer::renderToHMDSimple(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo)
{
    if (!mVRrunning)
    {
        initVR(pSample->getCurrentFbo().get());
    }

    VRSystem::instance()->refresh();
    updateValues();

    pRenderContext->clearFbo(mpHMDFbo.get(), skClearColor, 1.0f, 0, FboAttachmentType::Color);
    pRenderContext->clearFbo(pTargetFbo.get(), glm::vec4(0), 1.0f, 0, FboAttachmentType::Color);

    if (mpGraph->getScene() != nullptr)
    {
        pRenderContext->getGraphicsState()->setFbo(mpHMDFbo);

        gStereoTarget = 0;

        mHMDCamController.update();
        mpGraph->getScene()->update(pSample->getCurrentTime());
        mpGraph->execute(pRenderContext);

        uvec4 rectSrc = uvec4(pSample->getCurrentFbo()->getWidth() / 4, 0, pSample->getCurrentFbo()->getWidth() * 0.75f, pSample->getCurrentFbo()->getHeight());
        pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), mpHMDFbo->getRenderTargetView(0));

        gStereoTarget = 1;
        mpGraph->execute(pRenderContext);

        pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), mpHMDFbo->getRenderTargetView(1));

        mpVrSystem->submit(VRDisplay::Eye::Left, mpHMDFbo->getColorTexture(0), pRenderContext);
        mpVrSystem->submit(VRDisplay::Eye::Right, mpHMDFbo->getColorTexture(1), pRenderContext);
        uvec4 rectTarget = uvec4(0, 0, mpHMDFbo->getWidth(), mpHMDFbo->getHeight());
        pRenderContext->blit(mpHMDFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0), glm::uvec4(-1), rectTarget);
    }
}

void DeferredRenderer::renderToScreenReprojected(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo)
{
    if (sidebyside)
    {
        switch (mOutputImage)
        {
        case DeferredRenderer::Both:
        {
            uvec4 rectSrc = uvec4(pSample->getCurrentFbo()->getWidth() / 4, 0, pSample->getCurrentFbo()->getWidth() * 0.75f, pSample->getCurrentFbo()->getHeight());
            uvec4 leftRectDst = uvec4(0, 0, pSample->getCurrentFbo()->getWidth() / 2, pSample->getCurrentFbo()->getHeight());
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0), mCropOutput ? rectSrc : glm::uvec4(-1), leftRectDst);

            uvec4 rightRect = uvec4(pSample->getCurrentFbo()->getWidth() / 2, 0, pSample->getCurrentFbo()->getWidth() / 2 + pSample->getCurrentFbo()->getWidth() / 2, pSample->getCurrentFbo()->getHeight());
            pRenderContext->blit(mpGraph->getOutput(mRightOutput)->getSRV(), pTargetFbo->getRenderTargetView(0), mCropOutput ? rectSrc : glm::uvec4(-1), rightRect);
        }
        break;
        case DeferredRenderer::Left:
        {
            pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
        }
        break;
        case DeferredRenderer::Right:
        {
            pRenderContext->blit(mpGraph->getOutput(mRightOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
        }
        break;
        }
    }
    else
    {
        pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void DeferredRenderer::renderToHMDReprojected(SampleCallbacks * pSample, RenderContext * pRenderContext, const Fbo::SharedPtr & pTargetFbo)
{
    if (!mVRrunning)
    {
        initVR(pSample->getCurrentFbo().get());
    }

    VRSystem::instance()->refresh();
    updateValues();

    pRenderContext->clearFbo(mpHMDFbo.get(), skClearColor, 1.0f, 0, FboAttachmentType::Color);
    pRenderContext->clearFbo(pTargetFbo.get(), glm::vec4(0), 1.0f, 0, FboAttachmentType::Color);

    if (mpGraph->getScene() != nullptr)
    {
        pRenderContext->getGraphicsState()->setFbo(mpHMDFbo);

        gStereoTarget = 0;

        mHMDCamController.update();
        mpGraph->getScene()->update(pSample->getCurrentTime());
        mpGraph->execute(pRenderContext);

        uvec4 rectSrc = uvec4(pSample->getCurrentFbo()->getWidth() / 4, 0, pSample->getCurrentFbo()->getWidth() * 0.75f, pSample->getCurrentFbo()->getHeight());
        pRenderContext->blit(mpGraph->getOutput(mLeftOutput)->getSRV(), mpHMDFbo->getRenderTargetView(0));

        pRenderContext->blit(mpGraph->getOutput(mRightOutput)->getSRV(), mpHMDFbo->getRenderTargetView(1));

        mpVrSystem->submit(VRDisplay::Eye::Left, mpHMDFbo->getColorTexture(0), pRenderContext);
        mpVrSystem->submit(VRDisplay::Eye::Right, mpHMDFbo->getColorTexture(1), pRenderContext);
        uvec4 rectTarget = uvec4(0, 0, mpHMDFbo->getWidth(), mpHMDFbo->getHeight());
        pRenderContext->blit(mpHMDFbo->getColorTexture(1)->getSRV(), pTargetFbo->getRenderTargetView(0), glm::uvec4(-1), rectTarget);
    }
}

void DeferredRenderer::onShutdown(SampleCallbacks * pSample)
{
}

void DeferredRenderer::onResizeSwapChain(SampleCallbacks * pSample, uint32_t width, uint32_t height)
{
    if (mpGraph)
    {
        switch (mRenderMode)
        {
        case DeferredRenderer::RenderToScreen:
            if (mpGraph->getScene() != nullptr)
                mpGraph->getScene()->setCamerasAspectRatio((float)width / (float)height);
            mpGraph->onResize(pSample->getCurrentFbo().get());
            break;
        case DeferredRenderer::RenderToHMD:
            initVR(pSample->getCurrentFbo().get());
            break;
        default:
            break;
        }
    }
}

bool DeferredRenderer::onKeyEvent(SampleCallbacks * pSample, const KeyboardEvent & keyEvent)
{
    bool handled = false;
    if (mpGraph->getScene() != nullptr) handled = mpGraph->onKeyEvent(keyEvent);

    if (!keyEvent.mods.isAltDown) // disable if Alt is pressed
    {
        mbAltPressed = false;
        switch (mRenderMode)
        {
        case DeferredRenderer::RenderToScreen:
            handled ? true : mCamController.onKeyEvent(keyEvent);
        case DeferredRenderer::RenderToHMD:
            handled ? true : mHMDCamController.onKeyEvent(keyEvent);
        }
    }
    else
    {
        mbAltPressed = true;
    }

    return handled;
}

bool DeferredRenderer::onMouseEvent(SampleCallbacks * pSample, const MouseEvent & mouseEvent)
{
    bool handled = false;
    if (mpGraph->getScene() != nullptr) handled = mpGraph->onMouseEvent(mouseEvent);

    if (!mbAltPressed)
    {
        switch (mRenderMode)
        {
        case DeferredRenderer::RenderToScreen:
            return handled ? true : mCamController.onMouseEvent(mouseEvent);
        case DeferredRenderer::RenderToHMD:
            return handled ? true : mHMDCamController.onMouseEvent(mouseEvent);
        default:
            return handled;
        }
    }
    return handled;
}

void DeferredRenderer::onDataReload(SampleCallbacks * pSample)
{
    onClickResize();
}

void DeferredRenderer::onGuiRender(SampleCallbacks * pSample, Gui * pGui)
{
    pGui->setCurrentWindowSize((uint32_t)pGui->getCurrentWindowSize().x, 1000);

    if (pGui->addButton("Load Scene"))
    {
        assert(mpGraph != nullptr);
        std::string filename;
        if (openFileDialog(Scene::kFileExtensionFilters, filename))
        {
            loadScene(pSample, filename);
        }
    }

    //pGui->addIntVar("Light Count", mLightCount);

    if (pGui->addCheckBox("Use Camera Path", mUseCameraPath))
    {
        applyCameraPathState();
    }

    if (mRenderMode == RenderToScreen) {
        pGui->addCheckBox("Use Fixed Update", mUseFixedUpdate);
        if (mUseFixedUpdate)
        {
            pGui->addIntVar("Frame Count", mFrameCount, 0);
            pGui->addFloatVar("Fixed Speed", mFixedSpeed, 0.0f, 2.f);
            float fixedFrameTime = (float)mFixedFrameTime;
            pGui->addFloatVar("Current Fixed Time", fixedFrameTime);

            if (pGui->addButton("Reset"))
            {
                resetFixedTime();
            }
            if (pGui->addButton("Stop", true))
            {
                mFixedRunning = false;
            }
            if (pGui->addButton("Start", true))
            {
                mFixedRunning = true;
            }

            std::string sampleText = std::to_string(_PROFILING_LOG_BATCH_SIZE) + " Samples";
            pGui->addText(sampleText.c_str());
            pGui->addIntVar("Measure Cycles", mNumCyclesToRun);

            if (pGui->addButton("Start Measurement"))
            {
                startMeasurement();
            }

            if (pGui->addButton("Stop Measurement", true))
            {
                stopMeasurement();
            }
        }
    }

    pGui->addSeparator();

    if(pGui->addCheckBox("Reprojection Pass", mUseReprojection))
    {
        if (mUseReprojection)
        {
            mpGraph->markOutput("Reprojection.out");
        }
        else
        {
            mpGraph->unmarkOutput("Reprojection.out");
        }

        onClickResize();
    }

    if (pGui->addCheckBox("WarpField Pass", mUseWarpField))
    {
        if (mUseWarpField)
        {
            mpGraph->markOutput("BackwardWarping.out");
        }
        else
        {
            mpGraph->unmarkOutput("BackwardWarping.out");
        }
        mRightOutput = mUseWarpField ? "BackwardWarping.out" : "Reprojection.out";
        onClickResize();
    }

    Gui::DropdownList renderModeList;
    renderModeList.push_back({ 1, "Render To Screen" });
    if (initOpenVR) renderModeList.push_back({ 2, "Render To HMD" });
    pGui->addDropdown("Render Mode", renderModeList, (uint32_t&)mRenderMode);

    switch (mRenderMode)
    {
    case DeferredRenderer::RenderToScreen:
        pGui->addCheckBox("Side-by-Side Stereo", sidebyside);
        if (sidebyside)
        {
            Gui::DropdownList outputImageList;
            outputImageList.push_back({ 1, "Both" });
            outputImageList.push_back({ 2, "Left" });
            outputImageList.push_back({ 3, "Right" });
            pGui->addDropdown("Output Image", outputImageList, (uint32_t&)mOutputImage);

            pGui->addCheckBox("Crop Output", mCropOutput);
            pGui->addFloatSlider("IPD", mCamController.ipd, 0.05f, 0.08f);
            pGui->addFloatSlider("Z0", mCamController.z0, 0.f, 20.f);
            sIpd = mCamController.ipd;
        }
        break;
    case DeferredRenderer::RenderToHMD:
        break;
    default:
        break;
    }


    if (pGui->addCheckBox("Use FXAA", mUseFXAA))
    {
        if (mUseFXAA)
        {
            mpGraph->markOutput("FXAA_Left.dst");
            mpGraph->markOutput("FXAA_Right.dst");
        }
        else
        {
            mpGraph->unmarkOutput("FXAA_Left.dst");
            mpGraph->unmarkOutput("FXAA_Right.dst");
        }

        mLeftOutput = mUseFXAA ? "FXAA_Left.dst" : "Light.out";
        mRightOutput = mUseFXAA ? "FXAA_Right.dst" : "Reprojection.out";

        onClickResize();
    }

    if (pGui->addButton("4K Resolution"))
    {
        Fbo::Desc fboDesc;
        fboDesc.
            setColorTarget(0, mpSample->getCurrentFbo()->getColorTexture(0)->getFormat()).
            setDepthStencilTarget(mpSample->getCurrentFbo()->getDepthStencilTexture()->getFormat());

        mpHMDFbo = FboHelper::create2D(3840, 2160, fboDesc);
        mpGraph->onResize(mpHMDFbo.get());
    }

    pGui->addSeparator();

    if (mpGraph != nullptr)
    {
        mpGraph->renderUI(pGui, nullptr);
    }
}

void DeferredRenderer::onClickResize()
{
    mpGraph->onResize(mpSample->getCurrentFbo().get());
}

void DeferredRenderer::loadScene(SampleCallbacks* pSample, const std::string& filename)
{
    ProgressBar::SharedPtr pBar = ProgressBar::create("Loading Scene", 100);
    
#if _RTR
    RtScene::SharedPtr pScene = RtScene::loadFromFile(filename, RtBuildFlags::FastTrace, Model::LoadFlags::None, Scene::LoadFlags::None);
#else
    Scene::SharedPtr pScene = Scene::loadFromFile(filename, Model::LoadFlags::None, Scene::LoadFlags::None);
#endif
    if (pScene != nullptr)
    {
#if _USERAINBOW
        Material::SharedPtr rainbowMat = Material::create("rainbow");
        rainbowMat->setBaseColorTexture(mpRainbowTex);
        for (uint32_t i = 0; i < pScene->getModelCount(); i++)
        {
            auto& model = pScene->getModel(i);
            for (uint32_t j = 0; j < model->getMeshCount(); j++)
            {
                auto& mesh = model->getMesh(j);
                mesh->setMaterial(rainbowMat);
            }
        }
#endif // _USERAINBOW

        Fbo::SharedPtr pFbo = pSample->getCurrentFbo();
        pScene->setCamerasAspectRatio(float(pFbo->getWidth()) / float(pFbo->getHeight()));
        mpGraph->setScene(pScene);

        mCamController.attachCamera(mpGraph->getScene()->getActiveCamera());
        stereoCamIndex = mpGraph->getScene()->addCamera(mCamController.getStereoCamera());

        applyCameraPathState();

        //mLightCount = pScene->getLightCount();
    }
}

void DeferredRenderer::updateValues()
{
    switch (mRenderMode)
    {
    case DeferredRenderer::RenderToScreen:
        mpGraph->getScene()->setActiveCamera(stereoCamIndex);
        mHMDCamController.~HmdCameraController();
        break;
    case DeferredRenderer::RenderToHMD:
        mHMDCamController.attachCamera(mpGraph->getScene()->getActiveCamera());
        break;
    default:
        break;
    }
}

void DeferredRenderer::initVR(Fbo * pTargetFbo)
{
    if (VRSystem::instance())
    {
        mpVrSystem = VRSystem::instance();
        VRDisplay* pDisplay = mpVrSystem->getHMD().get();
        glm::ivec2 renderSize = pDisplay->getRecommendedRenderSize();

        Fbo::Desc fboDesc;
        fboDesc.
            setColorTarget(0, pTargetFbo->getColorTexture(0)->getFormat()).
            setColorTarget(1, pTargetFbo->getColorTexture(0)->getFormat()).
            setDepthStencilTarget(pTargetFbo->getDepthStencilTexture()->getFormat());

        mpHMDFbo = FboHelper::create2D(renderSize.x, renderSize.y, fboDesc);

        mpGraph->onResize(mpHMDFbo.get());

        mVRrunning = true;
    }
    else
    {
        msgBox("Can't initialize the VR system. Make sure that your HMD is connected properly");
    }
}

void DeferredRenderer::applyCameraPathState()
{
    if (mpGraph->getScene()->getPathCount())
    {
        if (mUseCameraPath)
        {
            mpGraph->getScene()->getPath(0)->attachObject(mpGraph->getScene()->getCamera(0));
        }
        else
        {
            mpGraph->getScene()->getPath(0)->detachObject(mpGraph->getScene()->getCamera(0));
        }
    }
}

float DeferredRenderer::getZ0()
{
    return mCamController.z0;
}

std::string DeferredRenderer::getSolutionDirectory()
{
    char buf[256];
    GetCurrentDirectoryA(256, buf);
    std::string projectPath = std::string(buf);
    std::string solutionPath = projectPath.substr(0, projectPath.find_last_of("/ \\"));
    return solutionPath + "\\";
}

void DeferredRenderer::resetFixedTime()
{
    mFrameCount = 0;
    mFixedFrameTime = 0.0;
}

void DeferredRenderer::startMeasurement()
{
    resetFixedTime();
    mFixedRunning = true;
    mMeasurementRunning = true;
    gProfileEnabled = true;
}

void DeferredRenderer::stopMeasurement()
{
    resetFixedTime();
    mFixedRunning = false;
    mMeasurementRunning = false;
    mNumCycles = 0;
    gProfileEnabled = false;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    logInfo("begin.");
    DeferredRenderer::UniquePtr pRenderer = std::make_unique<DeferredRenderer>();
    SampleConfig config;
    config.windowDesc.title = "Hybrid Stereo Renderer";

    config.windowDesc.width = 1920;
    config.windowDesc.height = 1080;

    config.windowDesc.resizableWindow = false;
    config.deviceDesc.enableVR = initOpenVR;
    logInfo("run.");
    Sample::run(config, pRenderer);
    logInfo("run End.");
    return 0;
}
