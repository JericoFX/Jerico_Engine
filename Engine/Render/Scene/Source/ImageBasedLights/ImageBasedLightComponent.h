/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ImageBasedLights/ImageBasedLightComponentController.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        class ImageBasedLightComponent final
            : public AzFramework::Components::ComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<ImageBasedLightComponentController, ImageBasedLightComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ImageBasedLightComponent, "AZ::Render::ImageBasedLightComponent", ImageBasedLightComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::ImageBasedLightComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::ImageBasedLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::ImageBasedLightComponent, AZ::ComponentAllocator);

            ImageBasedLightComponent() = default;
            ImageBasedLightComponent(const ImageBasedLightComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
