/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Sphere.h>
#include <JE/Render/Features/CoreLights/PhotometricValue.h>
#include <JE/Render/Features/CoreLights/PointLightFeatureProcessorInterface.h>
#include <JE/Render/Features/Utils/GpuBufferHandler.h>
#include <JE/Render/Features/Utils/MultiIndexedDataVector.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>

namespace AZ
{
    class Vector3;
    class Color;

    }

namespace JE::Render::Features
{
        class PointLightFeatureProcessor final
            : public PointLightFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PointLightFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Features::PointLightFeatureProcessor, "AZ::Render::PointLightFeatureProcessor", "{C16A39D6-0DDA-4511-9E35-42968702D3B4}", JE::Render::Features::PointLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            PointLightFeatureProcessor();
            virtual ~PointLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void Render(const RenderPacket& packet) override;

            // PointLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetBulbRadius(LightHandle handle, float bulbRadius) override;
            void SetShadowsEnabled(LightHandle handle, bool enabled) override;
            void SetShadowBias(LightHandle handle, float bias) override;
            void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) override;
            void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) override;
            void SetFilteringSampleCount(LightHandle handle, uint16_t count) override;
            void SetEsmExponent(LightHandle handle, float esmExponent) override;
            void SetUseCachedShadows(LightHandle handle, bool useCachedShadows) override;
            void SetNormalShadowBias(LightHandle handle, float bias) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;
            void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) override;
            void SetPointData(LightHandle handle, const PointLightData& data) override;

            const AZ::Data::Instance<JE::Render::RPI::Buffer> GetLightBuffer() const override;
            uint32_t GetLightCount() const override;

        private:
            PointLightFeatureProcessor(const PointLightFeatureProcessor&) = delete;
            using ShadowId = ProjectedShadowFeatureProcessor::ShadowId;

            static constexpr const char* FeatureProcessorName = "PointLightFeatureProcessor";
            void UpdateShadow(LightHandle handle);
            // Convenience function for forwarding requests to the ProjectedShadowFeatureProcessor
            template<typename Functor, typename ParamType>
            void SetShadowSetting(LightHandle handle, Functor&&, ParamType&& param);
            ProjectedShadowFeatureProcessor* m_shadowFeatureProcessor = nullptr;

            MultiIndexedDataVector<PointLightData, AZ::Sphere> m_lightData;
            GpuBufferHandler m_lightBufferHandler;
            AZ::RHI::Handle<uint32_t> m_lightMeshFlag;
            AZ::RHI::Handle<uint32_t> m_shadowMeshFlag;
            bool m_deviceBufferNeedsUpdate = false;

            AZStd::array<AZ::Transform, PointLightData::NumShadowFaces> m_pointShadowTransforms;
        };
    } // namespace JE::Render::Features
