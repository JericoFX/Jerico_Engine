/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <JE/Render/RPI/Reflect/Configuration.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        //! A descriptor used to create a render pipeline
        struct JE_RENDER_INTEGRATION_API GpuQuerySystemDescriptor final
        {
            AZ_TYPE_INFO(GpuQuerySystemDescriptor, "{952FE9DA-96EA-473E-BED5-1B12088BABD0}");
            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_occlusionQueryCount = 128u;
            uint32_t m_statisticsQueryCount = 256u;
            uint32_t m_timestampQueryCount = 256u;

            AZ::RHI::PipelineStatisticsFlags m_statisticsQueryFlags =
                AZ::RHI::PipelineStatisticsFlags::IAVertices    | AZ::RHI::PipelineStatisticsFlags::IAPrimitives    |
                AZ::RHI::PipelineStatisticsFlags::VSInvocations | AZ::RHI::PipelineStatisticsFlags::CInvocations    |
                AZ::RHI::PipelineStatisticsFlags::CPrimitives   | AZ::RHI::PipelineStatisticsFlags::PSInvocations   |
                AZ::RHI::PipelineStatisticsFlags::CSInvocations;
        }; 

    }

namespace AZ
{; // namespace RPI 
}; //namespace AZ
