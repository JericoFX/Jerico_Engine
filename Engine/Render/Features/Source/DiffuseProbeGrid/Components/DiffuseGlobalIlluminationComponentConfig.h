/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>

#include <AzCore/Component/Component.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <JE/Render/Features/DiffuseProbeGrid/DiffuseGlobalIlluminationFeatureProcessorInterface.h>

namespace JE::Render
{
    namespace Features
    {
        class JE_RENDER_FEATURES_API DiffuseGlobalIlluminationComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(DiffuseGlobalIlluminationComponentConfig, "{0D0835D6-6094-4EF8-BEAC-5FF8A4E4C119}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseGlobalIlluminationComponentConfig, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            DiffuseGlobalIlluminationQualityLevel m_qualityLevel = DiffuseGlobalIlluminationQualityLevel::Low;
        };
    }
}
