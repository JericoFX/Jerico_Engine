/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Runner.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzTest/Utils.h>

AZ_TOOLS_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV)


namespace UnitTest
{
    using SerializeContextToolsFixture = UnitTest::LeakDetectionFixture;

    TEST_F(SerializeContextToolsFixture, SerializeContextTools_HelpOption_Works)
    {
        AZStd::fixed_string<32> appName{ "SerializeContextTools" };
        AZStd::fixed_string<32> helpArg{ "--help" };
        AZStd::fixed_vector<char*, 2> commandArgs{ appName.data(), helpArg.data() };
        EXPECT_EQ(0, SerializeContextTools::LaunchSerializeContextTools(int(commandArgs.size()),
            commandArgs.data()));
    }

    TEST_F(SerializeContextToolsFixture, SerializeContextTools_DumpFiles_RejectsObjectStreamWithNoRoots)
    {
        AZ::Test::ScopedAutoTempDirectory temp;
        const auto project = temp.GetDirectoryAsPath();
        {
            const auto metadata = temp.Resolve("project.json");
            AZ::IO::SystemFileStream stream(metadata.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
            ASSERT_TRUE(stream.IsOpen());
            constexpr AZStd::string_view contents = R"({"project_name":"SerializeContextToolsDumpFixture"})";
            ASSERT_EQ(contents.size(), stream.Write(contents.size(), contents.data()));
        }
        const auto input = temp.Resolve("empty.objectstream");
        const auto expectedDump = temp.Resolve("empty.objectstream.dump.txt");
        auto resolvedDump = (project / input.LexicallyRelative(project)).LexicallyNormal();
        resolvedDump.Native() += ".dump.txt";
        ASSERT_EQ(expectedDump, resolvedDump);
        {
            AZ::SerializeContext context;
            AZ::IO::SystemFileStream stream(input.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
            ASSERT_TRUE(stream.IsOpen());
            AZ::ObjectStream* writer = AZ::ObjectStream::Create(&stream, context, AZ::ObjectStream::ST_BINARY);
            ASSERT_NE(nullptr, writer);
            ASSERT_TRUE(writer->Finalize());
        }
        AZStd::fixed_string<32> appName{ "SerializeContextTools" };
        AZStd::fixed_string<32> action{ "dumpfiles" };
        AZStd::string projectArg = AZStd::string::format("--project-path=%s", project.c_str());
        AZStd::string filesArg = AZStd::string::format("--files=%s", input.c_str());
        AZStd::string outputArg = AZStd::string::format("--output=%s", temp.GetDirectory());
        AZStd::fixed_vector<char*, 5> args{ appName.data(), action.data(), projectArg.data(), filesArg.data(), outputArg.data() };

        // A valid ObjectStream containing no roots must produce one explicit DumpFiles error and a failure exit.
        AZ_TEST_START_TRACE_SUPPRESSION;
        const int result = SerializeContextTools::LaunchSerializeContextTools(static_cast<int>(args.size()), args.data());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_NE(0, result);
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedDump.c_str()));
        EXPECT_EQ(0, AZ::IO::SystemFile::Length(expectedDump.c_str()));
    }
}
