#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "../DeferredRenderer.h"
#include "Lighting.h"
using namespace Falcor;

// Responsible for the reprojection main feature including ray traced hole-filling
class RightGather : public RenderPass, inherit_shared_from_this<RenderPass, RightGather>
{
public:

    using SharedPtr = std::shared_ptr<RightGather>;
    std::string getDesc(void) override { return "RightGather Pass"; }

    static SharedPtr create(const Dictionary& params = {});

    RenderPassReflection reflect() const override;
    void execute(RenderContext* pContext, const RenderData* pRenderData) override;
    void renderUI(Gui* pGui, const char* uiGroup) override;
    Dictionary getScriptingDictionary() const override;
    void onResize(uint32_t width, uint32_t height) override;
    void setScene(const std::shared_ptr<Scene>& pScene) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;

    DeferredRenderer* mpMainRenderObject;
    Lighting::SharedPtr mpLightPass;

    // Re-Raster Lighting
    static size_t sLightArrayOffset;
    static size_t sLightCountOffset;
    static size_t sCameraDataOffset;

private:
    RightGather() : RenderPass("RightGather") {}

    void initialize(const RenderData* pRenderData);

    Scene::SharedPtr                        mpScene;
    SkyBox::SharedPtr                       mpSkyBox;

    // Backward Image Warping
    GraphicsProgram::SharedPtr              mpProgram;
    GraphicsVars::SharedPtr                 mpVars;
    GraphicsState::SharedPtr                mpState;
    Fbo::SharedPtr                          mpFbo;
    Fbo::SharedPtr                          mpLowFbo;
    Sampler::SharedPtr                      mpLinearSampler = nullptr;
    SceneRenderer::SharedPtr                mpSceneRenderer;

    Sampler::SharedPtr                      mpLinearComparisonSampler;
    Texture::SharedPtr                      pLowDisTex;

    glm::vec3                               mClearColor = glm::vec3(0, 0, 0);

    // Re-Raster G-Buffer
    //SceneRenderer::SharedPtr                mpReRasterSceneRenderer;
    //Fbo::SharedPtr                          mpReRasterFbo;
    //GraphicsProgram::SharedPtr              mpReRasterProgram;
    //GraphicsVars::SharedPtr                 mpReRasterVars;
    //GraphicsState::SharedPtr                mpReRasterGraphicsState;

    // Re-Raster Lighting
    Fbo::SharedPtr                          mpReRasterLightingFbo;
    GraphicsVars::SharedPtr                 mpReRasterLightingVars;
    GraphicsState::SharedPtr                mpReRasterLightingState;
    FullScreenPass::UniquePtr               mpReRasterLightingPass;

    // Re-Raster Lighting
    void updateVariableOffsets(const ProgramReflection* pReflector);
    void setPerFrameData(const GraphicsVars* pVars);


    // GUI vars
    //bool mbShowDisocclusion = false;
    bool mIsInitialized = false;
    //bool mbFillHoles = true;
    bool mbRenderEnvMap = false;
};


