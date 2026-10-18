/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/RasterPass.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicDrawInterface.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Silhouette/SilhouetteFeatureProcessor.h>

AZ_CVAR(
    bool,
    r_silhouette,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Controls if the silhouette rendering feature is active.  0 : Inactive,  1 : Active (default)");

namespace JE::Render::Features
{
    SilhouetteFeatureProcessor::SilhouetteFeatureProcessor() = default;
    SilhouetteFeatureProcessor::~SilhouetteFeatureProcessor() = default;

    void SilhouetteFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SilhouetteFeatureProcessor, FeatureProcessor>()->Version(0);
        }
    }

    void SilhouetteFeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }

    void SilhouetteFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        m_rasterPass = nullptr;
        m_compositePass = nullptr;
    }

    void SilhouetteFeatureProcessor::SetPassesEnabled(bool enabled)
    {
        if (m_compositePass && m_rasterPass)
        {
            m_compositePass->SetEnabled(enabled);
            m_rasterPass->SetEnabled(enabled);
        }
    }

    void SilhouetteFeatureProcessor::OnRenderEnd()
    {
        SetPassesEnabled(r_silhouette);
    }

    void SilhouetteFeatureProcessor::AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        // Early return if pass is already found in render pipeline or if the pipeline is not the default one.
        if (renderPipeline->GetViewType() != JE::Render::RPI::ViewType::Default)
        {
            return;
        }

        // the silhouette passes are already added in another render pipeline
        if (m_renderPipeline && m_renderPipeline != renderPipeline)
        {
            return;
        }

        UpdatePasses(renderPipeline);

        // if we already have valid render pass members then we don't need to dynamically
        // create them and add them to the pipeline
        if (m_compositePass && m_rasterPass)
        {
            return;
        }

        // unset the render pipeline until we add the passes successfully 
        m_renderPipeline = nullptr;

        const auto mergeTemplateName = AZ::Name("SilhouettePassTemplate");
        const auto gatherTemplateName = AZ::Name("SilhouetteGatherPassTemplate");

        AZ::Name postProcessPassName = AZ::Name("PostProcessPass");
        if (renderPipeline->FindFirstPass(postProcessPassName) == nullptr)
        {
            AZ_Warning("SilhouetteFeatureProcessor", false, "Can't find %s in the render pipeline.", postProcessPassName.GetCStr());
            return;
        }

        AZ::Name forwardProcessPassName = AZ::Name("Forward");
        if (renderPipeline->FindFirstPass(forwardProcessPassName) == nullptr)
        {
            AZ_Warning("SilhouetteFeatureProcessor", false, "Can't find %s in the render pipeline.", forwardProcessPassName.GetCStr());
            return;
        }

        // Add the gather pass which draws all the silhouette objects into a render target
        // using depth and stencil to determine where to draw
        JE::Render::RPI::PassRequest gatherPassRequest;
        gatherPassRequest.m_passName = AZ::Name("SilhouetteGatherPass");
        gatherPassRequest.m_templateName = gatherTemplateName;
        gatherPassRequest.m_passEnabled = r_silhouette;
        gatherPassRequest.AddInputConnection(JE::Render::RPI::PassConnection{
            AZ::Name("DepthStencilInputOutput"), JE::Render::RPI::PassAttachmentRef{ forwardProcessPassName, AZ::Name("DepthStencilInputOutput") } });

        if (auto pass = JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromRequest(&gatherPassRequest); pass != nullptr)
        {
            m_rasterPass = static_cast<JE::Render::RPI::RasterPass*>(pass.get());
            renderPipeline->AddPassAfter(pass, forwardProcessPassName);
        }

        // Add the full screen silhouette pass which merges the silhouettes render target with
        // the framebuffer diffuse, and adds outlines to the silhouette shapes
        JE::Render::RPI::PassRequest compositePassRequest;
        compositePassRequest.m_passName = AZ::Name("SilhouettePass");
        compositePassRequest.m_templateName = mergeTemplateName;
        compositePassRequest.m_passEnabled = r_silhouette;
        compositePassRequest.AddInputConnection(
            JE::Render::RPI::PassConnection{ AZ::Name("InputOutput"), JE::Render::RPI::PassAttachmentRef{ postProcessPassName, AZ::Name("Output") } });

        if (auto pass = JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromRequest(&compositePassRequest); pass != nullptr)
        {
            m_compositePass = pass.get();
            renderPipeline->AddPassAfter(pass, postProcessPassName);
        }

        // remember which render pipeline we added our passes to
        m_renderPipeline = renderPipeline;
    }

    void SilhouetteFeatureProcessor::OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* pipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        // Only need to recache silhouette passes if the pipeline had them
        if (pipeline == m_renderPipeline)
        {
            if (changeType == JE::Render::RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                UpdatePasses(nullptr);
            }
            else
            {
                UpdatePasses(pipeline);
            }
        }
    }

    bool SilhouetteFeatureProcessor::NeedsCompositePass() const
    {
        if (auto scene = GetParentScene(); scene != nullptr)
        {
            if (m_rasterPass && m_rasterPass->GetRenderPipeline())
            {
                // Get DrawList from the dynamic draw interface and view
                AZStd::vector<AZ::RHI::DrawListView> drawLists = JE::Render::RPI::DynamicDrawInterface::Get()->GetDrawListsForPass(m_rasterPass);
                const AZStd::vector<JE::Render::RPI::ViewPtr>& views =
                    m_rasterPass->GetRenderPipeline()->GetViews(m_rasterPass->GetPipelineViewTag());
                AZ::RHI::DrawListView viewDrawList;
                if (!views.empty())
                {
                    const JE::Render::RPI::ViewPtr& view = views.front();
                    viewDrawList = view->GetDrawList(m_rasterPass->GetDrawListTag());
                }

                return !(drawLists.empty() && viewDrawList.empty());
            }
        }
        return false;
    }

    void SilhouetteFeatureProcessor::UpdatePasses(JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        m_compositePass = nullptr;
        m_rasterPass = nullptr;

        if (renderPipeline == nullptr)
        {
            m_renderPipeline = nullptr;
            return;
        }

        const auto mergeTemplateName = AZ::Name("SilhouettePassTemplate");
        auto compositePassFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(mergeTemplateName, renderPipeline);
        if (auto foundPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(compositePassFilter); foundPass)
        {
            m_compositePass = foundPass;
        }

        const auto gatherTemplateName = AZ::Name("SilhouetteGatherPassTemplate");
        auto gatherPassFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(gatherTemplateName, renderPipeline);
        if (auto foundPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(gatherPassFilter); foundPass)
        {
            m_rasterPass = static_cast<JE::Render::RPI::RasterPass*>(foundPass);
        }

        // remember which render pipeline we found our passes on
        m_renderPipeline = (m_compositePass && m_rasterPass) ? renderPipeline : nullptr;
    }
} // namespace JE::Render::Features
