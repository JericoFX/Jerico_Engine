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
#include <JE/Render/Scene/ScreenSpace/DeferredFogComponentConfig.h>
#include <ScreenSpace/DeferredFogComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace DeferredFog
        {
            inline constexpr AZ::TypeId DeferredFogComponentTypeId{ "{9492DC07-B3F7-4DF2-88FA-E4EEF1DD98E3}" };
        }

        class DeferredFogComponent final
            : public AzFramework::Components::ComponentAdapter<DeferredFogComponentController, DeferredFogComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DeferredFogComponentController, DeferredFogComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::DeferredFogComponent, "AZ::Render::DeferredFogComponent", JE::Render::Scene::DeferredFog::DeferredFogComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::DeferredFogComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::DeferredFogComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::DeferredFogComponent, AZ::ComponentAllocator);

            DeferredFogComponent() = default;
            DeferredFogComponent(const DeferredFogComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
