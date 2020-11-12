#pragma once
#include "Falcor.h"

using namespace Falcor;

class RightDepth : public RenderPass, inherit_shared_from_this<RenderPass, RightDepth>
{
public:
    using SharedPtr = std::shared_ptr<RightDepth>;

    static SharedPtr create(const Dictionary& dict = {});

    RenderPassReflection reflect() const override;
    void execute(RenderContext* pContext, const RenderData* pRenderData) override;
    void renderUI(Gui* pGui, const char* uiGroup) override;
    Dictionary getScriptingDictionary() const override;
    void onResize(uint32_t width, uint32_t height) override;
    void setScene(const std::shared_ptr<Scene>& pScene) override;
    std::string getDesc(void) override { return "Depth PrePass"; }

private:
    RightDepth() : RenderPass("RightDepth") {}
    void initialize(const RenderData* pRenderData);
    // Depth Pass
    SceneRenderer::SharedPtr                mpPreDepthRenderer;
    Fbo::SharedPtr                          mpPreDepthFbo;
    Fbo::SharedPtr                          mpLowPreDepthFbo;
    GraphicsProgram::SharedPtr              mpPreDepthProgram;
    GraphicsVars::SharedPtr                 mpPreDepthVars;
    GraphicsState::SharedPtr                mpPreDepthGraphicsState;
    Scene::SharedPtr                        mpScene;

    bool mIsInitialized = false;
};
