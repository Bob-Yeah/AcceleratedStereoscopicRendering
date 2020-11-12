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
    r.addInternal("depthStencil", "").format(ResourceFormat::D32Float).bindFlags(Resource::BindFlags::DepthStencil);
    r.addOutput("out", "").format(ResourceFormat::RGBA32Float).bindFlags(Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    return r;
}

void RightGather::initialize(const RenderData* pRenderData)
{
    // Right Gather Program

    mpState = GraphicsState::create();

    GraphicsProgram::Desc preDepthProgDesc;
    preDepthProgDesc
        .addShaderLibrary("StereoVS.slang").vsEntry("main")
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
    //mpFbo = Fbo::create();
    mpState->setProgram(mpProgram);
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
    ////dsDesc.setDepthWriteMask(true);
    ////dsDesc.setStencilTest(true);
    ////dsDesc.setStencilWriteMask(1);
    //////dsDesc.setStencilReadMask(1);
    ////dsDesc.setStencilRef(1);
    ////// ref & readmask (f) val & readmask
    ////// disocclusion: ref = 0 good: ref = 1
    //////Replace: disocclusion ref = 0 ; ref = 1
    ////dsDesc.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Always);
    //////stencil fail; stencil pass, depth fail; stencil pass, depth pass.
    ////dsDesc.setStencilOp(DepthStencilState::Face::Front, DepthStencilState::StencilOp::Keep, DepthStencilState::StencilOp::Replace, DepthStencilState::StencilOp::Keep);
    DepthStencilState::SharedPtr mpDepthTestDS = DepthStencilState::create(dsDesc);
    mpState->setDepthStencilState(mpDepthTestDS);
    //mpState->setProgram(mpProgram);

    ////Re -Lighting
    mpReRasterLightingState = GraphicsState::create();
    mpReRasterLightingPass = FullScreenPass::create("LightingFor2Pass.slang");
    mpReRasterLightingVars = GraphicsVars::create(mpReRasterLightingPass->getProgram()->getReflector());
    mpReRasterLightingFbo = Fbo::create();
    ////// Depth Stencil State
    //DepthStencilState::Desc reLightingDepthStencilState;
    //reLightingDepthStencilState.setDepthTest(false);
    //reLightingDepthStencilState.setDepthFunc(DepthStencilState::Func::Greater);
    ////reLightingDepthStencilState.setStencilTest(true);
    ////reLightingDepthStencilState.setStencilReadMask(1);
    ////// ref 1 & readMask (stencil func-Equal) value(already in the stencil buffer 0,1) & readmask 
    ////reLightingDepthStencilState.setStencilRef(1);
    ////reLightingDepthStencilState.setStencilFunc(DepthStencilState::Face::Front, DepthStencilState::Func::Always);
    //DepthStencilState::SharedPtr reRasterLightingDepthStencil = DepthStencilState::create(reLightingDepthStencilState);
    //mpReRasterLightingState->setDepthStencilState(reRasterLightingDepthStencil);

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

    // Right Gather Program ##########################
    Profiler::startEvent("Right Gather");
    const auto& pDisTex = pRenderData->getTexture("out");

    //mpFbo->attachColorTarget(pLowDisTex, 0);
    mpFbo->attachColorTarget(pDisTex, 0);
    mpFbo->attachDepthStencilTarget(pRenderData->getTexture("depthStencil"));
    pContext->clearFbo(mpFbo.get(), glm::vec4(mClearColor, 0), 1.0f, 0, FboAttachmentType::All);

    mpVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpVars["PerFrameCB"]["gLeftEyeViewProj"] = mpScene->getActiveCamera()->getViewProjMatrix();

    mpVars->setTexture("gLeftEyeTex", pRenderData->getTexture("leftIn"));
    mpVars->setSampler("gLinearSampler", mpLinearSampler);
    mpVars->setTexture("gLeftEyeDepthTex", pRenderData->getTexture("leftDepth"));

    mpState->setFbo(mpFbo);
    pContext->setGraphicsState(mpState);
    pContext->setGraphicsVars(mpVars);
    mpSceneRenderer->renderScene(pContext, mpScene->getActiveCamera().get());
    Profiler::endEvent("Right Gather");
    //pDisTex :: Warped Image

    ///---------------------Re Rasteration Lighting---------------------///
    ////// Lighting Hole Fill Pass
    Profiler::startEvent("Right ReLighting");
    mpReRasterLightingFbo->attachColorTarget(pDisTex, 0);
    //mpReRasterLightingFbo->attachDepthStencilTarget(pRenderData->getTexture("depthStencil"));
    pContext->clearFbo(mpReRasterLightingFbo.get(), glm::vec4(mClearColor, 0), 1.0f, 0, FboAttachmentType::All);

    updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());
    setPerFrameData(mpReRasterLightingVars.get());

    mpReRasterLightingVars["PerImageCB"]["gLightViewProj"] = mpLightPass->mpLightCamera->getViewProjMatrix();
    mpReRasterLightingVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    mpReRasterLightingVars["PerImageCB"]["gBias"] = mpLightPass->mBias;
    mpReRasterLightingVars["PerImageCB"]["gKernelSize"] = (uint32_t)mpLightPass->mPCFKernelSize;
    mpReRasterLightingVars->setSampler("gPCFCompSampler", mpLightPass->mpLinearComparisonSampler);

    mpReRasterLightingVars->setTexture("gPos", mpFbo->getColorTexture(1));
    mpReRasterLightingVars->setTexture("gNorm", mpFbo->getColorTexture(2));
    mpReRasterLightingVars->setTexture("gDiffuseMatl", mpFbo->getColorTexture(3));
    mpReRasterLightingVars->setTexture("gSpecMatl", mpFbo->getColorTexture(4));
    mpReRasterLightingVars->setTexture("gShadowMap", pRenderData->getTexture("shadowDepth"));
    
    mpReRasterLightingState->setFbo(mpReRasterLightingFbo);

    pContext->pushGraphicsState(mpReRasterLightingState);
    pContext->pushGraphicsVars(mpReRasterLightingVars);
    mpReRasterLightingPass->execute(pContext, mpReRasterLightingState->getDepthStencilState());
    pContext->popGraphicsVars();
    pContext->popGraphicsState();
    /////pContext->blit(pLowDisTex.get()->getSRV(), pDisTex.get()->getRTV());
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
        setColorTarget(0, Falcor::ResourceFormat::RGBA32Float).
        setColorTarget(1, Falcor::ResourceFormat::RGBA32Float).
        setColorTarget(2, Falcor::ResourceFormat::RGBA32Float).
        setColorTarget(3, Falcor::ResourceFormat::RGBA32Float).
        setColorTarget(3, Falcor::ResourceFormat::RGBA32Float);
    ////mpReRasterFbo = FboHelper::create2D(width/2, height/2, reRasterFboDesc);
    //mpReRasterFbo = FboHelper::create2D(width, height, reRasterFboDesc);
    mpFbo = FboHelper::create2D(width, height, reRasterFboDesc);
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
