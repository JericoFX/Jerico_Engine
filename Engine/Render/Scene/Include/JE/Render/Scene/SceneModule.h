// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once
#include <AzCore/Module/Module.h>

namespace JE::Render::Scene
{
    //! The existing application ModuleManager owns the returned module.
    AZ::Module* CreateModule();
}
