-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT

local registryKeys = {
    "/O3DE/Performance/FrameTimeRecording/Activate",
    "/O3DE/Performance/FrameTimeRecording/DelayCount",
    "/O3DE/Performance/FrameTimeRecording/CaptureCount",
    "/O3DE/Performance/FrameTimeRecording/ProfileName",
    "/O3DE/Performance/FrameTimeRecording/QuitOnComplete",
    "/O3DE/Runtime/FilePaths/SourceProjectUserPath",
    "/Amazon/AzCore/Runtime/ConsoleCommands/quit",
}

local outputPaths = {
    "InvalidPath/",
    "/Scripts/PerformanceBenchmarks/",
    "/benchmark_metadata.json",
}

local inputConfiguration = "some/file/in/some/folder.cfg"

return { registryKeys, outputPaths, inputConfiguration }
