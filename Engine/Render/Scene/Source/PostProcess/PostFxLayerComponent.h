/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <JE/Render/Scene/PostProcess/PostFxLayerComponentConfig.h>
#include <JE/Render/Scene/PostProcess/PostFxLayerComponentConstants.h>
#include <PostProcess/PostFxLayerComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class PostFxLayerComponent final
            : public AzFramework::Components::ComponentAdapter<PostFxLayerComponentController, PostFxLayerComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<PostFxLayerComponentController, PostFxLayerComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PostFxLayerComponent, "AZ::Render::PostFxLayerComponent", PostFxLayerComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::PostFxLayerComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::PostFxLayerComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::PostFxLayerComponent, AZ::ComponentAllocator);

            PostFxLayerComponent() = default;
            PostFxLayerComponent(const PostFxLayerComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
