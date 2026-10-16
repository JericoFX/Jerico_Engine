/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Debug/PerformanceCollector.h>
#include <JE/Render/Integration/RenderIntegrationExports.h>

namespace AZ::RPI
{
    class GpuPassProfiler;
}

namespace JE::Render
{
    namespace RPI
    {
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, AZ::CVarFixedString, r_metricsDataLogType);
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, AZ::u32, r_metricsWaitTimePerCaptureBatch);
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, AZ::u32, r_metricsFrameCountPerCaptureBatch);
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, bool, r_metricsMeasureGpuTime);
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, bool, r_metricsQuitUponCompletion);
        AZ_CVAR_API_EXTERNED(JE_RENDER_INTEGRATION_API, AZ::u32, r_metricsNumberOfCaptureBatches);

        // A simple helper function.
        JE_RENDER_INTEGRATION_API AZ::Debug::PerformanceCollector::DataLogType GetDataLogTypeFromCVar(const AZ::CVarFixedString& newCaptureType);

        //! This is a simple interface to help get a reference to an AZ::Debug::PerformanceCollector
        //! so we can feed CVar values into it.
        class IPerformanceCollectorOwner
        {
        public:
            AZ_CLASS_ALLOCATOR(IPerformanceCollectorOwner, AZ::SystemAllocator);
            AZ_RTTI(IPerformanceCollectorOwner, "{D157F48E-8D9C-4F4F-93CE-961860371965}");

            IPerformanceCollectorOwner() = default;
            virtual ~IPerformanceCollectorOwner() = default;

        protected:
            friend class PerformanceCvarManager;
            virtual AZ::Debug::PerformanceCollector* GetPerformanceCollector() { return nullptr; }
            virtual AZ::RPI::GpuPassProfiler* GetGpuPassProfiler() { return nullptr;  }
        };
        using PerformanceCollectorOwner = AZ::Interface<IPerformanceCollectorOwner>;

    } // namespace RPI
} // namespace JE::Render
