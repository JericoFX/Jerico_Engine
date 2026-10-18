/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/PostProcessSettingsInterface.h>

namespace JE::Render
{
    namespace Scene
    {
        class PostFxLayerRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PostFxLayerRequests, "AZ::Render::PostFxLayerRequests", "{30A215EB-8D51-43A9-B01E-8115E43636C6}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~PostFxLayerRequests() {}

            // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/PostProcessParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<PostFxLayerRequests> PostFxLayerRequestBus;
    }
}
