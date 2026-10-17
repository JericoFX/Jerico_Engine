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
#include <JE/Render/Scene/PostProcess/LookModification/LookModificationComponentConfig.h>
#include <JE/Render/Scene/PostProcess/LookModification/LookModificationComponentConstants.h>
#include <PostProcess/LookModification/LookModificationComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class LookModificationComponent final
            : public AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<LookModificationComponentController, LookModificationComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::LookModificationComponent, "AZ::Render::LookModificationComponent", LookModificationComponentTypeId , BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::LookModificationComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::LookModificationComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::LookModificationComponent, AZ::ComponentAllocator);

            LookModificationComponent() = default;
            LookModificationComponent(const LookModificationComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
