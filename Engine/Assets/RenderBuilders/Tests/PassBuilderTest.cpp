/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <JE/Render/RPI/Reflect/Pass/FullscreenTrianglePassData.h>
#include <JE/Render/RPI/Reflect/Pass/PassAsset.h>
#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>
#include <Pass/PassBuilder.h>

#include <BuilderTestFixture.h>

namespace UnitTest
{
    using namespace AZ;

    class PassBuilderTests
        : public BuilderTestFixture
    {
    protected:

        JE::Render::RPI::PassAssetHandler* m_assetHandler;

        void SetUp() override
        {
            BuilderTestFixture::SetUp();

            m_assetHandler = new JE::Render::RPI::PassAssetHandler();
            m_assetHandler->m_serializeContext = m_context.get();
            m_assetHandler->Register();
        }

        void TearDown() override
        {
            m_assetHandler->Unregister();
            delete m_assetHandler;
            BuilderTestFixture::TearDown();
        }

        template<typename T>
        void SaveAssetToFile(T& data, const char* saveFileName)
        {
            AZ::Utils::SaveObjectToFile(saveFileName, AZ::DataStream::ST_XML, &data, T::TYPEINFO_Uuid(), m_context.get());
        }

        AZ::Data::Asset<AZ::Data::AssetData> LoadAssetFromFile(const char* assetFile)
        {
            AZ::Data::Asset<AZ::Data::AssetData> outAsset(m_assetHandler->CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 1), AZ::Data::AssetType()),
                AZ::Data::AssetLoadBehavior::PreLoad);
            AZ::u64 fileLength = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(assetFile, fileLength);
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            stream->Open(assetFile, 0, fileLength);
            stream->BlockUntilLoadComplete();
            m_assetHandler->LoadAssetData(outAsset, stream, {});
            stream->Close();

            // Force a file streamer flush to ensure that file handles don't remain used
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZStd::binary_semaphore wait;
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
            streamer->SetRequestCompleteCallback(flushRequest, [&wait]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    wait.release();
                });
            streamer->QueueRequest(flushRequest);
            wait.acquire();

            return outAsset;
        }

        void SetPassTemplateForTestingOnly(JE::Render::RPI::PassAsset& passAsset, JE::Render::RPI::PassTemplate& passTemplate)
        {
            passAsset.SetPassTemplateForTestingOnly(passTemplate);
        }
    };

    TEST_F(PassBuilderTests, ProcessJob)
    {
        const char* testAssetName = "PassTestAsset.pass";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Basic test: test data before and after are same. Test data class doesn't have converter or asset reference.
        AssetBuilderSDK::ProcessJobRequest request;

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        // Dummy pass template
        JE::Render::RPI::PassTemplate passTemplate;
        passTemplate.m_name = "TemplateTestName";

        // Dummy pass data with random asset ID to test asset dependency
        AZStd::shared_ptr<JE::Render::RPI::FullscreenTrianglePassData> passData = AZStd::make_shared<JE::Render::RPI::FullscreenTrianglePassData>();
        passData->m_shaderAsset.m_assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 1);
        passTemplate.m_passData = passData;

        // Create and write pass data
        JE::Render::RPI::PassAsset passAsset;
        SetPassTemplateForTestingOnly(passAsset, passTemplate);
        AZ::JsonSerializationUtils::SaveObjectToFile(&passAsset, sourceFilePath.Native());

        // Process job
        JE::Assets::RenderBuilders::PassBuilder builder;
        AssetBuilderSDK::ProcessJobResponse response;
        builder.ProcessJob(request, response);

        // Verify job success
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);

        // Verify input and output names are the same
        AZ::Data::Asset<AZ::Data::AssetData> readAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        JE::Render::RPI::PassAsset* readPassAsset = static_cast<JE::Render::RPI::PassAsset*>(readAsset.GetData());
        EXPECT_TRUE(passTemplate.m_name == readPassAsset->GetPassTemplate()->m_name);
    }

} // namespace UnitTests
