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
#include <AzFramework/Components/ComponentAdapter.h>
#include <Components/DiffuseGlobalIlluminationComponentConfig.h>
#include <Components/DiffuseGlobalIlluminationComponentConstants.h>
#include <Components/DiffuseGlobalIlluminationComponentController.h>

namespace JE::Render
{
    namespace Features
    {
        class JE_RENDER_FEATURES_API DiffuseGlobalIlluminationComponent final
            : public AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DiffuseGlobalIlluminationComponentController, DiffuseGlobalIlluminationComponentConfig>;
            AZ_RTTI_WITH_NAME(DiffuseGlobalIlluminationComponent, "AZ::Render::DiffuseGlobalIlluminationComponent", DiffuseGlobalIlluminationComponentTypeId , BaseClass, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(DiffuseGlobalIlluminationComponent)
            AZ_COMPONENT_BASE(DiffuseGlobalIlluminationComponent)
            AZ_CLASS_ALLOCATOR(DiffuseGlobalIlluminationComponent, AZ::ComponentAllocator);

            DiffuseGlobalIlluminationComponent() = default;
            DiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Features
} // namespace JE::Render
