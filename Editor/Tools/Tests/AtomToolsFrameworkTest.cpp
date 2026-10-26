/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <JE/Editor/Tools/DynamicProperty/DynamicProperty.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <JE/Editor/Tools/Graph/AssetStatusReporter.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class AtomToolsFrameworkTestEnvironment : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
        }

        void TeardownEnvironment() override
        {
        }
    };

    class AtomToolsFrameworkTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
            AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

            char rootPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutableDirectory(rootPath, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@exefolder@", rootPath);

            m_assetSystemStub.Activate();

            RegisterSourceAsset("objects/upgrades/materials/supercondor.material");
            RegisterSourceAsset("materials/condor.material");
            RegisterSourceAsset("materials/talisman.material");
            RegisterSourceAsset("materials/city.material");
            RegisterSourceAsset("materials/totem.material");
            RegisterSourceAsset("textures/orange.png");
            RegisterSourceAsset("textures/red.png");
            RegisterSourceAsset("textures/gold.png");
            RegisterSourceAsset("textures/fuzz.png");

            m_assetSystemStub.RegisterScanFolder(JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/root1/projects/project1/assets/"));
            m_assetSystemStub.RegisterScanFolder(JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/root1/projects/project2/assets/"));
            m_assetSystemStub.RegisterScanFolder(JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/assets/"));
            m_assetSystemStub.RegisterScanFolder(JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/testdata/"));
            m_assetSystemStub.RegisterScanFolder(JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/"));
        }

        void TearDown() override
        {
            m_assetSystemStub.Deactivate();

            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            m_localFileIO.reset();
        }

        void RegisterSourceAsset(const AZStd::string& path)
        {
            const AZStd::string assetRoot = "@exefolder@/root1/project/assets/";
            AZ::IO::FixedMaxPath assetRootPath(JE::Editor::Tools::GetPathWithoutAlias(assetRoot));
            AZ::IO::FixedMaxPath normalizedPath(JE::Editor::Tools::GetPathWithoutAlias(assetRoot + path));

            AZ::Data::AssetInfo assetInfo = {};
            assetInfo.m_assetId = AZ::Uuid::CreateRandom();
            assetInfo.m_relativePath = normalizedPath.LexicallyRelative(assetRootPath).StringAsPosix();
            m_assetSystemStub.RegisterSourceInfo(normalizedPath.StringAsPosix().c_str(), assetInfo, assetRootPath.StringAsPosix().c_str());
        }

        AssetSystemStub m_assetSystemStub;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
    };

    class DynamicPropertyLifetimeTest : public LeakDetectionFixture
    {
    };

    TEST_F(DynamicPropertyLifetimeTest, CopyMoveAndRebuildOwnEditAttributes)
    {
        int changeCount = 0;
        JE::Editor::Tools::DynamicPropertyConfig config;
        config.m_name = "value";
        config.m_displayName = "Authored value";
        config.m_originalValue = 0.25f;
        config.m_dataChangeCallback = [&changeCount](const AZStd::any&)
        {
            ++changeCount;
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        };
        auto checkProperty = [&changeCount](JE::Editor::Tools::DynamicProperty& property, const char* name, float value)
        {
            const auto* data = JE::Editor::Tools::DynamicProperty::GetPropertyEditData(
                &property, AZStd::any_cast<void>(&property.GetValue()), property.GetValue().type());
            ASSERT_NE(data, nullptr);
            auto* label = azrtti_cast<AZ::AttributeFunction<AZStd::string()>*>(
                data->FindAttribute(AZ::Edit::Attributes::NameLabelOverride));
            auto* changed = azrtti_cast<AZ::AttributeFunction<AZ::u32()>*>(
                data->FindAttribute(AZ::Edit::Attributes::ChangeNotify));
            ASSERT_NE(label, nullptr);
            ASSERT_NE(changed, nullptr);
            EXPECT_EQ(label->Invoke(nullptr), name);
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(property.GetValue()), value);
            const int previousCount = changeCount;
            EXPECT_EQ(changed->Invoke(nullptr), AZ::Edit::PropertyRefreshLevels::AttributesAndValues);
            EXPECT_EQ(changeCount, previousCount + 1);
        };
        JE::Editor::Tools::DynamicProperty survivor(config);
        survivor.UpdateEditData();
        {
            JE::Editor::Tools::DynamicProperty original(config);
            original.SetValue(7.0f);
            original.UpdateEditData();
            JE::Editor::Tools::DynamicProperty copied(original);
            ASSERT_NO_FATAL_FAILURE(checkProperty(copied, "Authored value", 7.0f));
            JE::Editor::Tools::DynamicProperty assigned(config);
            assigned.UpdateEditData();
            assigned = original;
            ASSERT_NO_FATAL_FAILURE(checkProperty(assigned, "Authored value", 7.0f));
            auto* self = &assigned;
            assigned = *self;
            assigned = AZStd::move(*self);
            ASSERT_NO_FATAL_FAILURE(checkProperty(assigned, "Authored value", 7.0f));
            JE::Editor::Tools::DynamicProperty moved(AZStd::move(copied));
            ASSERT_NO_FATAL_FAILURE(checkProperty(moved, "Authored value", 7.0f));
            survivor = AZStd::move(moved);
        }
        ASSERT_NO_FATAL_FAILURE(checkProperty(survivor, "Authored value", 7.0f));
        config.m_displayName = "Changed configuration";
        survivor.SetConfig(config);
        ASSERT_NO_FATAL_FAILURE(checkProperty(survivor, "Changed configuration", 7.0f));
    }

    TEST_F(DynamicPropertyLifetimeTest, ReusingMovedFromPropertyRebuildsCacheForItsCurrentValue)
    {
        JE::Editor::Tools::DynamicPropertyConfig config;
        config.m_originalValue = 0.25f;
        config.m_min = -2.0f;
        config.m_max = 9.0f;
        auto checkReusedSource = [](JE::Editor::Tools::DynamicProperty& source)
        {
            // Vector values accept the same float bounds but need attributes absent from the old scalar cache.
            source.SetValue(AZ::Vector2(3.0f, 4.0f));
            JE::Editor::Tools::DynamicProperty control(source.GetConfig());
            control.SetValue(source.GetValue());
            auto getData = [](JE::Editor::Tools::DynamicProperty& property)
            {
                return JE::Editor::Tools::DynamicProperty::GetPropertyEditData(
                    &property, AZStd::any_cast<void>(&property.GetValue()), property.GetValue().type());
            };
            const auto* actual = getData(source);
            const auto* expected = getData(control);
            ASSERT_NE(actual, nullptr);
            ASSERT_NE(expected, nullptr);
            ASSERT_NE(actual->FindAttribute(AZ::Edit::Attributes::LabelForX), nullptr);
            for (const auto id : {AZ::Edit::Attributes::Min, AZ::Edit::Attributes::Max})
            {
                auto* actualAttribute = actual->FindAttribute(id);
                auto* expectedAttribute = expected->FindAttribute(id);
                ASSERT_NE(actualAttribute, nullptr);
                ASSERT_NE(expectedAttribute, nullptr);
                float actualBound = 0.0f;
                float expectedBound = 0.0f;
                ASSERT_TRUE(AZ::AttributeReader(nullptr, actualAttribute).Read<float>(actualBound));
                ASSERT_TRUE(AZ::AttributeReader(nullptr, expectedAttribute).Read<float>(expectedBound));
                EXPECT_FLOAT_EQ(actualBound, expectedBound);
            }
        };
        {
            JE::Editor::Tools::DynamicProperty source(config);
            source.UpdateEditData();
            JE::Editor::Tools::DynamicProperty destination(AZStd::move(source));
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(destination.GetValue()), 0.25f);
            ASSERT_NO_FATAL_FAILURE(checkReusedSource(source));
        }
        {
            JE::Editor::Tools::DynamicProperty source(config);
            source.UpdateEditData();
            JE::Editor::Tools::DynamicProperty destination(config);
            destination.UpdateEditData();
            destination = AZStd::move(source);
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(destination.GetValue()), 0.25f);
            ASSERT_NO_FATAL_FAILURE(checkReusedSource(source));
        }
    }

    TEST_F(AtomToolsFrameworkTest, GetPathToExteralReference_Succeeds)
    {
        ASSERT_EQ(JE::Editor::Tools::GetPathToExteralReference("", ""), "");
        ASSERT_EQ(JE::Editor::Tools::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/condor.material", ""), "");
        ASSERT_EQ(JE::Editor::Tools::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/talisman.material", ""), "");
        ASSERT_EQ(JE::Editor::Tools::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/talisman.material", "@exefolder@/root1/project/assets/textures/gold.png"), "../textures/gold.png");
        ASSERT_EQ(JE::Editor::Tools::GetPathToExteralReference("@exefolder@/root1/project/assets/objects/upgrades/materials/supercondor.material", "@exefolder@/root1/project/assets/materials/condor.material"), "../../../materials/condor.material");
    }

    TEST_F(AtomToolsFrameworkTest, IsDocumentPathInSupportedFolder_Succeeds)
    {
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/project/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project1/assets/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project2/assets/somerandomasset.json"));
        ASSERT_FALSE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root2/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
        ASSERT_TRUE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/assets/somerandomasset.json"));
        ASSERT_TRUE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_TRUE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project2/assets/somerandomasset.json"));
        ASSERT_TRUE(JE::Editor::Tools::IsDocumentPathInSupportedFolder("@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
    }

    TEST_F(AtomToolsFrameworkTest, ValidateDocumentPath_Succeeds)
    {
        AZStd::string testPath;
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "../somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/project/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project1/assets/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project2/assets/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_FALSE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/assets/somerandomasset.json";
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project2/assets/somerandomasset.json";
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(testPath));
    }

    class NativePreviewSettingsTest : public AtomToolsFrameworkTest
    {
    protected:
        void SetUp() override
        {
            AtomToolsFrameworkTest::SetUp();
            m_localFileIO->SetAlias("@engroot@", JE::Editor::Tools::GetPathWithoutAlias("@exefolder@/preview-engine").c_str());
            m_localFileIO->SetAlias("@gemroot:MaterialEditor@",
                JE::Editor::Tools::GetPathWithoutAlias("@engroot@/Gems/Atom/Tools/MaterialEditor").c_str());
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_serializeContext->RegisterGenericType<AZStd::set<AZStd::string>>();
            m_jsonContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            AZ::JsonSystemComponent::Reflect(m_jsonContext.get());
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_registry->SetContext(m_serializeContext.get());
            m_registry->SetContext(m_jsonContext.get());
            m_priorRegistry = AZ::SettingsRegistry::Get();
            if (m_priorRegistry)
            {
                AZ::SettingsRegistry::Unregister(m_priorRegistry);
            }
            AZ::SettingsRegistry::Register(m_registry.get());
        }

        void TearDown() override
        {
            AZ::SettingsRegistry::Unregister(m_registry.get());
            if (m_priorRegistry)
            {
                AZ::SettingsRegistry::Register(m_priorRegistry);
            }
            m_registry.reset();
            m_jsonContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonContext.get());
            m_jsonContext->DisableRemoveReflection();
            m_jsonContext.reset();
            m_serializeContext.reset();
            // Release reflected generic types before the fixture and its allocator lifetime end.
            AZ::GetGlobalSerializeContextModule().Cleanup();
            AtomToolsFrameworkTest::TearDown();
        }

        static AZStd::string NativePath(const AZStd::string& suffix)
        {
            return "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/" + suffix;
        }

        AZ::SettingsRegistryInterface* m_priorRegistry = nullptr;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonContext;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
    };

    TEST_F(NativePreviewSettingsTest, DefaultsUseTheNativePreviewOwner)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        EXPECT_EQ(settings.GetLastLightingPresetPath(), NativePath("LightingPresets/neutral_urban.lightingpreset.azasset"));
        EXPECT_EQ(settings.GetLastModelPresetPath(), NativePath("ViewportModels/Shaderball.modelpreset.azasset"));
        const auto lights = settings.GetRegisteredLightingPresetPaths();
        const auto models = settings.GetRegisteredModelPresetPaths();
        ASSERT_EQ(lights.size(), 2);
        ASSERT_EQ(models.size(), 15);
        for (const auto& path : lights)
        {
            EXPECT_TRUE(path.starts_with(NativePath("LightingPresets/")));
        }
        for (const auto& path : models)
        {
            EXPECT_TRUE(path.starts_with(NativePath("ViewportModels/")));
        }
    }

    TEST_F(NativePreviewSettingsTest, PersistedAliasAndAbsolutePathsKeepTheirSourceIdentity)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::string suffix = "ViewportModels/Shaderball.modelpreset.azasset";
        const AZStd::string legacy = "@gemroot:MaterialEditor@/Assets/MaterialEditor/" + suffix;
        const auto native = NativePath(suffix);
        const AZ::Data::AssetId sourceId(AZ::Uuid::CreateString("{AE9105C0-0FC4-555D-BB6F-A1BAA8F11C3E}"), 0);
        // AP retains the source GUID when the scan folder owner moves; both lookups represent that identity.
        m_assetSystemStub.RegisterSourceInfo(JE::Editor::Tools::GetPathWithoutAlias(legacy), sourceId);
        m_assetSystemStub.RegisterSourceInfo(JE::Editor::Tools::GetPathWithoutAlias(native), sourceId);
        for (const auto& oldPath : {legacy, JE::Editor::Tools::GetPathWithoutAlias(legacy)})
        {
            ASSERT_TRUE(m_registry->Set("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", oldPath));
            EXPECT_EQ(settings.GetLastModelPresetPath(), native);
            EXPECT_EQ(settings.GetLastModelPresetPathWithoutAlias(), JE::Editor::Tools::GetPathWithoutAlias(native));
            EXPECT_EQ(settings.GetLastModelPresetAssetId(), sourceId);
        }
        const AZStd::string lighting = "@gemroot:MaterialEditor@/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset";
        ASSERT_TRUE(m_registry->Set("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", lighting));
        EXPECT_EQ(settings.GetLastLightingPresetPath(), NativePath("LightingPresets/neutral_urban.lightingpreset.azasset"));
        const AZStd::string custom = "@engroot@/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditorCustom/custom.modelpreset.azasset";
        ASSERT_TRUE(m_registry->Set("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", custom));
        EXPECT_EQ(settings.GetLastModelPresetPath(), custom);
    }

    TEST_F(NativePreviewSettingsTest, FavoritesRegisterAndUnregisterUseOneCanonicalPathWithoutRemovingCustomPresets)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::string suffix = "ViewportModels/Shaderball.modelpreset.azasset";
        const AZStd::string legacy = "@gemroot:MaterialEditor@/Assets/MaterialEditor/" + suffix;
        const auto native = NativePath(suffix);
        const AZStd::string custom = "@exefolder@/custom/Shaderball.modelpreset.azasset";
        const AZStd::set<AZStd::string> stored{legacy, JE::Editor::Tools::GetPathWithoutAlias(legacy), native, custom};
        ASSERT_TRUE(JE::Editor::Tools::SetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths", stored));
        EXPECT_EQ(settings.GetRegisteredModelPresetPaths(), (AZStd::set<AZStd::string>{native, custom}));
        settings.RegisterModelPresetPath(legacy);
        settings.RegisterModelPresetPath(JE::Editor::Tools::GetPathWithoutAlias(native));
        EXPECT_EQ(settings.GetRegisteredModelPresetPaths(), (AZStd::set<AZStd::string>{native, custom}));
        settings.UnregisterModelPresetPath(JE::Editor::Tools::GetPathWithoutAlias(legacy));
        EXPECT_EQ(settings.GetRegisteredModelPresetPaths(), (AZStd::set<AZStd::string>{custom}));
        settings.RegisterModelPresetPath(legacy);
        EXPECT_EQ(settings.GetRegisteredModelPresetPaths(), (AZStd::set<AZStd::string>{native, custom}));

        const AZStd::string oldLighting = "@gemroot:MaterialEditor@/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset";
        const auto nativeLighting = NativePath("LightingPresets/neutral_urban.lightingpreset.azasset");
        const AZStd::string customLighting = "@exefolder@/custom/custom.lightingpreset.azasset";
        ASSERT_TRUE(JE::Editor::Tools::SetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths",
            AZStd::set<AZStd::string>{oldLighting, customLighting}));
        settings.RegisterLightingPresetPath(JE::Editor::Tools::GetPathWithoutAlias(oldLighting));
        EXPECT_EQ(settings.GetRegisteredLightingPresetPaths(), (AZStd::set<AZStd::string>{nativeLighting, customLighting}));
        settings.UnregisterLightingPresetPath(oldLighting);
        EXPECT_EQ(settings.GetRegisteredLightingPresetPaths(), (AZStd::set<AZStd::string>{customLighting}));
    }

    TEST_F(NativePreviewSettingsTest, RenderPipelineDefaultsUseTheNativeOwner)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::string native = "@engroot@/Engine/Render/Features/Assets/Passes/MainRenderPipeline.azasset";
        EXPECT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
        EXPECT_EQ(settings.GetLastRenderPipelinePath(), JE::Editor::Tools::GetPathWithoutAlias(native));
        EXPECT_EQ(settings.GetLastRenderPipelinePathWithoutAlias(), JE::Editor::Tools::GetPathWithoutAlias(native));
        EXPECT_EQ(settings.GetRegisteredRenderPipelinePaths(), (AZStd::set<AZStd::string>{native}));
        EXPECT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
    }

    TEST_F(NativePreviewSettingsTest, StoredLegacyRenderPipelineKeepsTheNativeSourceIdentity)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::string legacy = "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset";
        const AZStd::string native = "@engroot@/Engine/Render/Features/Assets/Passes/MainRenderPipeline.azasset";
        const auto absolute = JE::Editor::Tools::GetPathWithoutAlias(native);
        const AZ::Data::AssetId sourceId(AZ::Uuid::CreateString("{2D51D9FC-FA8A-4F5D-9E42-D0E923DA4D01}"), 0);
        // Only the native source exists; no legacy Gem alias or duplicate source is installed.
        m_assetSystemStub.RegisterSourceInfo(absolute, sourceId);
        ASSERT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
        for (const auto& stored : {legacy, native, absolute})
        {
            ASSERT_TRUE(m_registry->Set("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", stored));
            // Stop the RED case here rather than producing a secondary missing-source diagnostic.
            ASSERT_EQ(settings.GetLastRenderPipelinePath(), absolute);
            EXPECT_EQ(settings.GetLastRenderPipelinePathWithoutAlias(), absolute);
            EXPECT_EQ(settings.GetLastRenderPipelineAssetId(), sourceId);
            AZStd::string retained;
            ASSERT_TRUE(m_registry->Get(retained, "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath"));
            EXPECT_EQ(retained, stored);
        }
        EXPECT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
    }

    TEST_F(NativePreviewSettingsTest, LegacyPipelineFavoritesShareOneNativePathWithoutRewritingStoredValues)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::string legacy = "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset";
        const AZStd::string native = "@engroot@/Engine/Render/Features/Assets/Passes/MainRenderPipeline.azasset";
        const AZStd::string secondLegacy = "@gemroot:Atom_Feature_Common@/Assets/Passes/LowEndRenderPipeline.azasset";
        const AZStd::string secondNative = "@engroot@/Engine/Render/Features/Assets/Passes/LowEndRenderPipeline.azasset";
        const AZStd::string custom = "@exefolder@/custom/CustomRenderPipeline.azasset";
        const AZStd::set<AZStd::string> stored{legacy, native, secondLegacy, custom};
        ASSERT_TRUE(JE::Editor::Tools::SetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", stored));
        ASSERT_EQ(settings.GetRegisteredRenderPipelinePaths(), (AZStd::set<AZStd::string>{native, secondNative, custom}));
        EXPECT_EQ(JE::Editor::Tools::GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", {}), stored);
        settings.RegisterRenderPipelinePath(legacy);
        EXPECT_EQ(settings.GetRegisteredRenderPipelinePaths(), (AZStd::set<AZStd::string>{native, secondNative, custom}));
        settings.UnregisterRenderPipelinePath(legacy);
        EXPECT_EQ(settings.GetRegisteredRenderPipelinePaths(), (AZStd::set<AZStd::string>{secondNative, custom}));
        settings.RegisterRenderPipelinePath(legacy);
        EXPECT_EQ(settings.GetRegisteredRenderPipelinePaths(), (AZStd::set<AZStd::string>{native, secondNative, custom}));
        EXPECT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
    }

    TEST_F(NativePreviewSettingsTest, CustomAndFalsePipelinePrefixesRetainTheirPaths)
    {
        JE::Editor::Tools::EntityPreviewViewportSettingsSystem settings;
        const AZStd::set<AZStd::string> paths{
            "@exefolder@/custom/CustomRenderPipeline.azasset",
            "@gemroot:Atom_Feature_CommonExtra@/Assets/Passes/MainRenderPipeline.azasset",
            "@gemroot:Atom_Feature_Common@/Assets/PassesExtra/MainRenderPipeline.azasset",
            "@gemroot:atom_feature_common@/Assets/Passes/MainRenderPipeline.azasset"};
        for (const auto& path : paths)
        {
            ASSERT_TRUE(m_registry->Set("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", path));
            EXPECT_EQ(settings.GetLastRenderPipelinePath(), JE::Editor::Tools::GetPathWithoutAlias(path));
        }
        ASSERT_TRUE(JE::Editor::Tools::SetSettingsObject(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", paths));
        EXPECT_EQ(settings.GetRegisteredRenderPipelinePaths(), paths);
        EXPECT_EQ(m_localFileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
    }

    class ReporterJobSource final : public AzToolsFramework::AssetSystemJobRequestBus::Handler
    {
    public:
        ReporterJobSource() { BusConnect(); }
        ~ReporterJobSource() override { BusDisconnect(); }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfo(
            const AZStd::string& sourcePath, const bool escalateJobs) override
        {
            ++m_queryCount;
            m_sourcePath = sourcePath;
            EXPECT_FALSE(escalateJobs);
            return AZ::Success(AzToolsFramework::AssetSystem::JobInfoContainer{m_job});
        }
        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByAssetID(
            const AZ::Data::AssetId&, const bool, bool) override { return AZ::Failure(); }
        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByJobKey(
            const AZStd::string&, const bool) override { return AZ::Failure(); }
        AZ::Outcome<AzToolsFramework::AssetSystem::JobStatus> GetAssetJobsStatusByJobKey(
            const AZStd::string&, const bool) override { return AZ::Failure(); }
        AZ::Outcome<AZStd::string> GetJobLog(AZ::u64) override
        {
            ADD_FAILURE() << "Reporting a failure must not retrieve the full job log.";
            return AZ::Failure();
        }

        AzToolsFramework::AssetSystem::JobInfo m_job;
        AZStd::string m_sourcePath;
        int m_queryCount = 0;
    };

    class ReporterErrorCapture final : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        ReporterErrorCapture() { BusConnect(); }
        ~ReporterErrorCapture() override { BusDisconnect(); }
        bool OnPreError(const char* window, const char*, int, const char*, const char* message) override
        {
            if (AZStd::string_view(window) != "AssetStatusReporter")
            {
                return false;
            }
            m_messages.emplace_back(message);
            return true;
        }
        AZStd::vector<AZStd::string> m_messages;
    };

    TEST(AssetStatusReporterTest, FailedJobsReportIdentityOnceAndRemainTerminal)
    {
        for (const auto status : {AzToolsFramework::AssetSystem::JobStatus::Failed,
                                 AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit})
        {
            ReporterJobSource jobs;
            jobs.m_job.m_status = status;
            jobs.m_job.m_jobKey = "Material Builder";
            jobs.m_job.m_platform = "pc";
            jobs.m_job.m_jobRunKey = 18446744073709551614ULL;
            ReporterErrorCapture errors;
            JE::Editor::Tools::AssetStatusReporter reporter({"/project/graph.material"});

            EXPECT_EQ(reporter.Update(), JE::Editor::Tools::AssetStatusReporterState::Failed);
            ASSERT_EQ(errors.m_messages.size(), 1);
            EXPECT_NE(errors.m_messages.front().find(jobs.m_sourcePath), AZStd::string::npos);
            EXPECT_NE(errors.m_messages.front().find(jobs.m_job.m_jobKey), AZStd::string::npos);
            EXPECT_NE(errors.m_messages.front().find("platform 'pc'"), AZStd::string::npos);
            EXPECT_NE(errors.m_messages.front().find("18446744073709551614"), AZStd::string::npos);
            EXPECT_EQ(reporter.GetCurrentPath(), "/project/graph.material");
            EXPECT_EQ(reporter.Update(), JE::Editor::Tools::AssetStatusReporterState::Failed);
            EXPECT_EQ(jobs.m_queryCount, 1);
            EXPECT_EQ(errors.m_messages.size(), 1);
        }
    }

    TEST(AssetStatusReporterTest, CompletedJobAdvancesWithoutAnError)
    {
        ReporterJobSource jobs;
        jobs.m_job.m_status = AzToolsFramework::AssetSystem::JobStatus::Completed;
        ReporterErrorCapture errors;
        JE::Editor::Tools::AssetStatusReporter reporter({"/project/graph.material"});

        EXPECT_EQ(reporter.Update(), JE::Editor::Tools::AssetStatusReporterState::Succeeded);
        EXPECT_TRUE(reporter.GetCurrentPath().empty());
        EXPECT_TRUE(errors.m_messages.empty());
        EXPECT_EQ(jobs.m_queryCount, 1);
    }
    AZ_UNIT_TEST_HOOK(new AtomToolsFrameworkTestEnvironment);
} // namespace UnitTest
