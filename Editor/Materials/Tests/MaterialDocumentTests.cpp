/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "../Source/MaterialTextureSet.h"
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Common/JsonUtils.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySourceData.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentNotificationBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Tests/AssetSystemMocks.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyValueSerializer.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <QTemporaryDir>
#include <QDir>
#include <QFileInfo>
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
            // Reflection, JSON and Undo only; this fixture does not load application service modules.
            AZ::ComponentTypeList GetRequiredSystemComponents() const override { return {}; }
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
                    auto* group = static_cast<JE::Editor::Tools::DynamicPropertyGroup*>(object.m_objectPtr);
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
            auto group = AZStd::make_shared<JE::Editor::Tools::DynamicPropertyGroup>();
            group->m_name = "surface";
            JE::Editor::Tools::DynamicPropertyConfig property;
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

        void SeedColorProperty(JE::Editor::MaterialDocument& document, const AZ::Name& colorId, const AZ::Color& authoredColor)
        {
            auto* definitionGroup = document.m_materialTypeSourceData.AddPropertyGroup("baseColor");
            ASSERT_NE(definitionGroup, nullptr);
            auto* definition = definitionGroup->AddProperty("color");
            ASSERT_NE(definition, nullptr);
            definition->m_dataType = JE::Render::RPI::MaterialPropertyDataType::Color;
            definition->m_value = AZ::Color::CreateOne();

            auto group = AZStd::make_shared<JE::Editor::Tools::DynamicPropertyGroup>();
            group->m_name = "baseColor";
            JE::Editor::Tools::DynamicPropertyConfig colorConfig;
            colorConfig.m_id = colorId;
            colorConfig.m_defaultValue = AZ::Color::CreateOne();
            colorConfig.m_parentValue = colorConfig.m_defaultValue;
            colorConfig.m_originalValue = authoredColor;
            group->m_properties.emplace_back(colorConfig);
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

        bool BindLoadedSource(JE::Editor::MaterialDocument& document, const AZStd::string& path,
            JE::Render::RPI::MaterialSourceData source)
        {
            document.m_absolutePath = JE::Render::RPI::AssetUtils::ResolvePathReference("", path);
            source.m_parentMaterial = JE::Render::RPI::AssetUtils::ResolvePathReference(document.m_absolutePath, source.m_parentMaterial);
            source.m_materialType = JE::Render::RPI::AssetUtils::ResolvePathReference(document.m_absolutePath, source.m_materialType);
            source.m_materialType = JE::Render::RPI::MaterialUtils::PredictOriginalMaterialTypeSourcePath(source.m_materialType);
            document.m_materialSourceData = AZStd::move(source);

            auto* surface = document.m_materialTypeSourceData.AddPropertyGroup("surface");
            auto* roughness = surface->AddProperty("roughness");
            roughness->m_dataType = JE::Render::RPI::MaterialPropertyDataType::Float;
            roughness->m_value = 0.25f;
            auto* metallic = surface->AddProperty("metallic");
            metallic->m_dataType = JE::Render::RPI::MaterialPropertyDataType::Float;
            metallic->m_value = 0.0f;

            auto overview = AZStd::make_shared<JE::Editor::Tools::DynamicPropertyGroup>();
            overview->m_name = "overview";
            JE::Editor::Tools::DynamicPropertyConfig description;
            description.m_id = AZ::Name("overview.materialDescription");
            description.m_defaultValue = document.m_materialSourceData.m_description;
            description.m_originalValue = description.m_defaultValue;
            description.m_parentValue = description.m_defaultValue;
            overview->m_properties.emplace_back(description);
            document.m_groups.push_back(AZStd::move(overview));
            // Exercise the real source-notification bus after native source loading. No RPI instance is fabricated.
            return document.OpenSucceeded();
        }

        void AddSourceDependency(JE::Editor::MaterialDocument& document, const AZStd::string& path)
        {
            document.m_sourceDependencies.insert(JE::Render::RPI::AssetUtils::ResolvePathReference("", path));
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
        JE::Editor::Tools::AtomToolsDocument::Reflect(&context);
        JE::Editor::MaterialDocument::Reflect(&context);
        JE::Editor::MaterialDocument::Reflect(&context); // Host and standalone callers may share a context.
        const auto* data = context.FindClassData(AZ::Uuid("{90299628-AD02-4FEB-9527-7278FA2817AD}"));
        ASSERT_NE(data, nullptr);
        EXPECT_EQ(data->m_version, 0);
        EXPECT_STREQ(data->m_name, "MaterialDocument");
        for (auto owner : {HistoryOwner::Standalone, HistoryOwner::Editor})
        {
            const auto type = JE::Editor::MaterialDocument::BuildDocumentTypeInfo(owner);
            AZStd::unique_ptr<JE::Editor::Tools::AtomToolsDocumentRequests> document(type.CreateDocument(AZ::Crc32("FactoryTest")));
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
                JE::Editor::Tools::AtomToolsDocument::Reflect(&context);
            }
            JE::Editor::MaterialDocument::Reflect(&context);
            if (!baseFirst)
            {
                EXPECT_EQ(context.FindClassData(azrtti_typeid<JE::Editor::Tools::AtomToolsDocument>()), nullptr);
                JE::Editor::Tools::AtomToolsDocument::Reflect(&context);
            }
            JE::Editor::MaterialDocument::Reflect(&context);
            ASSERT_NE(context.FindClassData(azrtti_typeid<JE::Editor::MaterialDocument>()), nullptr);
            ASSERT_NE(context.FindClassData(azrtti_typeid<JE::Editor::Tools::AtomToolsDocument>()), nullptr);
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

namespace UnitTest
{
    namespace
    {
        void ReflectTextureMaterialJson(AZ::ReflectContext* context)
        {
            JE::Render::RPI::MaterialPropertyValue::Reflect(context);
            JE::Render::RPI::MaterialSourceData::Reflect(context);
            if (auto* json = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                json->Serializer<JE::Render::RPI::JsonMaterialPropertyValueSerializer>()->HandlesType<JE::Render::RPI::MaterialPropertyValue>();
            }
        }
    }

    class MaterialTextureSetTests : public LeakDetectionFixture
    {
    protected:
        using Role = JE::Editor::MaterialTextureRole;
        using Channel = JE::Editor::MaterialTextureChannel;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_application = AZStd::make_unique<AZ::ComponentApplication>();
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadDynamicModules = false;
            startup.m_loadStaticModules = false;
            startup.m_loadSettingsRegistry = false;
            startup.m_createEditContext = false;
            m_application->Create(descriptor, startup);
            auto* manager = AZ::ReflectionEnvironment::GetReflectionManager();
            manager->Reflect(azrtti_typeid<AZ::JsonSystemComponent>(), &AZ::JsonSystemComponent::Reflect);
            manager->Reflect(azrtti_typeid<JE::Render::RPI::JsonMaterialPropertyValueSerializer>(), &ReflectTextureMaterialJson);
            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            const char* alias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
            m_hadEngineAlias = alias != nullptr;
            m_engineAlias = alias ? alias : "";
            const AZ::IO::Path engineRoot = AZ::IO::Path(__FILE__).ParentPath().ParentPath().ParentPath().ParentPath();
            ASSERT_TRUE(engineRoot.IsAbsolute());
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engineRoot.c_str());
            m_directory = AZStd::make_unique<QTemporaryDir>();
            ASSERT_TRUE(m_directory->isValid());
        }

        void TearDown() override
        {
            m_directory.reset();
            auto* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (m_hadEngineAlias)
            {
                fileIO->SetAlias("@engroot@", m_engineAlias.c_str());
            }
            else
            {
                fileIO->ClearAlias("@engroot@");
            }
            AZStd::string().swap(m_engineAlias);
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
            }
            m_application->Destroy(); // Removes the registered serializers/reflection before allocator teardown.
            m_application.reset();
            LeakDetectionFixture::TearDown();
        }

        AZStd::string Path(const char* file) const { return m_directory->filePath(file).toUtf8().constData(); }
        AZStd::string File(const char* name, AZStd::string_view contents = "recognition fixture, not decoded image data")
        {
            const auto path = Path(name);
            EXPECT_TRUE(AZ::Utils::WriteFile(contents, path).IsSuccess());
            return path;
        }
        size_t Candidate(const JE::Editor::MaterialTextureSet& set, Role role, Channel channel = Channel::RGB) const
        {
            for (size_t i = 0; i < set.candidates.size(); ++i)
            {
                if (set.candidates[i].role == role && set.candidates[i].channel == channel)
                {
                    return i;
                }
            }
            return set.candidates.size();
        }
        template<typename T>
        T Property(const JE::Render::RPI::MaterialSourceData& data, const char* id) const
        {
            const auto& value = data.GetPropertyValue(AZ::Name(id));
            EXPECT_TRUE(value.Is<T>()) << id;
            return value.Is<T>() ? value.GetValue<T>() : T{};
        }
        AZStd::unique_ptr<AZ::ComponentApplication> m_application;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        AZStd::unique_ptr<QTemporaryDir> m_directory;
        AZStd::string m_engineAlias;
        bool m_hadEngineAlias = false;
    };

    TEST_F(MaterialTextureSetTests, PolyHavenUsesCompleteSuffixesAndOnlyTheRealSameResolutionSet)
    {
        const auto anchor = File("river_stone_diff_4k.jpg");
        File("river_stone_4K_nor_gl.exr");
        File("river_stone_4k_roughness.png");
        File("river_stone_diff_8k.jpg");
        File("river_stone_extra_diff_4k.jpg");
        File("river_stone.json", R"({"maps":["river_stone_metal_4k.png"]})");
        ASSERT_TRUE(QDir(m_directory->path()).mkpath("Thumbs"));
        File("Thumbs/river_stone_metal_4k.jpg");
        auto result = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(result.IsSuccess());
        const auto& set = result.GetValue();
        EXPECT_EQ(set.identity, "river_stone");
        EXPECT_EQ(set.resolution, "4k");
        ASSERT_EQ(set.candidates.size(), 3);
        EXPECT_EQ(set.selected.size(), 3);
        const auto normal = Candidate(set, Role::Normal);
        ASSERT_LT(normal, set.candidates.size());
        EXPECT_EQ(set.candidates[normal].normalConvention, JE::Editor::MaterialNormalConvention::OpenGL);
        EXPECT_EQ(set.candidates[normal].token, "nor_gl");
        EXPECT_NE(AZStd::find(set.missing.begin(), set.missing.end(), Role::Metallic), set.missing.end());
        EXPECT_FALSE(set.warnings.empty());
        EXPECT_TRUE(JE::Editor::IsMaterialTextureSource(anchor));
        ASSERT_TRUE(QDir(m_directory->path()).mkpath("directory.png"));
        EXPECT_FALSE(JE::Editor::IsMaterialTextureSource(Path("directory.png")));
        EXPECT_FALSE(JE::Editor::IsMaterialTextureSource(Path("river_stone_metal_4k.png")));
        EXPECT_FALSE(JE::Editor::AnalyzeMaterialTextureSet(Path("Thumbs/river_stone_metal_4k.jpg")).IsSuccess());
    }

    TEST_F(MaterialTextureSetTests, QuixelAndFabPreserveAssetIdAndLeaveDuplicatesUnselected)
    {
        const auto anchor = File("cliff_vjbode1mw_8K_Albedo.jpg");
        File("cliff_vjbode1mw_8K_Albedo.png");
        File("cliff_vjbode1mw_8K_Normal.jpg");
        File("cliff_vjbode1mw_4K_Normal.jpg");
        File("cliff_otherid_8K_Roughness.jpg");
        File("cliff_vjbode1mw_8K_Glossiness.jpg");
        auto result = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(result.IsSuccess());
        const auto& set = result.GetValue();
        EXPECT_EQ(set.identity, "cliff_vjbode1mw");
        EXPECT_EQ(set.resolution, "8k");
        ASSERT_EQ(set.candidates.size(), 4);
        ASSERT_EQ(set.selected.size(), 1);
        EXPECT_EQ(set.candidates[set.selected.front()].role, Role::Normal);
        EXPECT_EQ(set.candidates[set.selected.front()].normalConvention, JE::Editor::MaterialNormalConvention::Unspecified);
        EXPECT_GE(set.warnings.size(), 3);
        AZStd::vector<size_t> duplicateAlbedo;
        for (size_t i = 0; i < set.candidates.size(); ++i)
        {
            if (set.candidates[i].role == Role::BaseColor)
            {
                duplicateAlbedo.push_back(i);
            }
        }
        ASSERT_EQ(duplicateAlbedo.size(), 2);
        auto rejected = JE::Editor::BuildTextureMaterialSource(set, duplicateAlbedo, set.suggestedMaterialPath);
        EXPECT_FALSE(rejected.IsSuccess()); // The two Albedo files may not be resolved by enumeration order.
    }

    TEST_F(MaterialTextureSetTests, PackedChannelsRoundTripAsNativeMaterialSourceWithoutChangingNormalConvention)
    {
        const auto ormAnchor = File("other_4K_ORM.png");
        auto orm = JE::Editor::AnalyzeMaterialTextureSet(ormAnchor);
        ASSERT_TRUE(orm.IsSuccess());
        ASSERT_EQ(orm.GetValue().candidates.size(), 3);
        EXPECT_LT(Candidate(orm.GetValue(), Role::Occlusion, Channel::Red), 3);
        EXPECT_LT(Candidate(orm.GetValue(), Role::Roughness, Channel::Green), 3);
        EXPECT_LT(Candidate(orm.GetValue(), Role::Metallic, Channel::Blue), 3);
        const auto anchor = File("stone_4K_BaseColor.png");
        File("stone_ARM_4K.png");
        File("stone_4K_NormalDX.png");
        File("stone_4K_Displacement.exr");
        auto analysis = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(analysis.IsSuccess());
        const auto& set = analysis.GetValue();
        ASSERT_EQ(set.candidates.size(), 6);
        ASSERT_EQ(set.selected.size(), 6);
        auto source = JE::Editor::BuildTextureMaterialSource(set, set.selected, set.suggestedMaterialPath);
        ASSERT_TRUE(source.IsSuccess());
        const auto& data = source.GetValue();
        EXPECT_EQ(data.m_materialTypeVersion, 6);
        EXPECT_TRUE(data.m_parentMaterial.empty());
        EXPECT_FALSE(AZ::IO::PathView(data.m_materialType).IsAbsolute());
        EXPECT_TRUE(data.m_materialType.ends_with("StandardPBR.materialtype"));
        EXPECT_EQ(Property<AZStd::string>(data, "baseColor.textureMap"), "stone_4K_BaseColor.png");
        EXPECT_EQ(Property<AZStd::string>(data, "occlusion.diffuseTextureMapChannel"), "Red");
        EXPECT_EQ(Property<AZStd::string>(data, "roughness.textureMapChannel"), "Green");
        EXPECT_EQ(Property<AZStd::string>(data, "metallic.textureMapChannel"), "Blue");
        EXPECT_FALSE(Property<bool>(data, "parallax.useTexture"));
        EXPECT_TRUE(data.HasPropertyValue(AZ::Name("normal.flipY")));
        EXPECT_FALSE(Property<bool>(data, "normal.flipY"));
        EXPECT_NE(data.m_description.find("DirectX"), AZStd::string::npos);
        ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(data, set.suggestedMaterialPath).IsSuccess());
        auto reloaded = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(set.suggestedMaterialPath);
        ASSERT_TRUE(reloaded.IsSuccess());
        EXPECT_EQ(reloaded.GetValue().m_materialTypeVersion, data.m_materialTypeVersion);
        EXPECT_EQ(reloaded.GetValue().m_materialType, data.m_materialType);
        EXPECT_EQ(reloaded.GetValue().m_description, data.m_description);
        const auto& expectedProperties = data.GetPropertyValues();
        const auto& actualProperties = reloaded.GetValue().GetPropertyValues();
        EXPECT_EQ(actualProperties.size(), expectedProperties.size());
        for (const auto& [propertyName, expectedValue] : expectedProperties)
        {
            SCOPED_TRACE(propertyName.GetCStr());
            const auto actualValue = actualProperties.find(propertyName);
            ASSERT_NE(actualValue, actualProperties.end());
            EXPECT_EQ(actualValue->second.GetTypeId(), expectedValue.GetTypeId());
            EXPECT_EQ(actualValue->second, expectedValue);
        }
        EXPECT_TRUE(JE::Editor::IsMaterialForTextureSet(set, set.suggestedMaterialPath));
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(set.suggestedMaterialPath);
        ASSERT_TRUE(json.IsSuccess());
        EXPECT_FALSE(json.GetValue().HasMember("ClassName")); // Native .material files are unwrapped JSON.
        EXPECT_TRUE(json.GetValue()["propertyValues"]["metallic.textureMap"].IsString());
    }

    TEST_F(MaterialTextureSetTests, OrdpPreservesBlueHeightWithoutConnectingMetallicOrParallax)
    {
        const auto anchor = File("rock_2K_ORDp.png");
        auto result = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(result.IsSuccess());
        const auto& set = result.GetValue();
        ASSERT_EQ(set.candidates.size(), 3);
        ASSERT_EQ(set.selected.size(), 2);
        const auto height = Candidate(set, Role::Height, Channel::Blue);
        ASSERT_LT(height, set.candidates.size());
        EXPECT_FALSE(set.candidates[height].supported);
        auto source = JE::Editor::BuildTextureMaterialSource(set, set.selected, set.suggestedMaterialPath);
        ASSERT_TRUE(source.IsSuccess());
        EXPECT_FALSE(source.GetValue().HasPropertyValue(AZ::Name("metallic.textureMap")));
        EXPECT_FALSE(source.GetValue().HasPropertyValue(AZ::Name("parallax.textureMap")));
        EXPECT_EQ(Property<AZStd::string>(source.GetValue(), "roughness.textureMapChannel"), "Green");
        EXPECT_NE(source.GetValue().m_description.find("Height | Blue"), AZStd::string::npos);
        EXPECT_NE(source.GetValue().m_description.find("NOT CONNECTED"), AZStd::string::npos);
        EXPECT_FALSE(JE::Editor::BuildTextureMaterialSource(set, {height}, set.suggestedMaterialPath).IsSuccess());
    }

    TEST_F(MaterialTextureSetTests, CreateNewPreservesManualCollisionAndProvenanceDoesNotAdoptAnotherSet)
    {
        const auto anchor = File("wood_diff_1K.png");
        auto analysis = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(analysis.IsSuccess());
        const auto& set = analysis.GetValue();
        auto source = JE::Editor::BuildTextureMaterialSource(set, set.selected, set.suggestedMaterialPath);
        ASSERT_TRUE(source.IsSuccess());
        const AZStd::string sentinel = "manually authored destination, deliberately not JSON";
        ASSERT_TRUE(AZ::Utils::WriteFile(sentinel, set.suggestedMaterialPath).IsSuccess());
        EXPECT_FALSE(JE::Editor::WriteMaterialSourceFileNew(source.GetValue(), set.suggestedMaterialPath).IsSuccess());
        EXPECT_FALSE(JE::Editor::IsMaterialForTextureSet(set, set.suggestedMaterialPath)); // No load diagnostics/modal for manual files.
        auto bytes = AZ::Utils::ReadFile<AZStd::string>(set.suggestedMaterialPath);
        ASSERT_TRUE(bytes.IsSuccess());
        EXPECT_EQ(bytes.GetValue(), sentinel);
        ASSERT_TRUE(AZ::IO::SystemFile::Delete(set.suggestedMaterialPath.c_str()));
        ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(source.GetValue(), set.suggestedMaterialPath).IsSuccess());
        auto other = set;
        other.identity += "_another";
        EXPECT_FALSE(JE::Editor::IsMaterialForTextureSet(other, set.suggestedMaterialPath));
        EXPECT_TRUE(JE::Editor::IsMaterialForTextureSet(set, set.suggestedMaterialPath));
    }

    TEST_F(MaterialTextureSetTests, MissingOrInvalidBundledTypeVersionFailsBeforeWriting)
    {
        const auto anchor = File("metal_diff_1K.png");
        auto analysis = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(analysis.IsSuccess());
        const auto& set = analysis.GetValue();
        QTemporaryDir invalidEngine;
        ASSERT_TRUE(invalidEngine.isValid());
        const QString relativeType = "Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype";
        ASSERT_TRUE(QDir(invalidEngine.path()).mkpath(QFileInfo(relativeType).path()));
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", invalidEngine.path().toUtf8().constData());
        EXPECT_FALSE(JE::Editor::BuildTextureMaterialSource(set, set.selected, set.suggestedMaterialPath).IsSuccess());
        const AZStd::string typePath = invalidEngine.filePath(relativeType).toUtf8().constData();
        for (const char* invalid : {"not json", "{}", "{\"version\":-1}", "{\"version\":0}", "{\"version\":6.5}", "{\"version\":\"6\"}"})
        {
            ASSERT_TRUE(AZ::Utils::WriteFile(invalid, typePath).IsSuccess());
            EXPECT_FALSE(JE::Editor::BuildTextureMaterialSource(set, set.selected, set.suggestedMaterialPath).IsSuccess());
            EXPECT_FALSE(AZ::IO::SystemFile::Exists(set.suggestedMaterialPath.c_str()));
        }
    }
    TEST_F(MaterialTextureSetTests, ExplicitNormalConventionsEmitComplementaryFlipsAndGenericKeepsTheDefault)
    {
        const auto anchor = File("paired_nor_gl_4k.png");
        File("paired_nor_dx_4k.png");
        auto analysis = JE::Editor::AnalyzeMaterialTextureSet(anchor);
        ASSERT_TRUE(analysis.IsSuccess());
        const auto& set = analysis.GetValue();
        ASSERT_EQ(set.candidates.size(), 2);
        EXPECT_TRUE(set.selected.empty()); // Two conventions are alternatives, never picked by enumeration order.
        size_t gl = set.candidates.size();
        size_t dx = set.candidates.size();
        for (size_t i = 0; i < set.candidates.size(); ++i)
        {
            if (set.candidates[i].normalConvention == JE::Editor::MaterialNormalConvention::OpenGL) { gl = i; }
            if (set.candidates[i].normalConvention == JE::Editor::MaterialNormalConvention::DirectX) { dx = i; }
        }
        ASSERT_LT(gl, set.candidates.size());
        ASSERT_LT(dx, set.candidates.size());
        auto glSource = JE::Editor::BuildTextureMaterialSource(set, {gl}, Path("gl.material"));
        auto dxSource = JE::Editor::BuildTextureMaterialSource(set, {dx}, Path("dx.material"));
        ASSERT_TRUE(glSource.IsSuccess());
        ASSERT_TRUE(dxSource.IsSuccess());
        EXPECT_TRUE(Property<bool>(glSource.GetValue(), "normal.flipY"));
        EXPECT_FALSE(Property<bool>(dxSource.GetValue(), "normal.flipY"));
        EXPECT_EQ(Property<AZStd::string>(glSource.GetValue(), "normal.textureMap"), "paired_nor_gl_4k.png");
        EXPECT_EQ(Property<AZStd::string>(dxSource.GetValue(), "normal.textureMap"), "paired_nor_dx_4k.png");
        ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(glSource.GetValue(), Path("gl.material")).IsSuccess());
        ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(dxSource.GetValue(), Path("dx.material")).IsSuccess());
        auto glReload = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(Path("gl.material"));
        auto dxReload = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(Path("dx.material"));
        ASSERT_TRUE(glReload.IsSuccess());
        ASSERT_TRUE(dxReload.IsSuccess());
        EXPECT_TRUE(Property<bool>(glReload.GetValue(), "normal.flipY"));
        EXPECT_FALSE(Property<bool>(dxReload.GetValue(), "normal.flipY"));
        for (auto* source : {&glReload.GetValue(), &dxReload.GetValue()})
        {
            source->RemovePropertyValue(AZ::Name("normal.textureMap"));
            source->RemovePropertyValue(AZ::Name("normal.flipY"));
        }
        EXPECT_EQ(glReload.GetValue().GetPropertyValues(), dxReload.GetValue().GetPropertyValues());
        const auto generic = File("generic_normal_4k.png");
        auto genericSet = JE::Editor::AnalyzeMaterialTextureSet(generic);
        ASSERT_TRUE(genericSet.IsSuccess());
        auto genericSource = JE::Editor::BuildTextureMaterialSource(
            genericSet.GetValue(), genericSet.GetValue().selected, Path("generic.material"));
        ASSERT_TRUE(genericSource.IsSuccess());
        EXPECT_FALSE(genericSource.GetValue().HasPropertyValue(AZ::Name("normal.flipY")));
        EXPECT_FALSE(genericSet.GetValue().warnings.empty());
        // These are source/serialization contracts. Equivalent tangent-space shading of inverse GL/DX textures requires the real preview gate.
    }

}


namespace UnitTest
{
    class MaterialDocumentSourceChangeTests : public MaterialDocumentTests
    {
    protected:
        class SourceNotifications final : public JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        {
        public:
            explicit SourceNotifications(const AZ::Uuid& id) : m_id(id) { BusConnect(AZ::Crc32("MaterialHistoryTest")); }
            ~SourceNotifications() override { BusDisconnect(); }
            void OnDocumentExternallyModified(const AZ::Uuid& id) override { if (id == m_id) { ++external; } }
            void OnDocumentDependencyModified(const AZ::Uuid& id) override { if (id == m_id) { ++dependency; } }
            AZ::Uuid m_id;
            int external = 0;
            int dependency = 0;
        };

        void SetUp() override
        {
            MaterialDocumentTests::SetUp();
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadDynamicModules = false;
            startup.m_loadStaticModules = false;
            startup.m_loadSettingsRegistry = false;
            startup.m_createEditContext = false;
            m_application->Create(descriptor, startup);
            auto* manager = AZ::ReflectionEnvironment::GetReflectionManager();
            manager->Reflect(azrtti_typeid<AZ::JsonSystemComponent>(), &AZ::JsonSystemComponent::Reflect);
            manager->Reflect(azrtti_typeid<JE::Render::RPI::JsonMaterialPropertyValueSerializer>(), &ReflectTextureMaterialJson);
            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            ASSERT_TRUE(m_directory.isValid());
            ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":6})", Path("types/source.materialtype")).IsSuccess());
            ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":6})", Path("types/alternate.materialtype")).IsSuccess());
            auto parent = Source();
            parent.m_parentMaterial.clear();
            ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(parent, Path("parent.material")).IsSuccess());
            ASSERT_TRUE(JE::Editor::WriteMaterialSourceFileNew(parent, Path("alternate.material")).IsSuccess());
            // Supply only source-folder and registered-source lookup boundaries for native save/path resolution.
            ON_CALL(m_assetSystem, GetAssetSafeFolders(testing::_)).WillByDefault(
                [this](AZStd::vector<AZStd::string>& folders)
                {
                    folders.push_back(m_directory.path().toUtf8().constData());
                    return true;
                });
            ON_CALL(m_assetSystem, GetSourceInfoBySourcePath(testing::_, testing::_, testing::_)).WillByDefault(
                [this](const char* sourcePath, AZ::Data::AssetInfo& info, AZStd::string& watchFolder)
                {
                    if (!sourcePath) { return false; }
                    const AZ::IO::Path root(m_directory.path().toUtf8().constData());
                    const auto requested = AZ::IO::Path(sourcePath).LexicallyNormal();
                    for (const char* relative : {"generated.material", "parent.material", "alternate.material",
                        "types/source.materialtype", "types/alternate.materialtype"})
                    {
                        const AZ::IO::Path registered(relative);
                        if (requested == registered || requested == (root / registered).LexicallyNormal())
                        {
                            // Registered source metadata survives the explicit missing-file control until removal notification.
                            info = {};
                            info.m_relativePath = registered.String();
                            watchFolder = root.String();
                            return true;
                        }
                    }
                    return false;
                });
            m_assetSystem.BusConnect();
        }

        void TearDown() override
        {
            m_assetSystem.BusDisconnect();
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
            }
            m_application->Destroy();
            MaterialDocumentTests::TearDown();
        }

        AZStd::string Path(const char* relative) const { return m_directory.filePath(relative).toUtf8().constData(); }
        JE::Render::RPI::MaterialSourceData Source() const
        {
            JE::Render::RPI::MaterialSourceData source;
            source.m_description = "Native source-event fixture";
            source.m_materialType = "types/source.materialtype";
            source.m_parentMaterial = "parent.material";
            source.m_materialTypeVersion = 6;
            source.SetPropertyValue(AZ::Name("surface.roughness"), JE::Render::RPI::MaterialPropertyValue(0.25f));
            return source;
        }

        AZStd::unique_ptr<CpuPropertyDocument> OpenSource()
        {
            const auto path = Path("generated.material");
            const auto written = JE::Editor::WriteMaterialSourceFileNew(Source(), path);
            EXPECT_TRUE(written.IsSuccess());
            if (!written.IsSuccess()) { return {}; }
            auto loaded = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(path);
            EXPECT_TRUE(loaded.IsSuccess());
            if (!loaded.IsSuccess()) { return {}; }
            auto document = MakeDocument(HistoryOwner::Editor, path.c_str());
            EXPECT_TRUE(BindLoadedSource(*document, path, loaded.TakeValue()));
            return document;
        }

        void Notify(const char* relative = "generated.material")
        {
            const AZStd::string watchFolder(m_directory.path().toUtf8().constData());
            ASSERT_EQ(JE::Render::RPI::AssetUtils::ResolvePathReference(watchFolder, relative),
                JE::Render::RPI::AssetUtils::ResolvePathReference("", Path(relative)));
            AzToolsFramework::AssetSystemBus::QueueBroadcast(
                &AzToolsFramework::AssetSystemBus::Events::SourceFileChanged,
                AZStd::string(relative), watchFolder, AZ::Uuid::CreateNull());
            AzToolsFramework::AssetSystemBus::ExecuteQueuedEvents();
        }

        QTemporaryDir m_directory;
        testing::NiceMock<UnitTests::MockAssetSystemRequest> m_assetSystem;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    TEST_F(MaterialDocumentSourceChangeTests, IdenticalSourceEventsPreserveUnsavedOverridesAndUndo)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.75f);
        ASSERT_TRUE(document->IsModified());
        Notify();
        Notify();
        EXPECT_EQ(notifications.external, 0);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        EXPECT_TRUE(document->IsModified());
        auto onDisk = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(Path("generated.material"));
        ASSERT_TRUE(onDisk.IsSuccess());
        EXPECT_FLOAT_EQ(onDisk.GetValue().GetPropertyValue(AZ::Name("surface.roughness")).GetValue<float>(), 0.25f);
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
    }

    TEST_F(MaterialDocumentSourceChangeTests, ExternalPropertiesMetadataAndReferencesRemainObservable)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.75f);
        for (int field = 0; field < 5; ++field)
        {
            auto external = Source();
            switch (field)
            {
            case 0: external.m_description = "Externally changed description"; break;
            case 1: external.m_materialType = "types/alternate.materialtype"; break;
            case 2: external.m_parentMaterial = "alternate.material"; break;
            case 3: external.m_materialTypeVersion = 7; break;
            case 4: external.SetPropertyValue(AZ::Name("surface.roughness"), JE::Render::RPI::MaterialPropertyValue(0.5f)); break;
            }
            ASSERT_TRUE(JE::Render::RPI::JsonUtils::SaveObjectToFile(Path("generated.material"), external));
            Notify();
            EXPECT_EQ(notifications.external, field + 1) << field;
            EXPECT_EQ(notifications.dependency, 0);
            EXPECT_FLOAT_EQ(Value(*document), 0.75f);
            EXPECT_TRUE(document->IsModified());
        }
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
    }

    TEST_F(MaterialDocumentSourceChangeTests, DependencyChangesKeepTheirSeparateNotification)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        AddSourceDependency(*document, Path("types/source.materialtype"));
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.75f);
        ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":7})", Path("types/source.materialtype")).IsSuccess());
        Notify("types/source.materialtype");
        EXPECT_EQ(notifications.external, 0);
        EXPECT_EQ(notifications.dependency, 1);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        EXPECT_TRUE(document->IsModified());
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
    }

    TEST_F(MaterialDocumentSourceChangeTests, MissingOrUnreadableSourceStillReportsExternalChange)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());
        ASSERT_TRUE(AZ::IO::SystemFile::Delete(Path("generated.material").c_str()));
        Notify();
        EXPECT_EQ(notifications.external, 1);
        ASSERT_TRUE(AZ::Utils::WriteFile("{", Path("generated.material")).IsSuccess());
        Notify();
        EXPECT_EQ(notifications.external, 2);
        EXPECT_EQ(notifications.dependency, 0);
    }

    TEST_F(MaterialDocumentSourceChangeTests, ColorSaveEchoIsIgnoredButExternalColorChangeRemainsObservable)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());

        const AZ::Name colorId("baseColor.color");
        const AZ::Color authoredColor(0.417883575f, 0.022171360f, 0.686686516f, 1.0f);
        ASSERT_NO_FATAL_FAILURE(SeedColorProperty(*document, colorId, authoredColor));

        ASSERT_TRUE(document->GetPropertyValue("baseColor.color").is<AZ::Color>());
        ASSERT_TRUE(document->Save());
        auto saved = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(Path("generated.material"));
        ASSERT_TRUE(saved.IsSuccess());
        // The generic source serializer resolves an untyped four-number JSON array to Vector4.
        ASSERT_TRUE(saved.GetValue().GetPropertyValue(colorId).Is<AZ::Vector4>());
        EXPECT_EQ(saved.GetValue().GetPropertyValue(colorId).GetValue<AZ::Vector4>(),
            AZ::Vector4(authoredColor.GetR(), authoredColor.GetG(), authoredColor.GetB(), authoredColor.GetA()));

        Edit(*document, 0.9f);
        Notify();
        Notify();
        EXPECT_EQ(notifications.external, 0);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
        EXPECT_TRUE(document->IsModified());

        auto external = saved.TakeValue();
        external.SetPropertyValue(colorId,
            JE::Render::RPI::MaterialPropertyValue(AZ::Vector4(0.1f, 0.2f, 0.3f, 1.0f)));
        ASSERT_TRUE(JE::Render::RPI::JsonUtils::SaveObjectToFile(Path("generated.material"), external));
        Notify();
        EXPECT_EQ(notifications.external, 1);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
        EXPECT_TRUE(document->IsModified());
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.25f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
    }

    TEST_F(MaterialDocumentSourceChangeTests, SaveRefreshesLoadedBaselineAndAnExternalRevertIsNotHidden)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.75f);
        ASSERT_TRUE(document->Save());
        EXPECT_FALSE(document->IsModified());
        auto saved = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(Path("generated.material"));
        ASSERT_TRUE(saved.IsSuccess());
        EXPECT_FLOAT_EQ(saved.GetValue().GetPropertyValue(AZ::Name("surface.roughness")).GetValue<float>(), 0.75f);
        ASSERT_NE(document->GetMaterialSourceData(), nullptr);
        const auto& loadedProperties = document->GetMaterialSourceData()->GetPropertyValues();
        const auto& savedProperties = saved.GetValue().GetPropertyValues();
        EXPECT_EQ(loadedProperties.size(), savedProperties.size());
        for (const auto& [name, value] : savedProperties)
        {
            const auto found = loadedProperties.find(name);
            EXPECT_NE(found, loadedProperties.end()) << name.GetCStr();
            if (found != loadedProperties.end())
            {
                EXPECT_TRUE(found->second == value) << name.GetCStr();
            }
        }
        EXPECT_EQ(document->GetMaterialSourceData()->m_materialTypeVersion, saved.GetValue().m_materialTypeVersion);

        Edit(*document, 0.9f);
        Notify(); // Native Save's delayed source notification must not discard the subsequent unsaved edit.
        EXPECT_EQ(notifications.external, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
        EXPECT_TRUE(document->IsModified());

        ASSERT_TRUE(JE::Render::RPI::JsonUtils::SaveObjectToFile(Path("generated.material"), Source()));
        Notify(); // Revert to the pre-save disk contents is a genuine external change, not an old cached match.
        EXPECT_EQ(notifications.external, 1);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
    }


    TEST_F(MaterialDocumentSourceChangeTests, UnknownJsonFieldsDoNotTurnAnExternalChangeIntoAnIdenticalMatch)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.75f);
        const auto path = Path("generated.material");
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(path);
        ASSERT_TRUE(json.IsSuccess());
        json.GetValue().AddMember("unrecognizedExternalField", true, json.GetValue().GetAllocator());
        ASSERT_TRUE(AZ::JsonSerializationUtils::WriteJsonFile(json.GetValue(), path).IsSuccess());
        Notify();
        EXPECT_EQ(notifications.external, 1);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        EXPECT_TRUE(document->IsModified());
    }


    TEST_F(MaterialDocumentSourceChangeTests, ExternalRevertBeforeTheSaveEchoIsNotDiscarded)
    {
        auto document = OpenSource();
        ASSERT_NE(document, nullptr);
        Edit(*document, 0.75f);
        ASSERT_TRUE(document->Save());
        SourceNotifications notifications(document->GetId());
        Edit(*document, 0.9f);
        // No source notification has consumed Save's pending self-change flag yet.
        ASSERT_TRUE(JE::Render::RPI::JsonUtils::SaveObjectToFile(Path("generated.material"), Source()));
        Notify();
        EXPECT_EQ(notifications.external, 1);
        EXPECT_EQ(notifications.dependency, 0);
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
        EXPECT_TRUE(document->IsModified());
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        EXPECT_FLOAT_EQ(Value(*document), 0.75f);
        m_application->GetUndoStack()->Redo();
        EXPECT_FLOAT_EQ(Value(*document), 0.9f);
    }
}
