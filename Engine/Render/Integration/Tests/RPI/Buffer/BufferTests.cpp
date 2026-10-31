/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Buffer/BufferAssetCreator.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAssetCreator.h>

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/std/containers/array.h>
#include <cstring>

#include <Common/RHI/Factory.h>
#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>

namespace UnitTest
{
    class BufferTests
        : public RPITestFixture
    {
    protected:

        struct ExpectedBuffer
        {
            AZStd::vector<uint8_t> m_data;
            AZ::RHI::BufferDescriptor m_bufferDescriptor;
            AZ::RHI::BufferViewDescriptor m_viewDescriptor;
        };

        AZ::Data::Asset<JE::Render::RPI::ResourcePoolAsset> CreateTestBufferPoolAsset()
        {
            using namespace AZ;
            using namespace JE::Render::RPI;

            Data::Asset<ResourcePoolAsset> asset;

            auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
            bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::ShaderRead;
            bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;

            ResourcePoolAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());
            creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
            creator.SetPoolName("TestPool");
            creator.End(asset);

            return asset;
        }

        ExpectedBuffer CreateValidBuffer()
        {
            using namespace AZ;

            // Build a simple buffer with 100 elements where
            // each element is 3 floats in size
            const uint32_t elementSize = sizeof(float) * 3;
            const uint32_t elementCount = 100;

            const size_t bufferSize = elementSize * elementCount;
            AZStd::vector<uint8_t> bufferData;
            bufferData.resize(bufferSize);

            // The actual data doesn't matter
            const uint8_t bufferDataSize = static_cast<uint8_t>(bufferData.size());
            for (uint8_t i = 0; i < bufferDataSize; ++i)
            {
                bufferData[i] = i;
            }

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = bufferSize;

            RHI::BufferViewDescriptor viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize);

            ExpectedBuffer validBuffer;
            validBuffer.m_data = bufferData;
            validBuffer.m_bufferDescriptor = bufferDescriptor;
            validBuffer.m_viewDescriptor = viewDescriptor;

            return validBuffer;
        }

        AZ::Data::Asset<JE::Render::RPI::BufferAsset> BuildTestBuffer(ExpectedBuffer& expectedBuffer)
        {
            using namespace AZ;

            // Copy valid buffer to output buffer
            expectedBuffer = CreateValidBuffer();
            const size_t bufferSize = expectedBuffer.m_bufferDescriptor.m_byteCount;

            JE::Render::RPI::BufferAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetBuffer(expectedBuffer.m_data.data(), bufferSize, expectedBuffer.m_bufferDescriptor);
            creator.SetBufferViewDescriptor(expectedBuffer.m_viewDescriptor);
            creator.SetPoolAsset(CreateTestBufferPoolAsset());

            Data::Asset<JE::Render::RPI::BufferAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        void ValidateBufferAsset(JE::Render::RPI::BufferAsset* bufferAsset, const ExpectedBuffer& expectedBuffer)
        {
            using namespace AZ;

            EXPECT_NE(bufferAsset, nullptr);

            EXPECT_TRUE(bufferAsset->GetBuffer().size() == expectedBuffer.m_data.size());

            const bool sameBuffer = memcmp(&bufferAsset->GetBuffer()[0], expectedBuffer.m_data.data(), bufferAsset->GetBuffer().size()) == 0;
            EXPECT_TRUE(sameBuffer);

            const RHI::BufferDescriptor& assetBufferDescriptor = bufferAsset->GetBufferDescriptor();
            const RHI::BufferDescriptor& expectedBufferDescriptor = expectedBuffer.m_bufferDescriptor;

            EXPECT_TRUE(assetBufferDescriptor.GetHash() == expectedBufferDescriptor.GetHash());
            EXPECT_TRUE(assetBufferDescriptor.m_bindFlags == expectedBufferDescriptor.m_bindFlags);
            EXPECT_TRUE(assetBufferDescriptor.m_byteCount == expectedBufferDescriptor.m_byteCount);
            EXPECT_TRUE(assetBufferDescriptor.m_sharedQueueMask == expectedBufferDescriptor.m_sharedQueueMask);

            const RHI::BufferViewDescriptor& assetViewDescriptor = bufferAsset->GetBufferViewDescriptor();
            const RHI::BufferViewDescriptor& expectedViewDescriptor = expectedBuffer.m_viewDescriptor;

            EXPECT_TRUE(assetViewDescriptor.GetHash() == expectedViewDescriptor.GetHash());
            EXPECT_TRUE(assetViewDescriptor.m_elementCount == expectedViewDescriptor.m_elementCount);
            EXPECT_TRUE(assetViewDescriptor.m_elementFormat == expectedViewDescriptor.m_elementFormat);
            EXPECT_TRUE(assetViewDescriptor.m_elementOffset == expectedViewDescriptor.m_elementOffset);
            EXPECT_TRUE(assetViewDescriptor.m_elementSize == expectedViewDescriptor.m_elementSize);
        }

        void EncodedBufferFixture(
            uint32_t count, uint32_t stride, const JE::Render::RPI::BufferAsset::BufferData& encoded,
            JE::Render::RPI::BufferAsset::CompressionFormat compression, AZ::Data::Asset<JE::Render::RPI::BufferAsset>& asset)
        {
            using namespace JE::Render::RPI;
            BufferAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetUseCommonPool(CommonBufferPoolType::StaticInputAssembly);
            creator.SetCompressionFormat(compression);
            AZ::RHI::BufferDescriptor descriptor; descriptor.m_byteCount = static_cast<uint64_t>(count) * stride;
            descriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
            BufferAsset::BufferData initial(static_cast<size_t>(descriptor.m_byteCount), 0);
            creator.SetBuffer(initial.data(), initial.size(), descriptor);
            creator.SetBufferViewDescriptor(AZ::RHI::BufferViewDescriptor::CreateStructured(0, count, stride));
            ASSERT_TRUE(creator.End(asset));
            // Serialize enum metadata from the real producer, then replace only the encoded payload.
            // This seed must not be queried if preparation fails; callers propagate fatal assertions.
            AZ::SerializeContext::DataElementNode node;
            ASSERT_TRUE(node.Convert<BufferAsset>(*GetSerializeContext()));
            ASSERT_TRUE(node.SetData(*GetSerializeContext(), *asset.Get()));
            const int dataIndex = node.FindElement(AZ_CRC_CE("Buffer"));
            const int formatIndex = node.FindElement(AZ_CRC_CE("CompressionFormat"));
            ASSERT_GE(dataIndex, 0); ASSERT_GE(formatIndex, 0);
            const auto& formatNode = node.GetSubElement(formatIndex);
            const AZ::Uuid enumTypeId("{D6624140-8972-4356-9A88-56BA8DD24D0A}");
            ASSERT_EQ(formatNode.GetId(), enumTypeId);
            ASSERT_EQ(GetSerializeContext()->GetUnderlyingTypeId(enumTypeId), azrtti_typeid<uint8_t>());
            ASSERT_EQ(formatNode.GetRawDataElement().m_dataSize, sizeof(compression));
            // Real deserialization covers encoded size > decoded size without weakening Creator validation.
            ASSERT_TRUE(node.GetSubElement(dataIndex).SetData(*GetSerializeContext(), encoded));
            ASSERT_TRUE(node.GetData(*asset.Get()));
            ASSERT_EQ(asset->GetCompressionFormat(), compression);
        }

        JE::Render::RPI::BufferAsset::BufferData EncodedVertexFixture()
        {
            // meshopt v0, stride4: first byte values 0,2,4,6,6..., the other bytes zero.
            // One 2-bit group with three escaped deltas and three zero channels; 32-byte zero tail.
            JE::Render::RPI::BufferAsset::BufferData encoded{0xa0, 1, 0x3f, 0, 0, 0, 4, 4, 4, 0, 0, 0};
            encoded.resize(44, 0);
            return encoded;
        }

        AZStd::unique_ptr<JE::Render::RPI::BufferAssetHandler> m_bufferAssetHandler;
    };

    // Test that a buffer can be created and that the contents match 
    // what we expect.
    TEST_F(BufferTests, Creation)
    {
        using namespace AZ;

        ExpectedBuffer expectedBuffer;

        Data::Asset<JE::Render::RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);
        ValidateBufferAsset(bufferAsset.Get(), expectedBuffer);
    }

    // Test that a buffer can be created, serialized and de-serialized 
    // without losing any data.
    TEST_F(BufferTests, SerializeDeserialize)
    {
        using namespace AZ;

        ExpectedBuffer expectedBuffer;

        Data::Asset<JE::Render::RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);

        SerializeTester<JE::Render::RPI::BufferAsset> tester(GetSerializeContext());
        tester.SerializeOut(bufferAsset.Get());

        Data::Asset<JE::Render::RPI::BufferAsset> serializedBufferAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateBufferAsset(serializedBufferAsset.Get(), expectedBuffer);
    }

    // Test that if we try to set data on the BufferAssetCreator
    // before Begin has been called that it fails
    TEST_F(BufferTests, SetBufferNoBegin)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();
        const size_t bufferSize = validBuffer.m_bufferDescriptor.m_byteCount;

        JE::Render::RPI::BufferAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called", 2);

        creator.SetBuffer(validBuffer.m_data.data(), bufferSize, validBuffer.m_bufferDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set empty data on the BufferAssetCreator
    // that it works as intended
    TEST_F(BufferTests, SetEmptyBuffer)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer descriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        // Setting nullptr should succeed if the buffer descriptor is valid
        // as the BufferAsset should be usable for R/W buffers
        creator.SetBuffer(nullptr, 0, validBuffer.m_bufferDescriptor);

        ASSERT_EQ(0, creator.GetErrorCount());
        ASSERT_EQ(0, creator.GetWarningCount());
    }

    // Test that if we try to set an invalid BufferDescriptor
    // on the BufferAssetCreator that it fails
    TEST_F(BufferTests, SetInvalidBufferDescriptor)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer data
        ExpectedBuffer validBuffer = CreateValidBuffer();
        const size_t bufferSize = validBuffer.m_bufferDescriptor.m_byteCount;

        RHI::BufferDescriptor invalidBufferDescriptor;
        invalidBufferDescriptor.m_byteCount = 0;

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Size of the buffer in the descriptor was 0");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), bufferSize, invalidBufferDescriptor);
        

        // Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set initial buffer data that's more than 
    // we describe in the buffer descriptor that the creator fails as expected.
    TEST_F(BufferTests, SetBufferTooMuchInitialData)
    {
        using namespace AZ;

        //Use a valid buffer for the buffer data
        ExpectedBuffer validBuffer = CreateValidBuffer();

        const size_t invalidInitialDataSize = validBuffer.m_bufferDescriptor.m_byteCount + 1;

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("initialSize is larger than the total size in the descriptor.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), invalidInitialDataSize, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set an empty buffer with a non-zero
    // initial size that the creator fails as expected.
    TEST_F(BufferTests, SetEmptyBufferWithNonZeroSize)
    {
        using namespace AZ;

        //Use a valid buffer for the bufferDescriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Initial buffer data was not provided but the initial size was non-zero.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(nullptr, 1, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Test that if we try to set a buffer with an initial size of 0
    // that the creator fails as expected.
    TEST_F(BufferTests, SetBufferWithZeroSize)
    {
        using namespace AZ;

        //Use a valid buffer for the bufferDescriptor
        ExpectedBuffer validBuffer = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("Initial buffer data was not null but the initial size was zero.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBuffer(validBuffer.m_data.data(), 0, validBuffer.m_bufferDescriptor);

        // Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that if we try to set the view descriptor on the 
    // BufferAssetCreator before Begin has been called that it fails
    TEST_F(BufferTests, SetViewDescriptorNoBegin)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called", 2);
        
        creator.SetBufferViewDescriptor(validBuffer.m_viewDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    TEST_F(BufferTests, SetInvalidViewDescriptor)
    {
        using namespace AZ;

        ExpectedBuffer validBuffer = CreateValidBuffer();

        RHI::BufferViewDescriptor invalidViewDescriptor;

        JE::Render::RPI::BufferAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());

        ErrorMessageFinder messageFinder("BufferAssetCreator::SetBufferViewDescriptor was given a view descriptor with an element count of 0.");
        messageFinder.AddIgnoredErrorMessage("Cannot continue building BufferAsset", true);
        
        creator.SetBufferViewDescriptor(invalidViewDescriptor);

        //Ending the buffer here should also result in failure
        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Buffer Asset tests
    // Buffer asset without initial data
    TEST_F(BufferTests, BufferAssetCreation_NoInitialData_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(CreateTestBufferPoolAsset());
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);

        // empty initial data
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);

        Data::Asset<JE::Render::RPI::BufferAsset> asset;

        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);
    }

    // Neither pool asset specified nor common pool type specified
    TEST_F(BufferTests, BufferAssetCreation_NoPoolSpecified_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);

        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        
        // Failed
        ErrorMessageFinder messageFinder("BufferAssetCreator::ValidateBuffer Failed; need valid pool asset or select a valid common pool.", 1);
        ASSERT_FALSE(creator.End(asset));
        EXPECT_FALSE(asset.IsReady());
        EXPECT_EQ(asset.Get(), nullptr);
    }

    // Buffer system interface unit tests
    // Get buffer pools by type
    TEST_F(BufferTests, BufferSystem_GetCommonPools_Success)
    {
        using namespace AZ;

        for (uint32_t type = 0; type < static_cast<uint32_t>(JE::Render::RPI::CommonBufferPoolType::Count); type++)
        {
            ASSERT_NE(JE::Render::RPI::BufferSystemInterface::Get()->GetCommonBufferPool((JE::Render::RPI::CommonBufferPoolType)type).get(), nullptr);
        }
    }

    TEST_F(BufferTests, BufferSystem_CreateCommonBuffer_Success)
    {
        using namespace AZ;
        ExpectedBuffer bufferInfo = CreateValidBuffer();
        
        JE::Render::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_elementSize = bufferInfo.m_viewDescriptor.m_elementSize;
        desc.m_bufferData = bufferInfo.m_data.data();

        // with data
        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

        // buffer created
        ASSERT_NE(bufferInst.get(), nullptr);
        // buffer has same size as creation size
        EXPECT_EQ(bufferInst->GetBufferSize(), bufferInfo.m_bufferDescriptor.m_byteCount);
        // buffer has same buffer view descriptor as expected
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount, bufferInfo.m_viewDescriptor.m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, bufferInfo.m_viewDescriptor.m_elementSize);

        // without initial data
        JE::Render::RPI::CommonBufferDescriptor desc2;
        desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer2";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;

        Data::Instance<JE::Render::RPI::Buffer> bufferInst2 = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst2.get(), nullptr);
    }

    TEST_F(BufferTests, BufferSystem_FindCommonBuffer_Success_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_isUniqueName = true;

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        ASSERT_NE(bufferInst.get(), nullptr);

        // found
        Data::Instance<JE::Render::RPI::Buffer> bufferFound = JE::Render::RPI::BufferSystemInterface::Get()->FindCommonBuffer("Buffer1");
        EXPECT_EQ(bufferInst, bufferFound);

        // not found
        Data::Instance<JE::Render::RPI::Buffer> bufferFound2 = JE::Render::RPI::BufferSystemInterface::Get()->FindCommonBuffer("Buffer2");
        EXPECT_EQ(bufferFound2.get(), nullptr);
    }

    // Failed if creates a buffe which has a same name with existing buffer
    // and has m_isUniqueName is enabled
    TEST_F(BufferTests, BufferSystem_CreateDuplicatedNamedBufferEnableUniqueName_Fail)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;
        desc.m_isUniqueName = true;

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst.get(), nullptr);

        AZ_TEST_START_ASSERTTEST;
        Data::Instance<JE::Render::RPI::Buffer> bufferInst2 = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        AZ_TEST_STOP_ASSERTTEST(1);
        // buffer NOT created
        EXPECT_EQ(bufferInst2.get(), nullptr);
    }

    // create a buffer which has a same name with existing buffer 
    TEST_F(BufferTests, BufferSystem_CreateDuplicatedNamedBuffers_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadOnly;
        desc.m_bufferName = "Buffer1";
        desc.m_byteCount = bufferInfo.m_bufferDescriptor.m_byteCount;

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst.get(), nullptr);
                
        Data::Instance<JE::Render::RPI::Buffer> bufferInst2 = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        // buffer created
        EXPECT_NE(bufferInst2.get(), nullptr);
    }

    // Buffer instance creation unit tests
    TEST_F(BufferTests, Buffer_CreationUsingPoolAsset_Success)
    {
        using namespace AZ;
        
        ExpectedBuffer expectedBuffer;

        Data::Asset<JE::Render::RPI::BufferAsset> bufferAsset = BuildTestBuffer(expectedBuffer);

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::Buffer::FindOrCreate(bufferAsset);

        ASSERT_NE(bufferInst.get(), nullptr);
    }
    
    TEST_F(BufferTests, Buffer_CreationUsingCommonPool_Success)
    {
        using namespace AZ;

        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(JE::Render::RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        creator.End(asset);

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::Buffer::FindOrCreate(asset);

        ASSERT_NE(bufferInst.get(), nullptr);
    }

    // Resize buffer function: validate new size and buffer view descriptor
    TEST_F(BufferTests, Buffer_Resize_Success)
    {
        using namespace AZ;
        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(JE::Render::RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        creator.End(asset);

        uint64_t initialSize = bufferInfo.m_bufferDescriptor.m_byteCount;
        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::Buffer::FindOrCreate(asset);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), initialSize);

        // size up
        uint64_t newSize = 2 * initialSize;
        bufferInst->Resize(newSize);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), newSize);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount*bufferInst->GetBufferViewDescriptor().m_elementSize, newSize);

        // size down
        newSize = initialSize/2;
        bufferInst->Resize(newSize);
        ASSERT_NE(bufferInst.get(), nullptr);
        EXPECT_EQ(bufferInst->GetBufferSize(), newSize);
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementCount*bufferInst->GetBufferViewDescriptor().m_elementSize, newSize);
    }

    TEST_F(BufferTests, Buffer_SetAsStructured_Success)
    {
        using namespace AZ;
        
        ExpectedBuffer bufferInfo = CreateValidBuffer();

        JE::Render::RPI::BufferAssetCreator creator;
        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetBufferViewDescriptor(bufferInfo.m_viewDescriptor);
        creator.SetBuffer(nullptr, 0, bufferInfo.m_bufferDescriptor);
        creator.SetUseCommonPool(JE::Render::RPI::CommonBufferPoolType::ReadOnly);

        Data::Asset<JE::Render::RPI::BufferAsset> asset;
        creator.End(asset);

        Data::Instance<JE::Render::RPI::Buffer> bufferInst = JE::Render::RPI::Buffer::FindOrCreate(asset);
        ASSERT_NE(bufferInst.get(), nullptr);

        bufferInst->SetAsStructured<uint16_t>();
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, 2);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementCount, bufferInst->GetBufferViewDescriptor().m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementSize, bufferInst->GetBufferViewDescriptor().m_elementSize);

        bufferInst->SetAsStructured<uint64_t>();
        EXPECT_EQ(bufferInst->GetBufferViewDescriptor().m_elementSize, 8);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementCount, bufferInst->GetBufferViewDescriptor().m_elementCount);
        EXPECT_EQ(bufferInst->GetBufferView()->GetDescriptor().m_elementSize, bufferInst->GetBufferViewDescriptor().m_elementSize);
    }
    

    TEST_F(BufferTests, DecodedCpuPayloadDoesNotInferCompressionFromSmallerEqualOrLargerByteLengths)
    {
        using namespace JE::Render::RPI;
        const auto encoded = EncodedVertexFixture();
        for (uint32_t count : {10u, 11u, 12u})
        {
            AZ::Data::Asset<BufferAsset> asset;
            ASSERT_NO_FATAL_FAILURE(EncodedBufferFixture(count, 4, encoded, BufferAsset::CompressionFormat::Vertex, asset));
            BufferAsset::BufferData scratch;
            const auto decoded = asset->GetDecodedCpuBuffer(scratch, count * 4);
            ASSERT_TRUE(decoded) << count;
            ASSERT_EQ(decoded->size(), count * 4);
            for (uint32_t i = 0; i != count; ++i)
            {
                EXPECT_EQ((*decoded)[i * 4], (i < 3 ? i : 3) * 2);
                EXPECT_EQ((*decoded)[i * 4 + 1], 0); EXPECT_EQ((*decoded)[i * 4 + 2], 0); EXPECT_EQ((*decoded)[i * 4 + 3], 0);
            }
            EXPECT_EQ(asset->GetCompressionFormat(), BufferAsset::CompressionFormat::Vertex);
            EXPECT_EQ(asset->GetBuffer().size(), 44); // const decoding never rewrites cook bytes or their format.
            EXPECT_EQ(std::memcmp(asset->GetBuffer().data(), encoded.data(), encoded.size()), 0);
        }
    }

    TEST_F(BufferTests, HandlerDecodeSaveAndBothClonePathsKeepResidentBytesAndFormatCoherent)
    {
        using namespace JE::Render::RPI;
        for (uint32_t count : {10u, 11u, 12u})
        {
            const size_t decodedBytes = count * 4;
            AZ::Data::Asset<BufferAsset> asset;
            ASSERT_NO_FATAL_FAILURE(EncodedBufferFixture(count, 4, EncodedVertexFixture(), BufferAsset::CompressionFormat::Vertex, asset));
            auto* handler = AZ::Data::AssetManager::Instance().GetHandler(BufferAsset::RTTI_Type());
            ASSERT_TRUE(handler);
            for (bool decodedState : {false, true})
            {
                BufferAsset::BufferData expectedScratch;
                const auto expected = asset->GetDecodedCpuBuffer(expectedScratch, decodedBytes); ASSERT_TRUE(expected);
                auto serializedClone = AZStd::unique_ptr<BufferAsset>(GetSerializeContext()->CloneObject(asset.Get()));
                ASSERT_TRUE(serializedClone); EXPECT_EQ(serializedClone->GetCompressionFormat(), asset->GetCompressionFormat());
                BufferAsset::BufferData cloneScratch;
                const auto serializedDecoded = serializedClone->GetDecodedCpuBuffer(cloneScratch, decodedBytes);
                ASSERT_TRUE(serializedDecoded); EXPECT_EQ(std::memcmp(serializedDecoded->data(), expected->data(), decodedBytes), 0);
                // The producer/Creator contract excludes encoded bytes larger than the decoded descriptor.
                if (decodedState || count >= 11)
                {
                    AZ::Data::Asset<BufferAsset> creatorClone; AZ::Data::AssetId next(AZ::Uuid::CreateRandom());
                    ASSERT_TRUE(BufferAssetCreator::Clone(asset, creatorClone, next));
                    EXPECT_EQ(creatorClone->GetCompressionFormat(), asset->GetCompressionFormat());
                    BufferAsset::BufferData creatorScratch;
                    const auto creatorDecoded = creatorClone->GetDecodedCpuBuffer(creatorScratch, decodedBytes);
                    ASSERT_TRUE(creatorDecoded); EXPECT_EQ(std::memcmp(creatorDecoded->data(), expected->data(), decodedBytes), 0);
                }
                SerializeTester<BufferAsset> tester(GetSerializeContext());
                tester.SerializeOut(asset.Get(), AZ::DataStream::ST_BINARY);
                auto genericRoundtrip = tester.SerializeIn(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
                EXPECT_EQ(genericRoundtrip->GetCompressionFormat(), asset->GetCompressionFormat());
                BufferAsset::BufferData genericScratch;
                const auto genericDecoded = genericRoundtrip->GetDecodedCpuBuffer(genericScratch, decodedBytes);
                ASSERT_TRUE(genericDecoded); EXPECT_EQ(std::memcmp(genericDecoded->data(), expected->data(), decodedBytes), 0);
                AZStd::vector<AZ::u8> serialized;
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> output(&serialized);
                ASSERT_TRUE(handler->SaveAssetData(asset, &output));
                auto input = AZStd::make_shared<AZ::Data::AssetDataStream>(); input->Open(AZStd::move(serialized));
                ASSERT_EQ(handler->LoadAssetDataFromStream(asset, input, {}), AZ::Data::AssetHandler::LoadResult::LoadComplete);
                EXPECT_EQ(asset->GetCompressionFormat(), BufferAsset::CompressionFormat::Uncompressed);
                BufferAsset::BufferData untouched{0xab}; const auto capacity = untouched.capacity();
                const auto borrowed = asset->GetDecodedCpuBuffer(untouched, decodedBytes); ASSERT_TRUE(borrowed);
                EXPECT_EQ(borrowed->data(), asset->GetBuffer().data());
                EXPECT_EQ(untouched.capacity(), capacity); ASSERT_EQ(untouched.size(), 1); EXPECT_EQ(untouched[0], 0xab);
                EXPECT_EQ(asset->GetBuffer().size(), decodedBytes);
                // First pass loads encoded bytes; second pass roundtrips normalized bytes without decoding twice.
            }
        }
    }

    TEST_F(BufferTests, DecodeBudgetAndMalformedDescriptorsRejectBeforeAllocationAndNeverPublishPartialPayload)
    {
        using namespace JE::Render::RPI;
        AZ::Data::Asset<BufferAsset> asset;
        ASSERT_NO_FATAL_FAILURE(EncodedBufferFixture(11, 4, EncodedVertexFixture(), BufferAsset::CompressionFormat::Vertex, asset));
        BufferAsset::BufferData scratch{0xac}; const auto capacity = scratch.capacity();
        EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 43));
        EXPECT_EQ(scratch.capacity(), capacity); ASSERT_EQ(scratch.size(), 1); EXPECT_EQ(scratch[0], 0xac);
        auto& descriptor = const_cast<AZ::RHI::BufferDescriptor&>(asset->GetBufferDescriptor());
        ++descriptor.m_byteCount; EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 100)); --descriptor.m_byteCount;
        auto& view = const_cast<AZ::RHI::BufferViewDescriptor&>(asset->GetBufferViewDescriptor());
        const auto saved = view;
        view.m_elementSize = 0; EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 100));
        view.m_elementSize = 3; EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 100));
        view.m_elementSize = 512; EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 100));
        EXPECT_EQ(scratch.capacity(), capacity); view = saved;
        auto* bytes = const_cast<uint8_t*>(asset->GetBuffer().data()); const uint8_t header = bytes[0]; bytes[0] = 0;
        EXPECT_FALSE(asset->GetDecodedCpuBuffer(scratch, 44));
        EXPECT_EQ(asset->GetCompressionFormat(), BufferAsset::CompressionFormat::Vertex);
        EXPECT_EQ(asset->GetBuffer().size(), 44); EXPECT_EQ(asset->GetBuffer()[0], 0);
        bytes[0] = header; EXPECT_TRUE(asset->GetDecodedCpuBuffer(scratch, 44));
    }

    TEST_F(BufferTests, IndexDecodeUsesTriangleCountAndWidthConstraintsBeforeAllocating)
    {
        using namespace JE::Render::RPI;
        // meshoptimizer's published v0 decoder vector; expected triangle order is independent of the implementation.
        const BufferAsset::BufferData encoded{0xe0, 0xf0, 0x10, 0xfe, 0xff, 0xf0, 0x0c, 0xff, 0x02, 0x02, 0x02, 0x00,
            0x76, 0x87, 0x56, 0x67, 0x78, 0xa9, 0x86, 0x65, 0x89, 0x68, 0x98, 0x01, 0x69, 0x00, 0x00};
        const AZStd::array<uint32_t, 12> expected{{0, 1, 2, 2, 1, 3, 4, 6, 5, 7, 8, 9}};
        for (uint32_t stride : {2u, 4u})
        {
            AZ::Data::Asset<BufferAsset> asset;
            ASSERT_NO_FATAL_FAILURE(EncodedBufferFixture(12, stride, encoded, BufferAsset::CompressionFormat::Index, asset));
            BufferAsset::BufferData scratch; const auto decoded = asset->GetDecodedCpuBuffer(scratch, 48); ASSERT_TRUE(decoded);
            for (size_t i = 0; i != expected.size(); ++i)
            {
                uint32_t value = 0; std::memcpy(&value, decoded->data() + i * stride, stride); EXPECT_EQ(value, expected[i]);
            }
            auto& descriptor = const_cast<AZ::RHI::BufferDescriptor&>(asset->GetBufferDescriptor());
            descriptor.m_byteCount -= stride;
            BufferAsset::BufferData denied{0xae}; const auto capacity = denied.capacity();
            EXPECT_FALSE(asset->GetDecodedCpuBuffer(denied, 48)); EXPECT_EQ(denied.capacity(), capacity);
        }
    }
} // namespace UnitTest
