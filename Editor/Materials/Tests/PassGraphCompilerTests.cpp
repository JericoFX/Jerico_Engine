/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Editor/Materials/Graph/PassGraphCompiler.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <GraphModel/Model/GraphContext.h>

namespace UnitTest
{
    class PassGraphCompilerTests : public LeakDetectionFixture
    {
    protected:
        void TearDown() override
        {
            // GraphCompiler queues status messages; drain them while the test allocator is alive.
            AZ::SystemTickBus::ExecuteQueuedEvents();
            LeakDetectionFixture::TearDown();
        }
    };

    TEST_F(PassGraphCompilerTests, UnimplementedGenerationFailsWithoutPublishingOutputs)
    {
        constexpr AZ::Crc32 toolId = AZ_CRC_CE("PassGraphCompilerTest");
        AZ::Test::ScopedAutoTempDirectory directory;
        AtomToolsFramework::DynamicNodeManager nodeManager(toolId);
        AtomToolsFramework::DynamicNodeConfig config;
        config.m_title = "Pass";
        ASSERT_TRUE(nodeManager.RegisterConfig(config));
        const auto context = AZStd::make_shared<GraphModel::GraphContext>(
            "Pass Graph", ".passgraph", GraphModel::DataTypeList{});
        const auto graph = AZStd::make_shared<GraphModel::Graph>(context);
        graph->AddNode(AZStd::make_shared<AtomToolsFramework::DynamicNode>(graph, toolId, config.m_id));
        ASSERT_EQ(graph->GetNodeCount(), 1);
        const AZStd::string path = directory.Resolve("Valid.passgraph").Native();

        // The same real nonempty graph satisfies the base compiler's input preconditions.
        AtomToolsFramework::GraphCompiler baseCompiler(toolId);
        ASSERT_TRUE(baseCompiler.CompileGraph(graph, "Valid", path));
        JE::Editor::Materials::PassGraphCompiler compiler(toolId);
        EXPECT_FALSE(compiler.CompileGraph(graph, "Valid", path));
        EXPECT_EQ(compiler.GetState(), AtomToolsFramework::GraphCompiler::State::Failed);
        EXPECT_TRUE(compiler.GetGeneratedFilePaths().empty());
        size_t outputFiles = 0;
        const AZStd::string filter = directory.Resolve("*").Native();
        AZ::IO::SystemFile::FindFiles(filter.c_str(), [&outputFiles](const char*, bool isFile)
        {
            if (isFile)
            {
                ++outputFiles;
            }
            return true;
        });
        EXPECT_EQ(outputFiles, 0);
    }
}
