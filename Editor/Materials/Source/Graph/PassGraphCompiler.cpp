/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <JE/Editor/Materials/Graph/PassGraphCompiler.h>

namespace JE::Editor::Materials
{
    void PassGraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PassGraphCompiler, JE::Editor::Tools::GraphCompiler>()
                ->Version(0)
                ;
        }
    }

    PassGraphCompiler::PassGraphCompiler(const AZ::Crc32& toolId)
        : JE::Editor::Tools::GraphCompiler(toolId)
    {
    }

    PassGraphCompiler::~PassGraphCompiler()
    {
    }

    AZStd::string PassGraphCompiler::GetGraphPath() const
    {
        if (const auto& graphPath = JE::Editor::Tools::GraphCompiler::GetGraphPath(); graphPath.ends_with(".passgraph"))
        {
            return graphPath;
        }

        return AZStd::string::format("%s/Assets/Passes/Generated/untitled.passgraph", AZ::Utils::GetProjectPath().c_str());
    }

    bool PassGraphCompiler::CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath)
    {
        if (!JE::Editor::Tools::GraphCompiler::CompileGraph(graph, graphName, graphPath))
        {
            return false;
        }

        AZ_TracePrintf(
            "PassGraphCompiler", "Pass graph generation is not implemented for '%s'; no outputs were produced.\n", GetGraphPath().c_str());
        SetState(State::Failed);
        return false;
    }
} // namespace JE::Editor::Materials
