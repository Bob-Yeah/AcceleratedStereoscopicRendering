#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

// Main lighting pass after the G-buffer
class BackwardWarping : public RenderPass, inherit_shared_from_this<RenderPass, BackwardWarping>
{
public:

    using SharedPtr = std::shared_ptr<BackwardWarping>;
    std::string getDesc(void) override { return "BackwardWarping Pass"; }

    static SharedPtr create(const Dictionary& params = {});

    RenderPassReflection reflect() const override;
    void execute(RenderContext* pContext, const RenderData* pRenderData) override;
    void renderUI(Gui* pGui, const char* uiGroup) override;
    Dictionary getScriptingDictionary() const override;
    void onResize(uint32_t width, uint32_t height) override;
    void setScene(const std::shared_ptr<Scene>& pScene) override;

private:
    BackwardWarping() : RenderPass("BackwardWarping") {}

    void initialize(const Dictionary& dict);

    Scene::SharedPtr            mpScene;
    Fbo::SharedPtr              mpFbo;
    GraphicsVars::SharedPtr     mpVars;
    GraphicsState::SharedPtr    mpState;
    FullScreenPass::UniquePtr   mpPass;
    Sampler::SharedPtr          mpLinearSampler = nullptr;

    float gDelta = 0.0001f;
    int gIterNum = 3;

    bool mIsInitialized = false;
};

