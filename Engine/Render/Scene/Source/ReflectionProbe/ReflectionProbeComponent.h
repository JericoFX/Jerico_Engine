/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ReflectionProbe/ReflectionProbeComponentController.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        class ReflectionProbeComponent final
            : public AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<ReflectionProbeComponentController, ReflectionProbeComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ReflectionProbeComponent, "AZ::Render::ReflectionProbeComponent", ReflectionProbeComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::ReflectionProbeComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::ReflectionProbeComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::ReflectionProbeComponent, AZ::ComponentAllocator);

            ReflectionProbeComponent() = default;
            ReflectionProbeComponent(const ReflectionProbeComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
