/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Grid/GridComponentController.h>
#include <JE/Render/Scene/Grid/GridComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        class GridComponent final
            : public AzFramework::Components::ComponentAdapter<GridComponentController, GridComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<GridComponentController, GridComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::GridComponent, "AZ::Render::GridComponent", GridComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::GridComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::GridComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::GridComponent, AZ::ComponentAllocator);

            GridComponent() = default;
            GridComponent(const GridComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
