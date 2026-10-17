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
#include <SpecularReflections/SpecularReflectionsComponentConfig.h>
#include <SpecularReflections/SpecularReflectionsComponentConstants.h>
#include <SpecularReflections/SpecularReflectionsComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class SpecularReflectionsComponent final
            : public AzFramework::Components::ComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::SpecularReflectionsComponent, "AZ::Render::SpecularReflectionsComponent", SpecularReflectionsComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::SpecularReflectionsComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::SpecularReflectionsComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::SpecularReflectionsComponent, AZ::ComponentAllocator);

            SpecularReflectionsComponent() = default;
            SpecularReflectionsComponent(const SpecularReflectionsComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
