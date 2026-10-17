/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <Common/ShaderAssetTestUtils.h>

#include <JE/Render/RPI/Reflect/Image/ImageMipChainAsset.h>
#include <JE/Render/RPI/Reflect/Image/ImageMipChainAssetCreator.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAssetHandler.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAssetCreator.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImagePoolAsset.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImagePoolAssetCreator.h>
#include <JE/Render/RPI/Reflect/Asset/BuiltInAssetHandler.h>

#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <JE/Render/RPI/Public/Image/StreamingImagePool.h>
#include <JE/Render/RPI/Public/RPIUtils.h>

#include <AzCore/Instance/InstanceDatabase.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/intrusive_list.h>

#include <Common/RPITestFixture.h>
#include <Common/RHI/Stubs.h>
#include <Common/ErrorMessageFinder.h>
#include <Common/SerializeTester.h>

namespace JE::Render
{
    namespace RPI
    {
        class StreamingImageAssetTester
            : public UnitTest::AssetTester<StreamingImageAsset>
        {
        public:
            StreamingImageAssetTester() = default;
            ~StreamingImageAssetTester() override = default;

            void SetAssetReady(AZ::Data::Asset<StreamingImageAsset>& asset) override
            {
                asset->SetReady();
            }
        };

        class ImageMipChainAssetTester
            : public UnitTest::AssetTester<ImageMipChainAsset>
        {
        public:
            ImageMipChainAssetTester() = default;
            ~ImageMipChainAssetTester() override = default;

            void SetAssetReady(AZ::Data::Asset<ImageMipChainAsset>& asset) override
            {
                asset->SetReady();
            }
        };

        class StreamingImagePoolAssetTester
            : public UnitTest::SerializeTester<StreamingImagePoolAsset>
        {
            using Base = UnitTest::SerializeTester<StreamingImagePoolAsset>;
        public:
            StreamingImagePoolAssetTester(AZ::SerializeContext* serializeContext)
                : Base(serializeContext)
            {}

            AZ::Data::Asset<StreamingImagePoolAsset> SerializeInHelper(const AZ::Data::AssetId& assetId)
            {
                AZ::Data::Asset<StreamingImagePoolAsset> asset = Base::SerializeIn(assetId);
                asset->SetReady();
                return asset;
            }
        };
    }
}

namespace UnitTest
{
    struct TestStreamingImagePoolDescriptor
        : public AZ::RHI::StreamingImagePoolDescriptor
    {
        AZ_CLASS_ALLOCATOR(TestStreamingImagePoolDescriptor, AZ::SystemAllocator);
        AZ_RTTI(TestStreamingImagePoolDescriptor, "{8D0CA5A2-F886-42EF-9B00-09E6C9F6B90B}", AZ::RHI::StreamingImagePoolDescriptor);

        static constexpr uint32_t Magic = 0x1234;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestStreamingImagePoolDescriptor, AZ::RHI::StreamingImagePoolDescriptor>()
                    ->Version(0)
                    ->Field("m_magic", &TestStreamingImagePoolDescriptor::m_magic)
                    ;
            }
        }

        TestStreamingImagePoolDescriptor() = default;
        TestStreamingImagePoolDescriptor(size_t budgetInBytes)
        {
            m_budgetInBytes = budgetInBytes;
        }

        // A test value to ensure that serialization occurred correctly.
        uint32_t m_magic = Magic;
    };

    class TestStreamingImageContext
        : public JE::Render::RPI::StreamingImageContext
        , public AZStd::intrusive_list_node<TestStreamingImageContext>
    {
    public:
        AZ_CLASS_ALLOCATOR(TestStreamingImageContext, AZ::SystemAllocator);
        AZ_RTTI(TestStreamingImageContext, "{E2FC3EB5-4F66-41D0-9ABE-6EDD2622DD88}", JE::Render::RPI::StreamingImageContext);
    };

    class StreamingImageTests
        : public RPITestFixture
    {
    protected:

        AZ::Data::AssetHandler* m_imageHandler = nullptr;
        AZ::Data::AssetHandler* m_mipChainHandler = nullptr;
        AZ::Data::Instance<JE::Render::RPI::StreamingImagePool> m_defaultPool = nullptr;

        StreamingImageTests()
        {}

        void SetUp() override
        {
            using namespace AZ;

            RPITestFixture::SetUp();

            auto* serializeContext = GetSerializeContext();
            TestStreamingImagePoolDescriptor::Reflect(serializeContext);

            m_imageHandler = Data::AssetManager::Instance().GetHandler(JE::Render::RPI::StreamingImageAsset::RTTI_Type());
            m_mipChainHandler = Data::AssetManager::Instance().GetHandler(JE::Render::RPI::ImageMipChainAsset::RTTI_Type());

            AZ::Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(16 * 1024 * 1024);
            m_defaultPool = JE::Render::RPI::StreamingImagePool::FindOrCreate(poolAsset);
        }

        void TearDown() override
        {
            using namespace AZ;
            RHI::ResourceInvalidateBus::ExecuteQueuedEvents();

            m_defaultPool = nullptr;

            RPITestFixture::TearDown();
        }

        AZStd::vector<uint8_t> BuildImageData(uint32_t width, uint32_t height, uint32_t pixelSize)
        {
            const size_t imageSize = width * height * pixelSize;

            AZStd::vector<uint8_t> image;
            image.reserve(imageSize);

            uint8_t testValue = 0;

            for (uint32_t y = 0; y < height; ++y)
            {
                for (uint32_t x = 0; x < width; ++x)
                {
                    for (uint32_t channel = 0; channel < pixelSize; ++channel)
                    {
                        image.push_back(testValue);
                        testValue++;
                    }
                }
            }

            EXPECT_EQ(image.size(), imageSize);
            return image;
        }

        void ValidateImageData(AZStd::span<const uint8_t> data, const AZ::RHI::DeviceImageSubresourceLayout& layout)
        {
            const uint32_t pixelSize = layout.m_size.m_width / layout.m_bytesPerRow;

            uint32_t byteOffset = 0;

            for (uint32_t y = 0; y < layout.m_size.m_height; ++y)
            {
                for (uint32_t x = 0; x < layout.m_size.m_width; ++x)
                {
                    for (uint32_t channel = 0; channel < pixelSize; ++channel)
                    {
                        uint8_t value = data[byteOffset];

                        EXPECT_EQ(value, static_cast<uint8_t>(byteOffset));

                        byteOffset++;
                    }
                }
            }
        }

        void ValidateMipChainAsset(
            JE::Render::RPI::ImageMipChainAsset* mipChain,
            uint16_t expectedMipLevels,
            uint16_t expectedArraySize,
            uint32_t expectedPixelSize)
        {
            using namespace AZ;

            EXPECT_NE(mipChain, nullptr);
            EXPECT_EQ(expectedMipLevels, mipChain->GetMipLevelCount());
            EXPECT_EQ(expectedArraySize, mipChain->GetArraySize());
            EXPECT_EQ(expectedMipLevels * expectedArraySize, mipChain->GetSubImageCount());

            const uint32_t imageSize = 1 << mipChain->GetMipLevelCount();

            for (uint16_t mipLevel = 0; mipLevel < mipChain->GetMipLevelCount(); ++mipLevel)
            {
                RHI::DeviceImageSubresourceLayout layout = BuildSubImageLayout(imageSize >> mipLevel, expectedPixelSize);

                EXPECT_EQ(memcmp(&layout, &mipChain->GetSubImageLayout(mipLevel), sizeof(RHI::DeviceImageSubresourceLayout)), 0);

                for (uint16_t arrayIndex = 0; arrayIndex < mipChain->GetArraySize(); ++arrayIndex)
                {
                    AZStd::span<const uint8_t> imageData = mipChain->GetSubImageData(mipLevel, arrayIndex);
                    ValidateImageData(imageData, layout);
                }
            }
        }

        void ValidateImageAsset(JE::Render::RPI::StreamingImageAsset* imageAsset)
        {
            using namespace AZ;

            EXPECT_NE(imageAsset, nullptr);

            RHI::ImageDescriptor imageDesc = imageAsset->GetImageDescriptor();

            size_t mipCountTotal = 0;
            for (size_t i = 0; i < imageAsset->GetMipChainCount(); ++i)
            {
                // The last mip chain asset (tail mip chain) is expected to be empty since the actual mip chain asset data is embedded in StreamingImageAsset
                if (i != imageAsset->GetMipChainCount() - 1)
                {
                    EXPECT_TRUE(imageAsset->GetMipChainAsset(i).GetId().IsValid());
                }
                EXPECT_EQ(imageAsset->GetMipLevel(i), mipCountTotal);
                mipCountTotal += imageAsset->GetMipCount(i);
            }

            EXPECT_EQ(imageDesc.m_mipLevels, mipCountTotal);
        }

        void ValidateImagePoolAsset(JE::Render::RPI::StreamingImagePoolAsset* poolAsset, size_t budgetInBytes)
        {
            using namespace AZ;

            EXPECT_EQ(poolAsset->GetPoolDescriptor().m_budgetInBytes, budgetInBytes);

            {
                const auto* desc = azrtti_cast<const TestStreamingImagePoolDescriptor*>(&poolAsset->GetPoolDescriptor());
                EXPECT_NE(desc, nullptr);
                EXPECT_EQ(desc->m_magic, UnitTest::TestStreamingImagePoolDescriptor::Magic);
            }
        }

        void ValidateImageResidency(JE::Render::RPI::StreamingImage* imageInstance, JE::Render::RPI::StreamingImageAsset* imageAsset)
        {
            using namespace AZ;

            auto imageSystem = JE::Render::RPI::ImageSystemInterface::Get();

            const size_t mipChainTailIndex = imageAsset->GetMipChainCount() - 1;

            RHI::Ptr<RHI::Image> rhiImage = imageInstance->GetRHIImage();

            // This should no-op.
            imageInstance->TrimToMipChainLevel(mipChainTailIndex);

            // Validate that nothing was actually evicted, since we've set to NoEvict.
            for (size_t i = 0; i < mipChainTailIndex; ++i)
            {
                EXPECT_TRUE(imageAsset->GetMipChainAsset(i).IsReady());
            }
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

            // Expand to the most detailed mip chain.
            imageInstance->QueueExpandToMipChainLevel(0);

            // We should still be the same residency level, since it's queued.
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

            // Tick the streaming system.
            imageSystem->Update();

            // Now we should be at the desired residency level.
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);

            // Expanding 'down' is a no-op.
            imageInstance->QueueExpandToMipChainLevel(1);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);

            // Trimming down a notch. This happens instantly.
            imageInstance->TrimToMipChainLevel(1);
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(1));

            // Trim down again.
            imageInstance->TrimToMipChainLevel(2);
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(2));

            // Expanding back up to 1.
            imageInstance->QueueExpandToMipChainLevel(1);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), imageAsset->GetMipLevel(1));

            // Expanding back up to 0.
            imageInstance->QueueExpandToMipChainLevel(0);
            imageSystem->Update();
            EXPECT_EQ(rhiImage->GetResidentMipLevel(), 0);
        }

        AZ::RHI::DeviceImageSubresourceLayout BuildSubImageLayout(uint32_t imageSize, uint32_t pixelSize)
        {
            using namespace AZ;

            RHI::DeviceImageSubresourceLayout layout;
            layout.m_size = RHI::Size{ imageSize, imageSize, 1 };
            layout.m_rowCount = imageSize;
            layout.m_bytesPerRow = imageSize * pixelSize;
            layout.m_bytesPerImage = imageSize * imageSize * pixelSize;
            return layout;
        }

        AZ::Data::Asset<JE::Render::RPI::ImageMipChainAsset> BuildMipChainAsset(uint16_t mipOffset, uint16_t mipLevels, uint16_t arraySize, uint32_t pixelSize)
        {
            using namespace AZ;

            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;

            const uint32_t imageSize = 1 << (mipLevels + mipOffset);

            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);

            for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
            {
                const uint32_t mipSize = imageSize >> mipLevel;

                RHI::DeviceImageSubresourceLayout layout = BuildSubImageLayout(mipSize, pixelSize);

                assetCreator.BeginMip(layout);

                for (uint32_t arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex)
                {
                    AZStd::vector<uint8_t> data = BuildImageData(mipSize, mipSize, pixelSize);
                    assetCreator.AddSubImage(data.data(), data.size());
                }

                assetCreator.EndMip();
            }

            Data::Asset<JE::Render::RPI::ImageMipChainAsset> asset;
            EXPECT_TRUE(assetCreator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        AZ::Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> BuildImagePoolAsset(size_t budgetInBytes)
        {
            using namespace AZ;

            JE::Render::RPI::StreamingImagePoolAssetCreator assetCreator;

            assetCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            assetCreator.SetPoolDescriptor(AZStd::make_unique<TestStreamingImagePoolDescriptor>(budgetInBytes));

            Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> poolAsset;
            EXPECT_TRUE(assetCreator.End(poolAsset));

            EXPECT_TRUE(poolAsset.IsReady());
            EXPECT_NE(poolAsset.Get(), nullptr);
            return poolAsset;
        }

        AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> BuildTestImage(AZ::RHI::Format format = AZ::RHI::Format::R8G8B8A8_UNORM)
        {
            using namespace AZ;

            const uint32_t arraySize = 2;
            const uint32_t pixelSize = RHI::GetFormatSize(format);
            const uint32_t mipCountHead = 1;
            const uint32_t mipCountMiddle = 2;
            const uint32_t mipCountTail = 3;
            const uint32_t mipCountTotal = mipCountHead + mipCountMiddle + mipCountTail;
            const uint32_t imageWidth = 1 << mipCountTotal;
            const uint32_t imageHeight = 1 << mipCountTotal;

            Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipTail = BuildMipChainAsset(0, mipCountTail, arraySize, pixelSize);
            Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipMiddle = BuildMipChainAsset(mipCountTail, mipCountMiddle, arraySize, pixelSize);
            Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipHead = BuildMipChainAsset(mipCountTail + mipCountMiddle, mipCountHead, arraySize, pixelSize);

            JE::Render::RPI::StreamingImageAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2DArray(RHI::ImageBindFlags::ShaderRead, imageWidth, imageHeight, arraySize, format);
            imageDesc.m_mipLevels = static_cast<uint16_t>(mipCountTotal);

            assetCreator.SetImageDescriptor(imageDesc);
            assetCreator.AddMipChainAsset(*mipHead.Get());
            assetCreator.AddMipChainAsset(*mipMiddle.Get());
            assetCreator.AddMipChainAsset(*mipTail.Get());
            assetCreator.SetPoolAssetId(m_defaultPool->GetAssetId());

            Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset;
            EXPECT_TRUE(assetCreator.End(imageAsset));

            EXPECT_TRUE(imageAsset.IsReady());
            EXPECT_NE(imageAsset.Get(), nullptr);
            return imageAsset;
        }
    };

    TEST_F(StreamingImageTests, MipChainCreate)
    {
        using namespace AZ;

        const uint16_t mipLevels = 5;
        const uint16_t arraySize = 4;
        const uint16_t pixelSize = 4;

        Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipChain = BuildMipChainAsset(0, mipLevels, arraySize, pixelSize);

        ValidateMipChainAsset(mipChain.Get(), mipLevels, arraySize, pixelSize);
    }

    TEST_F(StreamingImageTests, MipChainAssetSuccessAfterErrorCases)
    {
        using namespace AZ;

        const uint16_t mipLevels = 1;
        const uint16_t arraySize = 1;

        Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipChain;

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;

            ErrorMessageFinder messageFinder("Begin() was not called");
            assetCreator.EndMip();
        }

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;

            ErrorMessageFinder messageFinder("Begin() was not called");
            assetCreator.End(mipChain);
        }


        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());

            ErrorMessageFinder messageFinder("Expected 1 sub-images in mip, but got 0.");
            assetCreator.EndMip();
        }

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());

            ErrorMessageFinder messageFinder("You must supply a valid data payload.");
            assetCreator.AddSubImage(nullptr, 0);
        }

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());

            ErrorMessageFinder messageFinder("You must supply a valid data payload.");
            assetCreator.AddSubImage(nullptr, 10);
        }

        uint8_t data[4] = { 0, 5, 10, 15 };
        const uint8_t dataSize = AZ_ARRAY_SIZE(data);

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);

            ErrorMessageFinder messageFinder("Exceeded the 1 array slices declared in Begin().");
            assetCreator.AddSubImage(data, dataSize);
        }

        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);

            ErrorMessageFinder messageFinder("Already building a mip. You must call EndMip() first.");
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());
        }

        // Finally, build a valid one
        {
            JE::Render::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);
            assetCreator.BeginMip(RHI::DeviceImageSubresourceLayout());
            assetCreator.AddSubImage(data, dataSize);
            assetCreator.EndMip();

            EXPECT_TRUE(assetCreator.End(mipChain));

            EXPECT_NE(mipChain.Get(), nullptr);
            EXPECT_EQ(mipChain->GetMipLevelCount(), mipLevels);
            EXPECT_EQ(mipChain->GetArraySize(), arraySize);
            EXPECT_EQ(mipChain->GetSubImageCount(), mipLevels * arraySize);

            AZStd::span<const uint8_t> dataView = mipChain->GetSubImageData(0);
            EXPECT_EQ(dataView[0], data[0]);
            EXPECT_EQ(dataView[1], data[1]);
            EXPECT_EQ(dataView[2], data[2]);
            EXPECT_EQ(dataView[3], data[3]);
        }
    }

    TEST_F(StreamingImageTests, MipChainAssetSerialize)
    {
        using namespace AZ;

        const uint16_t mipLevels = 6;
        const uint16_t arraySize = 2;
        const uint16_t pixelSize = 2;
        
        Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipChain = BuildMipChainAsset(0, mipLevels, arraySize, pixelSize);
        JE::Render::RPI::ImageMipChainAssetTester tester;
        tester.SerializeOut(mipChain);

        Data::Asset<JE::Render::RPI::ImageMipChainAsset> serializedMipChain = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        ValidateMipChainAsset(serializedMipChain.Get(), mipLevels, arraySize, pixelSize);
    }

    TEST_F(StreamingImageTests, PoolAssetCreation)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);
        ValidateImagePoolAsset(poolAsset.Get(), budgetInBytes);
    }

    TEST_F(StreamingImageTests, PoolAssetSerialize)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);
        JE::Render::RPI::StreamingImagePoolAssetTester tester(GetSerializeContext());
        tester.SerializeOut(poolAsset.Get());

        Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> serializedPoolAsset = tester.SerializeInHelper(Data::AssetId(Uuid::CreateRandom()));
        ValidateImagePoolAsset(serializedPoolAsset.Get(), budgetInBytes);
    }

    TEST_F(StreamingImageTests, PoolInstanceCreation)
    {
        using namespace AZ;

        const size_t budgetInBytes = 16 * 1024 * 1024;

        Data::Asset<JE::Render::RPI::StreamingImagePoolAsset> poolAsset = BuildImagePoolAsset(budgetInBytes);

        Data::Instance<JE::Render::RPI::StreamingImagePool> poolInstance = JE::Render::RPI::StreamingImagePool::FindOrCreate(poolAsset);
        EXPECT_NE(poolInstance.get(), nullptr);
        EXPECT_NE(poolInstance->GetRHIPool(), nullptr);
    }

    TEST_F(StreamingImageTests, ImageAssetCreation)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        ValidateImageAsset(imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageAssetSerialize)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        
        JE::Render::RPI::StreamingImageAssetTester tester;
        tester.SerializeOut(imageAsset);
        Data::Asset<JE::Render::RPI::StreamingImageAsset> serializedImageAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        ValidateImageAsset(serializedImageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInstanceCreation)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        Data::Instance<JE::Render::RPI::StreamingImage> imageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(imageAsset);
        EXPECT_NE(imageInstance.get(), nullptr);
        EXPECT_NE(imageInstance->GetRHIImage(), nullptr);
        EXPECT_NE(imageInstance->GetImageView(), nullptr);
        EXPECT_GE(imageAsset->GetMipChainCount(), 0);

        const size_t mipChainTailIndex = imageAsset->GetMipChainCount() - 1;

        EXPECT_EQ(imageInstance->GetRHIImage()->GetResidentMipLevel(), imageAsset->GetMipLevel(mipChainTailIndex));

        for (size_t i = 0; i < mipChainTailIndex; ++i)
        {
            Data::Asset<JE::Render::RPI::ImageMipChainAsset> mipChainAsset = imageAsset->GetMipChainAsset(i);

            EXPECT_TRUE(mipChainAsset.IsReady());
        }
    }

    TEST_F(StreamingImageTests, AssetReloadUpdatesRetainedShaderResourceGroupImageBinding)
    {
        using namespace AZ;

        const auto createImageAsset = [this](const Data::AssetId& assetId)
        {
            auto mipChain = BuildMipChainAsset(0, 1, 1, 4);
            JE::Render::RPI::StreamingImageAssetCreator creator;
            creator.Begin(assetId);
            creator.SetImageDescriptor(RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderRead, 2, 2, RHI::Format::R8G8B8A8_UNORM));
            creator.AddMipChainAsset(*mipChain.Get());
            creator.SetPoolAssetId(m_defaultPool->GetAssetId());
            Data::Asset<JE::Render::RPI::StreamingImageAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            return asset;
        };

        auto originalAsset = createImageAsset(Data::AssetId(Uuid::CreateRandom()));
        ASSERT_TRUE(originalAsset.IsReady());
        auto image = JE::Render::RPI::StreamingImage::FindOrCreate(originalAsset);
        ASSERT_NE(image.get(), nullptr);
        auto* rhiImage = image->GetRHIImage();
        ASSERT_NE(rhiImage, nullptr);
        constexpr int deviceIndex = 0;
        ASSERT_TRUE(rhiImage->IsDeviceSet(deviceIndex));
        auto originalDeviceImage = rhiImage->GetDeviceImage(deviceIndex);
        ASSERT_NE(originalDeviceImage.get(), nullptr);

        auto layout = RHI::ShaderResourceGroupLayout::Create();
        layout->SetName(Name("ReloadImageSrg"));
        layout->SetBindingSlot(0);
        layout->AddShaderInput(RHI::ShaderInputImageDescriptor{
            Name("m_image"), RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1 });
        ASSERT_TRUE(layout->Finalize());
        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), layout);
        ASSERT_TRUE(shaderAsset.IsReady());
        auto srg = JE::Render::RPI::ShaderResourceGroup::Create(shaderAsset, JE::Render::RPI::DefaultSupervariantIndex, layout->GetName());
        ASSERT_NE(srg.get(), nullptr);
        const RHI::ShaderInputImageIndex inputIndex(0);
        ASSERT_TRUE(srg->SetImage(inputIndex, image));
        srg->Compile();
        ProcessQueuedSrgCompilations(shaderAsset, layout->GetName());
        ASSERT_FALSE(srg->IsQueuedForCompile());
        auto deviceSrg = srg->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(deviceIndex);
        ASSERT_NE(deviceSrg.get(), nullptr);
        auto originalDeviceView = deviceSrg->GetData().GetImageView(inputIndex, 0);
        ASSERT_NE(originalDeviceView.get(), nullptr);
        ASSERT_EQ(&originalDeviceView->GetImage(), originalDeviceImage.get());

        // The creator publishes through AssetManager::AssignAssetData and the real AssetBus reload notification.
        auto replacementAsset = createImageAsset(originalAsset.GetId());
        ASSERT_TRUE(replacementAsset.IsReady());
        EXPECT_EQ(replacementAsset.GetId(), originalAsset.GetId());
        EXPECT_NE(replacementAsset.Get(), originalAsset.Get());
        EXPECT_EQ(image->GetRHIImage(), rhiImage);
        ASSERT_TRUE(image->IsInitialized());
        ASSERT_TRUE(rhiImage->IsDeviceSet(deviceIndex));
        ASSERT_NE(image->GetImageView(), nullptr);
        auto replacementDeviceImage = rhiImage->GetDeviceImage(deviceIndex);
        ASSERT_NE(replacementDeviceImage.get(), nullptr);
        ASSERT_NE(replacementDeviceImage.get(), originalDeviceImage.get());

        RHI::ResourceInvalidateBus::ExecuteQueuedEvents();
        if (!srg->IsQueuedForCompile())
        {
            srg->Compile();
        }
        ProcessQueuedSrgCompilations(shaderAsset, layout->GetName());
        EXPECT_FALSE(srg->IsQueuedForCompile());

        // A bound SRG must reference the current image after invalidation/compilation, without another SetImage.
        const auto& boundView = deviceSrg->GetData().GetImageView(inputIndex, 0);
        ASSERT_NE(boundView.get(), nullptr);
        EXPECT_EQ(&boundView->GetImage(), replacementDeviceImage.get());
        EXPECT_EQ(boundView.get(), image->GetImageView()->GetDeviceImageView(deviceIndex).get());
        EXPECT_EQ(srg->GetImage(inputIndex).get(), image.get());
        EXPECT_TRUE(originalAsset.IsReady());
        EXPECT_EQ(originalAsset->GetImageDescriptor().m_size.m_width, 2);
        EXPECT_EQ(originalAsset->GetImageDescriptor().m_format, RHI::Format::R8G8B8A8_UNORM);
    }

    TEST_F(StreamingImageTests, RedundantImageCreationDisconnectsItsAssetHandler)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        auto devicePool = m_defaultPool->GetRHIPool()->GetDeviceStreamingImagePool(0);
        ASSERT_NE(devicePool, nullptr);
        auto* stubPool = static_cast<StubRHI::StreamingImagePool*>(devicePool.get());
        Data::Instance<JE::Render::RPI::StreamingImage> retainedWinner;

        struct RestorePoolHook
        {
            StubRHI::StreamingImagePool& m_pool;
            AZStd::function<void()> m_previousCallback;
            uint32_t m_previousCount;
            ~RestorePoolHook()
            {
                m_pool.m_onNextInitImage = AZStd::move(m_previousCallback);
                m_pool.m_initImageCount = m_previousCount;
            }
        } restoreHook{ *stubPool, AZStd::move(stubPool->m_onNextInitImage), stubPool->m_initImageCount };

        class AssertObserver final : public Debug::TraceMessageBus::Handler
        {
        public:
            AssertObserver() { BusConnect(); }
            ~AssertObserver() override { BusDisconnect(); }
            bool OnPreAssert(const char*, int, const char*, const char*) override
            {
                ++m_count;
                return false;
            }
            uint32_t m_count = 0;
        } assertObserver;

        const uint32_t initialInitCount = stubPool->m_initImageCount;
        stubPool->m_onNextInitImage = [&]()
        {
            // Reenter before the outer instance can be inserted; the hook is already consumed.
            retainedWinner = JE::Render::RPI::StreamingImage::FindOrCreate(imageAsset);
        };
        auto imageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(imageAsset);

        EXPECT_EQ(stubPool->m_initImageCount - initialInitCount, 2u);
        EXPECT_FALSE(stubPool->m_onNextInitImage);
        ASSERT_NE(retainedWinner, nullptr);
        ASSERT_NE(imageInstance, nullptr);
        EXPECT_EQ(imageInstance, retainedWinner);
        EXPECT_TRUE(imageInstance->IsInitialized());
        EXPECT_EQ(imageInstance->GetAssetId(), imageAsset.GetId());
        EXPECT_EQ(Data::InstanceDatabase<JE::Render::RPI::StreamingImage>::Instance().Find(
            Data::InstanceId::CreateFromAsset(imageAsset)), retainedWinner);

        imageInstance = nullptr;
        retainedWinner = nullptr;
        EXPECT_EQ(assertObserver.m_count, 0u);
    }

    TEST_F(StreamingImageTests, ImageInstanceResidency)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();
        Data::Instance<JE::Render::RPI::StreamingImage> imageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(imageAsset);
        ValidateImageResidency(imageInstance.get(), imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInstanceResidencyWithSerialization)
    {
        using namespace AZ;

        // Keep the original around, which holds references to the image mip chain assets and pool asset. We need to
        // keep them in memory to avoid the asset manager trying to hit the catalog.
        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        JE::Render::RPI::StreamingImageAssetTester tester;
        tester.SerializeOut(imageAsset);
        Data::Asset<JE::Render::RPI::StreamingImageAsset> serializedImageAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        
        Data::Instance<JE::Render::RPI::StreamingImage> imageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(serializedImageAsset);
        ValidateImageResidency(imageInstance.get(), imageAsset.Get());
    }

    TEST_F(StreamingImageTests, ImageInternalReferenceTracking)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage();

        Data::Instance<JE::Render::RPI::StreamingImagePool> imagePoolInstance;

        {
            Data::Instance<JE::Render::RPI::StreamingImage> imageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(imageAsset);

            // Hold the pool instance to keep it around.
            imagePoolInstance = imageInstance->GetPool();

            // Tests that we can safely destroy an image after queueing something to the system,
            // and the system will properly avoid touching that data.
            imageInstance->QueueExpandToMipChainLevel(0);
        }

        JE::Render::RPI::ImageSystemInterface::Get()->Update();
    }

    TEST_F(StreamingImageTests, GetSubImagePixelValues)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::StreamingImageAsset> imageAsset = BuildTestImage(AZ::RHI::Format::R8_UNORM);

        auto streamingImageAsset = imageAsset.Get();
        EXPECT_NE(streamingImageAsset, nullptr);

        // Validate retrieving one pixel at a time
        auto size = streamingImageAsset->GetImageDescriptor().m_size;
        for (uint32_t y = 0; y < size.m_height; ++y)
        {
            for (uint32_t x = 0; x < size.m_width; ++x)
            {
                auto pixelDataValue = JE::Render::RPI::GetSubImagePixelValue<float>(imageAsset, x, y);
                auto pixelExpectedValue = static_cast<uint8_t>(y * size.m_width + x) / static_cast<float>(std::numeric_limits<AZ::u8>::max());

                EXPECT_NEAR(pixelDataValue, pixelExpectedValue, Constants::Tolerance);
            }
        }

        // Validate retrieving a region of pixels
        AZStd::vector<float> pixelValues;
        pixelValues.reserve(size.m_width * size.m_height);
        auto topLeft = AZStd::make_pair(0U, 0U);
        auto bottomRight = AZStd::make_pair(size.m_width, size.m_height);
        JE::Render::RPI::GetSubImagePixelValues(imageAsset, topLeft, bottomRight, [&pixelValues]([[maybe_unused]] const AZ::u32& x, [[maybe_unused]] const AZ::u32& y, const float& value) {
            pixelValues.push_back(value);
        });
        for (uint32_t index = 0; index < pixelValues.size(); ++index)
        {
            auto pixelDataValue = pixelValues[index];
            auto pixelExpectedValue = static_cast<uint8_t>(index) / static_cast<float>(std::numeric_limits<AZ::u8>::max());

            EXPECT_NEAR(pixelDataValue, pixelExpectedValue, Constants::Tolerance);
        }
    }
}
