/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Render/Integration/AtomBridgeBus.h>
#include <JE/Render/Integration/FlyCameraInputBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::Integration::AtomBridgeRequests);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::Integration::FlyCameraInputInterface);
