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
#include <JE/Render/Scene/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConstants.h>
#include <JE/Render/Scene/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConfig.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class ShapeWeightModifierComponent final
            : public AzFramework::Components::ComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ShapeWeightModifierComponent, "AZ::Render::ShapeWeightModifierComponent", "{0BB6438B-6DD1-4A09-927F-7757D39C940D}", BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::ShapeWeightModifierComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::ShapeWeightModifierComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::ShapeWeightModifierComponent, AZ::ComponentAllocator);

            ShapeWeightModifierComponent() = default;
            ShapeWeightModifierComponent(const ShapeWeightModifierComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
