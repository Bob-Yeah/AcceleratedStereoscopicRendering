#include "Framework.h"
#include "RightDepth.h"
#include "../DeferredRenderer.h"

namespace
{
    const std::string kFileRasterPrimary = "RightDepth.slang";
    const std::string kDepthFormat = "depthFormat";
}

static const Gui::DropdownList kDepthFormats =
{
    {(uint32_t)ResourceFormat::D16Unorm, "D16Unorm"},
    {(uint32_t)ResourceFormat::D32Float, "D32Float"},
    {(uint32_t)ResourceFormat::D24UnormS8, "D24UnormS8"},
    {(uint32_t)ResourceFormat::D32FloatS8X24, "D32FloatS8X24"}
};

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

RightDepth::RightDepth() : RenderPass("RightDepth")
{
    mpGraphicsState = GraphicsState::create();
    
    GraphicsProgram::Desc progDesc;
    progDesc.addShaderLibrary("RightDepth.slang").psEntry("main");
    mpProgram = GraphicsProgram::create(progDesc);

    mpVars = GraphicsVars::create(mpProgram->getReflector());
    mpGraphicsState->setProgram(mpProgram);
    mpFbo = Fbo::create();
}

void RightDepth::onResize(uint32_t width, uint32_t height)
{
}

void RightDepth::setScene(const std::shared_ptr<Scene>& pScene)
{
    mpSceneRenderer = (pScene == nullptr) ? nullptr : SceneRenderer::create(pScene);
}

void RightDepth::renderUI(Gui* pGui, const char* uiGroup)
{

}

void RightDepth::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    if (mpSceneRenderer == nullptr)
    {
        logWarning("Invalid SceneRenderer in GBufferRaster::execute()");
        return;
    }
    const auto& pDepth = pRenderData->getTexture("depthStencil");
    mpFbo->attachDepthStencilTarget(pDepth);
    mpGraphicsState->setFbo(mpFbo);

    pContext->clearDsv(pDepth->getDSV().get(), 1, 0);
    
    pContext->setGraphicsState(mpGraphicsState);
    pContext->setGraphicsVars(mpVars);
    mpSceneRenderer->renderScene(pContext);
}
