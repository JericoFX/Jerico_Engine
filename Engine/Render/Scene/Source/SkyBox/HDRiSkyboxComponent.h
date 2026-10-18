/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <JE/Render/Features/SkyBox/SkyboxConstants.h>
#include <JE/Render/Scene/SkyBox/HDRiSkyboxComponentConfig.h>
#include <SkyBox/HDRiSkyboxComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class HDRiSkyboxComponent final
            : public AzFramework::Components::ComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::HDRiSkyboxComponent, "AZ::Render::HDRiSkyboxComponent", JE::Render::Features::HDRiSkyboxComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::HDRiSkyboxComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::HDRiSkyboxComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::HDRiSkyboxComponent, AZ::ComponentAllocator);

            HDRiSkyboxComponent() = default;
            HDRiSkyboxComponent(const HDRiSkyboxComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
