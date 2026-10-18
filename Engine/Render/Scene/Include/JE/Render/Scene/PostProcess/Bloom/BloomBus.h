/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/Bloom/BloomConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class BloomRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::BloomRequests, "AZ::Render::BloomRequests", "{EE86AC6B-D496-4EFD-AFBF-BE06B0C886C0}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~BloomRequests() {}

            // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/Bloom/BloomParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<BloomRequests> BloomRequestBus;
    }
}
