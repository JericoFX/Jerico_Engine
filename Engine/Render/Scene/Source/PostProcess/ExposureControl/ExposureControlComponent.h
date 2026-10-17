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
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlComponentConfig.h>
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <PostProcess/ExposureControl/ExposureControlComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class ExposureControlComponent final
            : public AzFramework::Components::ComponentAdapter<ExposureControlComponentController, ExposureControlComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ExposureControlComponentController, ExposureControlComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ExposureControlComponent, "AZ::Render::ExposureControlComponent", ExposureControlComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::ExposureControlComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::ExposureControlComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::ExposureControlComponent, AZ::ComponentAllocator);

            ExposureControlComponent() = default;
            ExposureControlComponent(const ExposureControlComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
