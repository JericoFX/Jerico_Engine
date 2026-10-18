/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/PostProcess/FilmGrain/FilmGrainComponentConfig.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void FilmGrainComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FilmGrainComponentConfig, AZ::ComponentConfig>()->Version(0)

                // Auto-gen serialize context code...
#define SERIALIZE_CLASS FilmGrainComponentConfig
#include <JE/Render/Features/ParamMacros/StartParamSerializeContext.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void FilmGrainComponentConfig::CopySettingsFrom(JE::Render::Features::FilmGrainSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsFrom.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void FilmGrainComponentConfig::CopySettingsTo(JE::Render::Features::FilmGrainSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsTo.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ
