/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Render/Debug/AtomImGuiToolsBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(JE_RENDER_DEBUG_API, JE::Render::Debug::AtomImGuiToolsRequests);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_RENDER_DEBUG_API, JE::Render::Debug::AtomImGuiMeshCallbacks);
