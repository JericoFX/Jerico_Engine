/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Editor/Materials/Document/MaterialDocument.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    namespace
    {
        // Use the real ToolsApplication batch and UndoStack without starting services, Qt or an asset/GPU pipeline.
        class UndoOnlyToolsApplication final : public AzToolsFramework::ToolsApplication
        {
        public:
            UndoOnlyToolsApplication() { m_undoStack = aznew AzToolsFramework::UndoSystem::UndoStack(nullptr); }
            ~UndoOnlyToolsApplication() override
            {
                delete m_undoStack;
                m_undoStack = nullptr;
            }
            using AzToolsFramework::ToolsApplication::GetUndoStack;
        };

        class CpuPropertyDocument final : public JE::Editor::MaterialDocument
        {
        public:
            using MaterialDocument::MaterialDocument;

            // The history under test is production BeginEdit/EndEdit and its real command. Only GPU recompilation is omitted.
            void SetPropertyValue(const AZStd::string& propertyId, const AZStd::any& value) override
            {
                for (const auto& object : GetObjectInfo())
                {
                    auto* group = static_cast<AtomToolsFramework::DynamicPropertyGroup*>(object.m_objectPtr);
                    for (auto& property : group->m_properties)
                    {
                        if (property.GetId() == AZ::Name(propertyId))
                        {
                            property.SetValue(value);
                            return;
                        }
                    }
                }
                FAIL() << "Missing test property";
            }
        };

        class EntityNameUndo final : public AzToolsFramework::UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(EntityNameUndo, AZ::SystemAllocator);
            EntityNameUndo(AZ::Entity& entity, AZStd::string before, AZStd::string after)
                : URSequencePoint("Rename Entity"), m_entity(entity), m_before(AZStd::move(before)), m_after(AZStd::move(after)) {}
            bool Changed() const override { return m_before != m_after; }
            void Undo() override { m_entity.SetName(m_before); }
            void Redo() override { m_entity.SetName(m_after); }
        private:
            AZ::Entity& m_entity;
            AZStd::string m_before;
            AZStd::string m_after;
        };
    }

    class MaterialDocumentTests : public LeakDetectionFixture
    {
    protected:
        using HistoryOwner = JE::Editor::MaterialDocument::HistoryOwner;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_ownsNames = !AZ::NameDictionary::IsReady();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Create();
            }
            m_application = AZStd::make_unique<UndoOnlyToolsApplication>();
        }

        void TearDown() override
        {
            m_application.reset();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<CpuPropertyDocument> MakeDocument(HistoryOwner owner = HistoryOwner::Editor,
            const char* path = "F:/JericoEngine/JericoEngine/Projects/JEEditorFixture/Assets/history.material")
        {
            auto document = AZStd::make_unique<CpuPropertyDocument>(AZ::Crc32("MaterialHistoryTest"),
                JE::Editor::MaterialDocument::BuildDocumentTypeInfo(owner), owner);
            document->m_absolutePath = path;
            SeedProperties(*document);
            return document;
        }

        void SeedProperties(JE::Editor::MaterialDocument& document)
        {
            auto group = AZStd::make_shared<AtomToolsFramework::DynamicPropertyGroup>();
            group->m_name = "surface";
            AtomToolsFramework::DynamicPropertyConfig property;
            property.m_id = AZ::Name("surface.roughness");
            property.m_defaultValue = 0.25f;
            property.m_originalValue = 0.25f;
            property.m_parentValue = 0.25f;
            group->m_properties.emplace_back(property);
            property.m_id = AZ::Name("surface.metallic");
            property.m_defaultValue = 0.0f;
            property.m_originalValue = 0.0f;
            property.m_parentValue = 0.0f;
            group->m_properties.emplace_back(property);
            document.m_groups.push_back(AZStd::move(group));
        }

        float Value(const JE::Editor::MaterialDocument& document)
        {
            return AZStd::any_cast<float>(document.GetPropertyValue("surface.roughness"));
        }

        void Edit(CpuPropertyDocument& document, float value)
        {
            ASSERT_TRUE(document.BeginEdit());
            document.SetPropertyValue("surface.roughness", value);
            ASSERT_TRUE(document.EndEdit());
        }

        void ChangePath(JE::Editor::MaterialDocument& document, const char* path) { document.m_absolutePath = path; }

        void ReloadDependencyState(JE::Editor::MaterialDocument& document)
        {
            ASSERT_TRUE(document.ReopenRecordState());
            const auto path = document.GetAbsolutePath();
            document.Clear();
            document.m_absolutePath = path;
            SeedProperties(document); // Model the CPU property metadata reconstructed by Open, without GPU compilation.
            ASSERT_TRUE(document.ReopenRestoreState());
        }
        void Rename(AZ::Entity& entity, const char* name)
        {
            AzToolsFramework::ScopedUndoBatch batch("Rename Entity");
            auto* command = aznew EntityNameUndo(entity, entity.GetName(), name);
            entity.SetName(name);
            command->SetParent(batch.GetUndoBatch());
        }

        bool m_ownsNames = false;
        AZStd::unique_ptr<UndoOnlyToolsApplication> m_application;
    };

    TEST_F(MaterialDocumentTests, RegistryObjectWritesKeepReporterLifetimeWithinEachOperation)
    {
        AZ::SerializeContext serializeContext;
        AZ::JsonRegistrationContext registrationContext;
        AZ::JsonSystemComponent::Reflect(&registrationContext);
        struct JsonReflectionGuard
        {
            AZ::JsonRegistrationContext& m_context;
            ~JsonReflectionGuard()
            {
                m_context.EnableRemoveReflection();
                AZ::JsonSystemComponent::Reflect(&m_context);
                m_context.DisableRemoveReflection();
            }
        } reflectionGuard{registrationContext};
        struct ExpectedJsonWarnings final : AZ::Debug::TraceMessageBus::Handler
        {
            ExpectedJsonWarnings() { BusConnect(); }
            ~ExpectedJsonWarnings() override { BusDisconnect(); }
            bool OnPreWarning(const char* window, const char*, int, const char*, const char* message) override
            {
                if (AZStd::string_view(window) == "JSON Serialization" && AZStd::string_view(message).starts_with(
                    "Target object for Json Serialization is pointing to nothing during storing."))
                {
                    ++m_count;
                    return true;
                }
                return false;
            }
            int m_count = 0;
        } warnings;
        {
            AZ::SettingsRegistryImpl registry;
            registry.SetContext(&serializeContext);
            registry.SetContext(&registrationContext);
            int notifications = 0;
            auto notifier = registry.RegisterNotifier(
                [&](const AZ::SettingsRegistryInterface::NotifyEventArgs& args)
                {
                    if (args.m_jsonKeyPath != "/value")
                    {
                        return;
                    }
                    ++notifications;
                    // Reenter after Store returned, from a different stack depth. This must use a fresh reporter.
                    EXPECT_FALSE(registry.SetObject("/rejected", nullptr, azrtti_typeid<int>()));
                    const int mirror = notifications;
                    EXPECT_TRUE(registry.SetObject("/mirror", &mirror, azrtti_typeid<int>()));
                });
            for (int value = 1; value <= 10; ++value)
            {
                EXPECT_TRUE(registry.SetObject("/value", &value, azrtti_typeid<int>()));
                AZ::s64 actual = 0;
                EXPECT_TRUE(registry.Get(actual, "/value"));
                EXPECT_EQ(actual, value);
                EXPECT_TRUE(registry.Get(actual, "/mirror"));
                EXPECT_EQ(actual, value);
                EXPECT_EQ(registry.GetType("/rejected").m_type, AZ::SettingsRegistryInterface::Type::NoType);
            }
            EXPECT_EQ(warnings.m_count, 10);
            EXPECT_EQ(notifications, 10);
        }
    }

    TEST_F(MaterialDocumentTests, NativeDocumentPreservesTypeIdentityAndBothFactoryModes)
    {
        AZ::SerializeContext context;
        AtomToolsFramework::AtomToolsDocument::Reflect(&context);
        JE::Editor::MaterialDocument::Reflect(&context);
        JE::Editor::MaterialDocument::Reflect(&context); // Host and standalone callers may share a context.
        const auto* data = context.FindClassData(AZ::Uuid("{90299628-AD02-4FEB-9527-7278FA2817AD}"));
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->m_version, 0);
        EXPECT_STREQ(data->m_name, "MaterialDocument");
        for (auto owner : {HistoryOwner::Standalone, HistoryOwner::Editor})
        {
            const auto type = JE::Editor::MaterialDocument::BuildDocumentTypeInfo(owner);
            AZStd::unique_ptr<AtomToolsFramework::AtomToolsDocumentRequests> document(type.CreateDocument(AZ::Crc32("FactoryTest")));
            ASSERT_NE(document, nullptr);
            EXPECT_EQ(document->GetDocumentTypeInfo().m_documentTypeName, "Material");
            EXPECT_TRUE(JE::Editor::MaterialDocumentRequestBus::HasHandlers(document->GetId()));
        }
    }

    TEST_F(MaterialDocumentTests, OwnReflectionDoesNotClaimBaseInEitherRegistrationOrder)
    {
        for (bool baseFirst : {false, true})
        {
            AZ::SerializeContext context;
            if (baseFirst)
            {
                AtomToolsFramework::AtomToolsDocument::Reflect(&context);
            }
            JE::Editor::MaterialDocument::Reflect(&context);
            if (!baseFirst)
            {
                EXPECT_EQ(context.FindClassData(azrtti_typeid<AtomToolsFramework::AtomToolsDocument>()), nullptr);
                AtomToolsFramework::AtomToolsDocument::Reflect(&context);
            }
            JE::Editor::MaterialDocument::Reflect(&context);
            ASSERT_NE(context.FindClassData(azrtti_typeid<JE::Editor::MaterialDocument>()), nullptr);
            ASSERT_NE(context.FindClassData(azrtti_typeid<AtomToolsFramework::AtomToolsDocument>()), nullptr);
        }
    }

    TEST_F(MaterialDocumentTests, MaterialAndEntityEditsReplayInOneChronologicalHistory)
    {
        AZ::Entity entity("Before"); // Own the entity until all commands referencing it have been reset.
        auto document = MakeDocument();
        Edit(*document, 0.5f);
        Rename(entity, "After");
        Edit(*document, 0.75f);
        EXPECT_FALSE(document->CanUndo());
        auto* stack = m_application->GetUndoStack();
        stack->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.5f);
        EXPECT_EQ(entity.GetName(), "After");
        stack->Undo();
        EXPECT_EQ(entity.GetName(), "Before");
        EXPECT_FLOAT_EQ(Value(*document), 0.5f);
        stack->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        EXPECT_FALSE(stack->CanUndo());
        stack->Redo();
        stack->Redo();
        stack->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        EXPECT_EQ(entity.GetName(), "After");
        stack->Reset();
    }

    TEST_F(MaterialDocumentTests, SeparateDocumentsRemainIsolatedDuringGlobalUndo)
    {
        auto first = MakeDocument();
        auto second = MakeDocument(HistoryOwner::Editor, "F:/JericoEngine/JericoEngine/Projects/JEEditorFixture/Assets/second.material");
        Edit(*first, 0.5f);
        Edit(*second, 0.75f);
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*first), 0.5f);
        EXPECT_FLOAT_EQ(Value(*second), 0.25f);
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*first), 0.25f);
    }

    TEST_F(MaterialDocumentTests, StandaloneRetainsLocalUndoWithoutPostingToEditor)
    {
        auto document = MakeDocument(HistoryOwner::Standalone);
        Edit(*document, 0.75f);
        EXPECT_FALSE(m_application->GetUndoStack()->CanUndo());
        ASSERT_TRUE(document->Undo());
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        ASSERT_TRUE(document->Redo());
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
    }

    TEST_F(MaterialDocumentTests, UnchangedOrCancelledEditDoesNotPostHistory)
    {
        auto document = MakeDocument();
        ASSERT_TRUE(document->BeginEdit());
        ASSERT_TRUE(document->EndEdit());
        ASSERT_TRUE(document->BeginEdit());
        document->SetPropertyValue("surface.roughness", 0.75f);
        document->SetPropertyValue("surface.roughness", 0.25f);
        ASSERT_TRUE(document->EndEdit());
        EXPECT_FALSE(m_application->GetUndoStack()->CanUndo());
        EXPECT_FALSE(document->CanUndo());
    }

    TEST_F(MaterialDocumentTests, DestroyedDocumentRejectsReplayWithoutTargetingAnotherDocument)
    {
        auto document = MakeDocument();
        Edit(*document, 0.75f);
        const auto id = document->GetId();
        document.reset();
        auto replacement = MakeDocument(); // Same source path, different identity.
        EXPECT_NE(id, replacement->GetId());
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_application->GetUndoStack()->Undo();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(Value(*replacement), 0.25f);
    }

    TEST_F(MaterialDocumentTests, ChangedSourcePathRejectsReplayAndPreservesCurrentValues)
    {
        auto document = MakeDocument();
        Edit(*document, 0.75f);
        ChangePath(*document, "F:/JericoEngine/JericoEngine/Projects/JEEditorFixture/Assets/another.material");
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_application->GetUndoStack()->Undo();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
    }

    TEST_F(MaterialDocumentTests, ExternalContentChangeRejectsReplayInsteadOfOverwritingNewValues)
    {
        auto document = MakeDocument();
        Edit(*document, 0.75f);
        document->SetPropertyValue("surface.roughness", 0.9f); // Same path/ID/type, changed outside recorded edits.
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_application->GetUndoStack()->Undo();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
    }

    TEST_F(MaterialDocumentTests, ChangedPropertyTypeRejectsReplayWithoutApplyingOldType)
    {
        auto document = MakeDocument();
        ASSERT_TRUE(document->BeginEdit());
        document->SetPropertyValue("surface.roughness", 0.75f);
        document->SetPropertyValue("surface.metallic", 0.5f);
        ASSERT_TRUE(document->EndEdit());
        document->SetPropertyValue("surface.metallic", AZ::u32(7));
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_application->GetUndoStack()->Undo();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(AZStd::any_cast<AZ::u32>(document->GetPropertyValue("surface.metallic")), 7);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f); // No partial restoration of the otherwise compatible property.
    }

    TEST_F(MaterialDocumentTests, DependencyReopenStatePreservesEditorHistoryWithoutLocalHistory)
    {
        auto document = MakeDocument();
        Edit(*document, 0.75f);
        const auto id = document->GetId();
        ReloadDependencyState(*document);
        EXPECT_EQ(document->GetId(), id);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        EXPECT_FALSE(document->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
    }
}
