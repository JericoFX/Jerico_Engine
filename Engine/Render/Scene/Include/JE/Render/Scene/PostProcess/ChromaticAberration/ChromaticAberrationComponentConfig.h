/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class ChromaticAberrationComponentConfig final : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(ChromaticAberrationComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ChromaticAberrationComponentConfig, "AZ::Render::ChromaticAberrationComponentConfig", "{D0E889C8-C0B0-4B88-8672-91A1A2D21EFF}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(AZ::Render::ChromaticAberrationSettingsInterface* settings);
            void CopySettingsTo(AZ::Render::ChromaticAberrationSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
