/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>
#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

namespace JE::Render
{
    namespace Features
    {
        class SpecularReflectionsFeatureProcessor final
            : public SpecularReflectionsFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(SpecularReflectionsFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Features::SpecularReflectionsFeatureProcessor, "AZ::Render::SpecularReflectionsFeatureProcessor", "{3C08E4DD-B4A4-4FD6-A56A-D1D97A8C31CD}", JE::Render::Features::SpecularReflectionsFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SpecularReflectionsFeatureProcessor() = default;
            virtual ~SpecularReflectionsFeatureProcessor() = default;

            void Activate() override;
            void Deactivate() override;

            // ReflectionProbeFeatureProcessorInterface overrides
            void SetSSROptions(const SSROptions& ssrOptions) override;
            const SSROptions& GetSSROptions() const override { return m_ssrOptions; }

        private:
            AZ_DISABLE_COPY_MOVE(SpecularReflectionsFeatureProcessor);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* pipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void UpdatePasses();

            SSROptions m_ssrOptions;

            AZ::RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            AZ::RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            AZ::RHI::ShaderInputNameIndex m_reflectionMethodNameIndex = "m_reflectionMethod";
            AZ::RHI::ShaderInputNameIndex m_rayTraceFallbackSpecularNameIndex = "m_rayTraceFallbackSpecular";
        };
    } // namespace Features
} // namespace JE::Render
