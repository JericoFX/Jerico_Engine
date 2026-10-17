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
#include <PostProcess/ColorGrading/HDRColorGradingComponentController.h>
#include <JE/Render/Scene/PostProcess/ColorGrading/HDRColorGradingComponentConfig.h>

namespace JE::Render
{
    namespace Scene
    {
        class HDRColorGradingComponent final
            : public AzFramework::Components::ComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::HDRColorGradingComponent, "AZ::Render::HDRColorGradingComponent", "{51968E0F-4DF0-4851-8405-388CBB15B573}", BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::HDRColorGradingComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::HDRColorGradingComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::HDRColorGradingComponent, AZ::ComponentAllocator);

            HDRColorGradingComponent() = default;
            HDRColorGradingComponent(const HDRColorGradingComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
