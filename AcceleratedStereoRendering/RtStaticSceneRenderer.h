/*
  Copyrighted(c) 2020, TH Köln.All rights reserved. Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met :

  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
  * Neither the name of TH Köln nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER
  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Authors: Niko Wissmann
 */

#pragma once
#include "Experimental/Raytracing/RtScene.h"
#include "Experimental/Raytracing/RtProgramVars.h"
#include "Experimental/Raytracing/RtState.h"
#include "Graphics/Scene/SceneRenderer.h"

using namespace Falcor;

// Performance optimized RT renderer which avoids per-frame shader table updates 
class RtStaticSceneRenderer : public SceneRenderer, inherit_shared_from_this<SceneRenderer, RtStaticSceneRenderer>
{
public:
    using SharedPtr = std::shared_ptr<RtStaticSceneRenderer>;
    using SharedConstPtr = std::shared_ptr<const RtStaticSceneRenderer>;

    static SharedPtr create(RtScene::SharedPtr pScene);

    deprecate("3.3", "Ray dispatch now accepts depth as a parameter. Using the deprecated version will assume depth = 1.")
    void renderScene(RenderContext* pContext, std::shared_ptr<RtProgramVars> pRtVars, std::shared_ptr<RtState> pState, uvec2 targetDim, Camera* pCamera = nullptr);
    void renderScene(RenderContext* pContext, std::shared_ptr<RtProgramVars> pRtVars, std::shared_ptr<RtState> pState, uvec3 targetDim, Camera* pCamera = nullptr);

    bool mUpdateShaderState = true;
protected:
    RtStaticSceneRenderer(RtScene::SharedPtr pScene) : SceneRenderer(pScene) {}
    struct InstanceData
    {
        CurrentWorkingData currentData;
        uint32_t model;
        uint32_t modelInstance;
        uint32_t mesh;
        uint32_t meshInstance;
        uint32_t progId;
    };

    virtual void setPerFrameData(RtProgramVars* pRtVars, InstanceData& data);
    virtual bool setPerMeshInstanceData(const CurrentWorkingData& currentData, const Scene::ModelInstance* pModelInstance, const Model::MeshInstance* pMeshInstance, uint32_t drawInstanceID) override;
    virtual void setHitShaderData(RtProgramVars* pRtVars, InstanceData& data);
    virtual void setMissShaderData(RtProgramVars* pRtVars, InstanceData& data);
    virtual void setRayGenShaderData(RtProgramVars* pRtVars, InstanceData& data);
    virtual void setGlobalData(RtProgramVars* pRtVars, InstanceData& data);

    void initializeMeshBufferLocation(const ProgramReflection* pReflection);

    struct MeshBufferLocations
    {
        ParameterBlockReflection::BindLocation indices;
        ParameterBlockReflection::BindLocation normal;
        ParameterBlockReflection::BindLocation bitangent;
        ParameterBlockReflection::BindLocation position;
        ParameterBlockReflection::BindLocation prevPosition;
        ParameterBlockReflection::BindLocation texC;
        ParameterBlockReflection::BindLocation lightmapUVs;
    };
    MeshBufferLocations mMeshBufferLocations;
};

