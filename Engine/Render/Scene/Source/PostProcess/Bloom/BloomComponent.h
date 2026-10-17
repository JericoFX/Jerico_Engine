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
#include <JE/Render/Scene/PostProcess/Bloom/BloomComponentConfig.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>
#include <PostProcess/Bloom/BloomComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace Bloom
        {
            inline constexpr AZ::TypeId BloomComponentTypeId{ "{0D38705D-525D-4BA7-A805-26E3E9093F54}" };
        }

        class BloomComponent final
            : public AzFramework::Components::ComponentAdapter<BloomComponentController, BloomComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<BloomComponentController, BloomComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::BloomComponent, "AZ::Render::BloomComponent", JE::Render::Scene::Bloom::BloomComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::BloomComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::BloomComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::BloomComponent, AZ::ComponentAllocator);

            BloomComponent() = default;
            BloomComponent(const BloomComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
