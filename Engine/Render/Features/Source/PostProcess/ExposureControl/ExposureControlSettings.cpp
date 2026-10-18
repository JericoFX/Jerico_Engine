/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

#include <PostProcess/ExposureControl/ExposureControlSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/EyeAdaptationPass.h>
#include <PostProcessing/LookModificationTransformPass.h>

namespace JE::Render
{
    namespace Features
    {
        ExposureControlSettings::ExposureControlSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
            , m_eyeAdaptationPassTemplateNameId{EyeAdaptationPassTemplateName}
            , m_luminanceHeatmapNameId{"LuminanceHeatmap"}
            , m_luminanceHistogramGeneratorNameId{"LuminanceHistogramGenerator"}
        {
            InitCommonBuffer();
        }

        void ExposureControlSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void ExposureControlSettings::ApplySettingsTo(ExposureControlSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "ExposureControlSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <JE/Render/Features/ParamMacros/StartOverrideBlend.inl>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void ExposureControlSettings::UpdateExposureControlRelatedPassParameters()
        {
            // [GFX TODO][ATOM-13128] Adapting to render pipeline other than default.
            JE::Render::RPI::RenderPipelinePtr defaultRP = GetParentScene()->GetDefaultRenderPipeline();
            if (defaultRP)
            {
                const JE::Render::RPI::View* defaultView = defaultRP->GetDefaultView().get();
                if (defaultView != m_lastDefaultView)
                {
                    m_shouldUpdatePassParameters = true;
                    m_lastDefaultView = defaultView;
                }
            }

            if (m_shouldUpdatePassParameters)
            {
                UpdateLuminanceHeatmap();

                m_shouldUpdatePassParameters = false;
            }
        }

        void ExposureControlSettings::Simulate([[maybe_unused]] float deltaTime)
        {
            UpdateExposureControlRelatedPassParameters();

            // Update the eye adaptation shader parameters.
            UpdateShaderParameters();
        }

        void ExposureControlSettings::UpdateShaderParameters()
        {
            if (m_shouldUpdateShaderParameters)
            {
                m_shaderParameters.m_eyeAdaptationEnabled = m_exposureControlType == JE::Render::Features::ExposureControl::ExposureControlType::EyeAdaptation;
                m_shaderParameters.m_compensationValue = m_manualCompensationValue;
                m_shaderParameters.m_exposureMin = m_autoExposureMin;
                m_shaderParameters.m_exposureMax = m_autoExposureMax;
                m_shaderParameters.m_speedUp = m_autoExposureSpeedUp;
                m_shaderParameters.m_speedDown = m_autoExposureSpeedDown;

                m_shouldUpdateShaderParameters = false;
            }
        }

        bool ExposureControlSettings::InitCommonBuffer()
        {
            JE::Render::RPI::CommonBufferDescriptor desc;
            desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::Constant;
            desc.m_bufferName = ExposureControlBufferName;
            desc.m_byteCount = sizeof(ShaderParameters);
            desc.m_elementSize = sizeof(ShaderParameters);

            m_buffer = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

            if (!m_buffer)
            {
                AZ_Assert(false, "Failed to create the RPI::Buffer[%s] which is used for the exposure control feature.", desc.m_bufferName.c_str());
                return false;
            }

            return true;
        }

        void ExposureControlSettings::SetEnabled(bool value)
        {
            if (m_enabled != value)
            {
                m_enabled = value;
                m_shouldUpdatePassParameters = true;
            }
        }

        void ExposureControlSettings::SetHeatmapEnabled(bool value)
        {
            if (m_heatmapEnabled != value)
            {
                m_heatmapEnabled = value;
                // Update immediately so that the ExposureControlSettings can just be turned off and killed without having to wait for another Simulate() call
                UpdateLuminanceHeatmap();
            }
        }

        void ExposureControlSettings::SetExposureControlType(JE::Render::Features::ExposureControl::ExposureControlType type)
        {
            if (m_exposureControlType != type)
            {
                m_exposureControlType = type;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::SetManualCompensation(float value)
        {
            if (m_manualCompensationValue != value)
            {
                m_manualCompensationValue = value;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::SetEyeAdaptationExposureMin(float minExposure)
        {
            if (m_autoExposureMin != minExposure)
            {
                m_autoExposureMin = minExposure;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::SetEyeAdaptationExposureMax(float maxExposure)
        {
            if (m_autoExposureMax != maxExposure)
            {
                m_autoExposureMax = maxExposure;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::SetEyeAdaptationSpeedUp(float speedUp)
        {
            if (m_autoExposureSpeedUp != speedUp)
            {
                m_autoExposureSpeedUp = speedUp;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::SetEyeAdaptationSpeedDown(float speedDown)
        {
            if (m_autoExposureSpeedDown != speedDown)
            {
                m_autoExposureSpeedDown = speedDown;
                m_shouldUpdateShaderParameters = true;
            }
        }

        void ExposureControlSettings::UpdateLuminanceHeatmap()
        {
            // [GFX-TODO][ATOM-13194] Support multiple views for the luminance heatmap
            // [GFX-TODO][ATOM-13224] Remove UpdateLuminanceHeatmap and UpdateEyeAdaptationPass            
            JE::Render::RPI::PassFilter heatmapPassFilter = JE::Render::RPI::PassFilter::CreateWithPassName(m_luminanceHeatmapNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(heatmapPassFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    pass->SetEnabled(m_heatmapEnabled);
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });

            JE::Render::RPI::PassFilter histogramPassFilter = JE::Render::RPI::PassFilter::CreateWithPassName(m_luminanceHistogramGeneratorNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(histogramPassFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    pass->SetEnabled(m_heatmapEnabled);
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void ExposureControlSettings::UpdateBuffer()
        {
            m_buffer->UpdateData(&m_shaderParameters, sizeof(m_shaderParameters), 0);
        }

    } // namespace Features
} // namespace JE::Render
