/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <CoreLights/AreaLightComponentController.h>
#include <JE/Render/Scene/CoreLights/CoreLightsConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class AreaLightComponent final
            : public AzFramework::Components::ComponentAdapter<AreaLightComponentController, AreaLightComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<AreaLightComponentController, AreaLightComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::AreaLightComponent, "AZ::Render::AreaLightComponent", AreaLightComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::AreaLightComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::AreaLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::AreaLightComponent, AZ::ComponentAllocator);

            AreaLightComponent() = default;
            AreaLightComponent(const AreaLightComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
