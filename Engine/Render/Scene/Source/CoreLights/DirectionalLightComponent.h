/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/DirectionalLightComponentController.h>
#include <JE/Render/Scene/CoreLights/CoreLightsConstants.h>
#include <JE/Render/Scene/CoreLights/DirectionalLightComponentConfig.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        class DirectionalLightComponent final
            : public AzFramework::Components::ComponentAdapter<DirectionalLightComponentController, DirectionalLightComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DirectionalLightComponentController, DirectionalLightComponentConfig>;

            AZ_RTTI_WITH_NAME(JE::Render::Scene::DirectionalLightComponent, "AZ::Render::DirectionalLightComponent", DirectionalLightComponentTypeId, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::DirectionalLightComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::DirectionalLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::DirectionalLightComponent, AZ::ComponentAllocator);

            DirectionalLightComponent() = default;
            DirectionalLightComponent(const DirectionalLightComponentConfig& config);
            virtual ~DirectionalLightComponent() override = default;

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
