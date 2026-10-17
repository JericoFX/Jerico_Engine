/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <CoreLights/LightCommon.h>

namespace AZ::Render::LightCommon
{
    bool NeedsCPUCulling(
        const JE::Render::RPI::ViewPtr& view,
        const AZStd::unordered_map<const JE::Render::RPI::View*, AZStd::vector<const JE::Render::RPI::RenderPipeline*>>& cpuCulledPipelinesPerView)
    {
        auto viewPipelines = cpuCulledPipelinesPerView.find(view.get());
        if (viewPipelines == cpuCulledPipelinesPerView.end())
        {
            return false;
        }
        return (viewPipelines->second.size() > 0);
    }

    void CacheCPUCulledPipelineInfo(
        JE::Render::RPI::RenderPipeline* renderPipeline,
        JE::Render::RPI::ViewPtr newView,
        JE::Render::RPI::ViewPtr previousView,
        AZStd::unordered_map<const JE::Render::RPI::View*, AZStd::vector<const JE::Render::RPI::RenderPipeline*>>& cpuCulledPipelinesPerView)
    {
        // Check if render pipeline is using GPU culling
        if (renderPipeline->FindFirstPass(AZ::Name("LightCullingPass")))
        {
            return;
        }

        if (previousView)
        {
            erase(cpuCulledPipelinesPerView[previousView.get()], renderPipeline);
        }

        if (newView)
        {
            cpuCulledPipelinesPerView[newView.get()].emplace_back(renderPipeline);
        }
    }

    void UpdateVisibleBuffers(
        const AZStd::string_view& inputBufferName,
        const AZStd::string_view& inputBufferSrgName,
        const AZStd::string_view& inputElementCountSrgName,
        uint32_t inputVisibleBufferUsedCount,
        AZStd::vector<GpuBufferHandler>& outputVisibleBufferHandlers)
    {
        while (inputVisibleBufferUsedCount >= outputVisibleBufferHandlers.size())
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = inputBufferName;
            desc.m_bufferSrgName = inputBufferSrgName;
            desc.m_elementCountSrgName = inputElementCountSrgName;
            desc.m_elementFormat = AZ::RHI::Format::R32_UINT;
            desc.m_srgLayout = JE::Render::RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            outputVisibleBufferHandlers.emplace_back(GpuBufferHandler(desc));
        }
    }

} // namespace AZ::Render::LightCommon

