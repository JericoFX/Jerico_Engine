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
#include <JE/Render/Scene/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConstants.h>
#include <JE/Render/Scene/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConfig.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierController.h>

namespace JE::Render
{
    namespace Scene
    {
        class GradientWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<GradientWeightModifierComponentController, GradientWeightModifierComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::GradientWeightModifierComponent, "AZ::Render::GradientWeightModifierComponent", "{4DE2AD79-85BE-49FF-9DC5-D709720B013E}", BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::GradientWeightModifierComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::GradientWeightModifierComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::GradientWeightModifierComponent, AZ::ComponentAllocator);

            GradientWeightModifierComponent() = default;
            GradientWeightModifierComponent(const GradientWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
