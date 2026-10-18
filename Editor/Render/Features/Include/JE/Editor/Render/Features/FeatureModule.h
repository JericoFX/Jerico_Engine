// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once
#include <AzCore/Module/Module.h>

namespace JE::Editor::Render::Features
{
    //! Ownership is transferred to the application ModuleManager.
    AZ::Module* CreateModule();
}
