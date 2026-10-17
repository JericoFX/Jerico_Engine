/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionConstants.h>
#include <PostProcess/PaniniProjection/PaniniProjectionComponentController.h>
#include <JE/Render/Scene/PostProcess/PaniniProjection/PaniniProjectionComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace PaniniProjection
        {
            static constexpr const char* const PaniniProjectionComponentTypeId = "{87B77D17-1C0D-494B-88A2-15CB136BD9E0}";
        }

        class PaniniProjectionComponent final
            : public AzFramework::Components::ComponentAdapter<PaniniProjectionComponentController, PaniniProjectionComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<PaniniProjectionComponentController, PaniniProjectionComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PaniniProjectionComponent, "AZ::Render::PaniniProjectionComponent", JE::Render::Scene::PaniniProjection::PaniniProjectionComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::PaniniProjectionComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::PaniniProjectionComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::PaniniProjectionComponent, AZ::ComponentAllocator);

            PaniniProjectionComponent() = default;
            PaniniProjectionComponent(const PaniniProjectionComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
