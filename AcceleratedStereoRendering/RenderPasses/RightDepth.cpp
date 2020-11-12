#include "Framework.h"
#include "RightDepth.h"

RenderPassReflection RightDepth::reflect() const
{
    RenderPassReflection r;
    r.addOutput("depthStencil", "depth and stencil").format(ResourceFormat::D32Float).bindFlags(Resource::BindFlags::DepthStencil);
    return r;
}

RightDepth::SharedPtr RightDepth::create(const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new RightDepth);
    return pPass;
}

Dictionary RightDepth::getScriptingDictionary() const
{
    return Dictionary();
}

void RightDepth::initialize(const RenderData* pRenderData)
{

    // DepthPrePass
    GraphicsProgram::Desc preDepthProgDesc;
    preDepthProgDesc
        .addShaderLibrary("StereoVS.slang").vsEntry("main")
        .addShaderLibrary("RightDepth.slang").psEntry("main");
    mpPreDepthFbo = Fbo::create();
    //Fbo::Desc lowFboDesc;
    //lowFboDesc.
    //    setColorTarget(0, mpPreDepthFbo->getColorTexture(0)->getFormat()).
    //    setDepthStencilTarget(mpPreDepthFbo->getDepthStencilTexture()->getFormat());

    //mpLowPreDepthFbo = FboHelper::create2D(mpPreDepthFbo->getWidth()/2, mpPreDepthFbo->getHeight() / 2, lowFboDesc);

    mpPreDepthProgram = GraphicsProgram::create(preDepthProgDesc);
    mpPreDepthVars = GraphicsVars::create(mpPreDepthProgram->getReflector());
    mpPreDepthGraphicsState = GraphicsState::create();
    mpPreDepthGraphicsState->setProgram(mpPreDepthProgram);
    mIsInitialized = true;
}

void RightDepth::onResize(uint32_t width, uint32_t height)
{
    
}

void RightDepth::setScene(const std::shared_ptr<Scene>& pScene)
{
    mpPreDepthRenderer = (pScene == nullptr) ? nullptr : SceneRenderer::create(pScene);
}

void RightDepth::renderUI(Gui* pGui, const char* uiGroup)
{

}

void RightDepth::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    if (mpPreDepthRenderer == nullptr)
    {
        logWarning("Invalid SceneRenderer in GBufferRaster::execute()");
        return;
    }
    if (!mIsInitialized) initialize(pRenderData);

    mpPreDepthVars["PerImageCB"]["gStereoTarget"] = (uint32_t)1;
    const auto& pDepth = pRenderData->getTexture("depthStencil");
    //logWarning("pDepth width, height:" + std::to_string(pDepth->getWidth()) +","+  std::to_string(pDepth->getHeight()));
    //Texture::SharedPtr pLowDepth = Texture::create2D(pDepth->getWidth()/2, pDepth->getHeight()/2, pDepth->getFormat(),1,1,(const void*)nullptr,Resource::BindFlags::DepthStencil);
    //logWarning("pDepth width, height:" + std::to_string(pLowDepth->getWidth()) + "," + std::to_string(pLowDepth->getHeight()));
    mpPreDepthFbo->attachDepthStencilTarget(pDepth);
    mpPreDepthGraphicsState->setFbo(mpPreDepthFbo);
    pContext->clearDsv(pDepth->getDSV().get(), 1, 0);
    pContext->setGraphicsState(mpPreDepthGraphicsState);
    pContext->setGraphicsVars(mpPreDepthVars);
    mpPreDepthRenderer->renderScene(pContext);
    //pContext->blit(pLowDepth->getSRV(), pDepth->getRTV());
}
