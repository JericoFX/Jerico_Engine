/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MeshletsDispatchItem.h>

#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

#include <limits>

#include "MeshletsAssets.h"

namespace AZ
{
    namespace Meshlets
    {
        #define MESHLETS_THREAD_GROUP_SIZE 64

        MeshletsDispatchItem::~MeshletsDispatchItem()
        {
        }

        void MeshletsDispatchItem::InitDispatch(
            JE::Render::RPI::Shader* shader,
            Data::Instance<JE::Render::RPI::ShaderResourceGroup> meshletsDataSrg,
            uint32_t meshletsAmount )
        {
            RHI::DispatchDirect dispatchArgs(
                meshletsAmount, 1, 1,
                MESHLETS_THREAD_GROUP_SIZE, 1, 1
            );

            m_dispatchItem.SetArguments(dispatchArgs);
            AZStd::array<const RHI::ShaderResourceGroup*, 1> srgs { meshletsDataSrg->GetRHIShaderResourceGroup() };
            m_dispatchItem.SetShaderResourceGroups(srgs);       // Per pass srg can be added by the individual passes
            m_meshletsDataSrg = meshletsDataSrg;    // can also be retrieve directly from the m_dispatchItem

            m_shader = shader;
            if (shader)
            {
                RHI::PipelineStateDescriptorForDispatch pipelineDesc;
                m_shader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(pipelineDesc);
                m_dispatchItem.SetPipelineState(m_shader->AcquirePipelineState(pipelineDesc));
            }
        }

        void MeshletsDispatchItem::SetPipelineState(JE::Render::RPI::Shader* shader)
        {
            m_shader = shader;
            if (shader)
            {
                RHI::PipelineStateDescriptorForDispatch pipelineDesc;
                m_shader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(pipelineDesc);
                m_dispatchItem.SetPipelineState(m_shader->AcquirePipelineState(pipelineDesc));
            }
        }


    } // namespace Meshlets
} // namespace AZ
