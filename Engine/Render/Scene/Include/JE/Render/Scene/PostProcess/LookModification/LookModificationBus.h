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
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace Scene
    {
        class LookModificationRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::LookModificationRequests, "AZ::Render::LookModificationRequests", "{F1E49309-8963-4F96-9EF8-BFC791E0DC36}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~LookModificationRequests() {}
            // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        };

        typedef AZ::EBus<LookModificationRequests> LookModificationRequestBus;
    }
}
