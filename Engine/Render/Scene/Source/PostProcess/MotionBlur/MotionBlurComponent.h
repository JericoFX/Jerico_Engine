/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurConstants.h>
#include <PostProcess/MotionBlur/MotionBlurComponentController.h>
#include <JE/Render/Scene/PostProcess/MotionBlur/MotionBlurComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace MotionBlur
        {
            static constexpr const char* const MotionBlurComponentTypeId = "{87B77D17-1C0D-494B-88A2-1538136BD9E0}";
        }

        class MotionBlurComponent final
            : public AzFramework::Components::ComponentAdapter<MotionBlurComponentController, MotionBlurComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<MotionBlurComponentController, MotionBlurComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::MotionBlurComponent, "AZ::Render::MotionBlurComponent", JE::Render::Scene::MotionBlur::MotionBlurComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::MotionBlurComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::MotionBlurComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::MotionBlurComponent, AZ::ComponentAllocator);

            MotionBlurComponent() = default;
            MotionBlurComponent(const MotionBlurComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
