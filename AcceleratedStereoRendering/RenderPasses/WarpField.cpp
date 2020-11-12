#include "WarpField.h"
#include "../DeferredRenderer.h"

namespace {
    const char* kFileOutput = "WarpField.slang";
};

//size_t WarpField::sCameraDataOffset = ConstantBuffer::kInvalidOffset;

WarpField::SharedPtr WarpField::create(const Dictionary& params)
{
    WarpField::SharedPtr ptr(new WarpField());
    return ptr;
}

RenderPassReflection WarpField::reflect() const
{
    RenderPassReflection r;
    r.addInput("depth", "");
    r.addInput("leftIn", "");
    r.addOutput("out", "").format(ResourceFormat::RGBA32Float).bindFlags(Resource::BindFlags::ShaderResource | Resource::BindFlags::UnorderedAccess | Resource::BindFlags::RenderTarget);
    return r;
}

void WarpField::initialize(const Dictionary& dict)
{
    mpState = GraphicsState::create();
    mpPass = FullScreenPass::create(kFileOutput);
    mpVars = GraphicsVars::create(mpPass->getProgram()->getReflector());
    mpFbo = Fbo::create();
    mIsInitialized = true;
}
//
//void WarpField::updateVariableOffsets(const ProgramReflection* pReflector)
//{
//    const ParameterBlockReflection* pBlock = pReflector->getDefaultParameterBlock().get();
//    const ReflectionVar* pVar = pBlock->getResource("InternalPerFrameCB").get();
//    const ReflectionType* pType = pVar->getType().get();
//
//    sCameraDataOffset = pType->findMember("gCamera.projMat")->getOffset();
//}
//
//void WarpField::setPerFrameData(const GraphicsVars* currentData)
//{
//    ConstantBuffer* pCB = currentData->getConstantBuffer("InternalPerFrameCB").get();
//
//    // Set camera
//    if (mpScene->getActiveCamera())
//    {
//        mpScene->getActiveCamera()->setIntoConstantBuffer(pCB, sCameraDataOffset);
//    }
//}

void WarpField::execute(RenderContext* pContext, const RenderData* pRenderData)
{
    // On first execution, run some initialization
    if (!mIsInitialized) initialize(pRenderData->getDictionary());

    // Get our output buffer and clear it
    const auto& pDisTex = pRenderData->getTexture("out");
    pContext->clearUAV(pDisTex->getUAV().get(), vec4(0));
    mpFbo->attachColorTarget(pDisTex, 0);

    if (pDisTex == nullptr) return;

    // set scene lights (not auto by FullScreenPass program -> adapted from SceneRenderer.cpp)
    //updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());
    //setPerFrameData(mpVars.get());

    mpVars["PerImageCB"]["gNearZ"] = mpScene->getActiveCamera()->getNearPlane();
    mpVars["PerImageCB"]["gFarZ"] = mpScene->getActiveCamera()->getFarPlane();
    //mpVars["PerImageCB"]["gViewportDims"] = vec2(mpFbo->getWidth(), mpFbo->getHeight());
    mpVars["PerImageCB"]["ipd"] = DeferredRenderer::sIpd;

    mpVars->setTexture("gDepthTex", pRenderData->getTexture("depth"));
    mpVars->setTexture("gLeftTex", pRenderData->getTexture("leftIn"));

    // Sampler
    //Sampler::Desc samplerDesc;
    //samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    //mpLinearSampler = Sampler::create(samplerDesc);
    //mpVars->setSampler("gLinearSampler", mpLinearSampler);

    mpState->setFbo(mpFbo);
    pContext->pushGraphicsState(mpState);
    pContext->pushGraphicsVars(mpVars);

    mpPass->execute(pContext);
    pContext->popGraphicsVars();
    pContext->popGraphicsState();
}

void WarpField::renderUI(Gui* pGui, const char* uiGroup)
{

}

void WarpField::setDefine(std::string pName, bool flag)
{
    if (flag)
    {
        mpPass->getProgram()->addDefine(pName);
    }
    else
    {
        mpPass->getProgram()->removeDefine(pName);
    }
}

Dictionary WarpField::getScriptingDictionary() const
{
    return Dictionary();
}

void WarpField::onResize(uint32_t width, uint32_t height)
{
}

void WarpField::setScene(const std::shared_ptr<Scene>& pScene)
{
    mpScene = pScene;
}
