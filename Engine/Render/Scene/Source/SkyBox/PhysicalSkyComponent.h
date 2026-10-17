/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <JE/Render/Scene/SkyBox/PhysicalSkyComponentConfig.h>
#include <SkyBox/PhysicalSkyComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class PhysicalSkyComponent final
            : public AzFramework::Components::ComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PhysicalSkyComponent, "AZ::Render::PhysicalSkyComponent", AZ::Render::PhysicalSkyComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::PhysicalSkyComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::PhysicalSkyComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::PhysicalSkyComponent, AZ::ComponentAllocator);

            PhysicalSkyComponent() = default;
            PhysicalSkyComponent(const PhysicalSkyComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
