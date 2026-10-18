/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <ACES/Aces.h>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace Scene
    {
        class LookModificationComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(LookModificationComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::LookModificationComponentConfig, "AZ::Render::LookModificationComponentConfig", "{604D14B8-6374-4FE0-8F31-03A37B238429}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::LookModificationSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::LookModificationSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }

            bool IsUsingCustomShaper() const {
                return m_shaperPresetType == AZ::Render::ShaperPresetType::LinearCustomRange
                    || m_shaperPresetType == AZ::Render::ShaperPresetType::Log2CustomRange;
            }
        };
    }
}
