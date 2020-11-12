#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"

using namespace Falcor;

// Main lighting pass after the G-buffer
class WarpField : public RenderPass, inherit_shared_from_this<RenderPass, WarpField>
{
public:

    using SharedPtr = std::shared_ptr<WarpField>;
    std::string getDesc(void) override { return "WarpField Pass"; }

    static SharedPtr create(const Dictionary& params = {});

    RenderPassReflection reflect() const override;
    void execute(RenderContext* pContext, const RenderData* pRenderData) override;
    void renderUI(Gui* pGui, const char* uiGroup) override;
    Dictionary getScriptingDictionary() const override;
    void onResize(uint32_t width, uint32_t height) override;
    void setScene(const std::shared_ptr<Scene>& pScene) override;

    //static size_t sCameraDataOffset;

private:
    WarpField() : RenderPass("WarpField") {}

    void initialize(const Dictionary& dict);
    //void updateVariableOffsets(const ProgramReflection* pReflector);
    //void setPerFrameData(const GraphicsVars* pVars);
    void setDefine(std::string pName, bool flag);

    Scene::SharedPtr          mpScene;
    Fbo::SharedPtr              mpFbo;
    GraphicsVars::SharedPtr     mpVars;
    GraphicsState::SharedPtr    mpState;
    FullScreenPass::UniquePtr   mpPass;

    Sampler::SharedPtr          mpLinearSampler = nullptr;

    bool mIsInitialized = false;
};

