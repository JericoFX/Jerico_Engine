/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <Debug/RayTracingDebugFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ::Render
{
    void RayTracingDebugFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugFeatureProcessor, JE::Render::RPI::FeatureProcessor>()
                ->Version(0)
            ;
            // clang-format on
        }
    }

    RayTracingDebugSettingsInterface* RayTracingDebugFeatureProcessor::GetSettingsInterface()
    {
        return m_settings.get();
    }

    void RayTracingDebugFeatureProcessor::OnRayTracingDebugComponentAdded()
    {
        if (m_debugComponentCount == 0)
        {
            // force a rebuild, so we get a chance to add our debug-pass
            MarkPipelineNeedsRebuild();
        }
        m_debugComponentCount++;
    }

    void RayTracingDebugFeatureProcessor::OnRayTracingDebugComponentRemoved()
    {
        m_debugComponentCount--;
        if (m_debugComponentCount == 0)
        {
            // force a rebuild during which we won't add our debug-pass
            MarkPipelineNeedsRebuild();
        }
    }

    void RayTracingDebugFeatureProcessor::Activate()
    {
        m_settings = AZStd::make_unique<RayTracingDebugSettings>();

        FeatureProcessor::Activate();
        EnableSceneNotification();
    }

    void RayTracingDebugFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
        FeatureProcessor::Deactivate();

        m_sceneSrg = nullptr;
        m_settings.reset();
    }

    void RayTracingDebugFeatureProcessor::AddRenderPasses(JE::Render::RPI::RenderPipeline* pipeline)
    {
        m_pipeline = pipeline;
        if (m_debugComponentCount > 0)
        {
            auto pass = JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromTemplate(Name{ "DebugRayTracingPassTemplate" }, Name{ "DebugRayTracingPass" });
            if (!pass)
            {
                AZ_Assert(false, "Failed to create DebugRayTracingPass");
                return;
            }
            m_pipeline->AddPassAfter(pass, Name{ "AuxGeomPass" });
            m_isEnabled = m_settings->GetEnabled();
            pass->SetEnabled(m_isEnabled);
        }
        FeatureProcessor::AddRenderPasses(pipeline);
    }

    void RayTracingDebugFeatureProcessor::Render(const JE::Render::RPI::FeatureProcessor::RenderPacket& packet)
    {
        if (m_debugComponentCount == 0)
        {
            return;
        }
        if (m_settings->GetEnabled() != m_isEnabled)
        {
            m_isEnabled = m_settings->GetEnabled();
            auto filter = JE::Render::RPI::PassFilter::CreateWithTemplateName(Name{ "DebugRayTracingPassTemplate" }, m_pipeline);
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(
                filter,
                [&](JE::Render::RPI::Pass* pass)
                {
                    pass->SetEnabled(m_isEnabled);
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        if (!m_sceneSrg && m_settings->GetEnabled())
        {
            m_sceneSrg = GetParentScene()->GetFeatureProcessor<RayTracingFeatureProcessor>()->GetRayTracingSceneSrg();
        }

        if (m_sceneSrg && m_settings->GetEnabled())
        {
            if (!m_sceneSrg->SetConstant(m_debugOptionsIndex, m_settings->GetDebugViewMode()))
            {
                AZ_ErrorOnce(
                    "RayTracingDebugFeatureProcessor",
                    m_debugOptionsIndex.IsValid(),
                    "Failed to find shader input index for '%s' in the ray tracing scene SRG.",
                    m_debugOptionsIndex.GetNameForDebug().GetCStr());
            }
        }

        FeatureProcessor::Render(packet);
    }

    void RayTracingDebugFeatureProcessor::MarkPipelineNeedsRebuild()
    {
        if (m_pipeline)
        {
            m_pipeline->MarkPipelinePassChanges(JE::Render::RPI::PipelinePassChanges::PipelineChangedByFeatureProcessor);
        }
    }
} // namespace AZ::Render
