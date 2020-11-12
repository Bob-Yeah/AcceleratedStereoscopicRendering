#include "ReRaster.h"

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

size_t ReRaster::sLightArrayOffset = ConstantBuffer::kInvalidOffset;
size_t ReRaster::sLightCountOffset = ConstantBuffer::kInvalidOffset;
size_t ReRaster::sCameraDataOffset = ConstantBuffer::kInvalidOffset;

ReRaster::SharedPtr ReRaster::create(const Dictionary& params)
{
    ReRaster::SharedPtr ptr(new ReRaster());
    return ptr;
}

RenderPassReflection ReRaster::reflect() const
{
    RenderPassReflection r;
    r.addInput("reRasterDepthStencil", "");
    r.addInput("shadowDepth", "");
    //r.addInternal("depthStencil", "depth and stencil").format(ResourceFormat::D32Float).bindFlags(Resource::BindFlags::DepthStencil);
    r.addInputOutput("rightResultWithHoles", "").format(ResourceFormat::RGBA32Float).bindFlags(Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    return r;
}

void ReRaster::initialize(const RenderData* pRenderData)
{
    GraphicsProgram::Desc reRasterProgDesc;
    reRasterProgDesc
        .addShaderLibrary("StereoVS.slang").vsEntry("main")
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

    DepthStencilState::Desc reDepthStencilState;
    reDepthStencilState.setDepthTest(true);
    reDepthStencilState.setDepthFunc(DepthStencilState::Func::Less);
    reDepthStencilState.setStencilTest(true);
    reDepthStencilState.setStencilReadMask(1);
    //reDepthStencilState.setStencilWriteMask(2);
    // ref & readmask = 0 () val & readmask = 1 (normal) 0 (disocclusion)
    // normal 1 ; disocclusion: 0
    reDepthStencilState.setStencilRef(0);
    reDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Equal);
    //reDepthStencilState.setStencilOp(DepthStencilState::Face::Front, DepthStencilState::StencilOp::Increase, DepthStencilState::StencilOp::Increase, DepthStencilState::StencilOp::Increase);
    DepthStencilState::SharedPtr reRasterDepthStencil = DepthStencilState::create(reDepthStencilState);
    mpReRasterGraphicsState->setDepthStencilState(reRasterDepthStencil);

    //mpReRasterLightingState = GraphicsState::create();
    //mpReRasterLightingPass = FullScreenPass::create("Lighting.slang");
    //mpReRasterLightingVars = GraphicsVars::create(mpReRasterLightingPass->getProgram()->getReflector());
    //mpReRasterLightingFbo = Fbo::create();

    // Depth Stencil State
    //DepthStencilState::Desc reLightingDepthStencilState;
    //reLightingDepthStencilState.setDepthTest(false);
    //reLightingDepthStencilState.setDepthWriteMask(false);
    //reLightingDepthStencilState.setStencilTest(true);
    //reLightingDepthStencilState.setStencilReadMask(2);
    //reLightingDepthStencilState.setStencilRef(1);
    //reLightingDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Equal);
    //DepthStencilState::SharedPtr reRasterLightingDepthStencil = DepthStencilState::create(reLightingDepthStencilState);
    //mpReRasterLightingState->setDepthStencilState(reRasterLightingDepthStencil);


    //// Re-Raster Program G-Buffer
    //GraphicsProgram::Desc reRasterProgDesc;
    //reRasterProgDesc
    //    .addShaderLibrary("StereoVS.slang").vsEntry("main")
    //    .addShaderLibrary("RasterPrimarySimple.slang").psEntry("main");

    //mpReRasterProgram = GraphicsProgram::create(reRasterProgDesc);
    //mpReRasterVars = GraphicsVars::create(mpReRasterProgram->getReflector());
    //mpReRasterGraphicsState = GraphicsState::create();
    //mpReRasterGraphicsState->setProgram(mpReRasterProgram);



    //// Depth Stencil State
    //DepthStencilState::Desc reDepthStencilState;
    //reDepthStencilState.setDepthTest(true);
    //reDepthStencilState.setStencilTest(true);
    //reDepthStencilState.setStencilReadMask(1);
    //reDepthStencilState.setStencilWriteMask(2);
    //reDepthStencilState.setStencilRef(0);
    //reDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Equal);
    //reDepthStencilState.setStencilOp(DepthStencilState::Face::Front, DepthStencilState::StencilOp::Increase, DepthStencilState::StencilOp::Increase, DepthStencilState::StencilOp::Increase);
    //DepthStencilState::SharedPtr reRasterDepthStencil = DepthStencilState::create(reDepthStencilState);
    //mpReRasterGraphicsState->setDepthStencilState(reRasterDepthStencil);

    //// Re-Raster Program Lighting
    //mpReRasterLightingState = GraphicsState::create();
    //mpReRasterLightingPass = FullScreenPass::create("Lighting.slang");
    //mpReRasterLightingVars = GraphicsVars::create(mpReRasterLightingPass->getProgram()->getReflector());
    //mpReRasterLightingFbo = Fbo::create();

    //// Depth Stencil State
    //DepthStencilState::Desc reLightingDepthStencilState;
    //reLightingDepthStencilState.setDepthTest(false);
    //reLightingDepthStencilState.setDepthWriteMask(false);
    //reLightingDepthStencilState.setStencilTest(true);
    //reLightingDepthStencilState.setStencilReadMask(2);
    //reLightingDepthStencilState.setStencilRef(1);
    //reLightingDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Equal);
    //DepthStencilState::SharedPtr reRasterLightingDepthStencil = DepthStencilState::create(reLightingDepthStencilState);
    //mpReRasterLightingState->setDepthStencilState(reRasterLightingDepthStencil);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);

    mIsInitialized = true;
}

void ReRaster::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    // On first execution, run some initialization
    if (!mIsInitialized) initialize(pRenderData);
    const auto& pDisTex = pRenderData->getTexture("rightResultWithHoles");
    fillHolesRaster(pContext, pRenderData, pDisTex);
}

inline void ReRaster::fillHolesRaster(RenderContext* pContext, const RenderData* pRenderData, const Texture::SharedPtr& pTexture)
{
    //Profiler::startEvent("fillholes_raster");

    mpReRasterFbo->attachColorTarget(pTexture, 0);
    mpReRasterFbo->attachDepthStencilTarget(pRenderData->getTexture("reRasterDepthStencil"));

    pContext->clearFbo(mpReRasterFbo.get(), vec4(0), 1.f, 0, FboAttachmentType::Color | FboAttachmentType::Depth);
    mpReRasterVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpReRasterGraphicsState->setFbo(mpReRasterFbo);
    pContext->setGraphicsState(mpReRasterGraphicsState);
    pContext->setGraphicsVars(mpReRasterVars);
    mpReRasterSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());

    // Lighting Hole Fill Pass
    //pContext->clearUAV(pTexture->getUAV().get(), vec4(0));
    //mpReRasterLightingFbo->attachColorTarget(pTexture, 0);
    //
    ////mpReRasterLightingFbo->attachDepthStencilTarget(pRenderData->getTexture("reRasterDepthStencil"));
    
    //updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());
    //setPerFrameData(mpReRasterLightingVars.get());

    //mpReRasterLightingVars["PerImageCB"]["gLightViewProj"] = mpLightPass->mpLightCamera->getViewProjMatrix();
    //mpReRasterLightingVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    //mpReRasterLightingVars["PerImageCB"]["gBias"] = mpLightPass->mBias;
    //mpReRasterLightingVars["PerImageCB"]["gKernelSize"] = (uint32_t)mpLightPass->mPCFKernelSize;
    //mpReRasterLightingVars->setSampler("gPCFCompSampler", mpLightPass->mpLinearComparisonSampler);

    //mpReRasterLightingVars->setTexture("gPos", mpFbo->getColorTexture(0));
    //mpReRasterLightingVars->setTexture("gNorm", mpFbo->getColorTexture(1));
    //mpReRasterLightingVars->setTexture("gDiffuseMatl", mpFbo->getColorTexture(2));
    //mpReRasterLightingVars->setTexture("gSpecMatl", mpFbo->getColorTexture(3));
    //mpReRasterLightingVars->setTexture("gShadowMap", pRenderData->getTexture("shadowDepth"));

    //mpReRasterLightingState->setFbo(mpReRasterLightingFbo);
    //pContext->pushGraphicsState(mpReRasterLightingState);
    //pContext->pushGraphicsVars(mpReRasterLightingVars);

    //mpReRasterLightingPass->execute(pContext);
    //pContext->popGraphicsVars();
    //pContext->popGraphicsState();

    //mpReRasterLightingFbo->attachColorTarget(pTexture, 0);

    //// G-Buffer with Stencil Mask
    //mpReRasterFbo->attachDepthStencilTarget(pRenderData->getTexture("internalDepth"));
    //pContext->clearFbo(mpReRasterFbo.get(), vec4(0), 1.f, 0, FboAttachmentType::Color | FboAttachmentType::Depth);
    //mpReRasterVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    //mpReRasterGraphicsState->setFbo(mpReRasterFbo);
    //pContext->setGraphicsState(mpReRasterGraphicsState);
    //pContext->setGraphicsVars(mpReRasterVars);
    //mpReRasterSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());

    //// Lighting Hole Fill Pass
    //mpReRasterLightingFbo->attachColorTarget(pTexture, 0);
    //mpReRasterLightingFbo->attachDepthStencilTarget(pRenderData->getTexture("internalDepth"));

    //updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());
    //setPerFrameData(mpReRasterLightingVars.get());

    //mpReRasterLightingVars["PerImageCB"]["gLightViewProj"] = mpLightPass->mpLightCamera->getViewProjMatrix();
    //mpReRasterLightingVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    //mpReRasterLightingVars["PerImageCB"]["gBias"] = mpLightPass->mBias;
    //mpReRasterLightingVars["PerImageCB"]["gKernelSize"] = (uint32_t)mpLightPass->mPCFKernelSize;
    //mpReRasterLightingVars->setSampler("gPCFCompSampler", mpLightPass->mpLinearComparisonSampler);

    //mpReRasterLightingVars->setTexture("gPos", mpReRasterFbo->getColorTexture(0));
    //mpReRasterLightingVars->setTexture("gNorm", mpReRasterFbo->getColorTexture(1));
    //mpReRasterLightingVars->setTexture("gDiffuseMatl", mpReRasterFbo->getColorTexture(2));
    //mpReRasterLightingVars->setTexture("gSpecMatl", mpReRasterFbo->getColorTexture(3));
    //mpReRasterLightingVars->setTexture("gShadowMap", pRenderData->getTexture("shadowDepth"));

    //mpReRasterLightingState->setFbo(mpReRasterLightingFbo);
    //pContext->pushGraphicsState(mpReRasterLightingState);
    //pContext->pushGraphicsVars(mpReRasterLightingVars);

    //mpReRasterLightingPass->execute(pContext, mpReRasterLightingState->getDepthStencilState());
    //pContext->popGraphicsVars();
    //pContext->popGraphicsState();

    //Profiler::endEvent("fillholes_raster");
}

void ReRaster::updateVariableOffsets(const ProgramReflection* pReflector)
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

void ReRaster::setPerFrameData(const GraphicsVars* pVars)
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

void ReRaster::renderUI(Gui* pGui, const char* uiGroup)
{
}

void ReRaster::onResize(uint32_t width, uint32_t height)
{
    Fbo::Desc reRasterFboDesc;
    reRasterFboDesc.
        setColorTarget(0, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(1, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(2, Falcor::ResourceFormat::RGBA16Float).
        setColorTarget(3, Falcor::ResourceFormat::RGBA16Float);

    mpReRasterFbo = FboHelper::create2D(width, height, reRasterFboDesc);
}

bool ReRaster::onMouseEvent(const MouseEvent& mouseEvent)
{
    bool handled = false;
    return handled;
}

bool ReRaster::onKeyEvent(const KeyboardEvent& keyEvent)
{
    bool handled = false;
    return handled;
}

void ReRaster::setScene(const std::shared_ptr<Scene>& pScene)
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
    mpReRasterSceneRenderer = (pScene == nullptr) ? nullptr : SceneRenderer::create(pScene);
}

Dictionary ReRaster::getScriptingDictionary() const
{
    return Dictionary();
}
