/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Render/Materials/Base.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    // Called by the material component's existing reflection lifecycle.
    JE_RENDER_MATERIALS_API void ReflectMaterialTypes(AZ::ReflectContext* context);
    JE_RENDER_MATERIALS_API AZ::ComponentDescriptor* CreateMaterialComponentDescriptor();
}
