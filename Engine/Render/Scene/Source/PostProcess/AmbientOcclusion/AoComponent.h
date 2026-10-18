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
#include <JE/Render/Scene/PostProcess/AmbientOcclusion/AoComponentConfiguration.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoConstants.h>
#include <PostProcess/AmbientOcclusion/AoComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace Ao
        {
            inline constexpr AZ::TypeId AoComponentTypeId{ "{F1203F4B-89B6-409E-AB99-B9CC87AABC2E}" };
        }

        class AoComponent final
            : public AzFramework::Components::ComponentAdapter<AoComponentController, AoComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<AoComponentController, AoComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::AoComponent, "AZ::Render::AoComponent", JE::Render::Scene::Ao::AoComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::AoComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::AoComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::AoComponent, AZ::ComponentAllocator);

            AoComponent() = default;
            AoComponent(const AoComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
