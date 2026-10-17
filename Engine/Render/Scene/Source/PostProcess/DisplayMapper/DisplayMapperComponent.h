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
#include <JE/Render/Scene/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>
#include <JE/Render/Scene/PostProcess/DisplayMapper/DisplayMapperComponentConstants.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class DisplayMapperComponent final
            : public AzFramework::Components::ComponentAdapter<DisplayMapperComponentController, DisplayMapperComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DisplayMapperComponentController, DisplayMapperComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::DisplayMapperComponent, "AZ::Render::DisplayMapperComponent", DisplayMapperComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::DisplayMapperComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::DisplayMapperComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::DisplayMapperComponent, AZ::ComponentAllocator);

            DisplayMapperComponent() = default;
            DisplayMapperComponent(const DisplayMapperComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
