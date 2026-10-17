/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/EyeAdaptationPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const EyeAdaptationBufferName = "EyeAdaptationBuffer";

        JE::Render::RPI::Ptr<EyeAdaptationPass> EyeAdaptationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<EyeAdaptationPass> pass = aznew EyeAdaptationPass(descriptor);
            return pass;
        }

        EyeAdaptationPass::EyeAdaptationPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ComputePass(descriptor)
        {
        }
        
        void EyeAdaptationPass::InitBuffer()
        {
            ExposureCalculationData defaultData;
            JE::Render::RPI::CommonBufferDescriptor desc;
            desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = EyeAdaptationBufferName;
            desc.m_byteCount = sizeof(ExposureCalculationData);
            desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
            desc.m_bufferData = &defaultData;

            m_buffer = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        void EyeAdaptationPass::BuildInternal()
        {
            if (!m_buffer)
            {
                InitBuffer();
            }

            AttachBufferToSlot(EyeAdaptationDataInputOutputSlotName, m_buffer);
        }

        bool EyeAdaptationPass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled() || m_pipeline == nullptr)
            {
                return false;
            }

            JE::Render::RPI::Scene* scene = GetScene();
            bool enabled = false;

            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                JE::Render::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            enabled = true;
                        }
                    }
                }
            }

            return enabled;
        }


        void EyeAdaptationPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::ComputePass::FrameBeginInternal(params);

            JE::Render::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                if (fp)
                {
                    JE::Render::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            settings->UpdateBuffer();
                            view->GetShaderResourceGroup()->SetBufferView(m_exposureControlBufferInputIndex, settings->GetBufferView());
                        }
                    }
                }
            }
        }
    }   // namespace Render
}   // namespace AZ
