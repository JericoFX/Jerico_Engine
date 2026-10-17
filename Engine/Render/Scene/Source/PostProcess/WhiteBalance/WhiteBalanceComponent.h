/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceConstants.h>
#include <PostProcess/WhiteBalance/WhiteBalanceComponentController.h>
#include <JE/Render/Scene/PostProcess/WhiteBalance/WhiteBalanceComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace WhiteBalance
        {
            static constexpr const char* const WhiteBalanceComponentTypeId = "{DC96CC56-1850-4B8A-8E05-C0690EBEB396}";
        }

        class WhiteBalanceComponent final
            : public AzFramework::Components::ComponentAdapter<WhiteBalanceComponentController, WhiteBalanceComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<WhiteBalanceComponentController, WhiteBalanceComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::WhiteBalanceComponent, "AZ::Render::WhiteBalanceComponent", JE::Render::Scene::WhiteBalance::WhiteBalanceComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::WhiteBalanceComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::WhiteBalanceComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::WhiteBalanceComponent, AZ::ComponentAllocator);

            WhiteBalanceComponent() = default;
            WhiteBalanceComponent(const WhiteBalanceComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
