/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/PlatformDef.h>
#include <AzCore/Serialization/SerializeContext.h>

#endif

#include <JE/Render/RPI/Public/SceneBus.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadNotificationBus.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::RPI::SceneNotification);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::RPI::ShaderReloadNotifications);
