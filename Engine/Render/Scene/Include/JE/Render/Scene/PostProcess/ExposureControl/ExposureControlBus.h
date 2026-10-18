/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class ExposureControlRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ExposureControlRequests, "AZ::Render::ExposureControlRequests", "{2FAF1C0B-9306-4ECA-971B-1A17F9170F5B}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~ExposureControlRequests() {}

            // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<ExposureControlRequests> ExposureControlRequestBus;
    }
}
