#include "BackwardWarping.h"
#include "../DeferredRenderer.h"


namespace {
    const char* kFileOutput = "BackwardWarping.ps.slang";
};

BackwardWarping::SharedPtr BackwardWarping::create(const Dictionary& params)
{
    BackwardWarping::SharedPtr ptr(new BackwardWarping());
    return ptr;
}

RenderPassReflection BackwardWarping::reflect() const
{
    RenderPassReflection r;
    r.addInput("Vs", "");
    r.addInput("leftIn", "");
    r.addOutput("out", "").format(ResourceFormat::RGBA32Float).bindFlags(Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    return r;
}

void BackwardWarping::initialize(const Dictionary& dict)
{
    mpState = GraphicsState::create();
    mpPass = FullScreenPass::create(kFileOutput);
    mpVars = GraphicsVars::create(mpPass->getProgram()->getReflector());
    mpFbo = Fbo::create();

    mIsInitialized = true;
}

void BackwardWarping::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    // On first execution, run some initialization
    if (!mIsInitialized) initialize(pRenderData->getDictionary());

    // Get our output buffer and clear it
    const auto& pDisTex = pRenderData->getTexture("out");
    pContext->clearUAV(pDisTex->getUAV().get(), vec4(0));
    mpFbo->attachColorTarget(pDisTex, 0);

    if (pDisTex == nullptr) return;

    // mpVar Setting

    mpVars["PerImageCB"]["gDelta"] = gDelta;
    mpVars["PerImageCB"]["gIterNum"] = gIterNum;
    
    mpVars->setTexture("gLeftEyeTex", pRenderData->getTexture("leftIn"));
    mpVars->setTexture("gVs", pRenderData->getTexture("Vs"));

    // Sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    mpLinearSampler = Sampler::create(samplerDesc);
    mpVars->setSampler("gLinearSampler", mpLinearSampler);

    // mpVar Setting End

    mpState->setFbo(mpFbo);
    pContext->pushGraphicsState(mpState);
    pContext->pushGraphicsVars(mpVars);

    mpPass->execute(pContext);
    pContext->popGraphicsVars();
    pContext->popGraphicsState();
}

void BackwardWarping::renderUI(Gui* pGui, const char* uiGroup)
{
    pGui->addFloatSlider("Heuristic Delta", gDelta, 0.0001f, 0.9999f);
    pGui->addIntSlider("Iteration Number", gIterNum, 1, 10);
}

Dictionary BackwardWarping::getScriptingDictionary() const
{
    return Dictionary();
}

void BackwardWarping::onResize(uint32_t width, uint32_t height)
{
}

void BackwardWarping::setScene(const std::shared_ptr<Scene>& pScene)
{
    mpScene = pScene;
}
