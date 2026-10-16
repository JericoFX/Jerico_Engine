/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <AzCore/AzCore_Traits_Platform.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImageToProcess.h>
#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageFlags.h>
#include <ImageLoader/ImageLoaders.h>

#include <Compressors/Compressor.h>

#include <Converters/Cubemap.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/PresetSettings.h>

#include <Editor/EditorCommon.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <QFileInfo>
#include <QFile>
#include <QTemporaryDir>
#include <qdir.h>
#include <QDirIterator>
#include <QIODevice>

#include <array>
#include <utility>

//Enable generate image files for result of some tests.
//This is slow and only useful for debugging. This should be disabled for unit test
//#define DEBUG_OUTPUT_IMAGES

//There are some test functions in this test which are DISABLED. They were mainly for programming tests.
//It's only recommended to enable them for programming test purpose.

#include <AzCore/UnitTest/TestTypes.h>
#include <ImageBuilderComponent.h>
#include <ImageProcessingSystemComponent.h>
#include <Atom/ImageProcessing/ImageProcessingEditorBus.h>
#include <AzCore/Utils/Utils.h>
#include <Tests/AssetSystemMocks.h>
#include <QImage>

using namespace ImageProcessingAtom;

namespace UnitTest
{
    // Expose AZ::AssetManagerComponent::Reflect function for testing
    class MyAssetManagerComponent
        : public AZ::AssetManagerComponent
    {
    public:
        static void Reflect(ReflectContext* reflection)
        {
            AZ::AssetManagerComponent::Reflect(reflection);
        };
    };

    class ImageProcessingTest
        : public LeakDetectionFixture
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages.
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::SerializeContext* GetSerializeContext() override { return m_context.get(); }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return m_jsonRegistrationContext.get(); }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

    protected:

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        AZ::IO::Path m_gemFolder;
        AZ::IO::Path m_outputRootFolder;
        AZ::IO::Path m_outputFolder;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        void SetUp() override
        {
            // Adding this handler to allow utility functions access the serialize context
            ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);

            // AssetManager required to generate image assets
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            AZ::NameDictionary::Create();

            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::ImageMipChainAssetHandler>());
            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::StreamingImageAssetHandler>());
            m_assetHandlers.emplace_back(AZ::RPI::MakeAssetHandler<AZ::RPI::StreamingImagePoolAssetHandler>());

            BuilderSettingManager::CreateInstance();

            //prepare reflection
            m_context = AZStd::make_unique<AZ::SerializeContext>();
            AZ::Name::Reflect(m_context.get());
            BuilderPluginComponent::Reflect(m_context.get());
            AZ::DataPatch::Reflect(m_context.get());
            AZ::RHI::ReflectSystemComponent::Reflect(m_context.get());
            AZ::RPI::ImageMipChainAsset::Reflect(m_context.get());
            AZ::RPI::ImageAsset::Reflect(m_context.get());
            AZ::RPI::StreamingImageAsset::Reflect(m_context.get());
            MyAssetManagerComponent::Reflect(m_context.get());

            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            AZ::Name::Reflect(m_jsonRegistrationContext.get());
            BuilderPluginComponent::Reflect(m_jsonRegistrationContext.get());

            // Setup job context for job system
            JobManagerDesc jobManagerDesc;
            JobManagerThreadDesc threadDesc;
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
            threadDesc.m_cpuId = 0; // Don't set processors IDs on windows
#endif 

            uint32_t numWorkerThreads = jobManagerDesc.GetWorkerThreadCount(AZStd::thread::hardware_concurrency());

            for (unsigned int i = 0; i < numWorkerThreads; ++i)
            {
                jobManagerDesc.m_workerThreads.push_back(threadDesc);
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
                threadDesc.m_cpuId++;
#endif 
            }

            m_jobManager = AZStd::make_unique<JobManager>(jobManagerDesc);
            m_jobContext = AZStd::make_unique<JobContext>(*m_jobManager);
            JobContext::SetGlobalContext(m_jobContext.get());

            // Startup default local FileIO (hits OSAllocator) if not already setup.
            if (AZ::IO::FileIOBase::GetInstance() == nullptr)
            {
                AZ::IO::FileIOBase::SetInstance(aznew AZ::IO::LocalFileIO());
            }

            //load qt plug-ins for some image file formats support
            AzQtComponents::PrepareQtPaths();

            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AZ::SettingsRegistryImpl localRegistry;
            localRegistry.Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, AZ::Test::GetEngineRootPath());

            // Merge in the o3de manifest files gem root paths to the Settings Registry
            // and set the ImageProcessingAtom gem as an active gem which will add it to the active gem
            // section of the Settings Registry as well as add a @gemroot@ alias for it
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths(localRegistry);
            AZ::Test::AddActiveGem("ImageProcessingAtom", localRegistry, AZ::IO::FileIOBase::GetInstance());

            // Validate the path to the ImageProcessingAtom Gem root is registered in the Settings Registry
            ASSERT_TRUE(localRegistry.Get(m_gemFolder.Native(), FixedValueString::format("%s/ImageProcessingAtom/Path",
                AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey)));

            m_outputFolder = m_gemFolder/ "Code/Tests/TestAssets/temp/";

            m_defaultSettingFolder = m_gemFolder / "Assets/Config/";
            m_testFileFolder = m_gemFolder  /"Code/Tests/TestAssets/";

            InitialImageFilenames();

            ImageProcessingAtomEditor::EditorHelper::InitPixelFormatString();
        }

        void TearDown() override
        {
            m_gemFolder = AZ::IO::Path{};
            m_outputFolder = AZ::IO::Path{};
            m_defaultSettingFolder = AZ::IO::Path{};
            m_testFileFolder = AZ::IO::Path{};

            m_imagFileNameMap = AZStd::map<ImageFeature, AZStd::string>();
            m_assetHandlers = AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>();

            delete AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);

            JobContext::SetGlobalContext(nullptr);
            m_jobContext = nullptr;
            m_jobManager = nullptr;

            m_jsonRegistrationContext->EnableRemoveReflection();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            BuilderPluginComponent::Reflect(m_jsonRegistrationContext.get());
            AZ::Name::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();
            m_jsonRegistrationContext.reset();
            m_jsonSystemComponent.reset();

            m_context.reset();
            BuilderSettingManager::DestroyInstance();

            CPixelFormats::DestroyInstance();

            AZ::NameDictionary::Destroy();

            AZ::Data::AssetManager::Destroy();

            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            ComponentApplicationBus::Handler::BusDisconnect();

            AZ::GetGlobalSerializeContextModule().Cleanup();
        }

        //enum names for Images with specific identification
        enum ImageFeature
        {
            Image_20X16_RGBA8_Png = 0,
            Image_32X32_16bit_F_Tif,
            Image_32X32_32bit_F_Tif,
            Image_32X32_checkerboard_png,
            Image_32X32_halfRedHalfTransparentGreen_png,
            Image_200X200_RGB8_Jpg,
            Image_512X288_RGB8_Tga,
            Image_1024X1024_RGB8_Tif,
            Image_UpperCase_Tga,
            Image_512x512_normal_tiff,
            Image_128x128_Transparent_Tga,
            Image_237x177_RGB_Jpg,
            Image_GreyScale_Png,
            Image_Alpha8_64x64_Mip7_Dds,
            Image_BGRA_64x64_Mip7_Dds,
            Image_Luminance8bpp_66x33_dds,
            Image_BGR_64x64_dds,
            Image_defaultprobe_cm_1536x256_64bits_tif,
            Image_workshop_iblskyboxcm_exr
        };

        //image file names for testing
        AZStd::map<ImageFeature, AZStd::string> m_imagFileNameMap;

        AZ::IO::Path m_defaultSettingFolder;
        AZ::IO::Path m_testFileFolder;

        //initialize image file names for testing
        void InitialImageFilenames()
        {
            m_imagFileNameMap[Image_20X16_RGBA8_Png] = (m_testFileFolder / "20x16_32bit.png").Native();
            m_imagFileNameMap[Image_32X32_16bit_F_Tif] = (m_testFileFolder / "32x32_16bit_f.tif").Native();
            m_imagFileNameMap[Image_32X32_32bit_F_Tif] = (m_testFileFolder / "32x32_32bit_f.tif").Native();
            m_imagFileNameMap[Image_32X32_checkerboard_png] = (m_testFileFolder / "32x32_checkerboard.png").Native();
            m_imagFileNameMap[Image_32X32_halfRedHalfTransparentGreen_png] = (m_testFileFolder / "32x32_halfRedHalfTransparentGreen.png").Native();
            m_imagFileNameMap[Image_200X200_RGB8_Jpg] = (m_testFileFolder / "200x200_24bit.jpg").Native();
            m_imagFileNameMap[Image_512X288_RGB8_Tga] = (m_testFileFolder / "512x288_24bit.tga").Native();
            m_imagFileNameMap[Image_1024X1024_RGB8_Tif] = (m_testFileFolder / "1024x1024_24bit.tif").Native();
            m_imagFileNameMap[Image_UpperCase_Tga] = (m_testFileFolder / "uppercase.TGA").Native();
            m_imagFileNameMap[Image_512x512_normal_tiff] = (m_testFileFolder / "512x512_normal.tiff").Native();
            m_imagFileNameMap[Image_128x128_Transparent_Tga] = (m_testFileFolder / "128x128_RGBA8.tga").Native();
            m_imagFileNameMap[Image_237x177_RGB_Jpg] = (m_testFileFolder / "237x177_RGB.jpg").Native();
            m_imagFileNameMap[Image_GreyScale_Png] = (m_testFileFolder / "greyscale.png").Native();
            m_imagFileNameMap[Image_Alpha8_64x64_Mip7_Dds] = (m_testFileFolder / "Alpha8_64x64_Mip7.dds").Native();
            m_imagFileNameMap[Image_BGRA_64x64_Mip7_Dds] = (m_testFileFolder / "BGRA_64x64_MIP7.dds").Native();
            m_imagFileNameMap[Image_Luminance8bpp_66x33_dds] = (m_testFileFolder / "Luminance8bpp_66x33.dds").Native();
            m_imagFileNameMap[Image_BGR_64x64_dds] = (m_testFileFolder / "RGBA_64x64.dds").Native();
            m_imagFileNameMap[Image_defaultprobe_cm_1536x256_64bits_tif] = (m_testFileFolder / "defaultProbe_cm.tif").Native();
            m_imagFileNameMap[Image_workshop_iblskyboxcm_exr] = (m_testFileFolder / "workshop_iblskyboxcm.exr").Native();
        }

    public:
        void SetOutputSubFolder(const char* subFolderName)
        {
            if (subFolderName)
            {
                m_outputFolder = m_outputRootFolder / subFolderName;
            }
            else
            {
                m_outputFolder  = m_outputRootFolder;
            }
        }

        //helper function to save an image object to a file through QtImage
        void SaveImageToFile([[maybe_unused]] const IImageObjectPtr imageObject, [[maybe_unused]] const AZStd::string imageName, [[maybe_unused]] AZ::u32 maxMipCnt = 100)
        {
    #ifndef DEBUG_OUTPUT_IMAGES
            return;
    #else
            if (imageObject == nullptr)
            {
                return;
            }

            // create dir if it doesn't exist
            QDir dir;
            QDir outputDir(m_outputFolder.c_str());
            if (!outputDir.exists())
            {
                dir.mkpath(m_outputFolder.c_str());
            }

            //save origin file pixel format so we could use it to generate name later
            EPixelFormat originPixelFormat = imageObject->GetPixelFormat();

            //convert to RGBA8 before can be exported.
            ImageToProcess imageToProcess(imageObject);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);

            IImageObjectPtr finalImage = imageToProcess.Get();

            //for each mipmap
            for (uint32 mip = 0; mip < finalImage->GetMipCount() && mip < maxMipCnt; mip++)
            {
                uint8* imageBuf;
                uint32 pitch;
                finalImage->GetImagePointer(mip, imageBuf, pitch);
                uint32 width = finalImage->GetWidth(mip);
                uint32 height = finalImage->GetHeight(mip);
                uint32 originalSize = imageObject->GetMipBufSize(mip);

                //generate file name
                char filePath[2048];
                azsprintf(filePath, "%s%s_%s_mip%d_%dx%d_%d.png", m_outputFolder.data(), imageName.c_str()
                    , CPixelFormats::GetInstance().GetPixelFormatInfo(originPixelFormat)->szName
                    , mip, width, height, originalSize);

                QImage qimage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
                qimage.save(filePath);
            }
    #endif
        }

        static bool GetComparisonResult(IImageObjectPtr image1, IImageObjectPtr image2, QString& output)
        {
            bool isImageLoaded = true;
            bool isDifferent = false;

            if (image1 == nullptr)
            {
                isImageLoaded = false;
                output += ",Image 1 does not exist. ";
            }

            if (image2 == nullptr)
            {
                isImageLoaded = false;
                output += ",Image 2 does not exist. ";
            }

            if (!isImageLoaded)
            {
                return (!image1 && !image2) ? false : true;
            }

            // Mip
            int mip1 = image1->GetMipCount();
            int mip2 = image2->GetMipCount();
            int mipDiff = abs(mip1 - mip2);

            isDifferent |= mipDiff != 0;

            // Format
            EPixelFormat format1 = image1->GetPixelFormat();
            EPixelFormat format2 = image2->GetPixelFormat();

            isDifferent |= (format1 != format2);

            // Flag
            AZ::u32 flag1 = image1->GetImageFlags();
            AZ::u32 flag2 = image2->GetImageFlags();

            isDifferent |= (flag1 != flag2);

            // Size
            int memSize1 = image1->GetTextureMemory();
            int memSize2 = image2->GetTextureMemory();
            int memDiff = abs(memSize1 - memSize2);

            isDifferent |= memDiff != 0;

            // Error
            float error = GetErrorBetweenImages(image1, image2);

            static float EPSILON = 0.000001f;
            isDifferent |= abs(error) >= EPSILON;

            output += QString(",%1/%2,%3,%4/%5,%6/%7,").arg(QString::number(mip1, 'f', 1), QString::number(mip2, 'f', 1), QString::number(mipDiff),
                QString(ImageProcessingAtomEditor::EditorHelper::s_PixelFormatString[format1]),
                QString(ImageProcessingAtomEditor::EditorHelper::s_PixelFormatString[format2]),
                QString::number(flag1, 16), QString::number(flag2, 16));

            output += QString("%1/%2,%3,%4").arg(QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memSize1).c_str()),
                QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memSize2).c_str()),
                QString(ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memDiff).c_str()),
                QString::number(error, 'f', 8));


            return isDifferent;
        }

    };

    // test CPixelFormats related functions
    TEST_F(ImageProcessingTest, TestPixelFormats)
    {
        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();

        //for all the non-compressed textures, if there minimum required texture size is 1x1
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            if (pixelFormats.IsPixelFormatUncompressed(pixelFormat))
            {
                //square, power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 128) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 64, 64) == 7);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 4, 4) == 3);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 2) == 2);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 1, 1) == 1);

                //non-square, power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);

                //Non power of 2 sizes for uncompressed format which minimum required size is 1x1
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 64) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 128, 32) == 8);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 32, 2) == 6);
                ASSERT_TRUE(pixelFormats.ComputeMaxMipCount(pixelFormat, 2, 1) == 2);
            }
        }

        //check function IsImageSizeValid && EvaluateImageDataSize function
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 2, 1, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 34, 34, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_BC1, 256, 256, false) == true);

        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 2, 1, false) == false);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 34, 34, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_ASTC_4x4, 256, 256, false) == true);

        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 2, 1, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 16, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 16, 32, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 34, 34, false) == true);
        ASSERT_TRUE(pixelFormats.IsImageSizeValid(ePixelFormat_A8, 256, 256, false) == true);
    }

    TEST_F(ImageProcessingTest, TestCubemapLayouts)
    {
        {
            IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_defaultprobe_cm_1536x256_64bits_tif]));
            ImageToProcess imageToProcess(srcImage);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutVertical);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 6 == imageToProcess.Get()->GetHeight(0));
            SaveImageToFile(imageToProcess.Get(), "Vertical", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontalCross);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 3 == imageToProcess.Get()->GetHeight(0) * 4);
            SaveImageToFile(imageToProcess.Get(), "HorizontalCross", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutVerticalCross);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) * 4 == imageToProcess.Get()->GetHeight(0) * 3);
            SaveImageToFile(imageToProcess.Get(), "VerticalCross", 100);

            imageToProcess.ConvertCubemapLayout(CubemapLayoutHorizontal);
            ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == imageToProcess.Get()->GetHeight(0) * 6);
            SaveImageToFile(imageToProcess.Get(), "VerticalHorizontal", 100);
        }
    }

    // test image file loading
    TEST_F(ImageProcessingTest, TestImageLoaders)
    {
        //file extension support for different loader
        ASSERT_TRUE(IsExtensionSupported("jpg") == true);
        ASSERT_TRUE(IsExtensionSupported("JPG") == true);
        ASSERT_TRUE(IsExtensionSupported(".JPG") == false);
        ASSERT_TRUE(IsExtensionSupported("tga") == true);
        ASSERT_TRUE(IsExtensionSupported("TGA") == true);
        ASSERT_TRUE(IsExtensionSupported("tif") == true);
        ASSERT_TRUE(IsExtensionSupported("tiff") == true);

        IImageObjectPtr img;
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_1024X1024_RGB8_Tif]));

        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 1024);
        ASSERT_TRUE(img->GetHeight(0) == 1024);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8X8);

        //load png
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_20X16_RGBA8_Png]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 20);
        ASSERT_TRUE(img->GetHeight(0) == 16);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //load jpg
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 200);
        ASSERT_TRUE(img->GetHeight(0) == 200);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //tga
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_512X288_RGB8_Tga]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetWidth(0) == 512);
        ASSERT_TRUE(img->GetHeight(0) == 288);
        ASSERT_TRUE(img->GetMipCount() == 1);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8);

        // tga with transparency
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_128x128_Transparent_Tga]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8A8);

        //image with upper case extension
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_UpperCase_Tga]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R8G8B8);

        //16bits float tif
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_16bit_F_Tif]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R16G16B16A16F);

        //32bits float tif
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_32X32_32bit_F_Tif]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_R32G32B32A32F);

        // DDS files
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_Alpha8_64x64_Mip7_Dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_A8);
        ASSERT_TRUE(img->GetMipCount() == 7);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_BGRA_64x64_Mip7_Dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_B8G8R8A8);
        ASSERT_TRUE(img->GetMipCount() == 7);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_Luminance8bpp_66x33_dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_A8);
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_BGR_64x64_dds]));
        ASSERT_TRUE(img != nullptr);
        ASSERT_TRUE(img->GetPixelFormat() == ePixelFormat_B8G8R8);

        // Exr file
        img = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[Image_workshop_iblskyboxcm_exr]));
        ASSERT_TRUE(img != nullptr);
    }

    TEST_F(ImageProcessingTest, PresetSettingCopyAssignmentOperatorOverload_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

        // Explicit invoke assignment operator by splitting the operation into two lines.
        PresetSettings otherPresetSetting;
        otherPresetSetting = presetSetting;

        EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
        EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingCopyConstructor_WithDynamicallyAllocatedSettings_ReturnsTwoSeparateAllocations)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_cubemapSetting = AZStd::unique_ptr<CubemapSettings>(new CubemapSettings());

        PresetSettings otherPresetSetting(presetSetting);

        EXPECT_NE(otherPresetSetting.m_cubemapSetting, presetSetting.m_cubemapSetting);
        EXPECT_NE(otherPresetSetting.m_mipmapSetting, presetSetting.m_mipmapSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithIdenticalSettings_ReturnsEquivalent)
    {
        PresetSettings presetSetting;
        PresetSettings otherPresetSetting(presetSetting);

        EXPECT_TRUE(otherPresetSetting == presetSetting);
    }

    TEST_F(ImageProcessingTest, PresetSettingEqualityOperatorOverload_WithDifferingDynamicallyAllocatedSettings_ReturnsUnequivalent)
    {
        PresetSettings presetSetting;
        presetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        presetSetting.m_mipmapSetting->m_type = MipGenType::gaussian;

        PresetSettings otherPresetSetting(presetSetting);
        otherPresetSetting.m_mipmapSetting = AZStd::unique_ptr<MipmapSettings>(new MipmapSettings());
        otherPresetSetting.m_mipmapSetting->m_type = MipGenType::blackmanHarris;

        EXPECT_FALSE(otherPresetSetting == presetSetting);
    }

    //this test is to test image data won't be lost between uncompressed formats (for low to high precision or same precision)
    TEST_F(ImageProcessingTest, TestConvertFormatUncompressed)
    {
        //source image
        IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_200X200_RGB8_Jpg]));
        ImageToProcess imageToProcess(srcImage);

        //image pointers to hold precessed images for comparison
        IImageObjectPtr dstImage1, dstImage2, dstImage3, dstImage4, dstImage5;

        //compare four channels pixel formats
        //we will convert to target format then convert back to RGBX8 so they can compare to easy other
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16);
        ASSERT_FALSE(srcImage->CompareImage(imageToProcess.Get())); //this is different than source image
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16B16A16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage3 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32G32B32A32F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8A8);
        dstImage4 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

        // three channels formats
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R9G9B9E5);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));

        //convert image to all one channel formats then convert them back to RGBX8 for comparison
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage3 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R32F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage4 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage4->CompareImage(dstImage1));

        //convert image to all two channels formats then convert them back to RGBX8 for comparison
        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage1 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage2 = imageToProcess.Get();

        imageToProcess.Set(srcImage);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R16G16F);
        imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
        dstImage3 = imageToProcess.Get();

        ASSERT_TRUE(dstImage2->CompareImage(dstImage1));
        ASSERT_TRUE(dstImage3->CompareImage(dstImage1));
    }

    TEST_F(ImageProcessingTest, TestConvertFormatCompressed)
    {
        IImageObjectPtr srcImage;

        //images to be tested
        static const int imageCount = 4;
        ImageFeature images[imageCount] = {
            Image_20X16_RGBA8_Png,
            Image_237x177_RGB_Jpg,
            Image_128x128_Transparent_Tga,
            Image_defaultprobe_cm_1536x256_64bits_tif};

        // collect all compressed pixel formats
        AZStd::vector<EPixelFormat> compressedFormats;
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
            if (formatInfo->bCompressed)
            {
                // skip ASTC formats which are tested in TestConvertASTCCompressor
                if (!IsASTCFormat(pixelFormat))
                {
                    compressedFormats.push_back(pixelFormat);
                }
            }
        }

        for (int imageIdx = 0; imageIdx < imageCount; imageIdx++)
        {
            //get image's name and it will be used for output file name
            QFileInfo fi(m_imagFileNameMap[images[imageIdx]].c_str());
            AZStd::string imageName = fi.baseName().toUtf8().constData();

            srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[images[imageIdx]]));
            ImageToProcess imageToProcess(srcImage);

            //test ConvertFormat functions against all the pixel formats
            for (EPixelFormat pixelFormat : compressedFormats)
            {
                // 
                if (!CPixelFormats::GetInstance().IsImageSizeValid(pixelFormat, srcImage->GetWidth(0), srcImage->GetHeight(0), false))
                {
                    continue;
                }
#if defined(AZ_ENABLE_TRACING)
                auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
#endif
                ColorSpace sourceColorSpace = srcImage->HasImageFlags(EIF_SRGBRead) ? ColorSpace::sRGB : ColorSpace::linear;
                ICompressorPtr compressor = ICompressor::FindCompressor(pixelFormat, sourceColorSpace, true);

                if (!compressor)
                {
                    AZ_Warning("test", false, "unsupported format: %s",  formatInfo->szName);
                    continue;
                }

                imageToProcess.Set(srcImage);
                imageToProcess.ConvertFormat(pixelFormat);

                ASSERT_TRUE(imageToProcess.Get());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == pixelFormat);

                //convert back to an uncompressed format and expect it will be successful
                imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());

                // Save the image to a file so we can check the visual result
                AZStd::string outputName = AZStd::string::format("%s_%s", imageName.c_str(), compressor->GetName());
                SaveImageToFile(imageToProcess.Get(), outputName, 1);
            }
        }
    }
        
    TEST_F(ImageProcessingTest, Test_ConvertAllAstc_Success)
    {
        // Compress/Decompress to all astc formats (LDR)
        auto imageIdx = Image_237x177_RGB_Jpg;
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[imageIdx]));
        QFileInfo fi(m_imagFileNameMap[imageIdx].c_str());
        AZStd::string imageName = fi.baseName().toUtf8().constData();
        for (uint32 i = 0; i < ePixelFormat_Count; i++)
        {
            EPixelFormat pixelFormat = (EPixelFormat)i;
            if (IsASTCFormat(pixelFormat))
            {
                ImageToProcess imageToProcess(srcImage);
                imageToProcess.ConvertFormat(pixelFormat);

                ASSERT_TRUE(imageToProcess.Get());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == pixelFormat);
                ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == srcImage->GetWidth(0));
                ASSERT_TRUE(imageToProcess.Get()->GetHeight(0) == srcImage->GetHeight(0));

                // convert back to an uncompressed format and expect it will be successful
                imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
                ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());

                // save the image to a file so we can check the visual result
                AZStd::string outputName = AZStd::string::format("ASTC_%s", imageName.c_str());
                SaveImageToFile(imageToProcess.Get(), outputName, 1);
            }
        }
    }
        
    TEST_F(ImageProcessingTest, Test_ConvertHdrToAstc_Success)
    {
        // Compress/Decompress HDR
        auto imageIdx = Image_defaultprobe_cm_1536x256_64bits_tif;
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(m_imagFileNameMap[imageIdx]));

        EPixelFormat dstFormat = ePixelFormat_ASTC_4x4;
        ImageToProcess imageToProcess(srcImage);
        imageToProcess.ConvertFormat(ePixelFormat_ASTC_4x4);
                
        ASSERT_TRUE(imageToProcess.Get());
        ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == dstFormat);
        ASSERT_TRUE(imageToProcess.Get()->GetWidth(0) == srcImage->GetWidth(0));
        ASSERT_TRUE(imageToProcess.Get()->GetHeight(0) == srcImage->GetHeight(0));
                                
        //convert back to an uncompressed format and expect it will be successful
        imageToProcess.ConvertFormat(srcImage->GetPixelFormat());
        ASSERT_TRUE(imageToProcess.Get()->GetPixelFormat() == srcImage->GetPixelFormat());
                                
        //save the image to a file so we can check the visual result
        SaveImageToFile(imageToProcess.Get(), "ASTC_HDR", 1);
    }
    
    TEST_F(ImageProcessingTest, Test_AstcNormalPreset_Success)
    {
        // Normal.preset which uses ASTC as output format
        // This test compress a normal texture and its mipmaps

        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native());
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_512x512_normal_tiff];
        IImageObjectPtr srcImage = IImageObjectPtr(LoadImageFromFile(inputFile));

        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder.Native(), "ios", outProducts, m_context.get());

        const PresetSettings* preset = &process->GetInputDesc()->m_presetSetting;

        if (process != nullptr)
        {
            process->ProcessAll();

            //get process result
            ASSERT_TRUE(process->IsSucceed());
            auto outputImage = process->GetOutputImage();
            ASSERT_TRUE(outputImage->GetPixelFormat() == preset->m_pixelFormat);
            ASSERT_TRUE(outputImage->GetWidth(0) == srcImage->GetWidth(0));
            ASSERT_TRUE(outputImage->GetHeight(0) == srcImage->GetHeight(0));
            
            SaveImageToFile(outputImage, "ASTC_Normal", 10);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, DISABLED_TestImageFilter)
    {
        AZStd::string testImageFile = m_imagFileNameMap[Image_1024X1024_RGB8_Tif];
        IImageObjectPtr srcImage, dstImage;

        QFileInfo fi(testImageFile.c_str());
        AZStd::string imageName = fi.baseName().toUtf8().constData();

        //load source image and convert it to RGBA32F
        srcImage = IImageObjectPtr(LoadImageFromFile(testImageFile));
        ImageToProcess imageToProcess(srcImage);
        imageToProcess.ConvertFormat(ePixelFormat_R32G32B32A32F);
        srcImage = imageToProcess.Get();

        //create destination image with same size and mipmaps
        dstImage = IImageObjectPtr(
            IImageObject::CreateImage(srcImage->GetWidth(0), srcImage->GetHeight(0), 3,
                ePixelFormat_R32G32B32A32F));

        //for each filters
        const std::array<std::pair<MipGenType, AZStd::string>, 7> allFilters =
        {
            {
                {MipGenType::point, "point"},
                {MipGenType::box, "box" },
                { MipGenType::triangle, "triangle" },
                { MipGenType::quadratic, "Quadratic" },
                { MipGenType::blackmanHarris, "blackmanHarris" },
                { MipGenType::kaiserSinc, "kaiserSinc" }
            }
        };

        for (std::pair<MipGenType, AZStd::string> filter : allFilters)
        {
            for (uint mip = 0; mip < dstImage->GetMipCount(); mip++)
            {
                FilterImage(filter.first, MipGenEvalType::sum,
                    0, 0, imageToProcess.Get(), 0, dstImage, mip, nullptr, nullptr);
            }
            SaveImageToFile(dstImage, imageName + "_" + filter.second);
        }
    }

    TEST_F(ImageProcessingTest, TestAverageColor)
    {
        //load builder presets
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native());
        ASSERT_TRUE(outcome.IsSuccess());

        auto checkAverageColor = [&](ImageFeature figureKey, AZ::Color expectedAverage)
        {
            AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

            AZStd::string inputFile = m_imagFileNameMap[figureKey];
            ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder.Native(), "pc", outProducts, m_context.get());
            if (process != nullptr)
            {
                process->ProcessAll();

                ASSERT_TRUE(process->IsSucceed());
                ASSERT_TRUE(process->GetOutputImage());
                ASSERT_TRUE(process->GetOutputImage()->GetAverageColor().IsClose(expectedAverage));

                delete process;
            }
        };

        checkAverageColor(Image_32X32_checkerboard_png, AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));
        checkAverageColor(Image_32X32_halfRedHalfTransparentGreen_png, AZ::Color(1.0f, 0.0f, 0.0f, 0.5f));
    }

    TEST_F(ImageProcessingTest, TestColorSpaceConversion)
    {
        IImageObjectPtr srcImage(LoadImageFromFile(m_imagFileNameMap[Image_GreyScale_Png]));

        ImageToProcess imageToProcess(srcImage);
        imageToProcess.GammaToLinearRGBA32F(true);
        SaveImageToFile(imageToProcess.Get(), "GammaTolinear_DeGamma", 1);
        imageToProcess.LinearToGamma();
        SaveImageToFile(imageToProcess.Get(), "LinearToGamma_DeGamma", 1);
    }

    TEST_F(ImageProcessingTest, VerifyRestrictedPlatform)
    {
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native());
        ASSERT_TRUE(outcome.IsSuccess());
        PlatformNameList platforms = BuilderSettingManager::Instance()->GetPlatformList();

    #ifndef AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
        EXPECT_THAT(platforms, testing::UnorderedPointwise(testing::Eq(), {"pc", "linux", "mac", "ios", "android"}));
    #endif //AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
    }

    //test image conversion for builder
    TEST_F(ImageProcessingTest, TestBuilderImageConvertor)
    {
        //load builder presets
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native());
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_128x128_Transparent_Tga];
        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder.Native(), "pc", outProducts, m_context.get());

        if (process != nullptr)
        {
            //the process can be stopped if the job is canceled or the worker is shutting down
            while (!process->IsFinished())
            {
                process->UpdateProcess();
            }

            //get process result
            ASSERT_TRUE(process->IsSucceed());

            SaveImageToFile(process->GetOutputImage(), "rgb", 10);

            process->GetAppendOutputProducts(outProducts);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, TestIblSkyboxPreset)
    {
        //load builder presets
        auto outcome = BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native());
        ASSERT_TRUE(outcome.IsSuccess());

        AZStd::string inputFile;
        AZStd::vector<AssetBuilderSDK::JobProduct> outProducts;

        inputFile = m_imagFileNameMap[Image_workshop_iblskyboxcm_exr];
        ImageConvertProcess* process = CreateImageConvertProcess(inputFile, m_outputFolder.Native(), "pc", outProducts, m_context.get());

        if (process != nullptr)
        {
            process->ProcessAll();

            //get process result
            ASSERT_TRUE(process->IsSucceed());

            auto specularImage = process->GetOutputIBLSpecularCubemap();
            auto diffuseImage = process->GetOutputIBLDiffuseCubemap();
            ASSERT_TRUE(process->GetOutputImage());
            ASSERT_TRUE(specularImage);
            ASSERT_TRUE(diffuseImage);

            // output converted result if save image is enabled
            SaveImageToFile(process->GetOutputImage(), "ibl_skybox", 10);
            SaveImageToFile(specularImage, "ibl_specular", 10);
            SaveImageToFile(diffuseImage, "ibl_diffuse", 10);

            delete process;
        }
    }

    TEST_F(ImageProcessingTest, EditorTextureSetting_UnavailableExplicitPreset_PreservesSettingsUntilManualSelection)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        const auto platforms = BuilderSettingManager::Instance()->GetPlatformList();
        ASSERT_FALSE(platforms.empty());

        QTemporaryDir temporaryDirectory;
        ASSERT_TRUE(temporaryDirectory.isValid());
        const QString imagePath = temporaryDirectory.filePath("arbitrary_diff.png");
        QImage sourceImage(32, 32, QImage::Format_RGBA8888);
        sourceImage.fill(Qt::white);
        ASSERT_TRUE(sourceImage.save(imagePath, "PNG"));
        QFile imageFile(imagePath);
        ASSERT_TRUE(imageFile.open(QIODevice::ReadOnly));
        const QByteArray originalImageBytes = imageFile.readAll();
        imageFile.close();

        TextureSettings original;
        original.m_preset = "UnavailableExplicitPreset";
        original.m_sizeReduceLevel = 3;
        original.m_suppressEngineReduce = true;
        original.m_enableMipmap = false;
        original.m_mipGenType = MipGenType::quadratic;
        original.m_maintainAlphaCoverage = true;
        original.m_tags.insert("preserve-me");
        TextureSettings platformOverride = original;
        platformOverride.m_sizeReduceLevel = 4;
        ASSERT_TRUE(original.ApplySettings(platformOverride, platforms.front(), m_context.get()).IsSuccess());

        const AZStd::string sourcePath(imagePath.toUtf8().constData());
        const AZStd::string sidecarPath = sourcePath + TextureSettings::ExtensionName;
        ASSERT_TRUE(TextureSettings::WriteTextureSetting(sidecarPath, original, m_context.get()).IsSuccess());
        const auto expected = TextureSettings::GetMultiplatformTextureSetting(original, m_context.get());

        ImageProcessingAtomEditor::EditorTextureSetting editorSettings(sourcePath);
        ASSERT_TRUE(editorSettings.m_img);
        ASSERT_EQ(editorSettings.m_settingsMap.size(), expected.size());
        for (const auto& [platform, settings] : expected)
        {
            const auto actual = editorSettings.m_settingsMap.find(platform);
            ASSERT_NE(actual, editorSettings.m_settingsMap.end());
            EXPECT_TRUE(actual->second.Equals(settings, m_context.get()));
            EXPECT_EQ(actual->second.m_enableMipmap, settings.m_enableMipmap);
            AZ::u32 minReduce = 0;
            AZ::u32 maxReduce = 0;
            EXPECT_TRUE(editorSettings.GetResolutionInfo(platform, minReduce, maxReduce).empty());
            EXPECT_TRUE(editorSettings.GetResolutionInfoForMipmap(platform).empty());
        }

        // Mirror the existing Save path: recombine platform patches, then reopen.
        TextureSettings& saved = editorSettings.GetMultiplatformTextureSetting();
        for (const auto& [platform, settings] : editorSettings.m_settingsMap)
        {
            ASSERT_TRUE(saved.ApplySettings(settings, platform, m_context.get()).IsSuccess());
        }
        ASSERT_TRUE(TextureSettings::WriteTextureSetting(sidecarPath, saved, m_context.get()).IsSuccess());
        ImageProcessingAtomEditor::EditorTextureSetting reopened(sourcePath);
        for (const auto& [platform, settings] : expected)
        {
            const auto actual = reopened.m_settingsMap.find(platform);
            ASSERT_NE(actual, reopened.m_settingsMap.end());
            EXPECT_TRUE(actual->second.Equals(settings, m_context.get()));
            EXPECT_EQ(actual->second.m_enableMipmap, settings.m_enableMipmap);
        }

        // Changing the preset explicitly remains available after preserving it.
        reopened.SetToPreset(PresetName("Normals"));
        for (const auto& [platform, settings] : reopened.m_settingsMap)
        {
            EXPECT_EQ(settings.m_preset, PresetName("Normals"));
            EXPECT_FALSE(reopened.GetResolutionInfoForMipmap(platform).empty());
        }
        ASSERT_TRUE(imageFile.open(QIODevice::ReadOnly));
        EXPECT_EQ(imageFile.readAll(), originalImageBytes);
    }

    TEST_F(ImageProcessingTest, TextureSettingReflect_SerializingModernDataInAndOut_WritesAndParsesFileAccurately)
    {
        AZStd::string filepath = "test.xml";

        // Fill-in structure with test data
        TextureSettings fakeTextureSettings;
        fakeTextureSettings.m_preset = "testPreset";
        fakeTextureSettings.m_sizeReduceLevel = 0;
        fakeTextureSettings.m_suppressEngineReduce = true;
        fakeTextureSettings.m_enableMipmap = false;
        fakeTextureSettings.m_maintainAlphaCoverage = true;
        fakeTextureSettings.m_mipAlphaAdjust = { 0xDEAD, 0xBADBEEF, 0xBADC0DE, 0xFEEFEE, 0xBADF00D, 0xC0FFEE };
        fakeTextureSettings.m_mipGenEval = MipGenEvalType::max;
        fakeTextureSettings.m_mipGenType = MipGenType::quadratic;

        // Write test data to file
        auto writeOutcome = TextureSettings::WriteTextureSetting(filepath, fakeTextureSettings, m_context.get());
        EXPECT_TRUE(writeOutcome.IsSuccess());

        // Parse test data to file
        TextureSettings parsedFakeTextureSettings;
        auto readOutcome = TextureSettings::LoadTextureSetting(filepath, parsedFakeTextureSettings, m_context.get());
        EXPECT_TRUE(readOutcome.IsSuccess());
        EXPECT_TRUE(parsedFakeTextureSettings.Equals(fakeTextureSettings, m_context.get()));

        // Delete temp data
        AZ::IO::FileIOBase::GetInstance()->Remove(filepath.c_str());
    }

    namespace
    {
        class TextureSetPresetRequests final : public ImageProcessingSystemComponent
        {
        public:
            AZ_CLASS_ALLOCATOR(TextureSetPresetRequests, AZ::SystemAllocator);
            TextureSetPresetRequests() { ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusConnect(); }
            ~TextureSetPresetRequests() override { ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusDisconnect(); }
        };

        struct TextureSetProject final : testing::NiceMock<UnitTests::MockAssetSystemRequest>
        {
            explicit TextureSetProject(const QString& root) : m_root(root)
            {
                auto* fileIO = AZ::IO::FileIOBase::GetInstance();
                const char* original = fileIO->GetAlias("@projectroot@");
                m_hadAlias = original != nullptr;
                m_original = original ? original : "";
                fileIO->SetAlias("@projectroot@", root.toUtf8().constData());
                ON_CALL(*this, GenerateRelativeSourcePath(testing::_, testing::_, testing::_)).WillByDefault(
                    [this](const AZStd::string& path, AZStd::string& relative, AZStd::string& watch)
                    {
                        const QString local = QDir(m_root).relativeFilePath(QString::fromUtf8(path.c_str()));
                        if (QDir::isAbsolutePath(local) || local == ".." || local.startsWith("../"))
                        {
                            return false;
                        }
                        relative = local.toUtf8().constData();
                        watch = m_root.toUtf8().constData();
                        return true;
                    });
                BusConnect(); // Only the AP scan-folder boundary is supplied. The component, preset data and XML are real.
            }
            ~TextureSetProject()
            {
                BusDisconnect();
                auto* fileIO = AZ::IO::FileIOBase::GetInstance();
                if (m_hadAlias)
                {
                    fileIO->SetAlias("@projectroot@", m_original.c_str());
                }
                else
                {
                    fileIO->ClearAlias("@projectroot@");
                }
            }
            QString m_root;
            AZStd::string m_original;
            bool m_hadAlias = false;
        };

        class TextureSetExteriorTraceObserver final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            TextureSetExteriorTraceObserver() { BusConnect(); }
            ~TextureSetExteriorTraceObserver() override { BusDisconnect(); }
            bool OnError(const char*, const char*) override { ++m_errors; return false; }
            bool OnWarning(const char*, const char*) override { ++m_warnings; return false; }
            bool OnPreAssert(const char*, int, const char*, const char*) override { ++m_asserts; return false; }
            AZStd::atomic<int> m_errors{0};
            AZStd::atomic<int> m_warnings{0};
            AZStd::atomic<int> m_asserts{0};
        };

        AZ::Outcome<bool, AZStd::string> EnsureTextureSetPreset(const AZStd::string& path, const AZStd::string& preset)
        {
            AZ::Outcome<bool, AZStd::string> result = AZ::Failure(AZStd::string("Missing Image Processing handler"));
            ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::BroadcastResult(result,
                &ImageProcessingAtomEditor::ImageProcessingEditorRequests::EnsureTexturePresetIfUnset, path, preset);
            return result;
        }

        AZStd::string WriteTextureSetImage(const QTemporaryDir& directory, const char* name)
        {
            QImage image(8, 8, QImage::Format_RGBA8888);
            image.fill(qRgba(29, 101, 213, 255));
            const QString path = directory.filePath(QString::fromUtf8(name));
            EXPECT_TRUE(image.save(path, "PNG"));
            return path.toUtf8().constData();
        }
    }

    TEST_F(ImageProcessingTest, TextureSetPreset_UnmatchedNormalCreatesVerifiedSettingsAndPreservesSource)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        TextureSetProject project(directory.path());
        TextureSetPresetRequests requests;
        const auto path = WriteTextureSetImage(directory, "stone_nor_gl_4k.png");
        const auto imageBytes = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(imageBytes.IsSuccess());
        auto result = EnsureTextureSetPreset(path, "Normals");
        ASSERT_TRUE(result.IsSuccess()) << result.GetError().c_str();
        EXPECT_TRUE(result.GetValue());
        TextureSettings loaded;
        ASSERT_TRUE(TextureSettings::LoadTextureSetting(path + TextureSettings::ExtensionName, loaded, m_context.get()).IsSuccess());
        EXPECT_EQ(loaded.m_preset, PresetName("Normals"));
        EXPECT_EQ(loaded.m_sizeReduceLevel, 0);
        EXPECT_TRUE(loaded.m_enableMipmap);
        const auto originalSettings = AZ::Utils::ReadFile<AZStd::string>(path + TextureSettings::ExtensionName);
        ASSERT_TRUE(originalSettings.IsSuccess());
        result = EnsureTextureSetPreset(path, "Normals");
        EXPECT_TRUE(result.IsSuccess()) << result.GetError().c_str();
        if (result.IsSuccess())
        {
            EXPECT_FALSE(result.GetValue());
        }
        TextureSettings repeated;
        ASSERT_TRUE(TextureSettings::LoadTextureSetting(path + TextureSettings::ExtensionName, repeated, m_context.get()).IsSuccess());
        EXPECT_TRUE(repeated.Equals(loaded, m_context.get()));
        const auto currentSettings = AZ::Utils::ReadFile<AZStd::string>(path + TextureSettings::ExtensionName);
        ASSERT_TRUE(currentSettings.IsSuccess());
        EXPECT_EQ(currentSettings.GetValue(), originalSettings.GetValue());
        const auto currentImage = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(currentImage.IsSuccess());
        EXPECT_EQ(currentImage.GetValue(), imageBytes.GetValue());
        EXPECT_TRUE(QDir(directory.path()).entryList({"*.tmp"}, QDir::Files).empty());
    }

    TEST_F(ImageProcessingTest, TextureSetPreset_CompatibleFilenameInferenceDoesNotCreateSidecar)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        TextureSetProject project(directory.path());
        TextureSetPresetRequests requests;
        const auto path = WriteTextureSetImage(directory, "stone_4K_Normal.png");
        auto result = EnsureTextureSetPreset(path, "Normals");
        ASSERT_TRUE(result.IsSuccess()) << result.GetError().c_str();
        EXPECT_FALSE(result.GetValue());
        EXPECT_FALSE(QFileInfo::exists(QString::fromUtf8((path + TextureSettings::ExtensionName).c_str())));
        const auto albedo = WriteTextureSetImage(directory, "stone_diff_4k.png");
        result = EnsureTextureSetPreset(albedo, "Albedo");
        ASSERT_TRUE(result.IsSuccess()) << result.GetError().c_str();
        EXPECT_FALSE(result.GetValue()); // Both opaque/alpha defaults preserve the base-color RGB; no image decode is needed.
        EXPECT_FALSE(QFileInfo::exists(QString::fromUtf8((albedo + TextureSettings::ExtensionName).c_str())));
    }

    TEST_F(ImageProcessingTest, TextureSetPreset_RealPackedAndScalarPresetsPreserveLinearData)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        TextureSetProject project(directory.path());
        TextureSetPresetRequests requests;
        struct Assignment { const char* file; const char* preset; };
        const Assignment assignments[] = {
            Assignment{"stone_arm_4k.png", "LayerMask"}, {"stone_orm_4k.png", "LayerMask"}, {"stone_ordp_4k.png", "LayerMask"},
            {"stone_roughness_4k.png", "Reflectance"}, {"stone_metallic_4k.png", "Reflectance"},
            {"stone_ao_4k.png", "AmbientOcclusion"}, {"stone_disp_4k.png", "Displacement"}};
        for (const auto& assignment : assignments)
        {
            const auto path = WriteTextureSetImage(directory, assignment.file);
            auto result = EnsureTextureSetPreset(path, assignment.preset);
            ASSERT_TRUE(result.IsSuccess()) << result.GetError().c_str();
            EXPECT_TRUE(result.GetValue());
            TextureSettings loaded;
            ASSERT_TRUE(TextureSettings::LoadTextureSetting(path + TextureSettings::ExtensionName, loaded, m_context.get()).IsSuccess());
            EXPECT_EQ(loaded.m_preset, PresetName(assignment.preset));
            EXPECT_EQ(loaded.m_sizeReduceLevel, 0);
            for (const char* platform : {"pc", "linux"})
            {
                const auto* preset = BuilderSettingManager::Instance()->GetPreset(loaded.m_preset, platform);
                ASSERT_NE(preset, nullptr);
                EXPECT_EQ(preset->m_srcColorSpace, ColorSpace::linear);
                EXPECT_EQ(preset->m_destColorSpace, ColorSpace::linear);
                if (loaded.m_preset == PresetName("LayerMask"))
                {
                    EXPECT_EQ(preset->m_pixelFormat, ePixelFormat_R8G8B8X8);
                    EXPECT_TRUE(preset->m_swizzle.empty());
                    EXPECT_FALSE(preset->m_isMipRenormalize);
                    EXPECT_FALSE(preset->m_isPowerOf2);
                    EXPECT_EQ(preset->m_maxTextureSize, 0);
                    EXPECT_EQ(preset->m_minTextureSize, 0);
                    EXPECT_EQ(preset->m_sizeReduceLevel, 0);
                    EXPECT_EQ(preset->m_glossFromNormals, 0);
                    EXPECT_EQ(preset->m_mipmapSetting, nullptr);
                    EXPECT_TRUE(loaded.m_enableMipmap);
                    EXPECT_EQ(loaded.m_mipGenType, MipGenType::blackmanHarris);
                    EXPECT_FALSE(loaded.m_maintainAlphaCoverage);
                }
                else
                {
                    EXPECT_EQ(preset->m_pixelFormat, ePixelFormat_BC4); // Scalar Red is representable; this format is never used for packed RGB.
                }
            }
        }
        EXPECT_TRUE(QDir(directory.path()).entryList({"*.tmp"}, QDir::Files).empty());
    }

    TEST_F(ImageProcessingTest, TextureSetPreset_IncompatibleNativeBaseAndPlatformOverrideRequireReview)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        TextureSetProject project(directory.path());
        TextureSetPresetRequests requests;
        for (const bool platformOverride : {false, true})
        {
            SCOPED_TRACE(platformOverride);
            const auto path = WriteTextureSetImage(directory, platformOverride ? "override_nor_gl_4k.png" : "base_nor_gl_4k.png");
            const auto sidecar = path + TextureSettings::ExtensionName;
            TextureSettings authored;
            authored.m_preset = PresetName(platformOverride ? "Normals" : "Albedo");
            authored.m_sizeReduceLevel = 2;
            if (platformOverride)
            {
                TextureSettings overridden = authored;
                overridden.m_preset = PresetName("Albedo");
                ASSERT_TRUE(authored.ApplySettings(overridden, "pc", m_context.get()).IsSuccess());
            }
            ASSERT_TRUE(TextureSettings::WriteTextureSetting(sidecar, authored, m_context.get()).IsSuccess());
            const auto original = AZ::Utils::ReadFile<AZStd::string>(sidecar);
            ASSERT_TRUE(original.IsSuccess());
            EXPECT_FALSE(EnsureTextureSetPreset(path, "Normals").IsSuccess());
            const auto current = AZ::Utils::ReadFile<AZStd::string>(sidecar);
            ASSERT_TRUE(current.IsSuccess());
            EXPECT_EQ(current.GetValue(), original.GetValue());
            TextureSettings reopened;
            ASSERT_TRUE(TextureSettings::LoadTextureSetting(sidecar, reopened, m_context.get()).IsSuccess());
            EXPECT_TRUE(reopened.Equals(authored, m_context.get()));
        }
        EXPECT_TRUE(QDir(directory.path()).entryList({"*.tmp"}, QDir::Files).empty());
    }

    TEST_F(ImageProcessingTest, TextureSetPreset_ManualSettingsAndNonProjectSourcesAreNeverChanged)
    {
        ASSERT_TRUE(BuilderSettingManager::Instance()->LoadConfigFromFolder(m_defaultSettingFolder.Native()).IsSuccess());
        QTemporaryDir directory;
        QTemporaryDir outside;
        ASSERT_TRUE(directory.isValid());
        ASSERT_TRUE(outside.isValid());
        TextureSetProject project(directory.path());
        TextureSetPresetRequests requests;
        const auto path = WriteTextureSetImage(directory, "stone_nor_dx_4k.png");
        const auto sidecar = path + TextureSettings::ExtensionName;
        for (const AZStd::string manual : {AZStd::string("manual sidecar, deliberately not XML"),
                 AZStd::string("<ObjectStream version=\"3\"><Class")})
        {
            SCOPED_TRACE(manual.c_str());
            ASSERT_TRUE(AZ::Utils::WriteFile(manual, sidecar).IsSuccess());
            AZ::Outcome<bool, AZStd::string> result = AZ::Failure(AZStd::string("Not called"));
            {
                TextureSetExteriorTraceObserver trace;
                result = EnsureTextureSetPreset(path, "Normals");
                EXPECT_EQ(trace.m_errors.load(), 0);
                EXPECT_EQ(trace.m_warnings.load(), 0);
                EXPECT_EQ(trace.m_asserts.load(), 0);
            }
            EXPECT_FALSE(result.IsSuccess()); // Inline review, not a noisy deserialization attempt or a claim of incompatibility.
            auto bytes = AZ::Utils::ReadFile<AZStd::string>(sidecar);
            ASSERT_TRUE(bytes.IsSuccess());
            EXPECT_EQ(bytes.GetValue(), manual);
        }
        const auto foreign = WriteTextureSetImage(outside, "foreign_4K_Normal.png");
        EXPECT_FALSE(EnsureTextureSetPreset(foreign, "Normals").IsSuccess());
        EXPECT_FALSE(QFileInfo::exists(QString::fromUtf8((foreign + TextureSettings::ExtensionName).c_str())));
        const auto unconfigured = WriteTextureSetImage(directory, "stone_unknown_4k.png");
        EXPECT_FALSE(EnsureTextureSetPreset(unconfigured, "UnknownPreset").IsSuccess());
        EXPECT_FALSE(QFileInfo::exists(QString::fromUtf8((unconfigured + TextureSettings::ExtensionName).c_str())));
        EXPECT_TRUE(QDir(directory.path()).entryList({"*.tmp"}, QDir::Files).empty());
    }
} // UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);


