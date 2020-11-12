#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "../DeferredRenderer.h"
#include "Lighting.h"

using namespace Falcor;

// Responsible for the reprojection main feature including ray traced hole-filling
class ReRaster : public RenderPass, inherit_shared_from_this<RenderPass, ReRaster>
{
public:

    using SharedPtr = std::shared_ptr<ReRaster>;
    std::string getDesc(void) override { return "ReRaster Pass"; }
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
    ReRaster() : RenderPass("ReRaster") {}

    void initialize(const RenderData* pRenderData);

    // Invokes stencil raster hole-filling after reprojection
    inline void fillHolesRaster(RenderContext* pContext, const RenderData* pRenderData, const Texture::SharedPtr& pTexture);

    // Re-Raster Lighting
    void updateVariableOffsets(const ProgramReflection* pReflector);
    void setPerFrameData(const GraphicsVars* pVars);

    Scene::SharedPtr          mpScene;
    SkyBox::SharedPtr           mpSkyBox;


    //Fbo::SharedPtr              mpFbo;
    //GraphicsVars::SharedPtr     mpVars;
    //GraphicsProgram::SharedPtr  mpProgram;
    //FullScreenPass::UniquePtr   mpScreenPass;
    //GraphicsState::SharedPtr    mpState;

    // Re-Raster G-Buffer
    SceneRenderer::SharedPtr                mpReRasterSceneRenderer;
    Fbo::SharedPtr                          mpReRasterFbo;
    GraphicsProgram::SharedPtr              mpReRasterProgram;
    GraphicsVars::SharedPtr                 mpReRasterVars;
    GraphicsState::SharedPtr                mpReRasterGraphicsState;

    // Re-Raster Lighting
    Fbo::SharedPtr              mpReRasterLightingFbo;
    GraphicsVars::SharedPtr     mpReRasterLightingVars;
    GraphicsState::SharedPtr    mpReRasterLightingState;
    FullScreenPass::UniquePtr   mpReRasterLightingPass;

    Sampler::SharedPtr          mpLinearSampler = nullptr;
    // GUI vars
    bool mIsInitialized = false;
};



