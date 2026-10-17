/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class HDRColorGradingRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::HDRColorGradingRequests, "AZ::Render::HDRColorGradingRequests", "{E414A96B-0AA7-4574-ABC0-968B1F5CEE56}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~HDRColorGradingRequests() {}
            // Auto-gen virtual getters/setters...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

        typedef AZ::EBus<HDRColorGradingRequests> HDRColorGradingRequestBus;
    } // namespace Render
} // namespace AZ
