/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

namespace UnitTest
{
    namespace
    {
        class GraphHistoryApplication final : public AzToolsFramework::ToolsApplication
        {
        public:
            AZ_CLASS_ALLOCATOR(GraphHistoryApplication, AZ::SystemAllocator);
            GraphHistoryApplication()
            {
                m_undoStack = aznew AzToolsFramework::UndoSystem::UndoStack(nullptr);
                m_context.RegisterGenericType<AZStd::any>();
                m_context.RegisterGenericType<AZStd::unordered_map<AZStd::string, float>>();
                AZ::JsonSystemComponent::Reflect(&m_registrationContext);
                AZ::ComponentApplicationBus::Handler::BusConnect();
            }
            ~GraphHistoryApplication() override
            {
                AZ::ComponentApplicationBus::Handler::BusDisconnect();
                m_registrationContext.EnableRemoveReflection();
                AZ::JsonSystemComponent::Reflect(&m_registrationContext);
                delete m_undoStack;
                m_undoStack = nullptr;
            }
            AZ::SerializeContext* GetSerializeContext() override { return &m_context; }
            AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return &m_registrationContext; }
            using ToolsApplication::GetUndoStack;
        private:
            AZ::SerializeContext m_context;
            AZ::JsonRegistrationContext m_registrationContext;
        };

        class HistoryAnyDocument final : public AtomToolsFramework::AtomToolsAnyDocument
        {
        public:
            explicit HistoryAnyDocument(const AZStd::any& content = AZStd::any(1.0f))
                : AtomToolsAnyDocument(AZ::Crc32("GraphHistoryTest"),
                    BuildDocumentTypeInfo("Node Config", {"materialgraphnode"}, {}, content, AZ::Uuid::CreateNull()),
                    content, AZ::Uuid::CreateNull())
            {
                m_absolutePath = "F:/GraphHistoryTest/node.materialgraphnode";
            }
            void ChangePath() { m_absolutePath = "F:/GraphHistoryTest/other.materialgraphnode"; }
            void SetValue(float value)
            {
                *static_cast<float*>(GetObjectInfo().front().m_objectPtr) = value;
            }
            float Value() const { return AZStd::any_cast<float>(GetContent()); }
        };

        class RenameUndo final : public AzToolsFramework::UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(RenameUndo, AZ::SystemAllocator);
            explicit RenameUndo(AZ::Entity& entity) : URSequencePoint("Rename Entity"), m_entity(entity) {}
            bool Changed() const override { return true; }
            void Undo() override { m_entity.SetName("before"); }
            void Redo() override { m_entity.SetName("after"); }
        private:
            AZ::Entity& m_entity;
        };

        // Only the AP scan-folder boundary is supplied. JSON load/save and document history are production implementations.
        class HistorySourceFolder final : public AzToolsFramework::AssetSystemRequestBus::Handler
        {
        public:
            explicit HistorySourceFolder(AZStd::string path) : m_path(AZStd::move(path)) { BusConnect(); }
            ~HistorySourceFolder() override { BusDisconnect(); }
            bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string&, AZStd::string&) override { return false; }
            bool GenerateRelativeSourcePath(const AZStd::string&, AZStd::string&, AZStd::string&) override { return false; }
            bool GetFullSourcePathFromRelativeProductPath(const AZStd::string&, AZStd::string&) override { return false; }
            bool GetAssetInfoById(const AZ::Data::AssetId&, const AZ::Data::AssetType&, const AZStd::string&, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetSourceInfoBySourcePath(const char*, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetSourceInfoBySourceUUID(const AZ::Uuid&, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetScanFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool IsAssetPlatformEnabled(const char*) override { return false; }
            int GetPendingAssetsForPlatform(const char*) override { return 0; }
            bool GetAssetsProducedBySourceUUID(const AZ::Uuid&, AZStd::vector<AZ::Data::AssetInfo>&) override { return false; }
            bool ClearFingerprintForAsset(const AZStd::string&) override { return false; }
        private:
            AZStd::string m_path;
        };
    }

    class GraphHistoryTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_application = AZStd::make_unique<GraphHistoryApplication>();
            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            m_policy = JE::Editor::CreateMaterialCanvasDocumentHistoryPolicy();
        }
        void TearDown() override
        {
            m_policy = {};
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
            }
            m_application.reset();
            LeakDetectionFixture::TearDown();
        }
        void Edit(HistoryAnyDocument& document, float value)
        {
            ASSERT_TRUE(document.BeginEdit());
            document.SetValue(value);
            ASSERT_TRUE(document.EndEdit());
        }
        AZStd::unique_ptr<GraphHistoryApplication> m_application;
        AtomToolsFramework::DocumentHistoryPolicy m_policy;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    TEST_F(GraphHistoryTests, AnyDocumentAndEntityShareOneChronologicalHistory)
    {
        HistoryAnyDocument document;
        ASSERT_TRUE(document.SetHistoryPolicy(m_policy));
        Edit(document, 2.0f);
        AZ::Entity entity;
        entity.SetName("after");
        {
            AzToolsFramework::ScopedUndoBatch batch("Rename Entity");
            (aznew RenameUndo(entity))->SetParent(batch.GetUndoBatch());
        }
        Edit(document, 3.0f);
        ASSERT_TRUE(document.Undo());
        EXPECT_FLOAT_EQ(document.Value(), 2.0f);
        ASSERT_TRUE(document.Undo());
        EXPECT_EQ(entity.GetName(), "before");
        ASSERT_TRUE(document.Undo());
        EXPECT_FLOAT_EQ(document.Value(), 1.0f);
        EXPECT_FALSE(document.CanUndo());
        ASSERT_TRUE(document.Redo());
        ASSERT_TRUE(document.Redo());
        ASSERT_TRUE(document.Redo());
        EXPECT_FLOAT_EQ(document.Value(), 3.0f);
        EXPECT_EQ(entity.GetName(), "after");
    }

    TEST_F(GraphHistoryTests, StandaloneRetainsLocalHistory)
    {
        HistoryAnyDocument document;
        Edit(document, 2.0f);
        EXPECT_FALSE(m_application->GetUndoStack()->CanUndo());
        ASSERT_TRUE(document.Undo());
        EXPECT_FLOAT_EQ(document.Value(), 1.0f);
        ASSERT_TRUE(document.Redo());
        EXPECT_FLOAT_EQ(document.Value(), 2.0f);
    }

    TEST_F(GraphHistoryTests, DestroyedDocumentCannotTargetAnotherDocument)
    {
        auto document = AZStd::make_unique<HistoryAnyDocument>();
        ASSERT_TRUE(document->SetHistoryPolicy(m_policy));
        Edit(*document, 2.0f);
        document.reset();
        HistoryAnyDocument replacement;
        ASSERT_TRUE(replacement.SetHistoryPolicy(m_policy));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(replacement.Undo());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(replacement.Value(), 1.0f);
    }

    TEST_F(GraphHistoryTests, ChangedPathOrExternalContentRejectsReplayWithoutMutation)
    {
        HistoryAnyDocument document;
        ASSERT_TRUE(document.SetHistoryPolicy(m_policy));
        Edit(document, 2.0f);
        document.SetValue(4.0f);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(document.Undo());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(document.Value(), 4.0f);
        document.SetValue(2.0f);
        ASSERT_TRUE(document.RestoreHistoryState(document.GetHistoryState(), document.GetHistoryState()));
        Edit(document, 5.0f);
        document.ChangePath();
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(document.Undo());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(document.Value(), 5.0f);
    }

    TEST_F(GraphHistoryTests, InvalidSnapshotDoesNotReplaceAnyContent)
    {
        HistoryAnyDocument document;
        const auto state = document.GetHistoryState();
        ASSERT_FALSE(state.empty());
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(document.RestoreHistoryState(state, {}));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(document.Value(), 1.0f);
    }

    TEST_F(GraphHistoryTests, UnorderedContentReplayUsesValuesInsteadOfBinaryIterationOrder)
    {
        using Values = AZStd::unordered_map<AZStd::string, float>;
        Values values{{"first", 1.0f}, {"second", 2.0f}, {"third", 3.0f}};
        HistoryAnyDocument document{ AZStd::any(values) };
        const auto before = document.GetHistoryState();
        auto* current = static_cast<Values*>(document.GetObjectInfo().front().m_objectPtr);
        current->clear();
        current->emplace("third", 3.0f);
        current->emplace("second", 2.0f);
        current->emplace("first", 1.0f);
        current->rehash(101);
        ASSERT_TRUE(document.RestoreHistoryState(before, before));
        EXPECT_EQ(AZStd::any_cast<Values>(document.GetContent()), values);
        current = static_cast<Values*>(document.GetObjectInfo().front().m_objectPtr);
        (*current)["second"] = 8.0f;
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(document.RestoreHistoryState(before, before));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(AZStd::any_cast<Values>(document.GetContent()).at("second"), 8.0f);
    }

    TEST_F(GraphHistoryTests, OpeningThenFinishingWithoutAnEditAddsNoHistory)
    {
        AZ::Test::ScopedAutoTempDirectory directory;
        HistorySourceFolder scanFolder(directory.GetDirectory());
        const AZStd::string path = directory.Resolve("node.materialgraphnode").Native();
        const float value = 7.0f;
        ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&value, path).IsSuccess());
        HistoryAnyDocument document;
        ASSERT_TRUE(document.SetHistoryPolicy(m_policy));
        ASSERT_TRUE(document.Open(path));
        ASSERT_TRUE(document.EndEdit());
        ASSERT_TRUE(document.EndEdit());
        EXPECT_FLOAT_EQ(document.Value(), value);
        EXPECT_FALSE(document.IsModified());
        EXPECT_FALSE(m_application->GetUndoStack()->CanUndo());
    }

    TEST_F(GraphHistoryTests, ReopenPreservesGlobalHistoryButChangedSchemaRejectsOldCommands)
    {
        AZ::Test::ScopedAutoTempDirectory directory;
        HistorySourceFolder scanFolder(directory.GetDirectory());
        const AZStd::string path = directory.Resolve("node.materialgraphnode").Native();
        const float initial = 1.0f;
        ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, path).IsSuccess());
        HistoryAnyDocument document;
        ASSERT_TRUE(document.SetHistoryPolicy(m_policy));
        ASSERT_TRUE(document.Open(path));
        Edit(document, 2.0f);
        ASSERT_TRUE(document.Save());
        ASSERT_TRUE(document.Reopen());
        ASSERT_TRUE(document.Undo());
        EXPECT_FLOAT_EQ(document.Value(), initial);
        ASSERT_TRUE(document.Redo());
        EXPECT_FLOAT_EQ(document.Value(), 2.0f);
        const AZStd::string changedType("external schema");
        ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&changedType, path).IsSuccess());
        ASSERT_TRUE(document.Open(path));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(document.Undo());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(AZStd::any_cast<AZStd::string>(document.GetContent()), changedType);
    }
}
