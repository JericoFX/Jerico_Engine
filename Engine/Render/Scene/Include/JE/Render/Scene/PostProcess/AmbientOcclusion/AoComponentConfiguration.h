/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoSettingsInterface.h>

namespace JE::Render
{
    namespace Scene
    {
        class AoComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(AoComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::AoComponentConfig, "AZ::Render::AoComponentConfig", "{C1ACBBE7-C3BD-4245-B571-3B5FDCAC0B0B}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(AZ::Render::AoSettingsInterface* settings);
            void CopySettingsTo(AZ::Render::AoSettingsInterface* settings);

            bool IsSsaoInactive();
            bool IsGtaoInactive();
        };
    }
}
