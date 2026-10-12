/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <JE/Render/Integration/RenderIntegrationExports.h>

#include <AzCore/EBus/EBus.h>
namespace JE::Render::Integration
{
        class JE_RENDER_INTEGRATION_API AtomBridgeRequests
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            // Put your public methods here
        };
        using AtomBridgeRequestBus = AZ::EBus<AtomBridgeRequests>;
} // namespace JE::Render::Integration

AZ_DECLARE_EBUS_SINGLE_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::Integration::AtomBridgeRequests);
