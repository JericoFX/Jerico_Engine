/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    class Module;
}

namespace JE::Editor
{
    JE_EDITOR_MATERIALS_API AZ::ComponentDescriptor* CreateEditorMaterialComponentDescriptor();
    JE_EDITOR_MATERIALS_API AZ::ComponentDescriptor* CreateEditorMaterialSystemComponentDescriptor();
    JE_EDITOR_MATERIALS_API AZ::Module* CreateMaterialsModule();
}
