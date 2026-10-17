/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <JE/Render/RPI/Public/Pass/CopyPass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/Specific/SwapChainPass.h>
#include <JE/Render/RPI/Public/PipelinePassChanges.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Pass/CopyPassData.h>

#include <AFR/AFRFeatureProcessor.h>

namespace AFR
{
    void AFRFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AFRFeatureProcessor, JE::Render::RPI::FeatureProcessor>()->Version(0);
        }
    }

    void AFRFeatureProcessor::Activate()
    {
        // Check if AFR should be active
        m_afrPipelineName = AZ::RHI::GetCommandLineValue("afr");
        if (m_afrPipelineName.empty())
        {
            if (!AZ::RHI::QueryCommandLineOption("afr"))
                return;
        }

        m_deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();

        // Enable scene notifications to get notified on RenderPipeline changes and add CopyPasses as needed
        EnableSceneNotification();

        // Connect to the FrameEventBus to access the OnFrameBegin() event to affect the scheduling of passes
        AZ::RHI::FrameEventBus::Handler::BusConnect(
            AZ::RHI::RHISystemInterface::Get()->GetDevice(AZ::RHI::MultiDevice::DefaultDeviceIndex));
    }

    void AFRFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        AZ::RHI::FrameEventBus::Handler::BusDisconnect();
    }

    void AFRFeatureProcessor::OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* renderPipeline, RenderPipelineChangeType pipelineChangeType)
    {
        switch (pipelineChangeType)
        {
        case RenderPipelineChangeType::Added:
            {
                if (renderPipeline->GetId().GetStringView().contains(m_afrPipelineName))
                {
                    if(m_afrRenderPipeline.second)
                    {
                        AZ_Assert(!m_afrPipelineName.empty(), "AFR Pipeline had already been set, reset to this pipeline");
                        m_afrRenderPipeline = { JE::Render::RPI::RenderPipelineId(""), nullptr };
                        m_afrCopyPasses.clear();
                    }

                    JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("CopyToSwapChain"), renderPipeline);

                    JE::Render::RPI::Pass* copyToSwapchainPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

                    if (!copyToSwapchainPass)
                    {
                        // Sanity check, can only setup AFR in a pipeline that copies to the SwapChain in the end
                        AZ_Error(
                            "AFRFeatureProcessor",
                            m_afrPipelineName.empty(),
                            "The given RenderPipeline does not have a CopyToSwapChain pass!");
                        return;
                    }

                    // Remember this pipeline as the one AFR is running on
                    m_afrPipelineName = renderPipeline->GetId().GetStringView();
                    m_afrRenderPipeline = { renderPipeline->GetId(), renderPipeline };
                    CollectPassesToSchedule(renderPipeline);
                }

                return;
            }
        case RenderPipelineChangeType::PassChanged:
            {
                if(m_afrSetupState == AFRSetupState::INITIALIZING)
                {
                    // We triggered this update, no need to collect passes -> return
                    m_afrSetupState = AFRSetupState::SETUP_DONE;
                    return;
                }
                else if (renderPipeline->GetId() == m_afrRenderPipeline.first)
                {
                    CollectPassesToSchedule(renderPipeline);
                }

                return;
            }
        case RenderPipelineChangeType::Removed:
            {
                if (renderPipeline->GetId() == m_afrRenderPipeline.first)
                {
                    // Clear CopyPasses
                    m_afrRenderPipeline = { JE::Render::RPI::RenderPipelineId(""), nullptr };
                    m_afrCopyPasses.clear();
                    m_scheduledPasses.clear();
                    m_afrSetupState = AFRSetupState::NOT_INITIALIZED;
                    m_afrPipelineName = "";
                }

                return;
            }
        }
    }

    void AFRFeatureProcessor::CollectPassesToSchedule(JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        if(m_afrSetupState == AFRSetupState::SCHEDULING)
        {
            // To detect valid passes, reset our previous scheduling for now
            for(const auto& pass : m_scheduledPasses)
            {
                pass->SetDeviceIndex(AZ::RHI::MultiDevice::InvalidDeviceIndex);
            }
        }
        m_scheduledPasses.clear();

        AZStd::stack<JE::Render::RPI::Pass*> stack;
        stack.push(renderPipeline->GetRootPass().get());
        while (!stack.empty())
        {
            auto pass{ stack.top() };
            stack.pop();
            auto parentPass{ pass->AsParent() };

            // If pass is a ParentPass, go over all children
            if (parentPass != nullptr)
            {
                for (auto it{ parentPass->GetChildren().rbegin() }; it != parentPass->GetChildren().rend(); it++)
                {
                    stack.push(it->get());
                }
            }
            else
            {
                // We do not want to schedule the "CopyToSwapChain" pass or any pass that is
                // already scheduled to a fixed device index
                if (!pass->GetName().GetStringView().contains("CopyToSwapChain") &&
                    pass->GetDeviceIndex() == AZ::RHI::MultiDevice::InvalidDeviceIndex)
                {
                    m_scheduledPasses.emplace_back(pass);
                }
            }
        }
    }

    bool IsEnabled(JE::Render::RPI::Pass* pass)
    {
        if(!pass->IsEnabled())
        {
            return false;
        }
        // Check if parent (or its parent etc.) is enabled
        if (auto parentPass{pass->GetParent()}; parentPass != nullptr)
        {
            if(!parentPass->IsEnabled())
            {
                return false;
            }
            IsEnabled(parentPass);
        }
        return true;
    }

    void AFRFeatureProcessor::OnFrameBegin()
    {
        static int frameNumber{ 0 };

        if (auto& [_, renderPipeline]{ m_afrRenderPipeline }; renderPipeline && renderPipeline->NeedsRender())
        {
            if(m_afrSetupState == AFRSetupState::NOT_INITIALIZED)
            {
                // Add CopyPasses for all devices (except for the first, which is the display GPU)
                for(auto deviceIndex{1}; deviceIndex < m_deviceCount; ++deviceIndex)
                {
                    AddAFRCopyPass(renderPipeline, deviceIndex);
                }
                m_afrSetupState = AFRSetupState::INITIALIZING;
                // This will trigger a "OnRenderPipelineChanged::PassChanged" event
                renderPipeline->UpdatePasses();
            }

            auto deviceIndex{ frameNumber % m_deviceCount };
            for (auto pass : m_scheduledPasses)
            {
                if(IsEnabled(pass.get()))
                {
                    pass->SetDeviceIndex(deviceIndex);
                }
            }

            // Enable appropriate CopyPass for deviceIndex and disable rest
            for (auto& [index, copyPass] : m_afrCopyPasses)
            {
                copyPass->SetEnabled(deviceIndex == index);
            }

            m_afrSetupState = AFRSetupState::SCHEDULING;
        }

        ++frameNumber;
    }
    void AFRFeatureProcessor::AddAFRCopyPass(JE::Render::RPI::RenderPipeline* renderPipeline, int deviceIndex)
    {
        if (m_afrCopyPasses.contains(deviceIndex))
        {
            // Already setup
            JE::Render::RPI::PassFilter passFilter =
                JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("SwapchainMultiDeviceCopyPass"+ AZStd::to_string(deviceIndex)) , renderPipeline);

            JE::Render::RPI::Pass* pass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (m_afrRenderPipeline.second != renderPipeline || pass)
            {
                return;
            }
        }

        JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("CopyToSwapChain"), renderPipeline);

        JE::Render::RPI::Pass* copyToSwapchainPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

        if (!copyToSwapchainPass)
        {
            // Sanity check, can only setup AFR in a pipeline that copies to the SwapChain in the end
            return;
        }

        // Add a multi-device CopyPass to copy from deviceIndex to MultiDevice::DefaultDeviceIndex

        AZStd::shared_ptr<JE::Render::RPI::PassRequest> passRequest = AZStd::make_shared<JE::Render::RPI::PassRequest>();

        passRequest->m_templateName = AZ::Name("MultiDeviceCopyPassTemplate");

        passRequest->m_passName = AZ::Name("SwapchainMultiDeviceCopyPass" + AZStd::to_string(deviceIndex));

        AZStd::shared_ptr<JE::Render::RPI::CopyPassData> passData = AZStd::make_shared<JE::Render::RPI::CopyPassData>();
        passData->m_sourceDeviceIndex = deviceIndex;
        passData->m_destinationDeviceIndex = AZ::RHI::MultiDevice::DefaultDeviceIndex;
        passData->m_cloneInput = false;
        passRequest->m_passData = passData;

        JE::Render::RPI::PassConnection passConnection;

        passConnection.m_localSlot = AZ::Name{ "InputOutput" };

        // This works as long as this connection is built with a pass request and not a template (which is fine as it isn't done this
        // way for the CopyToSwapChain pass):
        [[maybe_unused]] bool found{ false };

        for (auto& connection : copyToSwapchainPass->GetPassDescriptor().m_passRequest->m_connections)
        {
            if (connection.m_localSlot.GetStringView() == AZ::Name{ "Input" }.GetStringView())
            {
                passConnection.m_attachmentRef = connection.m_attachmentRef;
                found = true;
                break;
            }
        }

        AZ_Assert(found, "Connection not found!");

        passRequest->m_connections.emplace_back(passConnection);

        auto afrCopyPass{ JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest.get()) };
        m_afrCopyPasses[deviceIndex] = afrCopyPass;

        auto* parent{ copyToSwapchainPass->GetParent() };
        parent->InsertChild(afrCopyPass, parent->FindChildPassIndex(copyToSwapchainPass->GetName()).GetIndex());
    }
} // namespace AFR
