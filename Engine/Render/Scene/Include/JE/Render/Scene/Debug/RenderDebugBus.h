/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/Debug/RenderDebugConstants.h>

namespace JE::Render::Scene
{
    class RenderDebugRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI_WITH_NAME(JE::Render::Scene::RenderDebugRequests, "AZ::Render::RenderDebugRequests", "{A5038F75-F397-42E7-B3B5-8EF7226EE1A4}");

        /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        virtual ~RenderDebugRequests() {}

        // Auto-gen virtual getters/setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

    };

    typedef AZ::EBus<RenderDebugRequests> RenderDebugRequestBus;

}
