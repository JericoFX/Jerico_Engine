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
#include <JE/Render/Scene/Debug/RenderDebugComponentConfig.h>
#include <Atom/Feature/Debug/RenderDebugConstants.h>
#include <Debug/RenderDebugComponentController.h>

namespace JE::Render::Scene
{
    namespace RenderDebug
    {
        inline constexpr AZ::TypeId RenderDebugComponentTypeId{ "{98A72F68-3DA3-451A-BC79-707370EE4AC0}" };
    }

    class RenderDebugComponent final
        : public AzFramework::Components::ComponentAdapter<RenderDebugComponentController, RenderDebugComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<RenderDebugComponentController, RenderDebugComponentConfig>;
        AZ_RTTI_WITH_NAME(JE::Render::Scene::RenderDebugComponent, "AZ::Render::RenderDebugComponent", JE::Render::Scene::RenderDebug::RenderDebugComponentTypeId , BaseClass, AZ::Component)
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::RenderDebugComponent)
        AZ_COMPONENT_BASE(JE::Render::Scene::RenderDebugComponent)
        AZ_CLASS_ALLOCATOR(JE::Render::Scene::RenderDebugComponent, AZ::ComponentAllocator);

        RenderDebugComponent() = default;
        RenderDebugComponent(const RenderDebugComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };

}
