/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ApplicationManagerTests.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <tests/assetmanager/MockAssetProcessorManager.h>
#include <tests/assetmanager/MockFileProcessor.h>
#include <AzToolsFramework/Archive/ArchiveComponent.h>
#include <native/FileWatcher/FileWatcher.h>
#include <native/utilities/AssetServerHandler.h>
#include <native/resourcecompiler/rcjob.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <unittests/UnitTestUtils.h>

namespace UnitTests
{
    void ApplicationManagerTest::SetUp()
    {
        LeakDetectionFixture::SetUp();

        AZ::IO::Path assetRootDir(m_databaseLocationListener.GetAssetRootDir());

        // We need a QCoreApplication to run the event loop
        int argc = 0;
        m_coreApplication = AZStd::make_unique<QCoreApplication>(argc, nullptr);

        m_applicationManager = AZStd::make_unique<MockBatchApplicationManager>(&argc, nullptr);
        m_applicationManager->m_platformConfiguration = new AssetProcessor::PlatformConfiguration{ nullptr };
        m_applicationManager->m_fileStateCache = AZStd::make_unique<AssetProcessor::FileStateCache>();

        m_mockAPM = CreateAssetProcessorManager();
        m_applicationManager->m_assetProcessorManager = m_mockAPM.get();

        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        m_applicationManager->m_platformConfiguration->EnablePlatform(AssetBuilderSDK::PlatformInfo{ "pc", { "tag" } });
        m_applicationManager->m_platformConfiguration->PopulatePlatformsForScanFolder(platforms);
        m_applicationManager->m_platformConfiguration->AddScanFolder(
            AssetProcessor::ScanFolderInfo{ assetRootDir.c_str(), "test", "test", true, true, platforms });

        m_apmThread = AZStd::make_unique<QThread>(nullptr);
        m_apmThread->setObjectName("APM Thread");
        m_applicationManager->m_assetProcessorManager->moveToThread(m_apmThread.get());
        m_apmThread->start();

        m_fileProcessorThread = AZStd::make_unique<QThread>(nullptr);
        m_fileProcessorThread->setObjectName("File Processor Thread");
        auto fileProcessor = AZStd::make_unique<MockFileProcessor>(m_applicationManager->m_platformConfiguration);
        fileProcessor->moveToThread(m_fileProcessorThread.get());
        m_mockFileProcessor = fileProcessor.get();
        m_applicationManager->m_fileProcessor = AZStd::move(fileProcessor); // The manager is taking ownership
        m_fileProcessorThread->start();

        m_applicationManager->InitUuidManager();

        auto fileWatcher = AZStd::make_unique<FileWatcher>();
        m_fileWatcher = fileWatcher.get();

        // This is what we're testing, it will set up connections between the fileWatcher and the 2 QObject handlers we'll check
        m_applicationManager->InitFileMonitor(AZStd::move(fileWatcher)); // The manager is going to take ownership of the file watcher

        m_applicationManager->InitUuidManager();
    }

    AZStd::unique_ptr<MockAssetProcessorManager> ApplicationManagerTest::CreateAssetProcessorManager()
    {
        return AZStd::make_unique<MockAssetProcessorManager>(m_applicationManager->m_platformConfiguration, nullptr);
    }

    void ApplicationManagerTest::TearDown()
    {
        m_applicationManager->DestroyFileMonitor();

        m_apmThread->quit();
        m_fileProcessorThread->quit();
        m_apmThread->wait();
        m_fileProcessorThread->wait();
        m_mockAPM = nullptr;

        LeakDetectionFixture::TearDown();
    }

    using BatchApplicationManagerTest = ::UnitTest::LeakDetectionFixture;

    struct ReprocessAssetProcessorManager : MockAssetProcessorManager
    {
        using MockAssetProcessorManager::MockAssetProcessorManager;

        void AssessModifiedFile(QString filePath) override
        {
            m_assessedPaths.push_back(AZStd::move(filePath));
        }

        QStringList m_assessedPaths;
    };

    class ReprocessFileListTest : public ApplicationManagerTest
    {
    protected:
        AZStd::unique_ptr<MockAssetProcessorManager> CreateAssetProcessorManager() override
        {
            auto processor = AZStd::make_unique<ReprocessAssetProcessorManager>(
                m_applicationManager->m_platformConfiguration, nullptr);
            m_reprocessAPM = processor.get();
            return processor;
        }

        void SetUp() override
        {
            ApplicationManagerTest::SetUp();

            m_scanFolder = QDir(m_databaseLocationListener.GetAssetRootDir().c_str());
            auto database = m_mockAPM->GetDatabaseConnection();
            ASSERT_NE(database, nullptr);

            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder(
                m_scanFolder.absolutePath().toUtf8().constData(), "test", "test");
            ASSERT_TRUE(database->SetScanFolder(scanFolder));
            m_scanFolderId = scanFolder.m_scanFolderID;
            m_applicationManager->m_platformConfiguration->AddScanFolder(AssetProcessor::ScanFolderInfo{
                m_scanFolder.absolutePath(), "test", "test", true, true,
                m_applicationManager->m_platformConfiguration->GetEnabledPlatforms(), 0, m_scanFolderId });

            m_firstJob = AzToolsFramework::AssetDatabase::JobDatabaseEntry(
                AzToolsFramework::AssetDatabase::InvalidEntryId, "compile", 101, "pc", AZ::Uuid::CreateRandom(),
                AzToolsFramework::AssetSystem::JobStatus::Completed, 1);
            m_secondJob = AzToolsFramework::AssetDatabase::JobDatabaseEntry(
                AzToolsFramework::AssetDatabase::InvalidEntryId, "compile", 202, "pc", AZ::Uuid::CreateRandom(),
                AzToolsFramework::AssetSystem::JobStatus::Completed, 2);
            CreateSource("no_jobs.azsli");
            CreateSource("first.shader", &m_firstJob);
            CreateSource("second.shader", &m_secondJob);
        }

        void CreateSource(const char* name, AzToolsFramework::AssetDatabase::JobDatabaseEntry* job = nullptr)
        {
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(m_scanFolder.absoluteFilePath(name)));
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry source(m_scanFolderId, name, AZ::Uuid::CreateRandom(), "fingerprint");
            auto database = m_mockAPM->GetDatabaseConnection();
            ASSERT_TRUE(database->SetSource(source));
            if (job)
            {
                job->m_sourcePK = source.m_sourceID;
                ASSERT_TRUE(database->SetJob(*job));
            }
        }

        void ExpectJobFingerprint(const AzToolsFramework::AssetDatabase::JobDatabaseEntry& original, AZ::u32 expected)
        {
            AzToolsFramework::AssetDatabase::JobDatabaseEntry actual;
            ASSERT_TRUE(m_mockAPM->GetDatabaseConnection()->GetJobByJobID(original.m_jobID, actual));
            EXPECT_EQ(actual.m_fingerprint, expected);
        }

        ReprocessAssetProcessorManager* m_reprocessAPM = nullptr; // Owned by m_mockAPM.
        QDir m_scanFolder;
        AZ::s64 m_scanFolderId = AzToolsFramework::AssetDatabase::InvalidEntryId;
        AzToolsFramework::AssetDatabase::JobDatabaseEntry m_firstJob;
        AzToolsFramework::AssetDatabase::JobDatabaseEntry m_secondJob;
    };

    TEST_F(ReprocessFileListTest, EmptyList_DoesNotWaitForWork)
    {
        EXPECT_FALSE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_filesToReprocess.isEmpty());
        ExpectJobFingerprint(m_firstJob, 101);
        ExpectJobFingerprint(m_secondJob, 202);
        EXPECT_TRUE(m_reprocessAPM->m_assessedPaths.isEmpty());
    }

    TEST_F(ReprocessFileListTest, SourcesWithoutJobs_ExhaustListWithoutWaitingForWork)
    {
        QTemporaryDir outsideScanFolder;
        ASSERT_TRUE(outsideScanFolder.isValid());
        const QString outsideSource = outsideScanFolder.filePath("unmapped.shader");
        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(outsideSource));
        const QString sourceWithoutJobs = m_scanFolder.absoluteFilePath("no_jobs.azsli");
        ASSERT_EQ(m_mockAPM->RequestReprocess(sourceWithoutJobs), 0);
        ASSERT_EQ(m_mockAPM->RequestReprocess(outsideSource), 0);
        m_applicationManager->m_filesToReprocess = QStringList{ sourceWithoutJobs, outsideSource };

        EXPECT_FALSE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_filesToReprocess.isEmpty());
        ExpectJobFingerprint(m_firstJob, 101);
        ExpectJobFingerprint(m_secondJob, 202);
        EXPECT_TRUE(m_reprocessAPM->m_assessedPaths.isEmpty());
    }

    TEST_F(ReprocessFileListTest, MixedList_RequestsOnlyFirstProductiveSourceAndPreservesOrder)
    {
        const QString sourceWithoutJobs = m_scanFolder.absoluteFilePath("no_jobs.azsli");
        const QString firstSource = m_scanFolder.absoluteFilePath("first.shader");
        const QString secondSource = m_scanFolder.absoluteFilePath("second.shader");
        m_applicationManager->m_filesToReprocess = QStringList{
            sourceWithoutJobs, firstSource, sourceWithoutJobs, secondSource, sourceWithoutJobs };

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_EQ(m_applicationManager->m_filesToReprocess, (QStringList{ sourceWithoutJobs, secondSource, sourceWithoutJobs }));
        ExpectJobFingerprint(m_firstJob, 0);
        ExpectJobFingerprint(m_secondJob, 202);

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_EQ(m_applicationManager->m_filesToReprocess, (QStringList{ sourceWithoutJobs }));
        ExpectJobFingerprint(m_firstJob, 0);
        ExpectJobFingerprint(m_secondJob, 0);

        EXPECT_FALSE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_filesToReprocess.isEmpty());
        EXPECT_EQ(m_reprocessAPM->m_assessedPaths, (QStringList{ firstSource, secondSource }));
    }

    TEST_F(ReprocessFileListTest, ProductiveSources_WaitAfterEachRequestIncludingLast)
    {
        const QString firstSource = m_scanFolder.absoluteFilePath("first.shader");
        const QString secondSource = m_scanFolder.absoluteFilePath("second.shader");
        m_applicationManager->m_filesToReprocess = QStringList{ firstSource, secondSource };

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_EQ(m_applicationManager->m_filesToReprocess, (QStringList{ secondSource }));
        ExpectJobFingerprint(m_firstJob, 0);
        ExpectJobFingerprint(m_secondJob, 202);

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_filesToReprocess.isEmpty());
        ExpectJobFingerprint(m_secondJob, 0);
        EXPECT_FALSE(m_applicationManager->CheckReprocessFileList());
        EXPECT_EQ(m_reprocessAPM->m_assessedPaths, (QStringList{ firstSource, secondSource }));
    }

    TEST_F(ReprocessFileListTest, FileListWithCrLf_RequestsSourcesInOrder)
    {
        const QString firstSource = m_scanFolder.absoluteFilePath("first.shader");
        const QString secondSource = m_scanFolder.absoluteFilePath("second.shader");
        QFile fileList(m_scanFolder.absoluteFilePath("reprocess-list.txt"));
        ASSERT_TRUE(fileList.open(QIODevice::WriteOnly));
        const QByteArray contents = firstSource.toUtf8() + "\r\n" + secondSource.toUtf8() + "\r\n";
        ASSERT_EQ(fileList.write(contents), contents.size());
        fileList.close();
        m_applicationManager->m_reprocessFileList = fileList.fileName();

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_reprocessFileList.isEmpty());
        EXPECT_EQ(m_applicationManager->m_filesToReprocess, (QStringList{ secondSource + "\r\n" }));
        ExpectJobFingerprint(m_firstJob, 0);
        ExpectJobFingerprint(m_secondJob, 202);

        EXPECT_TRUE(m_applicationManager->CheckReprocessFileList());
        EXPECT_TRUE(m_applicationManager->m_filesToReprocess.isEmpty());
        ExpectJobFingerprint(m_secondJob, 0);
        EXPECT_FALSE(m_applicationManager->CheckReprocessFileList());
        EXPECT_EQ(m_reprocessAPM->m_assessedPaths, (QStringList{ firstSource, secondSource }));
    }

    TEST_F(ApplicationManagerTest, FileWatcherEventsTriggered_ProperlySignalledOnCorrectThread_SUITE_sandbox)
    {
        AZ::IO::Path assetRootDir(m_databaseLocationListener.GetAssetRootDir());

        Q_EMIT m_fileWatcher->fileAdded((assetRootDir / "test").c_str());
        Q_EMIT m_fileWatcher->fileModified((assetRootDir / "test2").c_str());
        Q_EMIT m_fileWatcher->fileRemoved((assetRootDir / "test3").c_str());

        EXPECT_TRUE(m_mockAPM->m_events[Added].WaitAndCheck()) << "APM Added event failed";
        EXPECT_TRUE(m_mockAPM->m_events[Modified].WaitAndCheck()) << "APM Modified event failed";
        EXPECT_TRUE(m_mockAPM->m_events[Deleted].WaitAndCheck()) << "APM Deleted event failed";

        EXPECT_TRUE(m_mockFileProcessor->m_events[Added].WaitAndCheck()) << "File Processor Added event failed";
        EXPECT_TRUE(m_mockFileProcessor->m_events[Deleted].WaitAndCheck()) << "File Processor Deleted event failed";
    }

    TEST(AssetProcessorAZApplicationTest, AssetProcessorAZApplication_ArchiveComponent_Exists)
    {
        int argc = 0;
        AssetProcessorAZApplication assetProcessorAZApplication {&argc, nullptr};
        auto componentList = assetProcessorAZApplication.GetRequiredSystemComponents();
        auto iterator = AZStd::find(componentList.begin(), componentList.end(), azrtti_typeid<AzToolsFramework::ArchiveComponent>());
        EXPECT_NE(iterator, componentList.end()) << "AzToolsFramework::ArchiveComponent is not a required component";
    }

    TEST(AssetProcessorAssetServerHandler, AssetServerHandler_FutureCalls_FailsNoExceptions)
    {
        char executablePath[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);

        struct MockAssetServerInfoBus final
            : public AssetProcessor::AssetServerInfoBus::Handler
        {
            explicit MockAssetServerInfoBus(const char* filename)
                : m_filename(filename)
            {
                AssetProcessor::AssetServerInfoBus::Handler::BusConnect();
            }

            ~MockAssetServerInfoBus()
            {
                AssetProcessor::AssetServerInfoBus::Handler::BusDisconnect();
            }

            const AZStd::string& ComputeArchiveFilePath(const AssetProcessor::BuilderParams&) override
            {
                return m_filename;
            }

            AZStd::string m_filename;
        };

        AssetProcessor::JobDetails jobDetails;
        jobDetails.m_jobEntry.m_sourceFileUUID = AZ::Uuid::CreateRandom();
        jobDetails.m_checkServer = true;

        AssetProcessor::RCJob rcJob;
        rcJob.Init(jobDetails);

        AssetProcessor::BuilderParams buildParams { &rcJob };
        buildParams.m_processJobRequest.m_sourceFile = executablePath;
        buildParams.m_serverKey = "fake.product";

        // These should fail, but not throw an exception

        // mock storing an archive
        {
            MockAssetServerInfoBus mockAssetServerInfoBus("fake.asset");
            AssetProcessor::AssetServerHandler assetServerHandler{};
            AZStd::vector<AZStd::string> files({ buildParams.m_serverKey.toUtf8().toStdString().c_str() });
            EXPECT_FALSE(assetServerHandler.StoreJobResult(buildParams, files));
        }

        // mock retrieving an archive
        {
            MockAssetServerInfoBus mockAssetServerInfoBus(executablePath);
            AssetProcessor::AssetServerHandler assetServerHandler{};
            EXPECT_FALSE(assetServerHandler.RetrieveJobResult(buildParams));
        }
    }
}
