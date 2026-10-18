/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/Vignette/VignetteConstants.h>
#include <PostProcess/Vignette/VignetteComponentController.h>
#include <JE/Render/Scene/PostProcess/Vignette/VignetteComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        namespace Vignette
        {
            static constexpr const char* const VignetteComponentTypeId = "{93C2AD53-4722-4B33-BB23-BDBC1D423289}";
        }

        class VignetteComponent final
            : public AzFramework::Components::ComponentAdapter<VignetteComponentController, VignetteComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<VignetteComponentController, VignetteComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::VignetteComponent, "AZ::Render::VignetteComponent", JE::Render::Scene::Vignette::VignetteComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::VignetteComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::VignetteComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::VignetteComponent, AZ::ComponentAllocator);

            VignetteComponent() = default;
            VignetteComponent(const VignetteComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
