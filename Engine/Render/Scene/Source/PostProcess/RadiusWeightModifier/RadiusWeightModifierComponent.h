/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <JE/Render/Scene/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConstants.h>
#include <JE/Render/Scene/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConfig.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class RadiusWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<RadiusWeightModifierComponentController, RadiusWeightModifierComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::RadiusWeightModifierComponent, "AZ::Render::RadiusWeightModifierComponent", "{8F0FC718-50E1-425E-A1E7-9C0425879CEB}", BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::RadiusWeightModifierComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::RadiusWeightModifierComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::RadiusWeightModifierComponent, AZ::ComponentAllocator);

            RadiusWeightModifierComponent() = default;
            RadiusWeightModifierComponent(const RadiusWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
