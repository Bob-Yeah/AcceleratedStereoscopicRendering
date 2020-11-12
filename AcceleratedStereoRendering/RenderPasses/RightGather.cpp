// In: Left Color Texture
// In: Left Depth
// Out: Right Color Texture

#include "RightGather.h"
#include "glm/gtx/string_cast.hpp"

struct layoutsData
{
    uint32_t pos;
    std::string name;
    ResourceFormat format;
};

static const layoutsData kLayoutData[VERTEX_LOCATION_COUNT] =
{
    { VERTEX_POSITION_LOC,      VERTEX_POSITION_NAME,       ResourceFormat::RGB32Float },
    { VERTEX_NORMAL_LOC,        VERTEX_NORMAL_NAME,         ResourceFormat::RGB32Float },
    { VERTEX_BITANGENT_LOC,     VERTEX_BITANGENT_NAME,      ResourceFormat::RGB32Float },
    { VERTEX_TEXCOORD_LOC,      VERTEX_TEXCOORD_NAME,       ResourceFormat::RGB32Float }, //for some reason this is rgb
    { VERTEX_LIGHTMAP_UV_LOC,   VERTEX_LIGHTMAP_UV_NAME,    ResourceFormat::RGB32Float }, //for some reason this is rgb
    { VERTEX_BONE_WEIGHT_LOC,   VERTEX_BONE_WEIGHT_NAME,    ResourceFormat::RGBA32Float},
    { VERTEX_BONE_ID_LOC,       VERTEX_BONE_ID_NAME,        ResourceFormat::RGBA8Uint  },
    { VERTEX_DIFFUSE_COLOR_LOC, VERTEX_DIFFUSE_COLOR_NAME,  ResourceFormat::RGBA32Float},
    { VERTEX_QUADID_LOC,        VERTEX_QUADID_NAME,         ResourceFormat::R32Float   },
};

size_t RightGather::sLightArrayOffset = ConstantBuffer::kInvalidOffset;
size_t RightGather::sLightCountOffset = ConstantBuffer::kInvalidOffset;
size_t RightGather::sCameraDataOffset = ConstantBuffer::kInvalidOffset;

RightGather::SharedPtr RightGather::create(const Dictionary& params)
{
    RightGather::SharedPtr ptr(new RightGather());
    return ptr;
}

RenderPassReflection RightGather::reflect() const
{
    RenderPassReflection r;
    r.addInput("leftDepth", "");
    //r.addInput("rightDepth", "");
    r.addInput("leftIn", "");
    r.addInput("shadowDepth", "");
    r.addInternal("depthStencil", "").format(ResourceFormat::D32FloatS8X24).bindFlags(Resource::BindFlags::DepthStencil);
    r.addOutput("out", "").format(ResourceFormat::RGBA32Float).bindFlags(Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    return r;
}

void RightGather::initialize(const RenderData* pRenderData)
{
    // Right Gather Program

    mpState = GraphicsState::create();

    GraphicsProgram::Desc preDepthProgDesc;
    preDepthProgDesc
        .addShaderLibrary("DepthInVS.slang").vsEntry("main")
        .addShaderLibrary("RightGather.slang").psEntry("main");

    mpProgram = GraphicsProgram::create(preDepthProgDesc);

    // Cull Mode
    RasterizerState::Desc RasterStateDesc;
    RasterStateDesc.setCullMode(RasterizerState::CullMode::Back);
    RasterizerState::SharedPtr RasterState = RasterizerState::create(RasterStateDesc);
    mpState->setRasterizerState(RasterState);

    //mpPass = FullScreenPass::create("RightGather.slang",
    //    Program::DefineList(),false);
    //mpVars = GraphicsVars::create(mpPass->getProgram()->getReflector());
    mpVars = GraphicsVars::create(mpProgram->getReflector());

    //Fbo::Desc FboDesc;
    //FboDesc.
    //    setColorTarget(0, Falcor::ResourceFormat::RGBA32Float).
    //    setColorTarget(1, Falcor::ResourceFormat::RGBA32Float).
    //    setColorTarget(2, Falcor::ResourceFormat::RGBA32Float).
    //    setColorTarget(3, Falcor::ResourceFormat::RGBA32Float).
    //    setColorTarget(4, Falcor::ResourceFormat::RGBA32Float);
    ////mpReRasterFbo = FboHelper::create2D(width/2, height/2, reRasterFboDesc);
    //mpFbo = FboHelper::create2D(pRenderData->getTexture("out")->getWidth(),
    //    pRenderData->getTexture("out")->getHeight(), FboDesc);

    mpFbo = Fbo::create();
    //mpLowFbo = Fbo::create();

    // Sampler
    Sampler::Desc samplerDesc; 
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);

    // Unused
    Sampler::Desc samplerDepthDesc;
    samplerDesc.setLodParams(0.f, 0.f, 0.f);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::GreaterEqual);
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearComparisonSampler = Sampler::create(samplerDepthDesc);
    // Unused

    pLowDisTex = Texture::create2D(pRenderData->getTexture("out")->getWidth() / 2, pRenderData->getTexture("out")->getHeight() / 2, pRenderData->getTexture("out")->getFormat(),
        1, 1, (const void*)nullptr,
        Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    //
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthTest(true);
    dsDesc.setDepthFunc(DepthStencilState::Func::Less);
    //dsDesc.setStencilTest(true);
    //dsDesc.setStencilWriteMask(1);
    ////dsDesc.setStencilReadMask(1);
    //dsDesc.setStencilRef(1);
    //// ref & readmask (f) val & readmask
    //// disocclusion: ref = 0 good: ref = 1
    ////Replace: disocclusion ref = 0 ; ref = 1
    //dsDesc.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Always);
    ////stencil fail; stencil pass, depth fail; stencil pass, depth pass.
    //dsDesc.setStencilOp(DepthStencilState::Face::Front, DepthStencilState::StencilOp::Keep, DepthStencilState::StencilOp::Replace, DepthStencilState::StencilOp::Keep);
    DepthStencilState::SharedPtr mpDepthTestDS = DepthStencilState::create(dsDesc);
    mpState->setDepthStencilState(mpDepthTestDS);
    mpState->setProgram(mpProgram);

    //Re -GBuffer -------- 根据Depth设置Stencil
    //mpReRasterFbo = Fbo::create();
    GraphicsProgram::Desc reRasterProgDesc;
    reRasterProgDesc
        .addShaderLibrary("ReRasterStereoVS.slang").vsEntry("main")
        .addShaderLibrary("RasterPrimarySimple.slang").psEntry("main");

    mpReRasterProgram = GraphicsProgram::create(reRasterProgDesc);
    mpReRasterVars = GraphicsVars::create(mpReRasterProgram->getReflector());
    mpReRasterGraphicsState = GraphicsState::create();
    mpReRasterGraphicsState->setProgram(mpReRasterProgram);

    // Cull Mode
    RasterizerState::Desc reRasterStateDesc;
    reRasterStateDesc.setCullMode(RasterizerState::CullMode::Back);
    RasterizerState::SharedPtr reRasterState = RasterizerState::create(reRasterStateDesc);
    mpReRasterGraphicsState->setRasterizerState(reRasterState);

    // Depth Stencil State
    DepthStencilState::Desc reDepthStencilState;
    reDepthStencilState.setDepthTest(true);
    reDepthStencilState.setStencilTest(true);
    reDepthStencilState.setStencilReadMask(1); // readmask
    reDepthStencilState.setStencilWriteMask(1); // writemask
    reDepthStencilState.setStencilRef(1); // ref
    //// ref 1 & readMask (stencil func-Equal) value(already in the stencil buffer 0) & readmask 
    //// never always equal le gr notequal ge 
    reDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::GreaterEqual);
    //// stencil op:
    //// stencil fail:Keep
    //// stencil pass and depth fail:Keep
    //// stencil pass and depth pass:Increase
    reDepthStencilState.setStencilOp(DepthStencilState::Face::Front, DepthStencilState::StencilOp::Keep, DepthStencilState::StencilOp::Keep, DepthStencilState::StencilOp::Increase);
    DepthStencilState::SharedPtr reRasterDepthStencil = DepthStencilState::create(reDepthStencilState);
    mpReRasterGraphicsState->setDepthStencilState(reRasterDepthStencil);
    ///////////////////////////////////////////////////////////////////////////////////////

    ////Re -Lighting
    mpReRasterLightingState = GraphicsState::create();
    mpReRasterLightingPass = FullScreenPass::create("Lighting.slang");
    mpReRasterLightingVars = GraphicsVars::create(mpReRasterLightingPass->getProgram()->getReflector());
    mpReRasterLightingFbo = Fbo::create();
    // Depth Stencil State
    DepthStencilState::Desc reLightingDepthStencilState;
    reLightingDepthStencilState.setDepthTest(false);
    reLightingDepthStencilState.setStencilTest(true);
    reLightingDepthStencilState.setStencilReadMask(1);
    // ref 1 & readMask (stencil func-Equal) value(already in the stencil buffer 0,1) & readmask 
    reLightingDepthStencilState.setStencilRef(1);
    reLightingDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Always);
    DepthStencilState::SharedPtr reRasterLightingDepthStencil = DepthStencilState::create(reLightingDepthStencilState);
    mpReRasterLightingState->setDepthStencilState(reRasterLightingDepthStencil);

    mIsInitialized = true;
}

void RightGather::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    if (mpSceneRenderer == nullptr)
    {
        return;
    }
    // On first execution, run some initialization
    if (!mIsInitialized) initialize(pRenderData);

    //pContext->pushGraphicsState(mpState);
    //pContext->clearFbo(mpMainFbo.get(), glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), 1, 0, FboAttachmentType::All);
    //pContext->clearFbo(mpPostProcessFbo.get(), glm::vec4(), 1, 0, FboAttachmentType::Color);

    // Right Gather Program ##########################
    Profiler::startEvent("Right Gather");
    const auto& pDisTex = pRenderData->getTexture("out");

    //mpFbo->attachColorTarget(pLowDisTex, 0);
    mpFbo->attachColorTarget(pDisTex, 0);
    mpFbo->attachDepthStencilTarget(pRenderData->getTexture("depthStencil"));
    pContext->clearFbo(mpFbo.get(), glm::vec4(mClearColor, 0), 1.0f, 0, FboAttachmentType::All);

    mpVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpVars["PerFrameCB"]["gInvRightEyeView"] = glm::inverse(mpScene->getActiveCamera()->getRightEyeViewMatrix());
    mpVars["PerFrameCB"]["gInvRightEyeProj"] = glm::inverse(mpScene->getActiveCamera()->getRightEyeProjMatrix());
    mpVars["PerFrameCB"]["gLeftEyeViewProj"] = mpScene->getActiveCamera()->getViewProjMatrix();

    //logWarning("projectionMatRight:" + glm::to_string(mpScene->getActiveCamera()->getRightEyeProjMatrix()));
    //logWarning("getRightEyeViewMatrix:" + glm::to_string(mpScene->getActiveCamera()->getRightEyeViewMatrix()));
    //logWarning("RightEyeViewProjMatrix:" + glm::to_string(mpScene->getActiveCamera()->getRightEyeViewProjMatrix()));
    //logWarning("LeftEyeViewProjMatrix:" + glm::to_string(mpScene->getActiveCamera()->getViewProjMatrix()));

    mpVars->setTexture("gLeftEyeTex", pRenderData->getTexture("leftIn"));
    mpVars->setSampler("gLinearSampler", mpLinearSampler);
    //mpVars->setTexture("gRightEyeDepthTex", pRenderData->getTexture("rightDepth"));
    mpVars->setTexture("gLeftEyeDepthTex", pRenderData->getTexture("leftDepth"));

    mpState->setFbo(mpFbo);
    //pContext->pushGraphicsState(mpState);
    //pContext->pushGraphicsVars(mpVars);
    //mpPass->execute(pContext);
    //pContext->popGraphicsState();
    //pContext->popGraphicsVars();
    pContext->setGraphicsState(mpState);
    pContext->setGraphicsVars(mpVars);
    mpSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());
    Profiler::endEvent("Right Gather");
    //pDisTex :: Warped Image

    /////---------------------Re Rasteration---------------------///
    //mpReRasterFbo->attachColorTarget(pDisTex, 2);
    Profiler::startEvent("Right ReRaster");
    mpReRasterFbo->attachDepthStencilTarget(pRenderData->getTexture("depthStencil"));
    pContext->clearFbo(mpReRasterFbo.get(), vec4(0), 1.f, 0, FboAttachmentType::Color | FboAttachmentType::Stencil);
    //pContext->clearDsv(mpReRasterFbo.get()->getDepthStencilView().get(), 1, 0);
    mpReRasterVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpReRasterGraphicsState->setFbo(mpReRasterFbo);
    pContext->setGraphicsState(mpReRasterGraphicsState);
    pContext->setGraphicsVars(mpReRasterVars);
    //mpReRasterSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());
    mpSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());
    Profiler::endEvent("Right ReRaster");
    ///---------------------Re Rasteration Lighting---------------------///
    // Lighting Hole Fill Pass
    //pContext->clearUAV(pDisTex->getUAV().get(), vec4(0));
    Profiler::startEvent("Right ReLighting");
    mpReRasterLightingFbo->attachColorTarget(pDisTex, 0);
    //mpReRasterLightingFbo->attachColorTarget(pLowDisTex, 0);
    mpReRasterLightingFbo->attachDepthStencilTarget(pRenderData->getTexture("depthStencil"));
    //pContext->clearFbo(mpReRasterLightingFbo.get(), vec4(0), 1.f, 0,  FboAttachmentType::Depth);

    updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());
    setPerFrameData(mpReRasterLightingVars.get());

    mpReRasterLightingVars["PerImageCB"]["gLightViewProj"] = mpLightPass->mpLightCamera->getViewProjMatrix();
    mpReRasterLightingVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpReRasterLightingVars["PerImageCB"]["gBias"] = mpLightPass->mBias;
    mpReRasterLightingVars["PerImageCB"]["gKernelSize"] = (uint32_t)mpLightPass->mPCFKernelSize;
    mpReRasterLightingVars->setSampler("gPCFCompSampler", mpLightPass->mpLinearComparisonSampler);

    mpReRasterLightingVars->setTexture("gPos", mpReRasterFbo->getColorTexture(0));
    mpReRasterLightingVars->setTexture("gNorm", mpReRasterFbo->getColorTexture(1));
    mpReRasterLightingVars->setTexture("gDiffuseMatl", mpReRasterFbo->getColorTexture(2));
    mpReRasterLightingVars->setTexture("gSpecMatl", mpReRasterFbo->getColorTexture(3));
    mpReRasterLightingVars->setTexture("gShadowMap", pRenderData->getTexture("shadowDepth"));
    //mpReRasterLightingVars->setTexture("gShadowMap", mpReRasterFbo->getColorTexture(0));
    mpReRasterLightingState->setFbo(mpReRasterLightingFbo);

    pContext->pushGraphicsState(mpReRasterLightingState);
    pContext->pushGraphicsVars(mpReRasterLightingVars);
    mpReRasterLightingPass->execute(pContext, mpReRasterLightingState->getDepthStencilState());
    pContext->popGraphicsVars();
    pContext->popGraphicsState();
    //pContext->blit(pLowDisTex.get()->getSRV(), pDisTex.get()->getRTV());
    Profiler::endEvent("Right ReLighting");

}

void RightGather::renderUI(Gui* pGui, const char* uiGroup)
{
    pGui->addRgbColor("Clear Color", mClearColor);
}

void RightGather::onResize(uint32_t width, uint32_t height)
{
    Fbo::Desc reRasterFboDesc;
    reRasterFboDesc.
        setColorTarget(0, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(1, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(2, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(3, Falcor::ResourceFormat::RGBA16Float);
    //mpReRasterFbo = FboHelper::create2D(width/2, height/2, reRasterFboDesc);
    mpReRasterFbo = FboHelper::create2D(width, height, reRasterFboDesc);

    //mpLowFbo = 

}

bool RightGather::onMouseEvent(const MouseEvent& mouseEvent)
{
    bool handled = false;
    return handled;
}

bool RightGather::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool handled = false;
    return handled;
}

void RightGather::updateVariableOffsets(const ProgramReflection* pReflector)
{
    const ParameterBlockReflection* pBlock = pReflector->getDefaultParameterBlock().get();
    const ReflectionVar* pVar = pBlock->getResource("InternalPerFrameCB").get();
    const ReflectionType* pType = pVar->getType().get();

    sCameraDataOffset = pType->findMember("gCamera.viewMat")->getOffset();
    const auto& pCountOffset = pType->findMember("gLightsCount");
    sLightCountOffset = pCountOffset ? pCountOffset->getOffset() : ConstantBuffer::kInvalidOffset;
    const auto& pLightOffset = pType->findMember("gLights");
    sLightArrayOffset = pLightOffset ? pLightOffset->getOffset() : ConstantBuffer::kInvalidOffset;
}

void RightGather::setPerFrameData(const GraphicsVars* pVars)
{
    ConstantBuffer* pCB = pVars->getConstantBuffer("InternalPerFrameCB").get();

    // Set camera
    if (mpScene->getActiveCamera())
    {
        mpScene->getActiveCamera()->setIntoConstantBuffer(pCB, sCameraDataOffset);
    }

    // Set light data
    if (sLightArrayOffset != ConstantBuffer::kInvalidOffset)
    {
        for (uint32_t i = 0; i < mpScene->getLightCount(); i++)
        {
            mpScene->getLight(i)->setIntoProgramVars(&*mpReRasterLightingVars, pCB, sLightArrayOffset + (i * Light::getShaderStructSize()));
        }
    }

    // Set light count value
    if (sLightCountOffset != ConstantBuffer::kInvalidOffset)
    {
        pCB->setVariable(sLightCountOffset, mpScene->getLightCount());
    }

    // Set light probe
    if (mpScene->getLightProbeCount() > 0)
    {
        LightProbe::setCommonIntoProgramVars(&*mpReRasterLightingVars, "gProbeShared");
        mpScene->getLightProbe(0)->setIntoProgramVars(&*mpReRasterLightingVars, pCB, "gLightProbe");
    }

}

void RightGather::setScene(const std::shared_ptr<Scene>& pScene)
{
    mpScene = std::dynamic_pointer_cast<Scene>(pScene);

    if (mpScene != nullptr && mpScene->getEnvironmentMap() != nullptr)
    {
        mpSkyBox = SkyBox::create(mpScene->getEnvironmentMap());
    }
    else
    {
        mpSkyBox = nullptr;
    }
    //mpReRasterSceneRenderer = SceneRenderer::create(mpScene);
    mpSceneRenderer = SceneRenderer::create(mpScene);
    //mpSceneRenderer->toggleStaticMaterialCompilation(false);
}

Dictionary RightGather::getScriptingDictionary() const
{
    return Dictionary();
}
