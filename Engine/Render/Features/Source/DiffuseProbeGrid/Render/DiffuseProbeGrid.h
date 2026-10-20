/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <JE/Render/RPI/Public/Culling.h>
#include <JE/Render/RPI/Public/PipelineState.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Aabb.h>
#include <Render/DiffuseProbeGridTextureReadback.h>

namespace JE::Render
{
    namespace Features
    {
        class DiffuseProbeGridFeatureProcessor;

        struct DiffuseProbeGridRenderData
        {
            static const AZ::RHI::Format RayTraceImageFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
            static const AZ::RHI::Format IrradianceImageFormat = AZ::RHI::Format::R16G16B16A16_FLOAT;
            static const AZ::RHI::Format DistanceImageFormat = AZ::RHI::Format::R32G32_FLOAT;
            static const AZ::RHI::Format ProbeDataImageFormat = AZ::RHI::Format::R16G16B16A16_FLOAT;
            static const uint32_t GridDataBufferSize = 112;

            AZ::RHI::Ptr<AZ::RHI::ImagePool> m_imagePool;
            AZ::RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;

            AZ::RHI::GeometryView m_geometryView{ AZ::RHI::MultiDevice::AllDevices };

            // image views
            AZ::RHI::ImageViewDescriptor m_probeRayTraceImageViewDescriptor;
            AZ::RHI::ImageViewDescriptor m_probeIrradianceImageViewDescriptor;
            AZ::RHI::ImageViewDescriptor m_probeDistanceImageViewDescriptor;
            AZ::RHI::ImageViewDescriptor m_probeDataImageViewDescriptor;

            // buffer views
            AZ::RHI::BufferViewDescriptor m_gridDataBufferViewDescriptor;

            // render pipeline state
            JE::Render::RPI::Ptr<JE::Render::RPI::PipelineStateForDraw> m_pipelineState;

            // For the render Srg
            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;

            // render drawlist tag
            AZ::RHI::DrawListTag m_drawListTag;

            // Srg input indices
            AZ::RHI::ShaderInputNameIndex m_prepareSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgGridDataInitializedNameIndex = "m_gridDataInitialized";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridOriginNameIndex = "m_probeGrid.origin";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeHysteresisNameIndex = "m_probeGrid.probeHysteresis";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridRotationNameIndex = "m_probeGrid.rotation";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeRayRotationNameIndex = "m_probeGrid.probeRayRotation";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeMaxRayDistanceNameIndex = "m_probeGrid.probeMaxRayDistance";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeNormalBiasNameIndex = "m_probeGrid.probeNormalBias";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeViewBiasNameIndex = "m_probeGrid.probeViewBias";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeDistanceExponentNameIndex = "m_probeGrid.probeDistanceExponent";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeSpacingNameIndex = "m_probeGrid.probeSpacing";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridPacked0NameIndex = "m_probeGrid.packed0";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeIrradianceEncodingGammaNameIndex = "m_probeGrid.probeIrradianceEncodingGamma";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeIrradianceThresholdNameIndex = "m_probeGrid.probeIrradianceThreshold";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeBrightnessThresholdNameIndex = "m_probeGrid.probeBrightnessThreshold";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridPacked1NameIndex = "m_probeGrid.packed1";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridProbeMinFrontfaceDistanceNameIndex = "m_probeGrid.probeMinFrontfaceDistance";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridPacked2NameIndex = "m_probeGrid.packed2";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridPacked3NameIndex = "m_probeGrid.packed3";
            AZ::RHI::ShaderInputNameIndex m_prepareSrgProbeGridPacked4NameIndex = "m_probeGrid.packed4";

            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgProbeRayTraceNameIndex = "m_probeRayTrace";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgProbeIrradianceNameIndex = "m_probeIrradiance";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgProbeDistanceNameIndex = "m_probeDistance";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgAmbientMultiplierNameIndex = "m_ambientMultiplier";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgGiShadowsNameIndex = "m_giShadows";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgUseDiffuseIblNameIndex = "m_useDiffuseIbl";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgFrameUpdateCountNameIndex = "m_frameUpdateCount";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgFrameUpdateIndexNameIndex = "m_frameUpdateIndex";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgTransparencyModeNameIndex = "m_transparencyMode";
            AZ::RHI::ShaderInputNameIndex m_rayTraceSrgEmissiveMultiplierNameIndex = "m_emissiveMultiplier";

            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgProbeRayTraceNameIndex = "m_probeRayTrace";
            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgProbeIrradianceNameIndex = "m_probeIrradiance";
            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgFrameUpdateCountNameIndex = "m_frameUpdateCount";
            AZ::RHI::ShaderInputNameIndex m_blendIrradianceSrgFrameUpdateIndexNameIndex = "m_frameUpdateIndex";

            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgProbeRayTraceNameIndex = "m_probeRayTrace";
            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgProbeDistanceNameIndex = "m_probeDistance";
            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgFrameUpdateCountNameIndex = "m_frameUpdateCount";
            AZ::RHI::ShaderInputNameIndex m_blendDistanceSrgFrameUpdateIndexNameIndex = "m_frameUpdateIndex";

            AZ::RHI::ShaderInputNameIndex m_borderUpdateRowIrradianceSrgProbeTextureNameIndex = "m_probeTexture";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateRowIrradianceSrgNumTexelsNameIndex = "m_numTexels";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateColumnIrradianceSrgProbeTextureNameIndex = "m_probeTexture";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateColumnIrradianceSrgNumTexelsNameIndex = "m_numTexels";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateRowDistanceSrgProbeTextureNameIndex = "m_probeTexture";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateRowDistanceSrgNumTexelsNameIndex = "m_numTexels";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateColumnDistanceSrgProbeTextureNameIndex = "m_probeTexture";
            AZ::RHI::ShaderInputNameIndex m_borderUpdateColumnDistanceSrgNumTexelsNameIndex = "m_numTexels";

            AZ::RHI::ShaderInputNameIndex m_relocationSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_relocationSrgProbeRayTraceNameIndex = "m_probeRayTrace";
            AZ::RHI::ShaderInputNameIndex m_relocationSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_relocationSrgFrameUpdateCountNameIndex = "m_frameUpdateCount";
            AZ::RHI::ShaderInputNameIndex m_relocationSrgFrameUpdateIndexNameIndex = "m_frameUpdateIndex";

            AZ::RHI::ShaderInputNameIndex m_classificationSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_classificationSrgProbeRayTraceNameIndex = "m_probeRayTrace";
            AZ::RHI::ShaderInputNameIndex m_classificationSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_classificationSrgFrameUpdateCountNameIndex = "m_frameUpdateCount";
            AZ::RHI::ShaderInputNameIndex m_classificationSrgFrameUpdateIndexNameIndex = "m_frameUpdateIndex";

            AZ::RHI::ShaderInputNameIndex m_renderSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_renderSrgModelToWorldNameIndex = "m_modelToWorld";
            AZ::RHI::ShaderInputNameIndex m_renderSrgModelToWorldInverseNameIndex = "m_modelToWorldInverse";
            AZ::RHI::ShaderInputNameIndex m_renderSrgObbHalfLengthsNameIndex = "m_obbHalfLengths";
            AZ::RHI::ShaderInputNameIndex m_renderSrgEnableDiffuseGiNameIndex = "m_enableDiffuseGI";
            AZ::RHI::ShaderInputNameIndex m_renderSrgAmbientMultiplierNameIndex = "m_ambientMultiplier";
            AZ::RHI::ShaderInputNameIndex m_renderSrgEdgeBlendIblNameIndex = "m_edgeBlendIbl";
            AZ::RHI::ShaderInputNameIndex m_renderSrgProbeIrradianceNameIndex = "m_probeIrradiance";
            AZ::RHI::ShaderInputNameIndex m_renderSrgProbeDistanceNameIndex = "m_probeDistance";
            AZ::RHI::ShaderInputNameIndex m_renderSrgProbeDataNameIndex = "m_probeData";

            AZ::RHI::ShaderInputNameIndex m_visualizationPrepareSrgTlasInstancesNameIndex = "m_tlasInstances";
            AZ::RHI::ShaderInputNameIndex m_visualizationPrepareSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_visualizationPrepareSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_visualizationPrepareSrgProbeSphereRadiusNameIndex = "m_probeSphereRadius";

            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgTlasNameIndex = "m_tlas";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgProbeIrradianceNameIndex = "m_probeIrradiance";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgProbeDistanceNameIndex = "m_probeDistance";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgShowInactiveProbesNameIndex = "m_showInactiveProbes";
            AZ::RHI::ShaderInputNameIndex m_visualizationRayTraceSrgOutputNameIndex = "m_output";

            AZ::RHI::ShaderInputNameIndex m_querySrgGridDataNameIndex = "m_gridData";
            AZ::RHI::ShaderInputNameIndex m_querySrgProbeIrradianceNameIndex = "m_probeIrradiance";
            AZ::RHI::ShaderInputNameIndex m_querySrgProbeDistanceNameIndex = "m_probeDistance";
            AZ::RHI::ShaderInputNameIndex m_querySrgProbeDataNameIndex = "m_probeData";
            AZ::RHI::ShaderInputNameIndex m_querySrgAmbientMultiplierNameIndex = "m_ambientMultiplier";
        };

        //! This class manages contains the functionality necessary to update diffuse probes and
        //! generate diffuse global illumination.
        class DiffuseProbeGrid final
        {
        public:
            DiffuseProbeGrid();
            ~DiffuseProbeGrid();

            void Init(JE::Render::RPI::Scene* scene, DiffuseProbeGridRenderData* diffuseProbeGridRenderData);
            void Simulate(uint32_t probeIndex);

            void SetTransform(const AZ::Transform& transform);

            bool ValidateExtents(const AZ::Vector3& newExtents);
            const AZ::Vector3& GetExtents() const { return m_extents; }
            void SetExtents(const AZ::Vector3& extents);

            const AZ::Obb& GetObbWs() const { return m_obbWs; }

            bool ValidateProbeSpacing(const AZ::Vector3& newSpacing);
            const AZ::Vector3& GetProbeSpacing() const { return m_probeSpacing; }
            void SetProbeSpacing(const AZ::Vector3& probeSpacing);

            float GetNormalBias() const { return m_normalBias; }
            void SetNormalBias(float normalBias);

            float GetViewBias() const { return m_viewBias; }
            void SetViewBias(float viewBias);

            const DiffuseProbeGridNumRaysPerProbeEntry& GetNumRaysPerProbe() const { return DiffuseProbeGridNumRaysPerProbeArray[aznumeric_cast<uint32_t>(m_numRaysPerProbe)]; }
            void SetNumRaysPerProbe(DiffuseProbeGridNumRaysPerProbe numRaysPerProbe);

            float GetAmbientMultiplier() const { return m_ambientMultiplier; }
            void SetAmbientMultiplier(float ambientMultiplier);

            void Enable(bool enabled);

            bool GetGIShadows() const { return m_giShadows; }
            void SetGIShadows(bool giShadows) { m_giShadows = giShadows; }

            bool GetUseDiffuseIbl() const { return m_useDiffuseIbl; }
            void SetUseDiffuseIbl(bool useDiffuseIbl) { m_useDiffuseIbl = useDiffuseIbl; }

            DiffuseProbeGridMode GetMode() const { return m_mode; }
            void SetMode(DiffuseProbeGridMode mode);

            bool GetScrolling() const { return m_scrolling; }
            void SetScrolling(bool scrolling);

            bool GetEdgeBlendIbl() const { return m_edgeBlendIbl; }
            void SetEdgeBlendIbl(bool edgeBlendIbl);

            uint32_t GetFrameUpdateCount() const { return m_frameUpdateCount; }
            void SetFrameUpdateCount(uint32_t frameUpdateCount) { m_frameUpdateCount = frameUpdateCount; }

            uint32_t GetFrameUpdateIndex() const { return m_frameUpdateIndex; }

            DiffuseProbeGridTransparencyMode GetTransparencyMode() const { return m_transparencyMode; }
            void SetTransparencyMode(DiffuseProbeGridTransparencyMode transparencyMode) { m_transparencyMode = transparencyMode; }

            float GetEmissiveMultiplier() const { return m_emissiveMultiplier; }
            void SetEmissiveMultiplier(float emissiveMultiplier) { m_emissiveMultiplier = emissiveMultiplier; }

            bool GetVisualizationEnabled() const { return m_visualizationEnabled; }
            void SetVisualizationEnabled(bool visualizationEnabled);

            bool GetVisualizationShowInactiveProbes() const { return m_visualizationShowInactiveProbes; }
            void SetVisualizationShowInactiveProbes(bool visualizationShowInactiveProbes) { m_visualizationShowInactiveProbes = visualizationShowInactiveProbes; }

            float GetVisualizationSphereRadius() const { return m_visualizationSphereRadius; }
            void SetVisualizationSphereRadius(float visualizationSphereRadius);

            uint32_t GetRemainingRelocationIterations() const { return aznumeric_cast<uint32_t>(m_remainingRelocationIterations); }
            void DecrementRemainingRelocationIterations() { m_remainingRelocationIterations = AZStd::max(0, m_remainingRelocationIterations - 1); }
            void ResetRemainingRelocationIterations() { m_remainingRelocationIterations = DefaultNumRelocationIterations; }

            void ResetCullingVisibility();
            bool GetIsVisible() const;

            // compute total number of probes in the grid
            uint32_t GetTotalProbeCount() const;

            // compute probe counts for a 2D texture layout
            void GetTexture2DProbeCount(uint32_t& probeCountX, uint32_t& probeCountY) const;

            // Srgs
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetPrepareSrg() const { return m_prepareSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetRayTraceSrg() const { return m_rayTraceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBlendIrradianceSrg() const { return m_blendIrradianceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBlendDistanceSrg() const { return m_blendDistanceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBorderUpdateRowIrradianceSrg() const { return m_borderUpdateRowIrradianceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBorderUpdateColumnIrradianceSrg() const { return m_borderUpdateColumnIrradianceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBorderUpdateRowDistanceSrg() const { return m_borderUpdateRowDistanceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetBorderUpdateColumnDistanceSrg() const { return m_borderUpdateColumnDistanceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetRelocationSrg() const { return m_relocationSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetClassificationSrg() const { return m_classificationSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetRenderObjectSrg() const { return m_renderObjectSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetVisualizationPrepareSrg() const { return m_visualizationPrepareSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetVisualizationRayTraceSrg() const { return m_visualizationRayTraceSrg; }
            const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& GetQuerySrg() const { return m_querySrg; }

            // Srg updates
            void UpdatePrepareSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateRayTraceSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBlendIrradianceSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBlendDistanceSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateBorderUpdateSrgs(const AZ::Data::Instance<JE::Render::RPI::Shader>& rowShader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& rowSrgLayout,
                                        const AZ::Data::Instance<JE::Render::RPI::Shader>& columnShader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& columnSrgLayout);
            void UpdateRelocationSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateClassificationSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateRenderObjectSrg();
            void UpdateVisualizationPrepareSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);
            void UpdateVisualizationRayTraceSrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout, const AZ::RHI::ImageView* outputImageView);
            void UpdateQuerySrg(const AZ::Data::Instance<JE::Render::RPI::Shader>& shader, const AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout);

            // textures
            const AZ::RHI::Ptr<AZ::RHI::Image> GetRayTraceImage() { return m_rayTraceImage[m_currentImageIndex]; }
            const AZ::RHI::Ptr<AZ::RHI::Image> GetIrradianceImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_irradianceImage[m_currentImageIndex] : m_bakedIrradianceImage->GetRHIImage(); }
            const AZ::RHI::Ptr<AZ::RHI::Image> GetDistanceImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_distanceImage[m_currentImageIndex] : m_bakedDistanceImage->GetRHIImage(); }
            const AZ::RHI::Ptr<AZ::RHI::Image> GetProbeDataImage() { return m_mode == DiffuseProbeGridMode::RealTime ? m_probeDataImage[m_currentImageIndex] : m_bakedProbeDataImage->GetRHIImage(); }
            const AZ::RHI::Ptr<AZ::RHI::Buffer> GetGridDataBuffer() { return m_gridDataBuffer; }

            const AZStd::string& GetBakedIrradianceRelativePath() const { return m_bakedIrradianceRelativePath; }
            const AZStd::string& GetBakedDistanceRelativePath() const { return m_bakedDistanceRelativePath; }
            const AZStd::string& GetBakedProbeDataRelativePath() const { return m_bakedProbeDataRelativePath; }

            // attachment Ids
            const AZ::RHI::AttachmentId GetRayTraceImageAttachmentId() const { return m_rayTraceImageAttachmentId; }
            const AZ::RHI::AttachmentId GetIrradianceImageAttachmentId() const { return m_irradianceImageAttachmentId; }
            const AZ::RHI::AttachmentId GetDistanceImageAttachmentId() const { return m_distanceImageAttachmentId; }
            const AZ::RHI::AttachmentId GetProbeDataImageAttachmentId() const { return m_probeDataImageAttachmentId; }
            const AZ::RHI::AttachmentId GetGridDataBufferAttachmentId() const { return m_gridDataBufferAttachmentId; }
            const AZ::RHI::AttachmentId GetProbeVisualizationTlasAttachmentId() const { return m_visualizationTlasAttachmentId; }
            const AZ::RHI::AttachmentId GetProbeVisualizationTlasInstancesAttachmentId() const { return m_visualizationTlasInstancesAttachmentId; }

            const DiffuseProbeGridRenderData* GetRenderData() const { return m_renderData; }

            // texture readback
            DiffuseProbeGridTextureReadback& GetTextureReadback() { return m_textureReadback; }

            void SetBakedTextures(const DiffuseProbeGridBakedTextures& bakedTextures);
            bool HasValidBakedTextures() const;

            static constexpr uint32_t DefaultNumIrradianceTexels = 6;
            static constexpr uint32_t DefaultNumDistanceTexels = 14;
            static constexpr int32_t DefaultNumRelocationIterations = 100;

            // visualization TLAS
            const AZ::RHI::Ptr<AZ::RHI::RayTracingTlas>& GetVisualizationTlas() const { return m_visualizationTlas; }
            AZ::RHI::Ptr<AZ::RHI::RayTracingTlas>& GetVisualizationTlas() { return m_visualizationTlas; }

            bool GetVisualizationTlasUpdateRequired() const;
            void ResetVisualizationTlasUpdateRequired() { m_visualizationTlasUpdateRequired = false; }

            // query
            bool ContainsPosition(const AZ::Vector3& position) const;

        private:

            // helper functions
            void UpdateTextures();
            void ComputeProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing, uint32_t& probeCountX, uint32_t& probeCountY, uint32_t& probeCountZ);
            bool ValidateProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing);
            void UpdateProbeCount();
            void UpdateCulling();

            // scene
            JE::Render::RPI::Scene* m_scene = nullptr;

            // probe grid transform
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();

            // extents of the probe grid
            AZ::Vector3 m_extents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // expanded extents for rendering the volume
            AZ::Vector3 m_renderExtents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // probe grid OBB (world space), built from transform and extents
            AZ::Obb m_obbWs;

            // per-axis spacing of probes in the grid
            AZ::Vector3 m_probeSpacing = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // per-axis number of probes in the grid
            uint32_t m_probeCountX = 0;
            uint32_t m_probeCountY = 0;
            uint32_t m_probeCountZ = 0;

            // grid settings
            bool  m_enabled = true;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_probeMaxRayDistance = 30.0f;
            float m_probeDistanceExponent = 50.0f;
            float m_probeHysteresis = 0.95f;
            float m_probeIrradianceThreshold = 0.2f;
            float m_probeBrightnessThreshold = 1.0f;
            float m_probeIrradianceEncodingGamma = 5.0f;
            float m_probeMinFrontfaceDistance = 1.0f;
            float m_probeRandomRayBackfaceThreshold = 0.1f;
            float m_probeFixedRayBackfaceThreshold = 0.25f;
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            bool  m_giShadows = true;
            bool  m_useDiffuseIbl = true;
            bool  m_scrolling = false;
            bool  m_edgeBlendIbl = true;
            float m_emissiveMultiplier = DefaultDiffuseProbeGridEmissiveMultiplier;

            DiffuseProbeGridNumRaysPerProbe m_numRaysPerProbe = DefaultDiffuseProbeGridNumRaysPerProbe;
            DiffuseProbeGridTransparencyMode m_transparencyMode = DefaultDiffuseProbeGridTransparencyMode;

            // frame count and current frame index for alternating probe updates across frames
            uint32_t m_frameUpdateCount = 1;
            uint32_t m_frameUpdateIndex = 0;

            // rotation transform applied to probe rays
            AZ::Quaternion m_probeRayRotation;
            AZ::SimpleLcgRandom m_random;

            // probe relocation settings
            int32_t m_remainingRelocationIterations = DefaultNumRelocationIterations;

            // render data
            DiffuseProbeGridRenderData* m_renderData = nullptr;

            // render draw packet
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_drawPacket;

            // sort key for the draw item
            const AZ::RHI::DrawItemSortKey InvalidSortKey = static_cast<AZ::RHI::DrawItemSortKey>(-1);
            AZ::RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // culling
            JE::Render::RPI::Cullable m_cullable;

            // grid mode (RealTime or Baked)
            DiffuseProbeGridMode m_mode = DiffuseProbeGridMode::RealTime;

            // grid data buffer
            AZ::RHI::Ptr<AZ::RHI::Buffer> m_gridDataBuffer;
            bool m_gridDataInitialized = false;

            // real-time textures
            static const uint32_t MaxTextureDimension = 8192;
            static const uint32_t ImageFrameCount = 3;
            AZ::RHI::Ptr<AZ::RHI::Image> m_rayTraceImage[ImageFrameCount];
            AZ::RHI::Ptr<AZ::RHI::Image> m_irradianceImage[ImageFrameCount];
            AZ::RHI::Ptr<AZ::RHI::Image> m_distanceImage[ImageFrameCount];
            AZ::RHI::Ptr<AZ::RHI::Image> m_probeDataImage[ImageFrameCount];
            uint32_t m_currentImageIndex = 0;
            bool m_updateTextures = false;

            // baked textures
            AZ::Data::Instance<JE::Render::RPI::Image> m_bakedIrradianceImage;
            AZ::Data::Instance<JE::Render::RPI::Image> m_bakedDistanceImage;
            AZ::Data::Instance<JE::Render::RPI::Image> m_bakedProbeDataImage;

            // baked texture relative paths
            AZStd::string m_bakedIrradianceRelativePath;
            AZStd::string m_bakedDistanceRelativePath;
            AZStd::string m_bakedProbeDataRelativePath;

            // texture readback
            DiffuseProbeGridTextureReadback m_textureReadback;

            // Srgs
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_prepareSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_rayTraceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_blendIrradianceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_blendDistanceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_borderUpdateRowIrradianceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_borderUpdateColumnIrradianceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_borderUpdateRowDistanceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_borderUpdateColumnDistanceSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_relocationSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_classificationSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_renderObjectSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_querySrg;
            bool m_updateRenderObjectSrg = true;

            // attachment Ids
            AZ::RHI::AttachmentId m_rayTraceImageAttachmentId;
            AZ::RHI::AttachmentId m_irradianceImageAttachmentId;
            AZ::RHI::AttachmentId m_distanceImageAttachmentId;
            AZ::RHI::AttachmentId m_probeDataImageAttachmentId;
            AZ::RHI::AttachmentId m_gridDataBufferAttachmentId;

            // probe visualization
            bool m_visualizationEnabled = false;
            bool m_visualizationShowInactiveProbes = false;
            float m_visualizationSphereRadius = DefaultVisualizationSphereRadius;
            AZ::RHI::Ptr<AZ::RHI::RayTracingTlas> m_visualizationTlas;
            bool m_visualizationTlasUpdateRequired = false;
            AZ::RHI::AttachmentId m_visualizationTlasAttachmentId;
            AZ::RHI::AttachmentId m_visualizationTlasInstancesAttachmentId;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_visualizationPrepareSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_visualizationRayTraceSrg;
        };
    }   // namespace Features
}   // namespace JE::Render
