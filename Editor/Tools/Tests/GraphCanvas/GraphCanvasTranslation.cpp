/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <filesystem>

#include <AzTest/AzTest.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/thread.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <GraphCanvas/GraphCanvas.h>
#include <GraphCanvas/Translation/TranslationBus.h>

class GraphCanvasTranslationTests : public ::testing::Test
{
protected:

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::Mode::RECORD_NO_RECORDS;

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_systemEntity = m_app.Create(appDesc, startupParameters);
            ASSERT_NE(m_systemEntity, nullptr);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        AZ::ComponentApplication m_app;
        AZ::Entity* m_systemEntity = nullptr;

};

TEST_F(GraphCanvasTranslationTests, TranslationKey)
{
    JE::Editor::Tools::GraphCanvas::TranslationKey key1("Constructed");
    EXPECT_STRCASEEQ(key1.ToString().c_str(), "Constructed");

    JE::Editor::Tools::GraphCanvas::TranslationKey key2;

    // Test key assignment
    key2 = "START";
    EXPECT_EQ(key2, "START");

    // Test key concatenation with const char*
    key2 << "TEST";
    EXPECT_EQ(key2, "START.TEST");

    // Test key concatenation with AZStd::string
    AZStd::string test1 = "STRING";
    key2 << test1;
    EXPECT_EQ(key2, "START.TEST.STRING");

    // Test clear
    key2.clear();
    EXPECT_EQ(key2, "");

    // Test Key assignment into AZStd::string
    key2 << "NEW";
    AZStd::string test2 = key2;
    EXPECT_STRCASEEQ(test2.c_str(), key2.ToString().c_str());

    JE::Editor::Tools::GraphCanvas::TranslationKey key3 = key2;

    // Compare Key to Key
    EXPECT_EQ(key2, key3);

    // Compare Key to const char*
    EXPECT_EQ(key3, "NEW");
}

namespace
{
    class TranslationRegistrationObserver final
        : public AssetBuilderSDK::AssetBuilderBus::Handler
        , public AzToolsFramework::ToolsAssetSystemBus::Handler
    {
    public:
        TranslationRegistrationObserver()
        {
            AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusConnect();
        }

        ~TranslationRegistrationObserver() override
        {
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusDisconnect();
            AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();
        }

        void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& descriptor) override
        {
            ++m_builderCount;
            if (!descriptor.m_createJobFunction || !descriptor.m_processJobFunction)
            {
                ++m_emptyCallbackCount;
            }
        }

        void RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* filter) override
        {
            ++m_sourceTypeCount;
            EXPECT_EQ(assetType, AZ::Uuid::CreateString("{6A1A3B00-3DF2-4297-96BB-3BA067A978E6}"));
            EXPECT_STREQ(filter, ".names");
        }

        void UnregisterSourceAssetType(const AZ::Data::AssetType&) override {}

        unsigned int m_builderCount = 0;
        unsigned int m_emptyCallbackCount = 0;
        unsigned int m_sourceTypeCount = 0;
    };
}

TEST_F(GraphCanvasTranslationTests, NativeTranslationRegistrationKeepsHandlerWithoutLocalBuilder)
{
    ASSERT_FALSE(AssetBuilderSDK::AssetBuilderBus::HasHandlers());
    ASSERT_FALSE(AzToolsFramework::ToolsAssetSystemBus::HasHandlers());
    ASSERT_FALSE(AZ::Data::AssetManager::IsReady());
    TranslationRegistrationObserver registration;

    m_app.RegisterComponentDescriptor(AZ::TaskGraphSystemComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(JE::Editor::Tools::GraphCanvas::GraphCanvasSystemComponent::CreateDescriptor());
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::TaskGraphSystemComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::JobManagerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::StreamerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::AssetManagerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<JE::Editor::Tools::GraphCanvas::GraphCanvasSystemComponent>(), nullptr);
    m_systemEntity->Init();

    const AZ::Data::AssetType translationType = AZ::Uuid::CreateString("{6A1A3B00-3DF2-4297-96BB-3BA067A978E6}");
    for (unsigned int cycle = 0; cycle < 2; ++cycle)
    {
        m_systemEntity->Activate();
        ASSERT_EQ(m_systemEntity->GetState(), AZ::Entity::State::Active);
        ASSERT_TRUE(AZ::Data::AssetManager::IsReady());
        AZ::Data::AssetHandler* handler = AZ::Data::AssetManager::Instance().GetHandler(translationType);
        ASSERT_NE(handler, nullptr);
        EXPECT_TRUE(handler->RTTI_IsTypeOf(azrtti_typeid<JE::Editor::Tools::GraphCanvas::TranslationAssetHandler>()));
        EXPECT_TRUE(JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::HasHandlers());
        EXPECT_EQ(registration.m_sourceTypeCount, cycle + 1);
        EXPECT_EQ(registration.m_emptyCallbackCount, 0);
        EXPECT_EQ(registration.m_builderCount, 0);

        m_systemEntity->Deactivate();
        EXPECT_EQ(m_systemEntity->GetState(), AZ::Entity::State::Init);
        EXPECT_FALSE(JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::HasHandlers());
        EXPECT_FALSE(AZ::Data::AssetManager::IsReady());
    }
}

// Opt-in integration gate: cwd must contain the pinned, byteexact AP product copy.
TEST_F(GraphCanvasTranslationTests, DISABLED_NativeTranslationProductLoadsExactKey)
{
    using namespace JE::Editor::Tools::GraphCanvas;
    constexpr const char* productBasename = "jenativegraphcanvastranslationvalidation.names";
    const AZStd::string key = "NativeGraphCanvas.JENativeGraphCanvasTranslationValidation.details.name";
    const AZStd::string expectedValue = "Jerico Native Translation Validation";

    std::error_code pathError;
    const std::filesystem::path productPath = std::filesystem::absolute(productBasename, pathError);
    ASSERT_FALSE(pathError) << pathError.message();
    const AZStd::string productFilename(productPath.generic_string().c_str());
    ASSERT_TRUE(AZ::IO::SystemFile::Exists(productFilename.c_str()))
        << "Required AP product is missing from the gate cwd: " << productFilename.c_str();
    const AZ::u64 productSize = AZ::IO::SystemFile::Length(productFilename.c_str());
    ASSERT_GT(productSize, 0);

    ASSERT_FALSE(AssetBuilderSDK::AssetBuilderBus::HasHandlers());
    ASSERT_FALSE(AzToolsFramework::ToolsAssetSystemBus::HasHandlers());
    ASSERT_FALSE(AZ::Data::AssetManager::IsReady());
    TranslationRegistrationObserver registration;
    m_app.RegisterComponentDescriptor(AZ::TaskGraphSystemComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
    m_app.RegisterComponentDescriptor(GraphCanvasSystemComponent::CreateDescriptor());
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::TaskGraphSystemComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::JobManagerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::StreamerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<AZ::AssetManagerComponent>(), nullptr);
    ASSERT_NE(m_systemEntity->CreateComponent<GraphCanvasSystemComponent>(), nullptr);
    m_systemEntity->Init();
    m_systemEntity->Activate();
    ASSERT_EQ(m_systemEntity->GetState(), AZ::Entity::State::Active);
    ASSERT_TRUE(AZ::Data::AssetManager::IsReady());
    ASSERT_TRUE(TranslationRequestBus::HasHandlers());
    TranslationAssetHandler* handler = azrtti_cast<TranslationAssetHandler*>(
        AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<TranslationAsset>()));
    ASSERT_NE(handler, nullptr);

    AZStd::string value;
    bool found = false;
    TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
    ASSERT_FALSE(found) << "The key must be absent before loading the AP product.";

    auto stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
    stream->Open(productFilename, 0, static_cast<size_t>(productSize));
    stream->BlockUntilLoadComplete();
    ASSERT_TRUE(stream->IsOpen());
    ASSERT_TRUE(stream->IsFullyLoaded());
    ASSERT_EQ(stream->GetLoadedSize(), productSize);
    const AZ::Data::Asset<AZ::Data::AssetData> diagnosticAsset;
    ASSERT_EQ(handler->LoadAssetData(diagnosticAsset, stream, {}), AZ::Data::AssetHandler::LoadResult::LoadComplete);
    TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
    ASSERT_TRUE(found);
    EXPECT_EQ(value, expectedValue);
    RecordProperty("translation_key", key.c_str());
    RecordProperty("translation_value", value.c_str());
    EXPECT_EQ(registration.m_sourceTypeCount, 1);
    EXPECT_EQ(registration.m_builderCount, 0);
    EXPECT_EQ(registration.m_emptyCallbackCount, 0);

    stream->Close();
    stream.reset();
    m_systemEntity->Deactivate();
    EXPECT_EQ(m_systemEntity->GetState(), AZ::Entity::State::Init);
    EXPECT_FALSE(GraphCanvasRequestBus::HasHandlers());
    EXPECT_FALSE(AZ::Data::AssetManager::IsReady());
}

namespace
{
    // Real disk I/O for this isolated test process; no registry or alias is changed in the caller.
    class TranslationCatalogFileIoScope final
    {
    public:
        explicit TranslationCatalogFileIoScope(const char* cacheRoot)
            : m_previous(AZ::IO::FileIOBase::GetInstance())
            , m_previousDirect(AZ::IO::FileIOBase::GetDirectInstance())
        {
            m_local.SetAlias("@products@", cacheRoot);
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_local);
            AZ::IO::FileIOBase::SetDirectInstance(&m_local);
        }
        ~TranslationCatalogFileIoScope()
        {
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetDirectInstance(m_previousDirect);
            AZ::IO::FileIOBase::SetInstance(m_previous);
        }
    private:
        AZ::IO::LocalFileIO m_local;
        AZ::IO::FileIOBase* m_previous;
        AZ::IO::FileIOBase* m_previousDirect;
    };

    class TranslationCatalogObservation final
        : public AzFramework::AssetCatalogEventBus::Handler
        , public AZ::Data::AssetBus::Handler
    {
    public:
        TranslationCatalogObservation() { AzFramework::AssetCatalogEventBus::Handler::BusConnect(); }
        ~TranslationCatalogObservation() { DisconnectAndRelease(); }
        void Watch(const AZ::Data::AssetId& id) { AZ::Data::AssetBus::Handler::BusConnect(id); }
        void OnCatalogLoaded(const char* file) override { ++m_catalogLoaded; m_catalogFile = file; }
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override { ++m_ready; m_asset = asset; }
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData>) override { ++m_errors; }
        void DisconnectAndRelease()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            m_asset.Reset();
        }
        int m_catalogLoaded = 0;
        int m_ready = 0;
        int m_errors = 0;
        AZStd::string m_catalogFile;
        AZ::Data::Asset<AZ::Data::AssetData> m_asset;
    };

    // A single filtered process owns these queues. Also runs on fatal assertions.
    struct TranslationCatalogQueueCleanup final
    {
        AZ::Entity* m_entity = nullptr;
        ~TranslationCatalogQueueCleanup()
        {
            AZ::Data::AssetCatalogRequestBus::ClearQueuedEvents();
            AZ::TickBus::ClearQueuedEvents();
            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }
            if (m_entity && m_entity->GetState() == AZ::Entity::State::Active)
            {
                m_entity->Deactivate();
            }
        }
    };
}

TEST_F(GraphCanvasTranslationTests, DISABLED_NativeTranslationCatalogLoadsExactKey)
{
    using namespace JE::Editor::Tools::GraphCanvas;
    constexpr const char* relativeProduct = "assets/jenativegraphcanvastranslationvalidation.names";
    const AZStd::string key = "NativeGraphCanvas.JENativeGraphCanvasTranslationValidation.details.name";
    const AZStd::string expectedValue = "Jerico Native Translation Validation";
    std::error_code pathError;
    const std::filesystem::path catalogPath = std::filesystem::absolute("assetcatalog.xml", pathError);
    ASSERT_FALSE(pathError) << pathError.message();
    const AZStd::string catalogFilename(catalogPath.generic_string().c_str());
    const AZStd::string cacheRoot(catalogPath.parent_path().generic_string().c_str());
    const AZStd::string productFilename((catalogPath.parent_path() / relativeProduct).generic_string().c_str());
    ASSERT_TRUE(AZ::IO::SystemFile::Exists(catalogFilename.c_str())) << "Use the existing JEEditorFixture Cache/pc cwd.";
    ASSERT_TRUE(AZ::IO::SystemFile::Exists(productFilename.c_str()));
    ASSERT_EQ(AZ::IO::SystemFile::Length(productFilename.c_str()), 155);
    ASSERT_FALSE(AZ::Data::AssetCatalogRequestBus::HasHandlers());
    ASSERT_FALSE(AZ::Data::AssetManager::IsReady());
    ASSERT_FALSE(AssetBuilderSDK::AssetBuilderBus::HasHandlers());
    ASSERT_FALSE(AzToolsFramework::ToolsAssetSystemBus::HasHandlers());
    auto* previousIo = AZ::IO::FileIOBase::GetInstance();
    auto* previousDirectIo = AZ::IO::FileIOBase::GetDirectInstance();
    {
        TranslationCatalogFileIoScope io(cacheRoot.c_str());
        TranslationRegistrationObserver registration;
        m_app.RegisterComponentDescriptor(AZ::TaskGraphSystemComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(GraphCanvasSystemComponent::CreateDescriptor());
        ASSERT_NE(m_systemEntity->CreateComponent<AZ::TaskGraphSystemComponent>(), nullptr);
        ASSERT_NE(m_systemEntity->CreateComponent<AZ::JobManagerComponent>(), nullptr);
        ASSERT_NE(m_systemEntity->CreateComponent<AZ::StreamerComponent>(), nullptr);
        ASSERT_NE(m_systemEntity->CreateComponent<AZ::AssetManagerComponent>(), nullptr);
        ASSERT_NE(m_systemEntity->CreateComponent<GraphCanvasSystemComponent>(), nullptr);
        m_systemEntity->Init();
        AZStd::unique_ptr<AzFramework::AssetCatalog> catalog(aznew AzFramework::AssetCatalog());
        TranslationCatalogQueueCleanup queueCleanup{ m_systemEntity };
        TranslationCatalogObservation observation;
        m_systemEntity->Activate();
        ASSERT_EQ(m_systemEntity->GetState(), AZ::Entity::State::Active);
        ASSERT_TRUE(AZ::Data::AssetManager::IsReady());
        ASSERT_TRUE(TranslationRequestBus::HasHandlers());
        AZStd::string value;
        bool found = false;
        TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
        ASSERT_FALSE(found);

        // This public real loader reads the existing AP catalogue and queues its own notification.
        // LoadCatalog is intentionally not used: it may send SaveCatalog to a connected AP.
        ASSERT_EQ(observation.m_catalogLoaded, 0);
        catalog->InitializeCatalog(catalogFilename.c_str());
        const auto id = catalog->GetAssetIdByPath(relativeProduct, azrtti_typeid<TranslationAsset>(), false);
        ASSERT_TRUE(id.IsValid());
        const auto info = catalog->GetAssetInfoById(id);
        ASSERT_EQ(info.m_assetType, azrtti_typeid<TranslationAsset>());
        ASSERT_EQ(info.m_relativePath, relativeProduct);
        ASSERT_EQ(info.m_sizeBytes, 155);
        ASSERT_EQ(id.m_subId, 0);
        int translationAssetCount = 0;
        AZStd::string translationAssetPaths;
        catalog->EnumerateAssets(nullptr,
            [&translationAssetCount, &translationAssetPaths](const AZ::Data::AssetId, const AZ::Data::AssetInfo& entry)
            {
                if (entry.m_assetType == azrtti_typeid<TranslationAsset>())
                {
                    ++translationAssetCount;
                    translationAssetPaths += entry.m_relativePath;
                    translationAssetPaths += "\n";
                }
            }, nullptr);
        RecordProperty("catalog_translation_asset_count", translationAssetCount);
        RecordProperty("catalog_translation_asset_paths", translationAssetPaths.c_str());
        ASSERT_EQ(translationAssetCount, 1) << "Review catalogue inventory before enabling Restore loads.";
        // AssetBus connection resolves the id through the catalog bus, which may drain queued notifications.
        observation.Watch(id);
        TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
        ASSERT_FALSE(found);
        AZ::Data::AssetCatalogRequestBus::ExecuteQueuedEvents();
        ASSERT_EQ(observation.m_catalogLoaded, 1);
        EXPECT_EQ(observation.m_catalogFile, catalogFilename);
        TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
        ASSERT_FALSE(found) << "Catalogue notification must queue Restore work until Tick.";
        AZ::TickBus::ExecuteQueuedEvents();
        const auto deadline = AZStd::chrono::steady_clock::now() + AZStd::chrono::seconds(10);
        do
        {
            AZ::Data::AssetManager::Instance().DispatchEvents();
            TranslationRequestBus::BroadcastResult(found, &TranslationRequests::Get, key, value);
            if ((found && observation.m_ready != 0) || observation.m_errors != 0)
            {
                break;
            }
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
        } while (AZStd::chrono::steady_clock::now() < deadline);
        RecordProperty("catalog_file", catalogFilename.c_str());
        RecordProperty("catalog_asset_id", id.ToString<AZStd::string>().c_str());
        RecordProperty("catalog_asset_path", info.m_relativePath.c_str());
        RecordProperty("catalog_loaded_events", observation.m_catalogLoaded);
        RecordProperty("asset_ready_events", observation.m_ready);
        RecordProperty("asset_error_events", observation.m_errors);
        RecordProperty("translation_key", key.c_str());
        RecordProperty("translation_value", value.c_str());
        ASSERT_EQ(observation.m_errors, 0);
        ASSERT_EQ(observation.m_ready, 1);
        ASSERT_TRUE(observation.m_asset.IsReady());
        EXPECT_EQ(observation.m_asset.GetId(), id);
        EXPECT_EQ(observation.m_asset.GetType(), azrtti_typeid<TranslationAsset>());
        ASSERT_TRUE(found);
        EXPECT_EQ(value, expectedValue);
        EXPECT_EQ(registration.m_sourceTypeCount, 1);
        EXPECT_EQ(registration.m_builderCount, 0);
        EXPECT_EQ(registration.m_emptyCallbackCount, 0);
        observation.DisconnectAndRelease();
        AZ::Data::AssetManager::Instance().DispatchEvents();
        m_systemEntity->Deactivate();
        catalog.reset();
        EXPECT_FALSE(AZ::Data::AssetManager::IsReady());
        EXPECT_FALSE(GraphCanvasRequestBus::HasHandlers());
    }
    EXPECT_EQ(AZ::IO::FileIOBase::GetInstance(), previousIo);
    EXPECT_EQ(AZ::IO::FileIOBase::GetDirectInstance(), previousDirectIo);
}
