/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/ProjectManager/ProjectManager.h>

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
    const auto projectRootPath = AzFramework::ProjectManager::FindProjectPath(argc, argv);
    if (projectRootPath.empty())
    {
        constexpr const char* message = "No project path was provided or detected.\nPlease provide a --project-path argument.";
        AZ_Error("Editor", false, "%s", message);
        AZ::Utils::NativeErrorMessageBox("Project not found", message);
        return 2;
    }
    const AZ::IO::FixedMaxPath projectJsonPath = projectRootPath / "project.json";
    if (!AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
    {
        char message[2048];
        azsnprintf(message, sizeof(message), "No project was found at '%s'.\nPlease provide a valid --project-path argument.",
            projectJsonPath.c_str());
        AZ_Error("Editor", false, "%s", message);
        AZ::Utils::NativeErrorMessageBox("Project not found", message);
        return 2;
    }
    using CryEditMain = int (*)(int, char*[]);
    constexpr const char CryEditMainName[] = "CryEditMain";

    auto handle = AZ::DynamicModuleHandle::Create("EditorLib");
    [[maybe_unused]] const bool loaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
    AZ_Assert(loaded, "EditorLib could not be loaded");

    int ret = 1;
    if (auto fn = handle->GetFunction<CryEditMain>(CryEditMainName); fn != nullptr)
    {
        ret = AZStd::invoke(fn, argc, argv);
    }

    handle = {};
    return ret;
}
