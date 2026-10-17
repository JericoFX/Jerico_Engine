/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/PostProcess/LookModification/LookModificationComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <ACES/Aces.h>

namespace JE::Render
{
    namespace Scene
    {
        void LookModificationComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LookModificationComponentConfig, AZ::ComponentConfig>()
                    ->Version(0)

                    // Auto-gen serialize context code...
#define SERIALIZE_CLASS LookModificationComponentConfig
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void LookModificationComponentConfig::CopySettingsFrom(AZ::Render::LookModificationSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void LookModificationComponentConfig::CopySettingsTo(AZ::Render::LookModificationSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ
