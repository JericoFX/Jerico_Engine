/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>

namespace JE::Render
{
    namespace Scene
    {
        class DisplayMapperComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(DisplayMapperComponentConfig, "{98560BEC-EA7F-4600-AA55-F76FDC07E950}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(DisplayMapperComponentConfig, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AZ::Render::DisplayMapperOperationType m_displayMapperOperation = AZ::Render::DisplayMapperOperationType::Aces;
            bool m_ldrColorGradingLutEnabled = false;
            AZ::Data::Asset<JE::Render::RPI::AnyAsset> m_ldrColorGradingLut = {};
            AZ::Render::AcesParameterOverrides m_acesParameterOverrides;
        };
    }
}
