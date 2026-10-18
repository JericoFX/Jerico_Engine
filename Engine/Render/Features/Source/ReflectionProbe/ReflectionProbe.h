/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <JE/Render/Features/RenderCommon.h>
#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RHI/GeometryView.h>
#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/MeshDrawPacket.h>
#include <JE/Render/RPI/Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <JE/Render/RPI/Public/PipelineState.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <CubeMapCapture/CubeMapRenderer.h>

namespace JE::Render
{
    namespace Features
    {
        // shared data for rendering reflections, loaded and stored by the ReflectionProbeFeatureProcessor and passed to all probes
        struct ReflectionRenderData
        {
            AZ::RHI::GeometryView m_geometryView{ AZ::RHI::MultiDevice::AllDevices };

            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_stencilPipelineState;
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_blendWeightPipelineState;
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_renderOuterPipelineState;
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_renderInnerPipelineState;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_stencilShader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_stencilSrgLayout;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_blendWeightShader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_blendWeightSrgLayout;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_renderOuterShader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_renderOuterSrgLayout;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_renderInnerShader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_renderInnerSrgLayout;

            AZ::RHI::DrawListTag m_stencilDrawListTag;
            AZ::RHI::DrawListTag m_blendWeightDrawListTag;
            AZ::RHI::DrawListTag m_renderOuterDrawListTag;
            AZ::RHI::DrawListTag m_renderInnerDrawListTag;

            AZ::RHI::ShaderInputNameIndex m_modelToWorldStencilConstantIndex = "m_modelToWorld";
            AZ::RHI::ShaderInputNameIndex m_modelToWorldRenderConstantIndex = "m_modelToWorld";
            AZ::RHI::ShaderInputNameIndex m_modelToWorldInverseRenderConstantIndex = "m_modelToWorldInverse";
            AZ::RHI::ShaderInputNameIndex m_outerObbHalfLengthsRenderConstantIndex = "m_outerObbHalfLengths";
            AZ::RHI::ShaderInputNameIndex m_innerObbHalfLengthsRenderConstantIndex = "m_innerObbHalfLengths";
            AZ::RHI::ShaderInputNameIndex m_useParallaxCorrectionRenderConstantIndex = "m_useParallaxCorrection";
            AZ::RHI::ShaderInputNameIndex m_exposureConstantIndex = "m_exposure";
            AZ::RHI::ShaderInputNameIndex m_reflectionCubeMapRenderImageIndex = "m_reflectionCubeMap";
        };

        // ReflectionProbe manages all aspects of a single probe, including rendering, visualization, and cubemap generation
        class ReflectionProbe final
            : public AZ::Data::AssetBus::Handler
            , private CubeMapRenderer
        {
        public:
            ReflectionProbe() = default;
            ~ReflectionProbe();

            void Init(JE::Render::RPI::Scene* scene, ReflectionRenderData* reflectionRenderData);
            void Simulate(uint32_t probeIndex);
            void OnRenderEnd();

            AZ::Vector3 GetPosition() const { return m_transform.GetTranslation(); }
            const AZ::Transform& GetTransform() const { return m_transform; }
            void SetTransform(const AZ::Transform& transform);

            const AZ::Vector3& GetOuterExtents() const { return m_outerExtents; }
            void SetOuterExtents(const AZ::Vector3& outerExtents);

            const AZ::Vector3& GetInnerExtents() const { return m_innerExtents; }
            void SetInnerExtents(const AZ::Vector3& innerExtents);

            const AZ::Obb& GetOuterObbWs() const { return m_outerObbWs; }
            const AZ::Obb& GetInnerObbWs() const { return m_innerObbWs; }

            const AZ::Data::Instance<JE::Render::RPI::Image>& GetCubeMapImage() const { return m_cubeMapImage; }
            void SetCubeMapImage(const AZ::Data::Instance<JE::Render::RPI::Image>& cubeMapImage, const AZStd::string& relativePath);

            const AZStd::string& GetCubeMapRelativePath() const { return m_cubeMapRelativePath; }

            bool GetUseParallaxCorrection() const { return m_useParallaxCorrection; }
            void SetUseParallaxCorrection(bool useParallaxCorrection) { m_useParallaxCorrection = useParallaxCorrection; }

            const AZ::Uuid& GetUuid() const { return m_uuid; }

            // initiates the reflection probe bake and invokes the callback when the cubemap is finished rendering
            void Bake(RenderCubeMapCallback callback);

            // called by the feature processor so the probe can set the default view for the pipeline
            void OnRenderPipelinePassesChanged(JE::Render::RPI::RenderPipeline* renderPipeline);

            // enables or disables rendering of the visualization sphere
            void ShowVisualization(bool showVisualization);

            // the exposure to use when rendering meshes with this probe's cubemap
            void SetRenderExposure(float renderExposure);
            float GetRenderExposure() const { return m_renderExposure; }

            // the exposure to use when baking the probe cubemap
            void SetBakeExposure(float bakeExposure);
            float GetBakeExposure() const { return m_bakeExposure; }

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbe);

            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> BuildDrawPacket(
                const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& srg,
                const JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw>& pipelineState,
                const AZ::RHI::DrawListTag& drawListTag,
                uint32_t stencilRef);

            void UpdateCulling();

            // AZ::Data::AssetBus::Handler overrides...
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // scene
            JE::Render::RPI::Scene* m_scene = nullptr;

            // probe volume transform
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();

            // extents of the probe volume
            AZ::Vector3 m_outerExtents = AZ::Vector3(0.0f, 0.0f, 0.0f);
            AZ::Vector3 m_innerExtents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // probe volume OBBs (world space), built from position and extents
            AZ::Obb m_outerObbWs;
            AZ::Obb m_innerObbWs;

            // cubemap
            AZ::Data::Instance<JE::Render::RPI::Image> m_cubeMapImage;
            AZStd::string m_cubeMapRelativePath;
            bool m_useParallaxCorrection = false;

            // probe visualization
            JE::Render::Features::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            AZ::Data::Asset<JE::Render::RPI::ModelAsset> m_visualizationModelAsset;
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_visualizationMaterialAsset;
            JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle m_visualizationMeshHandle;

            // reflection rendering
            ReflectionRenderData* m_reflectionRenderData = nullptr;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_stencilSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_blendWeightSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_renderOuterSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_renderInnerSrg;
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_stencilDrawPacket;
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_blendWeightDrawPacket;
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_renderOuterDrawPacket;
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_renderInnerDrawPacket;
            float m_renderExposure = 0.0f;
            float m_bakeExposure = 0.0f;
            bool m_updateSrg = false;

            const AZ::RHI::DrawItemSortKey InvalidSortKey = static_cast<AZ::RHI::DrawItemSortKey>(-1);
            AZ::RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // culling
            JE::Render::RPI::Cullable m_cullable;
            AZ::Uuid m_uuid = AZ::Uuid::Create();
        };

    } // namespace Features
} // namespace JE::Render
