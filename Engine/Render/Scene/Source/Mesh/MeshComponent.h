/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Mesh/MeshComponentController.h>
#include <JE/Render/Scene/Mesh/MeshComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>


namespace JE::Render
{
    namespace Scene
    {
        class MeshComponent final
            : public AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<MeshComponentController, MeshComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::MeshComponent, "AZ::Render::MeshComponent", MeshComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::MeshComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::MeshComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::MeshComponent, AZ::ComponentAllocator);

            MeshComponent() = default;
            explicit MeshComponent(const MeshComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
