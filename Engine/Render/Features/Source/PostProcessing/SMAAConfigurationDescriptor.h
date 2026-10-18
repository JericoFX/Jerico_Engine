/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcessing/SMAAFeatureProcessorInterface.h>

namespace JE::Render
{

    namespace Features
    {
        //! A descriptor used to configure the SMAA feature
        struct SMAAConfigurationDescriptor final
        {
            AZ_TYPE_INFO(SMAAConfigurationDescriptor, "{0A546684-7E31-4C00-874C-7DFB3D12A0A6}");
            static void Reflect(AZ::ReflectContext* context);

            //! Configuration name
            AZStd::string m_name;

            uint32_t m_enable;
            uint32_t m_edgeDetectionMode;
            uint32_t m_outputMode;
            uint32_t m_quality;
        };
    } // namespace Features
} // namespace JE::Render
