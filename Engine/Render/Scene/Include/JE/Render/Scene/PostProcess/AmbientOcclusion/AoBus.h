/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoSettingsInterface.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoConstants.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class AoRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::AoRequests, "AZ::Render::AoRequests", "{6F518EA6-D912-44C0-A79E-ABC16A30D2F7}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~AoRequests() {}

            // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <JE/Render/Features/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<AoRequests> AoRequestBus;
    }
}
