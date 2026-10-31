/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Buffer/BufferAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelKdTree.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAsset.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAssetCreator.h>
#include <JE/Render/RPI/Public/Model/UvStreamTangentBitmask.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/std/limits.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <cstring>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/iterator.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

namespace UnitTest
{
    AZ::Data::Asset<JE::Render::RPI::BufferAsset> BuildTestBuffer(const uint32_t elementCount, const uint32_t elementSize)
    {
        using namespace AZ;

        const uint32_t bufferSize = elementCount * elementSize;

        AZStd::vector<uint8_t> bufferData;
        bufferData.resize(bufferSize);

        //The actual data doesn't matter
        for (size_t i = 0; i < bufferData.size(); ++i)
        {
            bufferData[i] = static_cast<uint8_t>(i);
        }

        Data::Asset<JE::Render::RPI::ResourcePoolAsset> bufferPoolAsset;

        {
            auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
            bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;

            JE::Render::RPI::ResourcePoolAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());
            creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
            creator.SetPoolName("TestPool");
            EXPECT_TRUE(creator.End(bufferPoolAsset));
        }

        Data::Asset<JE::Render::RPI::BufferAsset> asset;

        {
            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            bufferDescriptor.m_byteCount = bufferSize;

            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize);

            JE::Render::RPI::BufferAssetCreator creator;

            creator.Begin(AZ::Uuid::CreateRandom());
            creator.SetPoolAsset(bufferPoolAsset);
            creator.SetBuffer(bufferData.data(), bufferDescriptor.m_byteCount, bufferDescriptor);
            creator.SetBufferViewDescriptor(bufferViewDescriptor);

            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);
        }

        return asset;
    }

    class ModelTests
        : public RPITestFixture
    {
    protected:

        struct ExpectedMesh
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            uint32_t m_indexCount = 0;
            uint32_t m_vertexCount = 0;
            JE::Render::RPI::ModelMaterialSlot::StableId m_materialSlotId = JE::Render::RPI::ModelMaterialSlot::InvalidStableId;
        };

        struct ExpectedLod
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::vector<ExpectedMesh> m_meshes;
        };

        struct ExpectedModel
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::vector<ExpectedLod> m_lods;
        };

        void SetUp() override
        {
            RPITestFixture::SetUp();

            auto assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            auto typeId = AZ::AzTypeInfo<JE::Render::RPI::MaterialAsset>::Uuid();
            m_materialAsset = AZ::Data::Asset<JE::Render::RPI::MaterialAsset>(assetId, typeId, "");

            // Some tests attempt to serialize-in the model asset, which should not attempt to actually load this dummy asset reference.
            m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehaviorNamespace::NoLoad);
        }

        AZ::RHI::ShaderSemantic GetPositionSemantic() const
        {
            return AZ::RHI::ShaderSemantic(AZ::Name("POSITION"));
        }

        bool CalculateAABB(const AZ::RHI::BufferViewDescriptor& bufferViewDesc, const JE::Render::RPI::BufferAsset& bufferAsset, AZ::Aabb& aabb)
        {
            const uint32_t elementSize = bufferViewDesc.m_elementSize;
            const uint32_t elementCount = bufferViewDesc.m_elementCount;

            // Position is 3 floats
            if (elementSize == sizeof(float) * 3)
            {
                const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&bufferAsset.GetBuffer()[0]);

                for (uint32_t i = 0; i < elementCount; ++i)
                {
                    const uint8_t* p = buffer + (i * 3);
                    aabb.AddPoint(AZ::Vector3(float(p[0]), float(p[1]), float(p[2])));
                }
            }
            else
            {
                // No idea what type of position stream this is
                return false;
            }

            return true;
        }

        //! This function assumes the model has "sharedMeshCount + separateMeshCount" unique material slots, with incremental IDs starting at 0.
        AZ::Data::Asset<JE::Render::RPI::ModelLodAsset> BuildTestLod(const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedLod& expectedLod)
        {
            using namespace AZ;

            //Create an Lod with a given number of meshes
            JE::Render::RPI::ModelLodAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

            const uint32_t indexCount = 36;
            const uint32_t vertexCount = 36;

            JE::Render::RPI::ModelMaterialSlot::StableId materialSlotId = 0;

            if(sharedMeshCount > 0)
            {
                const uint32_t sharedIndexCount = indexCount * sharedMeshCount;
                const uint32_t sharedVertexCount = vertexCount * sharedMeshCount;

                Data::Asset<JE::Render::RPI::BufferAsset> sharedIndexBuffer = BuildTestBuffer(sharedIndexCount, sizeof(uint32_t));
                Data::Asset<JE::Render::RPI::BufferAsset> sharedPositionBuffer = BuildTestBuffer(sharedVertexCount, sizeof(uint32_t));

                creator.SetLodIndexBuffer(sharedIndexBuffer);
                creator.AddLodStreamBuffer(sharedPositionBuffer);

                for (uint32_t i = 0; i < sharedMeshCount; ++i)
                {
                    ExpectedMesh expectedMesh;
                    expectedMesh.m_indexCount = indexCount;
                    expectedMesh.m_vertexCount = vertexCount;
                    expectedMesh.m_materialSlotId = i;

                    RHI::BufferViewDescriptor indexBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateStructured(i * indexCount, indexCount, sizeof(uint32_t));

                    RHI::BufferViewDescriptor vertexBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateStructured(i * vertexCount, vertexCount, sizeof(float) * 3);

                    if (!CalculateAABB(vertexBufferViewDescriptor, *sharedPositionBuffer.Get(), expectedMesh.m_aabb))
                    {
                        return {};
                    }

                    creator.BeginMesh();
                    Aabb aabb = expectedMesh.m_aabb;
                    creator.SetMeshAabb(AZStd::move(aabb));
                    creator.SetMeshMaterialSlot(materialSlotId++);
                    creator.SetMeshIndexBuffer({ sharedIndexBuffer, indexBufferViewDescriptor });
                    creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { sharedPositionBuffer, vertexBufferViewDescriptor });
                    creator.EndMesh();

                    expectedLod.m_aabb.AddAabb(expectedMesh.m_aabb);
                    expectedLod.m_meshes.emplace_back(AZStd::move(expectedMesh));
                }
            }

            for (uint32_t i = 0; i < separateMeshCount; ++i)
            {
                ExpectedMesh expectedMesh;
                expectedMesh.m_indexCount = indexCount;
                expectedMesh.m_vertexCount = vertexCount;
                expectedMesh.m_materialSlotId = sharedMeshCount + i;

                RHI::BufferViewDescriptor indexBufferViewDescriptor =
                    RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

                RHI::BufferViewDescriptor positionBufferViewDescriptor =
                    RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

                Data::Asset<JE::Render::RPI::BufferAsset> positonBuffer = BuildTestBuffer(vertexCount, sizeof(float) * 3);

                if (!CalculateAABB(positionBufferViewDescriptor, *positonBuffer.Get(), expectedMesh.m_aabb))
                {
                    return {};
                }

                creator.BeginMesh();
                Aabb aabb = expectedMesh.m_aabb;
                creator.SetMeshAabb(AZStd::move(aabb));
                creator.SetMeshMaterialSlot(materialSlotId++);
                creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
                creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { positonBuffer, positionBufferViewDescriptor });

                creator.EndMesh();

                expectedLod.m_aabb.AddAabb(expectedMesh.m_aabb);
                expectedLod.m_meshes.emplace_back(AZStd::move(expectedMesh));
            }

            Data::Asset<JE::Render::RPI::ModelLodAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        AZ::Data::Asset<JE::Render::RPI::ModelAsset> BuildTestModel(
            const uint32_t lodCount, const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedModel& expectedModel)
        {
            using namespace AZ;

            JE::Render::RPI::ModelAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetName("TestModel");

            for (JE::Render::RPI::ModelMaterialSlot::StableId materialSlotId = 0; materialSlotId < sharedMeshCount + separateMeshCount; ++materialSlotId)
            {
                JE::Render::RPI::ModelMaterialSlot slot;
                slot.m_defaultMaterialAsset = m_materialAsset;
                slot.m_displayName = AZStd::string::format("Slot%d", materialSlotId);
                slot.m_stableId = materialSlotId;
                creator.AddMaterialSlot(slot);
            }

            for (uint32_t i = 0; i < lodCount; ++i)
            {
                ExpectedLod expectedLod;

                creator.AddLodAsset(BuildTestLod(sharedMeshCount, separateMeshCount, expectedLod));

                expectedModel.m_aabb.AddAabb(expectedLod.m_aabb);
                expectedModel.m_lods.emplace_back(AZStd::move(expectedLod));
            }

            Data::Asset<JE::Render::RPI::ModelAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        void ValidateMesh(const JE::Render::RPI::ModelLodAsset::Mesh& mesh, const ExpectedMesh& expectedMesh)
        {
            EXPECT_TRUE(mesh.GetAabb() == expectedMesh.m_aabb);
            EXPECT_TRUE(mesh.GetIndexCount() == expectedMesh.m_indexCount);
            EXPECT_TRUE(mesh.GetVertexCount() == expectedMesh.m_vertexCount);
            EXPECT_TRUE(mesh.GetMaterialSlotId() == expectedMesh.m_materialSlotId);
        }

        void ValidateLodAsset(const JE::Render::RPI::ModelLodAsset* lodAsset, const ExpectedLod& expectedLod)
        {
            ASSERT_NE(lodAsset, nullptr);
            EXPECT_TRUE(lodAsset->GetAabb().IsValid());
            EXPECT_TRUE(lodAsset->GetMeshes().size() == expectedLod.m_meshes.size());
            EXPECT_TRUE(lodAsset->GetAabb() == expectedLod.m_aabb);

            for (size_t i = 0; i < lodAsset->GetMeshes().size(); ++i)
            {
                const auto meshes = lodAsset->GetMeshes();
                const JE::Render::RPI::ModelLodAsset::Mesh& mesh = meshes[i];
                const ExpectedMesh& expectedMesh = expectedLod.m_meshes[i];

                ValidateMesh(mesh, expectedMesh);
            }
        }

        void ValidateModelAsset(const JE::Render::RPI::ModelAsset* modelAsset, const ExpectedModel& expectedModel)
        {
            ASSERT_NE(modelAsset, nullptr);
            EXPECT_TRUE(modelAsset->GetAabb().IsValid());
            EXPECT_TRUE(modelAsset->GetLodAssets().size() == expectedModel.m_lods.size());
            EXPECT_TRUE(modelAsset->GetAabb() == expectedModel.m_aabb);

            for (size_t i = 0; i < modelAsset->GetLodAssets().size(); ++i)
            {
                const JE::Render::RPI::ModelLodAsset* lodAsset = modelAsset->GetLodAssets()[i].Get();
                const ExpectedLod& expectedLod = expectedModel.m_lods[i];

                ValidateLodAsset(lodAsset, expectedLod);
            }
        }

        const uint32_t m_manyMesh = 100; // Not too much to hold up the tests but enough to stress them
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_materialAsset;

    };

    TEST_F(ModelTests, SerializeModelOneLodOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = 1;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodOneSharedMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 0;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = 1;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 0;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodManySeparateMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodManySharedMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 0;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySeparateMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 0;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodOneSharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 1;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 1;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 1;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMeshManySeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshManySeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = JE::Render::RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<JE::Render::RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<JE::Render::RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<JE::Render::RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    // Tests that if we try to set the name on a Model
    // before calling Begin that it will fail.
    TEST_F(ModelTests, SetNameNoBegin)
    {
        using namespace AZ;

        JE::Render::RPI::ModelAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called");

        creator.SetName("TestName");
    }

    // Tests that if we try to add a ModelLod to a Model
    // before calling Begin that it will fail.
    TEST_F(ModelTests, AddLodNoBegin)
    {
        using namespace AZ;

        JE::Render::RPI::ModelAssetCreator creator;

        //Build a valid lod
        ExpectedLod expectedLod;
        Data::Asset<JE::Render::RPI::ModelLodAsset> lod = BuildTestLod(0, 1, expectedLod);

        ErrorMessageFinder messageFinder("Begin() was not called");

        creator.AddLodAsset(AZStd::move(lod));
    }

    // Tests that if we create a ModelAsset without adding
    // any ModelLodAssets that the creator will properly fail to produce an asset.
    TEST_F(ModelTests, CreateModelNoLods)
    {
        using namespace AZ;

        JE::Render::RPI::ModelAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        ErrorMessageFinder messageFinder("No valid ModelLodAssets have been added to this ModelAsset.");

        // Since there are no LODs set on this model it
        // we should not be able to successfully end the creator
        Data::Asset<JE::Render::RPI::ModelAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that if we call SetLodIndexBuffer without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, SetLodIndexBufferNoBegin)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::BufferAsset> validIndexBuffer = BuildTestBuffer(10, sizeof(uint32_t));

        ErrorMessageFinder messageFinder("Begin() was not called");

        JE::Render::RPI::ModelLodAssetCreator creator;
        creator.SetLodIndexBuffer(validIndexBuffer);
    }

    // Tests that if we call AddLodStreamBuffer without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, AddLodStreamBufferNoBegin)
    {
        using namespace AZ;

        Data::Asset<JE::Render::RPI::BufferAsset> validStreamBuffer = BuildTestBuffer(10, sizeof(float) * 3);

        ErrorMessageFinder messageFinder("Begin() was not called");

        JE::Render::RPI::ModelLodAssetCreator creator;
        creator.AddLodStreamBuffer(validStreamBuffer);
    }

    // Tests that if we call BeginMesh without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, BeginMeshNoBegin)
    {
        using namespace AZ;

        ErrorMessageFinder messageFinder("Begin() was not called");

        JE::Render::RPI::ModelLodAssetCreator creator;
        creator.BeginMesh();
    }

    // Tests that if we try to set an AABB on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetAabbNoBeginNoBeginMesh)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1.0f);
        ASSERT_TRUE(aabb.IsValid());

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            AZ::Aabb testAabb = aabb;
            creator.SetMeshAabb(AZStd::move(testAabb));
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            AZ::Aabb testAabb = aabb;
            creator.SetMeshAabb(AZStd::move(testAabb));
        }
    }

    // Tests that if we try to set the material slot on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetMaterialSlotNoBeginNoBeginMesh)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.SetMeshMaterialSlot(0);
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.SetMeshMaterialSlot(0);
        }
    }

    // Tests that if we try to set the index buffer on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetIndexBufferNoBeginNoBeginMesh)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        const uint32_t indexCount = 36;
        const uint32_t indexSize = sizeof(uint32_t);

        RHI::BufferViewDescriptor validIndexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, indexSize);

        Data::Asset<JE::Render::RPI::BufferAsset> validIndexBuffer = BuildTestBuffer(indexCount, indexSize);
        ASSERT_TRUE(validIndexBuffer.Get() != nullptr);

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.SetMeshIndexBuffer({ AZStd::move(validIndexBuffer), validIndexBufferViewDescriptor });
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        validIndexBuffer = BuildTestBuffer(indexCount, indexSize);
        ASSERT_TRUE(validIndexBuffer.Get() != nullptr);

        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.SetMeshIndexBuffer({ AZStd::move(validIndexBuffer), validIndexBufferViewDescriptor });
        }
    }

    // Tests that if we try to add a stream buffer on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, AddStreamBufferNoBeginNoBeginMesh)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        const uint32_t vertexCount = 36;
        const uint32_t vertexSize = sizeof(float) * 3;

        RHI::BufferViewDescriptor validStreamBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, vertexSize);

        Data::Asset<JE::Render::RPI::BufferAsset> validStreamBuffer = BuildTestBuffer(vertexCount, vertexSize);

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { AZStd::move(validStreamBuffer), validStreamBufferViewDescriptor });
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        validStreamBuffer = BuildTestBuffer(vertexCount, vertexSize);

        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { AZStd::move(validStreamBuffer), validStreamBufferViewDescriptor });
        }
    }

    // Tests that if we try to end the creation of a
    // ModelLodAsset that has no meshes that it fails
    // as expected.
    TEST_F(ModelTests, CreateLodNoMeshes)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        ErrorMessageFinder messageFinder("No meshes have been provided for this LOD");

        Data::Asset<JE::Render::RPI::ModelLodAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that validation still fails when expected
    // even after producing a valid mesh due to a missing
    // BeginMesh call
    TEST_F(ModelTests, SecondMeshFailureNoBeginMesh)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        uint32_t indexCount = 36;
        uint32_t vertexCount = 36;

        RHI::BufferViewDescriptor indexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

        RHI::BufferViewDescriptor vertexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

        //Creating this first mesh should work as expected
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // This second mesh should fail at every point since we have forgotten to
        // call BeginMesh again
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            ErrorMessageFinder messageFinder("BeginMesh() was not called", 5);

            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // We should still be able to produce a valid asset however
        Data::Asset<JE::Render::RPI::ModelLodAsset> asset;
        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);

        // Make sure that this lod only has one mesh like we expect
        ASSERT_EQ(asset->GetMeshes().size(), 1);
    }

    // Tests that validation still fails when expected
    // even after producing a valid mesh due to SetMeshX
    // calls coming after End
    TEST_F(ModelTests, SecondMeshAfterEnd)
    {
        using namespace AZ;

        JE::Render::RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        uint32_t indexCount = 36;
        uint32_t vertexCount = 36;

        RHI::BufferViewDescriptor indexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

        RHI::BufferViewDescriptor vertexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

        //Creating this first mesh should work as expected
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // This asset creation should be valid
        Data::Asset<JE::Render::RPI::ModelLodAsset> asset;
        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);

        // This second mesh should fail at every point since we have already
        // called End
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            ErrorMessageFinder messageFinder("Begin() was not called", 6);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }
    }

    TEST_F(ModelTests, UvStream)
    {
        JE::Render::RPI::UvStreamTangentBitmask uvStreamTangentBitmask;
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0u);

        uvStreamTangentBitmask.ApplyTangent(1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x10000001);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 1u);

        uvStreamTangentBitmask.ApplyTangent(5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(1u), 5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x20000051);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 2u);

        uvStreamTangentBitmask.ApplyTangent(100u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(1u), 5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(2u), JE::Render::RPI::UvStreamTangentBitmask::UnassignedTangent);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x30000F51);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 3u);

        for (uint32_t i = 3; i < JE::Render::RPI::UvStreamTangentBitmask::MaxUvSlots; ++i)
        {
            uvStreamTangentBitmask.ApplyTangent(0u);
        }

        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x70000F51);

        AZ_TEST_START_TRACE_SUPPRESSION;
        uvStreamTangentBitmask.ApplyTangent(0u);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x70000F51);
    }

    /*
         +----+
        /    /|
       +----+ |
       |    | +
       |    |/
       +----+
    */
    static constexpr AZStd::array CubePositions = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,
                                                    -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f };
    static constexpr AZStd::array CubeIndices = AZStd::to_array<uint32_t>({
        0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 4, 2, 4, 6, 2, 1, 3, 5, 5, 3, 7, 0, 1, 4, 4, 1, 5, 2, 6, 3, 6, 7, 3,
    });

    static constexpr AZStd::array QuadPositions = { -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
    static constexpr AZStd::array QuadIndices = AZStd::to_array<uint32_t>({ 0, 2, 1, 1, 2, 3 });

    /*
       This class creates a Model with one LOD, whose mesh contains 2 planes. Plane 1 is in the XY plane at Z=-0.5, and
       plane 2 is in the XY plane at Z=0.5. The two planes each have 9 quads which have been triangulated. It only has
       a position and index buffer.

            -0.33
          -1     0.33  1
       0.5 *---*---*---*
            \ / \ / \ / \
             *---*---*---*
              \ / \ / \ / \
       -0.5 *- *---*---*---*
             \  \ / \ / \ / \
              *- *---*---*---*
               \   \   \   \
                *---*---*---*
                 \ / \ / \ / \
                  *---*---*---*
    */
    static constexpr AZStd::array TwoSeparatedPlanesPositions{
        -1.0f,   -0.333f, -0.5f, -0.333f, -1.0f,   -0.5f, -0.333f, -0.333f, -0.5f, 0.333f, -0.333f, -0.5f, 1.0f,    -1.0f,   -0.5f,
        1.0f,    -0.333f, -0.5f, 0.333f,  -1.0f,   -0.5f, 0.333f,  1.0f,    -0.5f, 1.0f,   0.333f,  -0.5f, 1.0f,    1.0f,    -0.5f,
        0.333f,  0.333f,  -0.5f, -0.333f, 1.0f,    -0.5f, -0.333f, 0.333f,  -0.5f, -1.0f,  1.0f,    -0.5f, -1.0f,   0.333f,  -0.5f,
        -1.0f,   -0.333f, 0.5f,  -0.333f, -1.0f,   0.5f,  -0.333f, -0.333f, 0.5f,  0.333f, -0.333f, 0.5f,  1.0f,    -1.0f,   0.5f,
        1.0f,    -0.333f, 0.5f,  -0.333f, -0.333f, 0.5f,  0.333f,  -1.0f,   0.5f,  0.333f, -0.333f, 0.5f,  0.333f,  1.0f,    0.5f,
        1.0f,    0.333f,  0.5f,  1.0f,    1.0f,    0.5f,  0.333f,  0.333f,  0.5f,  1.0f,   -0.333f, 0.5f,  -0.333f, 1.0f,    0.5f,
        -0.333f, 0.333f,  0.5f,  0.333f,  -0.333f, 0.5f,  0.333f,  0.333f,  0.5f,  -1.0f,  1.0f,    0.5f,  -0.333f, 0.333f,  0.5f,
        -1.0f,   0.333f,  0.5f,  -1.0f,   -1.0f,   -0.5f, -1.0f,   -1.0f,   0.5f,  0.333f, -0.333f, 0.5f,  0.333f,  -1.0f,   0.5f,
        1.0f,    -1.0f,   0.5f,  0.333f,  -1.0f,   0.5f,  0.333f,  0.333f,  0.5f,  0.333f, -0.333f, 0.5f,  1.0f,    -0.333f, 0.5f,
        -0.333f, 0.333f,  0.5f,  -0.333f, -0.333f, 0.5f,  0.333f,  -0.333f, 0.5f,
    };
    // clang-format off
    static constexpr AZStd::array TwoSeparatedPlanesIndices = AZStd::to_array<uint32_t>({
        0,  1,  2,  3,  4,  5,  2,  6,  3,  7,  8,  9,  10, 5,  8,  11, 10, 7,  12, 3,  10, 13, 12, 11, 14, 2,  12,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 25, 29, 27, 24, 30, 31, 32, 33, 34, 29, 35, 17, 34,
        0,  36, 1,  3,  6,  4,  2,  1,  6,  7,  10, 8,  10, 3,  5,  11, 12, 10, 12, 2,  3,  13, 14, 12, 14, 0,  2,
        15, 37, 16, 38, 39, 40, 17, 16, 41, 24, 27, 25, 42, 43, 44, 29, 34, 27, 45, 46, 47, 33, 35, 34, 35, 15, 17,
    });
    // clang-format on

    // Ensure that the index buffer references all the positions in the position buffer
    static constexpr inline auto minmaxElement = AZStd::minmax_element(begin(TwoSeparatedPlanesIndices), end(TwoSeparatedPlanesIndices));
    static_assert(*minmaxElement.second == (TwoSeparatedPlanesPositions.size() / 3) - 1);

    class TestMesh
    {
    public:
        TestMesh() = default;

        TestMesh(const float* positions, size_t positionCount, const uint32_t* indices, size_t indicesCount)
        {
            JE::Render::RPI::ModelLodAssetCreator lodCreator;
            Begin(lodCreator);
            Add(lodCreator, positions, positionCount, /*positionOffset=*/0, indices, indicesCount, /*indexOffset=*/0);
            End(lodCreator);
        }

        // initiate the asset lod creation process (note: End must be called after meshes have been added).
        void Begin(JE::Render::RPI::ModelLodAssetCreator& lodCreator)
        {
            lodCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        }

        // add a sub mesh and reuse existing position/index buffer (be very careful with the offsets used)
        void Add(
            JE::Render::RPI::ModelLodAssetCreator& lodCreator,
            const float* positions,
            size_t positionCount,
            size_t positionOffset,
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> positionBuffer,
            const uint32_t* indices,
            size_t indexCount,
            size_t indexOffset,
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> indexBuffer,
            AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId> materialSlotId = AZStd::nullopt)
        {
            lodCreator.BeginMesh();
            lodCreator.SetMeshAabb(AZ::Aabb::CreateFromMinMax({ -1.0f, -1.0f, -0.5f }, { 1.0f, 1.0f, 0.5f }));
            lodCreator.SetMeshMaterialSlot(materialSlotId ? *materialSlotId : AZ::Sfmt::GetInstance().Rand32());

            AZStd::copy(
                indices, indices + indexCount,
                reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(indexBuffer->GetBuffer().data())) + indexOffset);
            lodCreator.SetMeshIndexBuffer(
                { indexBuffer,
                  AZ::RHI::BufferViewDescriptor::CreateStructured(
                      aznumeric_cast<uint32_t>(indexOffset), aznumeric_cast<uint32_t>(indexCount), sizeof(uint32_t)) });
            AZStd::copy(
                positions, positions + positionCount,
                reinterpret_cast<float*>(const_cast<uint8_t*>(positionBuffer->GetBuffer().data())) + positionOffset);
            lodCreator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                { positionBuffer,
                  AZ::RHI::BufferViewDescriptor::CreateStructured(
                      aznumeric_cast<uint32_t>(positionOffset / 3), aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3) });

            lodCreator.EndMesh();
        }

        // overload of Add - here a new index/position buffer is created for the new data instead of potentially reusing an existing buffer
        void Add(
            JE::Render::RPI::ModelLodAssetCreator& lodCreator,
            const float* positions,
            size_t positionCount,
            size_t positionOffset,
            const uint32_t* indices,
            size_t indexCount,
            size_t indexOffset,
            AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId> materialSlotId = AZStd::nullopt)
        {
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> indexBuffer = BuildTestBuffer(aznumeric_cast<uint32_t>(indexCount), sizeof(uint32_t));
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> positionBuffer =
                BuildTestBuffer(aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3);

            Add(lodCreator, positions, positionCount, positionOffset, positionBuffer, indices, indexCount, indexOffset, indexBuffer,
                materialSlotId);
        }

        // complete the asset lod creation process
        void End(JE::Render::RPI::ModelLodAssetCreator& lodCreator)
        {
            AZ::Data::Asset<JE::Render::RPI::ModelLodAsset> lodAsset;
            lodCreator.End(lodAsset);

            JE::Render::RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            modelCreator.SetName("TestModel");
            modelCreator.AddLodAsset(AZStd::move(lodAsset));
            modelCreator.End(m_modelAsset);
        }

        [[nodiscard]] AZ::Data::Asset<JE::Render::RPI::ModelAsset> GetModel() const
        {
            return m_modelAsset;
        }

    private:
        AZ::Data::Asset<JE::Render::RPI::ModelAsset> m_modelAsset;
    };

    struct IntersectParams
    {
        float xpos;
        float ypos;
        float zpos;
        float xdir;
        float ydir;
        float zdir;
        float expectedDistance;
        bool expectedShouldIntersect;

        friend std::ostream& operator<<(std::ostream& os, const IntersectParams& param)
        {
            return os
                << "xpos:" << param.xpos
                << ", ypos:" << param.ypos
                << ", zpos:" << param.zpos
                << ", dist:" << param.expectedDistance
                << ", shouldIntersect:" << param.expectedShouldIntersect;
        }
    };

    class KdTreeIntersectsParameterizedFixture
        : public ModelTests
        , public ::testing::WithParamInterface<IntersectParams>
    {
    };

    TEST_P(KdTreeIntersectsParameterizedFixture, KdTreeIntersects)
    {
        TestMesh mesh(
            TwoSeparatedPlanesPositions.data(), TwoSeparatedPlanesPositions.size(), TwoSeparatedPlanesIndices.data(),
            TwoSeparatedPlanesIndices.size());

        JE::Render::RPI::ModelKdTree kdTree;
        ASSERT_TRUE(kdTree.Build(mesh.GetModel().Get()));

        float distance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;

        EXPECT_THAT(
            kdTree.RayIntersection(
                AZ::Vector3(GetParam().xpos, GetParam().ypos, GetParam().zpos),
                AZ::Vector3(GetParam().xdir, GetParam().ydir, GetParam().zdir), distance, normal),
            testing::Eq(GetParam().expectedShouldIntersect));
        EXPECT_THAT(distance, testing::FloatEq(GetParam().expectedDistance));
    }

    static constexpr AZStd::array KdTreeIntersectTestData{
        IntersectParams{ -0.1f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.1f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },

        // Test the center of each triangle
        IntersectParams{ -0.111f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.111f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.111f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f,
                         true }, // Should intersect triangle with indices {29, 34, 27} and {11, 12, 10}
        IntersectParams{ -0.555f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.555f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.555f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
    };

    INSTANTIATE_TEST_SUITE_P(KdTreeIntersectsPlane, KdTreeIntersectsParameterizedFixture, ::testing::ValuesIn(KdTreeIntersectTestData));

    class KdTreeIntersectsFixture
        : public ModelTests
    {
    public:
        void SetUp() override
        {
            ModelTests::SetUp();

            m_mesh = AZStd::make_unique<TestMesh>(
                TwoSeparatedPlanesPositions.data(), TwoSeparatedPlanesPositions.size(), TwoSeparatedPlanesIndices.data(),
                TwoSeparatedPlanesIndices.size());

            m_kdTree = AZStd::make_unique<JE::Render::RPI::ModelKdTree>();
            ASSERT_TRUE(m_kdTree->Build(m_mesh->GetModel().Get()));
        }

        void TearDown() override
        {
            m_kdTree.reset();
            m_mesh.reset();

            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
        AZStd::unique_ptr<JE::Render::RPI::ModelKdTree> m_kdTree;
    };

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionReturnsNormalizedDistance)
    {
        float t = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;

        constexpr float rayLength = 100.0f;
        EXPECT_THAT(
            m_kdTree->RayIntersection(
                AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(-rayLength), t, normal), testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.005f));
    }

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionHandlesInvalidStartingNormalizedDistance)
    {
        float t = -0.5f; // invalid starting distance
        AZ::Vector3 normal;

        constexpr float rayLength = 10.0f;
        EXPECT_THAT(
            m_kdTree->RayIntersection(AZ::Vector3::CreateAxisZ(0.75f), AZ::Vector3::CreateAxisZ(-rayLength), t, normal), testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.025f));
    }

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionDoesNotScaleRayByStartingDistance)
    {
        float t = 10.0f; // starting distance (used to check it is not read from initially by RayIntersection)
        AZ::Vector3 normal;

        EXPECT_THAT(
            m_kdTree->RayIntersection(AZ::Vector3::CreateAxisZ(5.0f), -AZ::Vector3::CreateAxisZ(), t, normal), testing::Eq(false));
    }

    class BruteForceIntersectsParameterizedFixture
        : public ModelTests
        , public ::testing::WithParamInterface<IntersectParams>
    {
    };

    TEST_P(BruteForceIntersectsParameterizedFixture, BruteForceIntersectsCube)
    {
        TestMesh mesh(CubePositions.data(), CubePositions.size(), CubeIndices.data(), CubeIndices.size());

        float distance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;
        constexpr bool AllowBruteForce = false;

        EXPECT_THAT(
            mesh.GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(GetParam().xpos, GetParam().ypos, GetParam().zpos),
                AZ::Vector3(GetParam().xdir, GetParam().ydir, GetParam().zdir), AllowBruteForce, distance, normal),
            testing::Eq(GetParam().expectedShouldIntersect));
        EXPECT_THAT(distance, testing::FloatEq(GetParam().expectedDistance));
    }

    static constexpr AZStd::array BruteForceIntersectTestData{
        IntersectParams{ 5.0f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, AZStd::numeric_limits<float>::max(), false },
        IntersectParams{ 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 5.0f, 0.0f, 0.0f, -10.0f, 0.0f, 0.0f, 0.4f, true },
        IntersectParams{ -5.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f, 0.2f, true },
        IntersectParams{ 0.0f, -10.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.45f, true },
        IntersectParams{ 0.0f,  20.0f, 0.0f, 0.0f, -40.0f, 0.0f, 0.475f, true },
        IntersectParams{ 0.0f,  20.0f, 0.0f, 0.0f, -19.0f, 0.0f, 1.0f, true },
    };

    INSTANTIATE_TEST_SUITE_P(
        BruteForceIntersects, BruteForceIntersectsParameterizedFixture, ::testing::ValuesIn(BruteForceIntersectTestData));

    class BruteForceModelIntersectsFixture
        : public ModelTests
    {
    public:
        void SetUp() override
        {
            ModelTests::SetUp();
            m_mesh = AZStd::make_unique<TestMesh>(CubePositions.data(), CubePositions.size(), CubeIndices.data(), CubeIndices.size());
        }

        void TearDown() override
        {
            m_mesh.reset();
            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
    };

    TEST_F(BruteForceModelIntersectsFixture, BruteForceIntersectionDetectedWithCube)
    {
        float t = 0.0f;
        AZ::Vector3 normal;

        // firing down the negative z axis, positioned 5 units from cube (cube is 2x2x2 so intersection
        // happens at 1 in z)
        constexpr bool AllowBruteForce = false;
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3::CreateAxisZ(5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.4f));
    }

    TEST_F(BruteForceModelIntersectsFixture, BruteForceIntersectionDetectedAndNormalSetAtEndOfRay)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal

        // ensure the intersection happens right at the end of the ray
        constexpr bool AllowBruteForce = false;
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(9.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(1.0f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisY()));
    }

    // test to verify that each secondary sub meshes are still intersected with correctly when using brute-force
    // ray intersection
    class BruteForceMultiModelIntersectsFixture : public ModelTests
    {
    public:
        inline static const float QuadOffsetX = 15.0f;

        void SetUp() override
        {
            ModelTests::SetUp();
            m_mesh = AZStd::make_unique<TestMesh>();

            JE::Render::RPI::ModelLodAssetCreator lodCreator;
            m_mesh->Begin(lodCreator);

            // take default quad positions and offset in X by set amount
            AZStd::vector<float> offsetQuadPositions;
            offsetQuadPositions.resize(QuadPositions.size());
            AZStd::copy(QuadPositions.begin(), QuadPositions.end(), offsetQuadPositions.begin());
            for (size_t xVertIndex = 0; xVertIndex < offsetQuadPositions.size(); xVertIndex += 3)
            {
                offsetQuadPositions[xVertIndex] += QuadOffsetX;
            }

            // create shared buffer to store cube and quad mesh in the same buffer
            const size_t indicesCount = QuadIndices.size() + CubeIndices.size();
            const size_t positionCount = QuadPositions.size() + CubePositions.size();
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> indexBuffer = BuildTestBuffer(aznumeric_cast<uint32_t>(indicesCount), sizeof(uint32_t));
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> positionBuffer =
                BuildTestBuffer(aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3);

            // add the cube mesh
            m_mesh->Add(
                lodCreator, CubePositions.data(), CubePositions.size(), 0, positionBuffer, CubeIndices.data(), CubeIndices.size(), 0,
                indexBuffer);
            // add the quad mesh (offset by the cube position and index data into the same buffer)
            m_mesh->Add(
                lodCreator, offsetQuadPositions.data(), offsetQuadPositions.size(), /*offset=*/CubePositions.size(), positionBuffer,
                QuadIndices.data(), QuadIndices.size(), /*offset=*/CubeIndices.size(), indexBuffer);

            m_mesh->End(lodCreator);
        }

        void TearDown() override
        {
            m_mesh.reset();
            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
        inline static constexpr bool AllowBruteForce = false;
    };

    TEST_F(BruteForceMultiModelIntersectsFixture, RayIntersectsWithFirstSubMesh)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal
        // fire a ray at the first sub mesh and ensure a successful hit is returned
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(0.0f, 0.0f, 5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.4f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(BruteForceMultiModelIntersectsFixture, RayIntersectsWithSecondSubMesh)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal
        // fire a ray at the second sub mesh and ensure a successful hit is returned
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(QuadOffsetX, 0.0f, 5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.5f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TestMesh CreateMaterialSlotRayMesh(bool reverseOrder, JE::Render::RPI::ModelMaterialSlot::StableId nearSlotId,
        float halfExtent = 0.1f, float planeOffset = 0.4f)
    {
        TestMesh mesh;
        JE::Render::RPI::ModelLodAssetCreator creator;
        mesh.Begin(creator);
        // 32 triangles: ModelAsset uses brute force, while the KD tree splits the separated planes.
        AZStd::array<uint32_t, 48> indices;
        for (size_t i = 0; i < indices.size(); ++i) { indices[i] = QuadIndices[i % QuadIndices.size()]; }
        for (bool nearPlane : {reverseOrder, !reverseOrder})
        {
            auto positions = QuadPositions;
            for (size_t i = 0; i < positions.size(); i += 3)
            {
                positions[i] *= halfExtent;
                positions[i + 1] *= halfExtent;
                positions[i + 2] = nearPlane ? planeOffset : -planeOffset;
            }
            mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0,
                nearPlane ? nearSlotId : 22u);
        }
        mesh.End(creator);
        return mesh;
    }

    TEST_F(ModelTests, RayIntersectionReturnsNearestMaterialSlotInEitherMeshOrder)
    {
        for (bool reverseOrder : {false, true})
        {
            auto mesh = CreateMaterialSlotRayMesh(reverseOrder, 11u);
            JE::Render::RPI::ModelKdTree tree;
            ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
            for (bool useKdTree : {false, true})
            {
                SCOPED_TRACE(::testing::Message() << "reverse=" << reverseOrder << " kd=" << useKdTree);
                const auto query = [&](const AZ::Vector3& start, const AZ::Vector3& direction, float& t, AZ::Vector3& normal,
                                       AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId>& slot)
                {
                    return useKdTree ? tree.RayIntersection(start, direction, t, normal, slot)
                                     : mesh.GetModel()->LocalRayIntersectionAgainstModel(start, direction, false, t, normal, slot);
                };
                float t = -1.0f;
                AZ::Vector3 normal = AZ::Vector3::CreateOne();
                AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId> slot = 99u;
                const auto start = AZ::Vector3::CreateAxisZ();
                const auto direction = -AZ::Vector3::CreateAxisZ(2.0f);
                ASSERT_TRUE(query(start, direction, t, normal, slot));
                EXPECT_FLOAT_EQ(t, 0.3f);
                EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
                ASSERT_TRUE(slot.has_value());
                EXPECT_EQ(*slot, 11u);

                float legacyT = -1.0f;
                AZ::Vector3 legacyNormal = AZ::Vector3::CreateOne();
                EXPECT_TRUE(useKdTree ? tree.RayIntersection(start, direction, legacyT, legacyNormal)
                                     : mesh.GetModel()->LocalRayIntersectionAgainstModel(start, direction, false, legacyT, legacyNormal));
                EXPECT_FLOAT_EQ(legacyT, t);
                EXPECT_THAT(legacyNormal, IsClose(normal));

                ASSERT_TRUE(query(AZ::Vector3::CreateZero(), -AZ::Vector3::CreateAxisZ(), t, normal, slot));
                EXPECT_FLOAT_EQ(t, 0.4f);
                ASSERT_TRUE(slot.has_value());
                EXPECT_EQ(*slot, 22u);
                EXPECT_FALSE(query(AZ::Vector3(2.0f, 0.0f, 1.0f), direction, t, normal, slot));
                EXPECT_FALSE(slot.has_value()) << "A miss must not retain the preceding hit's material slot";
            }
        }
    }

    TEST_F(ModelTests, WorldRayIntersectionPreservesNearestSlotThroughTransformAndNonUniformScale)
    {
        for (const bool reverseOrder : {false, true})
        {
            SCOPED_TRACE(::testing::Message() << "reverse=" << reverseOrder);
            auto mesh = CreateMaterialSlotRayMesh(reverseOrder, 11u, 0.125f, 0.5f);
            JE::Render::RPI::ModelAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetName("World material slot ray");
            for (const auto stableId : {11u, 22u})
            {
                JE::Render::RPI::ModelMaterialSlot materialSlot;
                materialSlot.m_stableId = stableId;
                materialSlot.m_displayName = AZStd::string::format("Slot%u", stableId);
                creator.AddMaterialSlot(materialSlot);
            }
            auto lodAsset = mesh.GetModel()->GetLodAssets().front();
            creator.AddLodAsset(AZStd::move(lodAsset));
            AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset;
            ASSERT_TRUE(creator.End(modelAsset));
            auto model = JE::Render::RPI::Model::FindOrCreate(modelAsset);
            ASSERT_NE(model, nullptr);
            ASSERT_TRUE(modelAsset->SupportLocalRayIntersection());

            auto worldFromModel = AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.0f, 1.0f, 0.0f, 0.0f));
            worldFromModel.SetUniformScale(2.0f);
            worldFromModel.SetTranslation(AZ::Vector3(3.0f, 4.0f, 5.0f));
            const AZ::Vector3 nonUniformScale(2.0f, 4.0f, 0.5f);
            // Exact binary geometry and Y half-turn: world=(3-4*x, 4+8*y, 5-z).
            // The literal world ray maps to local (-.09375,.0625,1), direction (0,0,-2).
            // Omitting rotation reverses nearest plane; omitting non-uniform scale misses both small planes.
            const AZ::Vector3 start(3.375f, 4.5f, 4.0f);
            const AZ::Vector3 direction(0.0f, 0.0f, 2.0f);
            float t = -1.0f;
            AZ::Vector3 normal = AZ::Vector3::CreateOne();
            AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId> slot = 99u;
            ASSERT_TRUE(model->RayIntersection(worldFromModel, nonUniformScale, start, direction, t, normal, slot));
            EXPECT_FLOAT_EQ(t, 0.25f);
            ASSERT_TRUE(slot.has_value());
            EXPECT_EQ(*slot, 11u);
            EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));

            float legacyT = -1.0f;
            AZ::Vector3 legacyNormal = AZ::Vector3::CreateOne();
            ASSERT_TRUE(model->RayIntersection(worldFromModel, nonUniformScale, start, direction, legacyT, legacyNormal));
            EXPECT_FLOAT_EQ(legacyT, 0.25f);
            EXPECT_THAT(legacyNormal, IsClose(normal));

            ASSERT_TRUE(model->RayIntersection(worldFromModel, nonUniformScale,
                AZ::Vector3(3.375f, 4.5f, 5.0f), AZ::Vector3(0.0f, 0.0f, 1.0f), t, normal, slot));
            EXPECT_FLOAT_EQ(t, 0.5f);
            ASSERT_TRUE(slot.has_value());
            EXPECT_EQ(*slot, 22u);
            EXPECT_FALSE(model->RayIntersection(worldFromModel, nonUniformScale,
                AZ::Vector3(3.75f, 4.5f, 4.0f), direction, t, normal, slot));
            EXPECT_FALSE(slot.has_value()) << "A transformed miss must clear the previous material slot";
        }
    }

    TEST_F(ModelTests, RayIntersectionDoesNotUseFarSlotWhenNearestSlotIsInvalid)
    {
        for (bool reverseOrder : {false, true})
        {
            auto mesh = CreateMaterialSlotRayMesh(reverseOrder, JE::Render::RPI::ModelMaterialSlot::InvalidStableId);
            JE::Render::RPI::ModelKdTree tree;
            ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
            for (bool useKdTree : {false, true})
            {
                SCOPED_TRACE(::testing::Message() << "reverse=" << reverseOrder << " kd=" << useKdTree);
                float t = -1.0f;
                AZ::Vector3 normal = AZ::Vector3::CreateOne();
                AZStd::optional<JE::Render::RPI::ModelMaterialSlot::StableId> slot = 22u;
                EXPECT_TRUE(useKdTree
                    ? tree.RayIntersection(AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ(2.0f), t, normal, slot)
                    : mesh.GetModel()->LocalRayIntersectionAgainstModel(
                          AZ::Vector3::CreateAxisZ(), -AZ::Vector3::CreateAxisZ(2.0f), false, t, normal, slot));
                EXPECT_FLOAT_EQ(t, 0.3f);
                EXPECT_FALSE(slot.has_value());
            }
        }
    }

    namespace ModelRayQueries
    {
        using namespace JE::Render::RPI;

        void AddTriangle(AZStd::vector<float>& positions, AZStd::vector<uint32_t>& indices,
            const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c)
        {
            for (const auto& vertex : {a, b, c})
            {
                indices.push_back(aznumeric_cast<uint32_t>(positions.size() / 3));
                positions.insert(positions.end(), {vertex.GetX(), vertex.GetY(), vertex.GetZ()});
            }
        }

        void AddPlane(AZStd::vector<float>& positions, AZStd::vector<uint32_t>& indices, float z, bool reverse = false)
        {
            const AZ::Vector3 a(-2, -2, z), b(2, -2, z), c(0, 2, z);
            AddTriangle(positions, indices, a, reverse ? c : b, reverse ? b : c);
        }

        ModelRayQueryResult Query(TestMesh& mesh, ModelKdTree& tree, bool kd,
            const AZ::Vector3& start, const AZ::Vector3& direction, ModelRaySidedness side,
            AZStd::span<ModelRayHit> output)
        {
            return kd ? tree.RayIntersections(start, direction, side, output)
                      : mesh.GetModel()->LocalRayIntersectionsAgainstModel(start, direction, true, side, output);
        }

        ModelRayQueryResult WaitForPublished(TestMesh& mesh, const AZ::Vector3& start, const AZ::Vector3& direction,
            AZStd::span<ModelRayHit> output)
        {
            const auto deadline = AZStd::chrono::steady_clock::now() + AZStd::chrono::seconds(5);
            ModelRayQueryResult result;
            do
            {
                result = mesh.GetModel()->LocalRayIntersectionsAgainstModel(
                    start, direction, false, ModelRaySidedness::FrontFace, output);
                if (result.m_status != ModelRayQueryStatus::Unavailable) { return result; }
                AZStd::this_thread::yield();
            } while (AZStd::chrono::steady_clock::now() < deadline);
            return result;
        }

        void ExpectHit(const ModelRayHit& hit, float distance, const AZ::Vector3& normal,
            AZ::u32 meshIndex, AZ::u32 primitive, ModelMaterialSlot::StableId materialSlot)
        {
            EXPECT_FLOAT_EQ(hit.m_distanceNormalized, distance);
            EXPECT_THAT(hit.m_normal, IsClose(normal));
            EXPECT_EQ(hit.m_meshIndex, meshIndex);
            EXPECT_EQ(hit.m_primitiveIndex, primitive);
            ASSERT_TRUE(hit.m_materialSlotId.has_value());
            EXPECT_EQ(*hit.m_materialSlotId, materialSlot);
        }

        void ExpectSentinel(const ModelRayHit& hit)
        {
            EXPECT_FLOAT_EQ(hit.m_distanceNormalized, 42.0f);
            EXPECT_EQ(hit.m_meshIndex, 99u);
            EXPECT_EQ(hit.m_primitiveIndex, 100u);
        }
    }

    TEST_F(ModelTests, VolumeIntersectionsCrossingBoundaryAndAabbOnlyMissAgreeForKdAndBrute)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions; AZStd::vector<uint32_t> indices;
        AddPlane(positions, indices, 0);
        TestMesh mesh(positions.data(), positions.size(), indices.data(), indices.size());
        ModelKdTree tree; ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        const auto pose = AZ::Transform::CreateIdentity();
        const auto scale = AZ::Vector3::CreateOne();
        // All three vertices are outside: the box cuts the interior of the actual triangle.
        const auto crossing = AZ::Aabb::CreateFromMinMax({-0.1f, -0.1f, -0.2f}, {0.1f, 0.1f, 0.2f});
        // Touching the closed z boundary is a real hit, too.
        const auto touching = AZ::Aabb::CreateFromMinMax({-0.1f, -0.1f, 0}, {0.1f, 0.1f, 0.2f});
        // Inside the triangle's AABB, outside its sloping edge: broadphase is not contact.
        const auto miss = AZ::Aabb::CreateFromMinMax({1.7f, 1.7f, -0.2f}, {1.9f, 1.9f, 0.2f});
        for (bool kd : {false, true})
        {
            for (const auto& bounds : {crossing, touching, miss})
            {
                AZStd::array<ModelVolumeHit, 1> hits;
                const auto result = kd ? tree.VolumeIntersections(bounds, pose, scale, hits)
                    : mesh.GetModel()->WorldVolumeIntersectionsAgainstModel(bounds, pose, scale, true, hits);
                ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
                ASSERT_EQ(result.m_hitCount, bounds.GetMin().GetX() > 1 ? 0 : 1);
                if (result.m_hitCount)
                {
                    EXPECT_TRUE(bounds.Contains(hits[0].m_worldPosition));
                    EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 0);
                    EXPECT_THAT(hits[0].m_worldNormal, IsClose(AZ::Vector3::CreateAxisZ()));
                    EXPECT_EQ(hits[0].m_meshIndex, 0u); EXPECT_EQ(hits[0].m_primitiveIndex, 0u);
                    EXPECT_FLOAT_EQ(hits[0].m_distance, (hits[0].m_worldPosition - bounds.GetCenter()).GetLength());
                }
            }
        }
    }

    TEST_F(ModelTests, VolumeIntersectionsReuseSplitTreeDedupOriginalPrimitiveAndReportCapacity)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions; AZStd::vector<uint32_t> indices;
        for (float x : {-4.0f, 4.0f})
        {
            for (size_t i = 0; i < 60; ++i)
            { AddTriangle(positions, indices, {x - 0.1f, -0.1f, 0}, {x + 0.1f, -0.1f, 0}, {x, 0.1f, 0}); }
        }
        AddTriangle(positions, indices, {-5, -2, 0}, {5, -2, 0}, {0, 3, 0});
        indices.insert(indices.end(), {360, 361, 362}); // Distinct coincident primitive, same indices.
        TestMesh mesh(positions.data(), positions.size(), indices.data(), indices.size());
        ModelKdTree tree; ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        AZStd::vector<AZ::Aabb> leaves;
        tree.GetPenetratedBoxes({-0.25f, 0, 2}, {0.5f, 0, -4}, leaves); ASSERT_GT(leaves.size(), 1);
        const auto bounds = AZ::Aabb::CreateFromMinMax({-0.1f, -0.1f, -0.2f}, {0.1f, 0.1f, 0.2f});
        AZStd::array<ModelVolumeHit, 2> brute, direct, published;
        // Direct tree construction never installs the ModelAsset's private tree. First call takes explicit fallback.
        const auto fallback = mesh.GetModel()->WorldVolumeIntersectionsAgainstModel(
            bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), true, brute);
        ASSERT_EQ(fallback.m_hitCount, 2); ASSERT_EQ(fallback.m_status, ModelRayQueryStatus::Complete);
        AZStd::array<ModelRayHit, 1> ignored;
        ASSERT_NE(WaitForPublished(mesh, {0, 0, 2}, {0, 0, -4}, ignored).m_status, ModelRayQueryStatus::Unavailable);
        ASSERT_EQ(mesh.GetModel()->WorldVolumeIntersectionsAgainstModel(
            bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), false, published).m_hitCount, 2);
        ASSERT_EQ(tree.VolumeIntersections(bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), direct).m_hitCount, 2);
        for (size_t i = 0; i < 2; ++i)
        {
            EXPECT_EQ(brute[i].m_primitiveIndex, 120u + i);
            EXPECT_EQ(direct[i].m_primitiveIndex, brute[i].m_primitiveIndex);
            EXPECT_EQ(published[i].m_primitiveIndex, brute[i].m_primitiveIndex);
            EXPECT_EQ(direct[i].m_meshIndex, brute[i].m_meshIndex);
            EXPECT_EQ(direct[i].m_materialSlotId, brute[i].m_materialSlotId);
            EXPECT_THAT(direct[i].m_worldPosition, IsClose(brute[i].m_worldPosition));
            EXPECT_THAT(published[i].m_worldNormal, IsClose(brute[i].m_worldNormal));
            EXPECT_FLOAT_EQ(direct[i].m_distance, brute[i].m_distance);
        }
        auto limited = tree.VolumeIntersections(bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), {direct.data(), 1});
        EXPECT_EQ(limited.m_status, ModelRayQueryStatus::Truncated); EXPECT_EQ(limited.m_hitCount, 1);
        EXPECT_EQ(limited.m_discardedHitCount, 1); EXPECT_EQ(direct[0].m_primitiveIndex, 120u);
        limited = tree.VolumeIntersections(bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), {});
        EXPECT_EQ(limited.m_hitCount, 0); EXPECT_EQ(limited.m_discardedHitCount, 2);
    }

    TEST_F(ModelTests, VolumeIntersectionsInvalidUnavailableAndLateNonfiniteGeometryDoNotPublishPartialOutput)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions; AZStd::vector<uint32_t> indices;
        AddPlane(positions, indices, 0); AddPlane(positions, indices, 1);
        TestMesh mesh(positions.data(), positions.size(), indices.data(), indices.size());
        ModelKdTree tree;
        const auto bounds = AZ::Aabb::CreateFromMinMax({-1, -1, -2}, {1, 1, 2});
        ModelVolumeHit hit; hit.m_distance = 42;
        EXPECT_EQ(tree.VolumeIntersections(bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), {&hit, 1}).m_status,
            ModelRayQueryStatus::Unavailable);
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        auto collapsed = AZ::Transform::CreateIdentity(); collapsed.SetUniformScale(0);
        EXPECT_EQ(tree.VolumeIntersections(bounds, collapsed, AZ::Vector3::CreateOne(), {&hit, 1}).m_status,
            ModelRayQueryStatus::InvalidInput);
        EXPECT_FLOAT_EQ(hit.m_distance, 42);
        // Mutate resident payload after Build: the second primitive must invalidate the whole call before commit.
        const auto meshes = mesh.GetModel()->GetLodAssets()[0]->GetMeshes();
        auto vertices = ModelKdTree::GetPositionsBuffer(meshes[0]);
        auto* changed = const_cast<float*>(vertices.data());
        const float original = changed[9]; changed[9] = AZStd::numeric_limits<float>::quiet_NaN();
        const auto result = tree.VolumeIntersections(bounds, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne(), {&hit, 1});
        changed[9] = original;
        EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidGeometry); EXPECT_FLOAT_EQ(hit.m_distance, 42);
    }

    TEST_F(ModelTests, RayIntersectionsDefaultsDoNotInventIdentityOrCompletion)
    {
        using namespace ModelRayQueries;
        const ModelRayQueryResult result;
        const ModelRayHit hit;
        EXPECT_EQ(result.m_status, ModelRayQueryStatus::Unavailable);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_EQ(hit.m_meshIndex, AZStd::numeric_limits<AZ::u32>::max());
        EXPECT_EQ(hit.m_primitiveIndex, AZStd::numeric_limits<AZ::u32>::max());
        EXPECT_FALSE(hit.m_materialSlotId.has_value());
        EXPECT_THAT(hit.m_normal, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(ModelTests, RayIntersectionsCavityAndWindingAgreeForKdAndBruteForce)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions;
        AZStd::vector<uint32_t> indices;
        AddPlane(positions, indices, -3, true);
        AddPlane(positions, indices, 1, true);
        AddPlane(positions, indices, 3);
        AddPlane(positions, indices, -1);
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, 17u);
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 4> hits;
            auto result = Query(mesh, tree, kd, {0, 0, 5}, {0, 0, -10}, ModelRaySidedness::DoubleSided, hits);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 4);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            ExpectHit(hits[0], 0.2f, AZ::Vector3::CreateAxisZ(), 0, 2, 17u);
            ExpectHit(hits[1], 0.4f, -AZ::Vector3::CreateAxisZ(), 0, 1, 17u);
            ExpectHit(hits[2], 0.6f, AZ::Vector3::CreateAxisZ(), 0, 3, 17u);
            ExpectHit(hits[3], 0.8f, -AZ::Vector3::CreateAxisZ(), 0, 0, 17u);
            result = Query(mesh, tree, kd, {0, 0, 5}, {0, 0, -10}, ModelRaySidedness::FrontFace, hits);
            ASSERT_EQ(result.m_hitCount, 2);
            EXPECT_EQ(hits[0].m_primitiveIndex, 2);
            EXPECT_EQ(hits[1].m_primitiveIndex, 3);
            result = Query(mesh, tree, kd, {0, 0, -5}, {0, 0, 10}, ModelRaySidedness::DoubleSided, hits);
            ASSERT_EQ(result.m_hitCount, 4);
            ExpectHit(hits[0], 0.2f, -AZ::Vector3::CreateAxisZ(), 0, 0, 17u);
            EXPECT_EQ(hits[3].m_primitiveIndex, 2);
        }
    }

    TEST_F(ModelTests, RayIntersectionsNearestCapacityAndZeroCapacityCountAllDiscardedHits)
    {
        using namespace ModelRayQueries;
        auto mesh = CreateMaterialSlotRayMesh(false, 11u);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 3> hits;
            auto result = Query(mesh, tree, kd, {0.025f, -0.025f, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, hits);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Truncated);
            ASSERT_EQ(result.m_hitCount, 3);
            EXPECT_EQ(result.m_discardedHitCount, 13);
            for (AZ::u32 i = 0; i < 3; ++i)
            {
                EXPECT_FLOAT_EQ(hits[i].m_distanceNormalized, 0.3f);
                EXPECT_EQ(hits[i].m_meshIndex, 1);
                EXPECT_EQ(hits[i].m_primitiveIndex, i * 2 + 1);
                ASSERT_TRUE(hits[i].m_materialSlotId.has_value());
                EXPECT_EQ(*hits[i].m_materialSlotId, 11u);
            }
            result = Query(mesh, tree, kd, {0.025f, -0.025f, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, {});
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::Truncated);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(result.m_discardedHitCount, 16);
            result = Query(mesh, tree, kd, {5, 5, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, {});
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            EXPECT_EQ(result.m_discardedHitCount, 0);
        }
    }

    TEST_F(ModelTests, RayIntersectionsSplitLeafDuplicatesDoNotMergeDistinctCoincidentPrimitives)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions;
        AZStd::vector<uint32_t> indices;
        for (float x : {-4.0f, 4.0f})
        {
            for (size_t i = 0; i < 24; ++i)
            {
                AddTriangle(positions, indices, {x - 0.1f, -0.1f, 0}, {x + 0.1f, -0.1f, 0}, {x, 0.1f, 0});
            }
        }
        AddTriangle(positions, indices, {-5, -2, 0}, {5, -2, 0}, {0, 3, 0});
        // A distinct original primitive with the SAME three indices must remain a distinct hit.
        indices.insert(indices.end(), {144, 145, 146});
        TestMesh mesh(positions.data(), positions.size(), indices.data(), indices.size());
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        const AZ::Vector3 start(-0.25f, 0, 2), direction(0.5f, 0, -4);
        AZStd::vector<AZ::Aabb> boxes;
        tree.GetPenetratedBoxes(start, direction, boxes);
        ASSERT_GT(boxes.size(), 1); // The real tree must split; otherwise the duplicate-leaf oracle is vacuous.
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 3> hits;
            const auto result = Query(mesh, tree, kd, start, direction, ModelRaySidedness::DoubleSided, hits);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 2);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            EXPECT_EQ(hits[0].m_primitiveIndex, 48);
            EXPECT_EQ(hits[1].m_primitiveIndex, 49);
            EXPECT_FLOAT_EQ(hits[0].m_distanceNormalized, 0.5f);
            EXPECT_FLOAT_EQ(hits[1].m_distanceNormalized, 0.5f);
        }
    }

    TEST_F(ModelTests, RayIntersectionsMoreThan255SubmeshesPreserveLastMeshAndLegacyNearest)
    {
        using namespace ModelRayQueries;
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        const AZStd::array<uint32_t, 3> indices{0, 1, 2};
        for (AZ::u32 i = 0; i < 257; ++i)
        {
            const float x = i == 256 ? 0.0f : 10.0f;
            const AZStd::array<float, 9> positions{x - 1, -1, 0, x + 1, -1, 0, x, 1, 0};
            mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, i + 1);
        }
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        AZStd::array<ModelRayHit, 2> hits;
        const auto result = tree.RayIntersections({0, 0, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, hits);
        ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        ExpectHit(hits[0], 0.5f, AZ::Vector3::CreateAxisZ(), 256, 0, 257u);
        float t = -1;
        AZ::Vector3 normal = AZ::Vector3::CreateZero();
        AZStd::optional<ModelMaterialSlot::StableId> materialSlot;
        ASSERT_TRUE(tree.RayIntersection({0, 0, 1}, {0, 0, -2}, t, normal, materialSlot));
        EXPECT_FLOAT_EQ(t, 0.5f);
        ASSERT_TRUE(materialSlot.has_value());
        EXPECT_EQ(*materialSlot, 257u);
        // The direct tree and its legacy query never populate this fresh ModelAsset's private KD cache.
        // Its first >100-triangle query with allowBruteForce=true therefore executes the real fallback.
        AZStd::array<ModelRayHit, 2> bruteHits;
        const auto brute = mesh.GetModel()->LocalRayIntersectionsAgainstModel(
            {0, 0, 1}, {0, 0, -2}, true, ModelRaySidedness::FrontFace, bruteHits);
        ASSERT_EQ(brute.m_status, ModelRayQueryStatus::Complete);
        ASSERT_EQ(brute.m_hitCount, 1);
        ExpectHit(bruteHits[0], 0.5f, AZ::Vector3::CreateAxisZ(), 256, 0, 257u);
        const auto published = WaitForPublished(mesh, {0, 0, 1}, {0, 0, -2}, hits);
        ASSERT_EQ(published.m_status, ModelRayQueryStatus::Complete);
        ASSERT_EQ(published.m_hitCount, 1);
        ExpectHit(hits[0], 0.5f, AZ::Vector3::CreateAxisZ(), 256, 0, 257u);
    }

    TEST_F(ModelTests, RayIntersectionsInvalidRequestAndUnbuiltTreeDoNotWriteOutput)
    {
        using namespace ModelRayQueries;
        auto mesh = CreateMaterialSlotRayMesh(false, 11u);
        ModelKdTree tree;
        ModelRayHit hit;
        hit.m_distanceNormalized = 42;
        hit.m_meshIndex = 99;
        hit.m_primitiveIndex = 100;
        const auto unavailable = tree.RayIntersections({0, 0, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, {&hit, 1});
        EXPECT_EQ(unavailable.m_status, ModelRayQueryStatus::Unavailable);
        ExpectSentinel(hit);
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            const float nan = AZStd::numeric_limits<float>::quiet_NaN();
            const float infinity = AZStd::numeric_limits<float>::infinity();
            for (const AZ::Vector3& direction : {AZ::Vector3::CreateZero(), AZ::Vector3(nan, 0, 0), AZ::Vector3(0, infinity, 0)})
            {
                const auto result = Query(mesh, tree, kd, {0, 0, 1}, direction, ModelRaySidedness::DoubleSided, {&hit, 1});
                EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidInput);
                EXPECT_EQ(result.m_hitCount, 0);
                ExpectSentinel(hit);
            }
            const auto result = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, static_cast<ModelRaySidedness>(255), {&hit, 1});
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidInput);
            ExpectSentinel(hit);
            for (const auto& start : {AZ::Vector3(nan, 0, 0), AZ::Vector3(AZStd::numeric_limits<float>::max(), 0, 0)})
            {
                const auto invalid = Query(mesh, tree, kd, start, {AZStd::numeric_limits<float>::max(), 0, 0},
                    ModelRaySidedness::FrontFace, {&hit, 1});
                EXPECT_EQ(invalid.m_status, ModelRayQueryStatus::InvalidInput);
                ExpectSentinel(hit);
            }
        }
    }

    TEST_F(ModelTests, RayIntersectionsLaterInvalidGeometryRejectsWholeQueryWithoutPartialOutput)
    {
        using namespace ModelRayQueries;
        for (bool corruptPosition : {false, true})
        {
            auto mesh = CreateMaterialSlotRayMesh(false, 11u);
            auto& secondMesh = mesh.GetModel()->GetLodAssets()[0]->GetMeshes()[1];
            if (corruptPosition)
            {
                const auto* view = secondMesh.GetSemanticBufferAssetView(AZ::Name("POSITION"));
                auto* data = const_cast<uint8_t*>(view->GetBufferAsset()->GetBuffer().data());
                reinterpret_cast<float*>(data)[0] = AZStd::numeric_limits<float>::quiet_NaN();
            }
            else
            {
                auto& view = const_cast<BufferAssetView&>(secondMesh.GetIndexBufferAssetView());
                auto descriptor = view.GetBufferViewDescriptor();
                descriptor.m_elementOffset = AZStd::numeric_limits<AZ::u32>::max();
                view = {view.GetBufferAsset(), descriptor};
            }
            ModelKdTree tree;
            EXPECT_FALSE(tree.Build(mesh.GetModel().Get()));
            for (bool kd : {false, true})
            {
                ModelRayHit hit;
                hit.m_distanceNormalized = 42;
                hit.m_meshIndex = 99;
                hit.m_primitiveIndex = 100;
                const auto result = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::DoubleSided, {&hit, 1});
                EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidGeometry);
                EXPECT_EQ(result.m_hitCount, 0);
                EXPECT_EQ(result.m_discardedHitCount, 0);
                ExpectSentinel(hit);
            }
        }
    }

    TEST_F(ModelTests, RayIntersectionsResidentTreeRevalidatesFiniteGeometryBeforePublishing)
    {
        using namespace ModelRayQueries;
        auto mesh = CreateMaterialSlotRayMesh(false, 11u);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        const auto* view = mesh.GetModel()->GetLodAssets()[0]->GetMeshes()[1].GetSemanticBufferAssetView(AZ::Name("POSITION"));
        auto* data = const_cast<uint8_t*>(view->GetBufferAsset()->GetBuffer().data());
        reinterpret_cast<float*>(data)[0] = AZStd::numeric_limits<float>::infinity();
        ModelRayHit hit;
        hit.m_distanceNormalized = 42;
        hit.m_meshIndex = 99;
        hit.m_primitiveIndex = 100;
        const auto result = tree.RayIntersections({0, 0, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, {&hit, 1});
        EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidGeometry);
        ExpectSentinel(hit);
    }

    TEST_F(ModelTests, RayIntersectionsRealAsyncTreeRequestReportsUnavailableOrUsesExplicitFallback)
    {
        using namespace ModelRayQueries;
        // 101 separated planes exceed the real async threshold and force the direct KD to have multiple leaves.
        // Each iteration owns a fresh asset; direct KD construction does not initialize ModelAsset's private cache.
        for (bool allowBruteForce : {false, true})
        {
            AZStd::vector<float> positions;
            AZStd::vector<uint32_t> indices;
            for (size_t i = 0; i < 101; ++i) { AddPlane(positions, indices, 10.0f - static_cast<float>(i) * 0.2f); }
            TestMesh mesh;
            ModelLodAssetCreator creator;
            mesh.Begin(creator);
            mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, 23u);
            mesh.End(creator);
            const AZ::Vector3 start(0, 0, 12), direction(0, 0, -24);
            ModelKdTree directTree;
            ASSERT_TRUE(directTree.Build(mesh.GetModel().Get()));
            AZStd::vector<AZ::Aabb> boxes;
            directTree.GetPenetratedBoxes(start, direction, boxes);
            ASSERT_GT(boxes.size(), 3);
            AZStd::array<ModelRayHit, 101> directHits;
            const auto direct = directTree.RayIntersections(start, direction, ModelRaySidedness::FrontFace, directHits);
            ASSERT_EQ(direct.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(direct.m_hitCount, 101);
            EXPECT_EQ(direct.m_discardedHitCount, 0);

            const auto checkAll = [&](const AZStd::array<ModelRayHit, 101>& actual)
            {
                for (AZ::u32 i = 0; i < 101; ++i)
                {
                    const float z = 10.0f - static_cast<float>(i) * 0.2f;
                    const float expectedT = (12.0f - z) / 24.0f;
                    ExpectHit(actual[i], expectedT, AZ::Vector3::CreateAxisZ(), 0, i, 23u);
                    EXPECT_FLOAT_EQ(actual[i].m_distanceNormalized, directHits[i].m_distanceNormalized);
                    EXPECT_THAT(actual[i].m_normal, IsClose(directHits[i].m_normal));
                    EXPECT_EQ(actual[i].m_meshIndex, directHits[i].m_meshIndex);
                    EXPECT_EQ(actual[i].m_primitiveIndex, directHits[i].m_primitiveIndex);
                    EXPECT_EQ(actual[i].m_materialSlotId, directHits[i].m_materialSlotId);
                }
            };
            AZStd::array<ModelRayHit, 101> actual;
            actual[0].m_distanceNormalized = 42;
            actual[0].m_meshIndex = 99;
            actual[0].m_primitiveIndex = 100;
            const auto first = mesh.GetModel()->LocalRayIntersectionsAgainstModel(
                start, direction, allowBruteForce, ModelRaySidedness::FrontFace, actual);
            if (allowBruteForce)
            {
                ASSERT_EQ(first.m_status, ModelRayQueryStatus::Complete);
                ASSERT_EQ(first.m_hitCount, 101);
                EXPECT_EQ(first.m_discardedHitCount, 0);
                checkAll(actual); // First query on this fresh asset takes the explicit brute-force branch.
            }
            else
            {
                EXPECT_EQ(first.m_status, ModelRayQueryStatus::Unavailable);
                EXPECT_EQ(first.m_hitCount, 0);
                ExpectSentinel(actual[0]);
            }
            const auto published = WaitForPublished(mesh, start, direction, actual);
            ASSERT_EQ(published.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(published.m_hitCount, 101);
            EXPECT_EQ(published.m_discardedHitCount, 0);
            checkAll(actual); // allowBruteForce=false on the published ModelAsset KD.
        }
    }

    TEST_F(ModelTests, RayIntersectionsWallAndBufferViewOffsetsUseLocalGeometry)
    {
        using namespace ModelRayQueries;
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        const AZStd::array<float, 9> wall{2, -2, -2, 2, 2, -2, 2, 0, 2};
        const AZStd::array<uint32_t, 3> indices{0, 1, 2};
        auto positionBuffer = BuildTestBuffer(6, sizeof(float) * 3);
        auto indexBuffer = BuildTestBuffer(6, sizeof(uint32_t));
        mesh.Add(creator, wall.data(), wall.size(), 9, positionBuffer,
            indices.data(), indices.size(), 3, indexBuffer, 55u);
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 2> hits;
            auto result = Query(mesh, tree, kd, {4, 0, 0}, {-4, 0, 0}, ModelRaySidedness::FrontFace, hits);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 1);
            ExpectHit(hits[0], 0.5f, AZ::Vector3::CreateAxisX(), 0, 0, 55u);
            result = Query(mesh, tree, kd, {0, 0, 0}, {4, 0, 0}, ModelRaySidedness::FrontFace, hits);
            EXPECT_EQ(result.m_hitCount, 0);
            result = Query(mesh, tree, kd, {0, 0, 0}, {4, 0, 0}, ModelRaySidedness::DoubleSided, hits);
            ASSERT_EQ(result.m_hitCount, 1);
            ExpectHit(hits[0], 0.5f, AZ::Vector3::CreateAxisX(), 0, 0, 55u);
            result = Query(mesh, tree, kd, {4, 0, 0}, {-1, 0, 0}, ModelRaySidedness::DoubleSided, hits);
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            EXPECT_EQ(result.m_hitCount, 0);
        }
    }

    TEST_F(ModelTests, RayIntersectionsInvalidIndicesAndIncompleteTrianglesAreNotMisses)
    {
        using namespace ModelRayQueries;
        for (bool incompleteTriangle : {false, true})
        {
            auto mesh = CreateMaterialSlotRayMesh(false, ModelMaterialSlot::InvalidStableId);
            const auto& second = mesh.GetModel()->GetLodAssets()[0]->GetMeshes()[1];
            auto& view = const_cast<BufferAssetView&>(second.GetIndexBufferAssetView());
            if (incompleteTriangle)
            {
                auto descriptor = view.GetBufferViewDescriptor();
                --descriptor.m_elementCount;
                view = {view.GetBufferAsset(), descriptor};
            }
            else
            {
                reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(view.GetBufferAsset()->GetBuffer().data()))[0] = 4;
            }
            ModelKdTree tree;
            ASSERT_FALSE(tree.Build(mesh.GetModel().Get()));
            for (bool kd : {false, true})
            {
                ModelRayHit hit;
                hit.m_distanceNormalized = 42;
                hit.m_meshIndex = 99;
                hit.m_primitiveIndex = 100;
                const auto result = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::DoubleSided, {&hit, 1});
                EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidGeometry);
                ExpectSentinel(hit);
            }
        }
    }

    TEST_F(ModelTests, RayIntersectionsInvalidMaterialSlotAndDegeneratePrimitivesRemainDistinctFromGeometryErrors)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions;
        AZStd::vector<uint32_t> indices;
        AddTriangle(positions, indices, {0, 0, 0}, {0, 0, 0}, {0, 0, 0});
        AddPlane(positions, indices, 0);
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, ModelMaterialSlot::InvalidStableId);
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 2> hits;
            const auto result = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, hits);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            EXPECT_EQ(hits[0].m_primitiveIndex, 1);
            EXPECT_FALSE(hits[0].m_materialSlotId.has_value());
        }
    }

    TEST_F(ModelTests, RayIntersectionsMissingCpuPayloadWithPositiveOffsetIsUnavailableAndResidentBoundsRemainValidated)
    {
        using namespace ModelRayQueries;
        auto mesh = CreateMaterialSlotRayMesh(false, 11u);
        const auto& first = mesh.GetModel()->GetLodAssets()[0]->GetMeshes()[0];
        auto& view = const_cast<BufferAssetView&>(first.GetIndexBufferAssetView());
        const auto resident = view.GetBufferAsset();
        auto descriptor = view.GetBufferViewDescriptor();
        descriptor.m_elementOffset = 3;
        descriptor.m_elementCount -= 3;

        // Public creator explicitly supports buffers without initial CPU bytes, e.g. read/write buffers.
        BufferAssetCreator creator;
        creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        creator.SetPoolAsset(resident->GetPoolAsset());
        creator.SetBuffer(nullptr, 0, resident->GetBufferDescriptor());
        creator.SetBufferViewDescriptor(resident->GetBufferViewDescriptor());
        AZ::Data::Asset<BufferAsset> noCpuPayload;
        ASSERT_TRUE(creator.End(noCpuPayload));
        ASSERT_TRUE(noCpuPayload.IsReady());
        ASSERT_TRUE(noCpuPayload->GetBuffer().empty());
        ASSERT_GT(descriptor.m_elementOffset, 0);
        ASSERT_GT(descriptor.m_elementCount, 0);
        view = {noCpuPayload, descriptor};

        ModelKdTree tree;
        auto checkFailure = [&](ModelRayQueryStatus expected)
        {
            EXPECT_FALSE(tree.Build(mesh.GetModel().Get()));
            for (bool kd : {false, true})
            {
                ModelRayHit hit;
                hit.m_distanceNormalized = 42;
                hit.m_meshIndex = 99;
                hit.m_primitiveIndex = 100;
                const auto result = Query(mesh, tree, kd, {0.025f, -0.025f, 1}, {0, 0, -2},
                    ModelRaySidedness::FrontFace, {&hit, 1});
                EXPECT_EQ(result.m_status, expected);
                EXPECT_EQ(result.m_hitCount, 0);
                EXPECT_EQ(result.m_discardedHitCount, 0);
                ExpectSentinel(hit);
            }
        };
        checkFailure(ModelRayQueryStatus::Unavailable);

        auto badFormat = descriptor;
        badFormat.m_elementSize = sizeof(uint32_t) * 2;
        view = {noCpuPayload, badFormat};
        checkFailure(ModelRayQueryStatus::InvalidGeometry); // Absence must not hide an unsupported descriptor.

        auto badRange = descriptor;
        badRange.m_elementOffset = AZStd::numeric_limits<AZ::u32>::max();
        view = {resident, badRange};
        checkFailure(ModelRayQueryStatus::InvalidGeometry); // Resident views still require actual range validation.

        view = {resident, descriptor};
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            ModelRayHit hit;
            const auto result = Query(mesh, tree, kd, {0.025f, -0.025f, 1}, {0, 0, -2},
                ModelRaySidedness::FrontFace, {&hit, 1});
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Truncated);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_FLOAT_EQ(hit.m_distanceNormalized, 0.3f);
            EXPECT_EQ(hit.m_meshIndex, 1);
        }
    }

    TEST_F(ModelTests, RayIntersectionsMaximumLocalIndexIsRejectedBeforeSplitOrNearestTraversal)
    {
        using namespace ModelRayQueries;
        auto mesh = CreateMaterialSlotRayMesh(false, 11u);
        const auto& view = mesh.GetModel()->GetLodAssets()[0]->GetMeshes()[1].GetIndexBufferAssetView();
        auto* bytes = const_cast<uint8_t*>(view.GetBufferAsset()->GetBuffer().data());
        reinterpret_cast<uint32_t*>(bytes)[0] = AZStd::numeric_limits<uint32_t>::max();
        ModelKdTree tree;
        ASSERT_FALSE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            ModelRayHit hit;
            hit.m_distanceNormalized = 42;
            hit.m_meshIndex = 99;
            hit.m_primitiveIndex = 100;
            const auto result = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::DoubleSided, {&hit, 1});
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::InvalidGeometry);
            EXPECT_EQ(result.m_hitCount, 0);
            ExpectSentinel(hit);
        }
    }
    TEST_F(ModelTests, RayIntersectionsBackFaceFiltersBeforeCapacityAndPreservesWinding)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions;
        AZStd::vector<uint32_t> indices;
        AddPlane(positions, indices, 1);
        AddPlane(positions, indices, 0, true);
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, 19u);
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            SCOPED_TRACE(kd);
            AZStd::array<ModelRayHit, 1> hit;
            auto result = Query(mesh, tree, kd, {0, 0, 2}, {0, 0, -4}, ModelRaySidedness::BackFace, hit);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            ExpectHit(hit[0], 0.5f, -AZ::Vector3::CreateAxisZ(), 0, 1, 19u);
            result = Query(mesh, tree, kd, {0, 0, 2}, {0, 0, -4}, ModelRaySidedness::FrontFace, hit);
            ASSERT_EQ(result.m_hitCount, 1);
            ExpectHit(hit[0], 0.25f, AZ::Vector3::CreateAxisZ(), 0, 0, 19u);
            result = Query(mesh, tree, kd, {0, 0, 2}, {0, 0, -4}, ModelRaySidedness::DoubleSided, hit);
            ASSERT_EQ(result.m_status, ModelRayQueryStatus::Truncated);
            EXPECT_EQ(result.m_discardedHitCount, 1);
            ExpectHit(hit[0], 0.25f, AZ::Vector3::CreateAxisZ(), 0, 0, 19u);
            result = Query(mesh, tree, kd, {0, 0, 2}, {0, 0, -4}, ModelRaySidedness::BackFace, {});
            EXPECT_EQ(result.m_status, ModelRayQueryStatus::Truncated);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(result.m_discardedHitCount, 1);
        }
    }

    TEST_F(ModelTests, RayIntersectionsReflectedVerticesUseOppositeFaceAndOriginalWinding)
    {
        using namespace ModelRayQueries;
        AZStd::vector<float> positions;
        AZStd::vector<uint32_t> indices;
        AddPlane(positions, indices, 0);
        for (size_t i = 0; i < positions.size(); i += 3) { positions[i] = -positions[i]; }
        TestMesh mesh;
        ModelLodAssetCreator creator;
        mesh.Begin(creator);
        mesh.Add(creator, positions.data(), positions.size(), 0, indices.data(), indices.size(), 0, 21u);
        mesh.End(creator);
        ModelKdTree tree;
        ASSERT_TRUE(tree.Build(mesh.GetModel().Get()));
        for (bool kd : {false, true})
        {
            AZStd::array<ModelRayHit, 1> hit;
            const auto front = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::FrontFace, hit);
            EXPECT_EQ(front.m_hitCount, 0);
            const auto back = Query(mesh, tree, kd, {0, 0, 1}, {0, 0, -2}, ModelRaySidedness::BackFace, hit);
            ASSERT_EQ(back.m_hitCount, 1);
            ExpectHit(hit[0], 0.5f, -AZ::Vector3::CreateAxisZ(), 0, 0, 21u);
        }
    }
    namespace RevisionRpi = JE::Render::RPI;

    class ModelContentRevisionTests : public ModelTests
    {
    protected:
        AZ::Data::Asset<RevisionRpi::BufferAsset> RevisionBuffer(const AZ::Data::AssetId& id, const void* payload,
            uint32_t count, uint32_t stride, uint8_t outsideNoise, bool noCpu, AZ::RHI::Format format, uint32_t contentWidth = 0)
        {
            AZStd::vector<uint8_t> bytes((count + 2) * stride, outsideNoise);
            if (contentWidth == 0) { contentWidth = stride; }
            for (uint32_t i = 0; i != count; ++i)
            {
                std::memcpy(bytes.data() + (i + 1) * stride, static_cast<const uint8_t*>(payload) + i * contentWidth, contentWidth);
            }
            AZ::RHI::BufferDescriptor descriptor;
            descriptor.m_byteCount = bytes.size(); descriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
            const auto view = format == AZ::RHI::Format::Unknown ? AZ::RHI::BufferViewDescriptor::CreateStructured(1, count, stride) :
                AZ::RHI::BufferViewDescriptor::CreateTyped(1, count, format);
            RevisionRpi::BufferAssetCreator creator;
            creator.Begin(id); creator.SetUseCommonPool(RevisionRpi::CommonBufferPoolType::StaticInputAssembly);
            creator.SetBuffer(noCpu ? nullptr : bytes.data(), noCpu ? 0 : bytes.size(), descriptor);
            creator.SetBufferViewDescriptor(view);
            AZ::Data::Asset<RevisionRpi::BufferAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            asset.SetHint("a/non-content/buffer/hint");
            return asset;
        }
        AZ::Data::Asset<RevisionRpi::ModelAsset> RevisionModel(const AZ::Data::AssetId& id, float uv = 0.25f,
            uint8_t outsideNoise = 0, bool noCpu = false, uint32_t materialSlot = 7, size_t lodCount = 1, bool paddedPosition = false,
            const RevisionRpi::ModelSurfaceIdentityTable& identities = {})
        {
            const AZStd::array<float, 9> positions{{-1, -1, 0, 1, -1, 0, 0, 1, 0}};
            const AZStd::array<float, 9> normals{{0, 0, 1, 0, 0, 1, 0, 0, 1}};
            const AZStd::array<float, 6> uvs{{0, 0, 1, 0, uv, 1}};
            const AZStd::array<uint32_t, 3> indices{{0, 1, 2}};
            const auto bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1, -1, 0), AZ::Vector3(1, 1, 0));
            RevisionRpi::ModelAssetCreator creator;
            creator.Begin(id); creator.SetName("Model content fixture");
            for (size_t lodIndex = 0; lodIndex != lodCount; ++lodIndex)
            {
                const uint32_t subId = 100 + static_cast<uint32_t>(lodIndex) * 5;
                auto index = RevisionBuffer({id.m_guid, subId}, indices.data(), 3, sizeof(uint32_t), outsideNoise, noCpu, AZ::RHI::Format::R32_UINT);
                auto position = RevisionBuffer({id.m_guid, subId + 1}, positions.data(), 3, paddedPosition ? 16 : 12, outsideNoise, noCpu,
                    paddedPosition ? AZ::RHI::Format::Unknown : AZ::RHI::Format::R32G32B32_FLOAT, 12);
                auto normal = RevisionBuffer({id.m_guid, subId + 2}, normals.data(), 3, sizeof(float) * 3, outsideNoise, noCpu, AZ::RHI::Format::R32G32B32_FLOAT);
                auto texcoord = RevisionBuffer({id.m_guid, subId + 3}, uvs.data(), 3, sizeof(float) * 2, outsideNoise, noCpu, AZ::RHI::Format::R32G32_FLOAT);
                RevisionRpi::ModelLodAssetCreator lod;
                lod.Begin({id.m_guid, subId + 4}); lod.BeginMesh(); lod.SetMeshAabb(bounds); lod.SetMeshMaterialSlot(materialSlot);
                lod.SetMeshSurfaceIdentityTable(identities);
                lod.SetMeshIndexBuffer({index, index->GetBufferViewDescriptor()});
                lod.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                    {position, position->GetBufferViewDescriptor()});
                lod.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("NORMAL")), AZ::Name(),
                    {normal, normal->GetBufferViewDescriptor()});
                lod.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("UV")), AZ::Name("SurfaceUV"),
                    {texcoord, texcoord->GetBufferViewDescriptor()});
                lod.EndMesh(); AZ::Data::Asset<RevisionRpi::ModelLodAsset> lodAsset;
                EXPECT_TRUE(lod.End(lodAsset)); creator.AddLodAsset(AZStd::move(lodAsset));
            }
            AZ::Data::Asset<RevisionRpi::ModelAsset> asset;
            EXPECT_TRUE(creator.End(asset)); return asset;
        }
    };

    TEST_F(ModelContentRevisionTests, SameAssetIdUvOnlyChangeRevisesContentWhileIdenticalRebakeDoesNot)
    {
        const AZ::Data::AssetId id(AZ::Uuid::CreateRandom());
        auto asset = RevisionModel(id);
        ASSERT_TRUE(asset); ASSERT_EQ(asset->GetContentRevision().m_schema, RevisionRpi::ModelContentRevision::Schema);
        const auto baseline = asset->GetContentRevision(); const auto bounds = asset->GetAabb();
        ASSERT_TRUE(asset->ValidateContentRevision());
        asset.Release(); AZ::Data::AssetManager::Instance().DispatchEvents();
        asset = RevisionModel(id);
        EXPECT_EQ(asset.GetId(), id); EXPECT_EQ(asset->GetContentRevision(), baseline);
        asset.Release(); AZ::Data::AssetManager::Instance().DispatchEvents();
        asset = RevisionModel(id, 0.75f);
        EXPECT_EQ(asset.GetId(), id); EXPECT_EQ(asset->GetAabb(), bounds);
        EXPECT_NE(asset->GetContentRevision(), baseline); EXPECT_TRUE(asset->ValidateContentRevision());
    }

    TEST_F(ModelContentRevisionTests, CodecHasIndependentNonemptyPublishedViewVectorAndExactByteBudget)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const AZStd::array<AZ::u32, 5> expected{{0xB9AA31E0u, 0x76C4D56Cu, 0xC19D4D45u, 0x57B16DC1u, 0x82CE2E51u}};
        EXPECT_EQ(asset->GetContentRevision().m_schema, RevisionRpi::ModelContentRevision::Schema); EXPECT_EQ(asset->GetContentRevision().m_digest, expected);
        // Independent codec-vectors-v2.json enumerates 430 bytes, including all three stream payloads and nonzero view offsets.
        const auto exact = asset->ComputeContentRevision(430); ASSERT_TRUE(exact); EXPECT_EQ(exact->m_digest, expected);
        EXPECT_FALSE(asset->ComputeContentRevision(429)); EXPECT_FALSE(asset->ValidateContentRevision(429));
        EXPECT_FALSE(asset->ComputeContentRevision(370)); // Earlier published views consume the budget before the last view is decoded.
        EXPECT_FALSE(asset->ComputeContentRevision(0));
        EXPECT_EQ(asset->ComputeContentRevision(AZStd::numeric_limits<size_t>::max()), exact);
        EXPECT_EQ(asset->GetContentRevision().m_digest, expected); // A denied computation never rewrites the stored claim.
    }

    TEST_F(ModelContentRevisionTests, IdentityHintsUnreferencedBytesAndCloneDoNotReplaceLocalPayloadRevision)
    {
        auto a = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0x11);
        auto b = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0xee);
        a.SetHint("old/location.azmodel"); b.SetHint("new/location.azmodel");
        EXPECT_NE(a.GetId(), b.GetId()); EXPECT_EQ(a->GetContentRevision(), b->GetContentRevision());
        AZ::Data::Asset<RevisionRpi::ModelAsset> clone;
        ASSERT_TRUE(RevisionRpi::ModelAssetCreator::Clone(a, clone, AZ::Data::AssetId(AZ::Uuid::CreateRandom())));
        EXPECT_NE(clone.GetId(), a.GetId()); EXPECT_EQ(clone->GetContentRevision(), a->GetContentRevision());
        EXPECT_TRUE(clone->ValidateContentRevision());
    }

    TEST_F(ModelContentRevisionTests, BinaryRoundtripPersistsClaimAndLegacyOrUnknownSchemaCannotBeValidated)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto baseline = asset->GetContentRevision();
        SerializeTester<RevisionRpi::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(asset.Get(), AZ::DataStream::ST_BINARY);
        auto loaded = tester.SerializeIn(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        ASSERT_TRUE(loaded); EXPECT_EQ(loaded->GetContentRevision(), baseline);
        EXPECT_EQ(loaded->ValidateContentRevision(), asset->ValidateContentRevision());
        AZ::SerializeContext::DataElementNode node;
        ASSERT_TRUE(node.Convert<RevisionRpi::ModelAsset>(*GetSerializeContext()));
        ASSERT_TRUE(node.SetData(*GetSerializeContext(), *asset.Get()));
        const int revisionIndex = node.FindElement(AZ_CRC_CE("ContentRevision")); ASSERT_GE(revisionIndex, 0);
        auto unknown = baseline; ++unknown.m_schema;
        ASSERT_TRUE(node.GetSubElement(revisionIndex).SetData(*GetSerializeContext(), unknown));
        auto unknownAsset = AZStd::make_unique<RevisionRpi::ModelAsset>(); ASSERT_TRUE(node.GetData(*unknownAsset));
        EXPECT_EQ(unknownAsset->GetContentRevision().m_schema, unknown.m_schema);
        EXPECT_FALSE(unknownAsset->ValidateContentRevision());
        ASSERT_TRUE(node.RemoveElementByName(AZ_CRC_CE("ContentRevision"))); node.SetVersion(1);
        auto legacy = AZStd::make_unique<RevisionRpi::ModelAsset>(); ASSERT_TRUE(node.GetData(*legacy));
        EXPECT_EQ(legacy->GetContentRevision().m_schema, 0); EXPECT_FALSE(legacy->ValidateContentRevision());
        EXPECT_EQ(legacy->GetContentRevision().m_schema, 0); // No runtime upgrade of an old product.
    }

    TEST_F(ModelContentRevisionTests, EveryPublishedStreamIndexSlotAndLodMetadataAffectsRevision)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto baseline = asset->GetContentRevision();
        const auto& mesh = asset->GetLodAssets()[0]->GetMeshes()[0];
        AZStd::array<const RevisionRpi::BufferAssetView*, 4> views{{&mesh.GetIndexBufferAssetView(),
            &mesh.GetStreamBufferInfoList()[0].m_bufferAssetView, &mesh.GetStreamBufferInfoList()[1].m_bufferAssetView,
            &mesh.GetStreamBufferInfoList()[2].m_bufferAssetView}};
        for (const auto* view : views)
        {
            const auto& descriptor = view->GetBufferViewDescriptor();
            auto* data = const_cast<uint8_t*>(view->GetBufferAsset()->GetBuffer().data()) + descriptor.m_elementOffset * descriptor.m_elementSize;
            const uint8_t saved = data[0]; data[0] ^= 1;
            const auto changed = asset->ComputeContentRevision(); ASSERT_TRUE(changed); EXPECT_NE(*changed, baseline);
            EXPECT_FALSE(asset->ValidateContentRevision()); data[0] = saved;
            EXPECT_EQ(asset->ComputeContentRevision(), AZStd::optional<RevisionRpi::ModelContentRevision>(baseline));
        }
        auto& stream = const_cast<RevisionRpi::ModelLodAsset::Mesh::StreamBufferInfo&>(mesh.GetStreamBufferInfoList()[2]);
        const auto savedIndex = stream.m_semantic.m_index; ++stream.m_semantic.m_index;
        EXPECT_NE(asset->ComputeContentRevision(), AZStd::optional<RevisionRpi::ModelContentRevision>(baseline));
        stream.m_semantic.m_index = savedIndex;
        auto changedSlot = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0, false, 9);
        EXPECT_NE(changedSlot->GetContentRevision(), baseline);
        auto extraLod = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0, false, 7, 2);
        EXPECT_NE(extraLod->GetContentRevision(), baseline);
        auto& bounds = const_cast<AZ::Aabb&>(asset->GetAabb()); const auto savedBounds = bounds;
        bounds.AddPoint(AZ::Vector3(-2, -2, -2));
        EXPECT_NE(asset->ComputeContentRevision(), AZStd::optional<RevisionRpi::ModelContentRevision>(baseline));
        bounds = savedBounds; EXPECT_TRUE(asset->ValidateContentRevision());
    }

    TEST_F(ModelContentRevisionTests, MissingOrReleasedCpuPayloadRemainsUnavailableWithoutReloadOrRetention)
    {
        auto missing = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0, true);
        EXPECT_EQ(missing->GetContentRevision().m_schema, 0); EXPECT_FALSE(missing->ComputeContentRevision());
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto stored = asset->GetContentRevision(); ASSERT_EQ(stored.m_schema, RevisionRpi::ModelContentRevision::Schema);
        asset->ReleaseRefBufferAssets(); ASSERT_FALSE(asset->SupportLocalRayIntersection());
        EXPECT_FALSE(asset->ComputeContentRevision()); EXPECT_FALSE(asset->ValidateContentRevision());
        EXPECT_FALSE(asset->SupportLocalRayIntersection()); EXPECT_EQ(asset->GetContentRevision(), stored);
    }

    TEST_F(ModelContentRevisionTests, MalformedViewOffsetsCountsFormatsAndZeroWidthAreUnavailableWithoutPartialClaim)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto stored = asset->GetContentRevision();
        const auto& view = asset->GetLodAssets()[0]->GetMeshes()[0].GetStreamBufferInfoList()[0].m_bufferAssetView;
        auto& descriptor = const_cast<AZ::RHI::BufferViewDescriptor&>(view.GetBufferViewDescriptor());
        const auto original = descriptor;
        for (int invalid = 0; invalid != 5; ++invalid)
        {
            descriptor = original;
            if (invalid == 0) { descriptor.m_elementOffset = AZStd::numeric_limits<uint32_t>::max(); }
            if (invalid == 1) { descriptor.m_elementCount = AZStd::numeric_limits<uint32_t>::max(); }
            if (invalid == 2) { descriptor.m_elementSize = 0; }
            if (invalid == 3) { descriptor.m_elementFormat = static_cast<AZ::RHI::Format>(AZ::RHI::Format::Count); }
            if (invalid == 4) { descriptor.m_elementFormat = AZ::RHI::Format::R32G32_FLOAT; } // Eight bytes cannot describe a twelve-byte element.
            EXPECT_FALSE(asset->ComputeContentRevision()) << invalid; EXPECT_FALSE(asset->ValidateContentRevision()) << invalid;
            EXPECT_EQ(asset->GetContentRevision(), stored);
        }
        descriptor = original; EXPECT_TRUE(asset->ValidateContentRevision());
    }

    TEST_F(ModelContentRevisionTests, AuditedUnknownPositionPaddingIsExcludedAndOtherUnknownLayoutsAreUnavailable)
    {
        auto a = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0x11, false, 7, 1, true);
        auto b = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0xee, false, 7, 1, true);
        ASSERT_EQ(a->GetContentRevision().m_schema, RevisionRpi::ModelContentRevision::Schema);
        EXPECT_EQ(a->GetContentRevision(), b->GetContentRevision());
        const auto& normal = a->GetLodAssets()[0]->GetMeshes()[0].GetStreamBufferInfoList()[1].m_bufferAssetView;
        auto& descriptor = const_cast<AZ::RHI::BufferViewDescriptor&>(normal.GetBufferViewDescriptor());
        const auto saved = descriptor;
        descriptor.m_elementFormat = AZ::RHI::Format::Unknown;
        EXPECT_FALSE(a->ComputeContentRevision()); EXPECT_FALSE(a->ValidateContentRevision());
        descriptor = saved; EXPECT_TRUE(a->ValidateContentRevision());
    }

    TEST_F(ModelContentRevisionTests, FloatPayloadsCanonicalizeZeroAndRejectNonfiniteValuesWithoutRewritingClaim)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto baseline = asset->GetContentRevision();
        const auto& view = asset->GetLodAssets()[0]->GetMeshes()[0].GetStreamBufferInfoList()[2].m_bufferAssetView;
        auto* bytes = const_cast<uint8_t*>(view.GetBufferAsset()->GetBuffer().data()) + 8;
        const uint32_t minusZero = 0x80000000u;
        std::memcpy(bytes, &minusZero, sizeof(minusZero));
        EXPECT_TRUE(asset->ValidateContentRevision());
        for (uint32_t invalid : {0x7f800000u, 0xff800000u, 0x7fc00001u})
        {
            std::memcpy(bytes, &invalid, sizeof(invalid));
            EXPECT_FALSE(asset->ComputeContentRevision()); EXPECT_FALSE(asset->ValidateContentRevision());
            EXPECT_EQ(asset->GetContentRevision(), baseline);
        }
        // The same six UV scalars represented as half floats exercise the typed half codec.
        auto& descriptor = const_cast<AZ::RHI::BufferViewDescriptor&>(view.GetBufferViewDescriptor());
        const auto saved = descriptor;
        descriptor = AZ::RHI::BufferViewDescriptor::CreateTyped(2, 3, AZ::RHI::Format::R16G16_FLOAT);
        const AZStd::array<uint16_t, 6> half{{0, 0, 0x3c00, 0, 0x3400, 0x3c00}};
        std::memcpy(bytes, half.data(), sizeof(half));
        const auto halfBaseline = asset->ComputeContentRevision(); ASSERT_TRUE(halfBaseline);
        const uint16_t halfMinusZero = 0x8000u; std::memcpy(bytes, &halfMinusZero, sizeof(halfMinusZero));
        EXPECT_EQ(asset->ComputeContentRevision(), halfBaseline);
        for (uint16_t invalid : {uint16_t{0x7c00}, uint16_t{0xfc00}, uint16_t{0x7e01}})
        {
            std::memcpy(bytes, &invalid, sizeof(invalid)); EXPECT_FALSE(asset->ComputeContentRevision());
        }
        descriptor = saved;
    }

    TEST_F(ModelContentRevisionTests, InconsistentVertexCountsAndOutOfRangeOrIncompleteTrianglesAreUnavailable)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto baseline = asset->GetContentRevision();
        const auto& mesh = asset->GetLodAssets()[0]->GetMeshes()[0];
        auto& normal = const_cast<AZ::RHI::BufferViewDescriptor&>(mesh.GetStreamBufferInfoList()[1].m_bufferAssetView.GetBufferViewDescriptor());
        --normal.m_elementCount; EXPECT_FALSE(asset->ComputeContentRevision()); ++normal.m_elementCount;
        auto& indices = const_cast<AZ::RHI::BufferViewDescriptor&>(mesh.GetIndexBufferAssetView().GetBufferViewDescriptor());
        ++indices.m_elementCount; EXPECT_FALSE(asset->ComputeContentRevision()); --indices.m_elementCount;
        auto* bytes = const_cast<uint8_t*>(mesh.GetIndexBufferAssetView().GetBufferAsset()->GetBuffer().data()) + 4;
        const uint32_t outOfRange = 3; std::memcpy(bytes, &outOfRange, sizeof(outOfRange));
        EXPECT_FALSE(asset->ComputeContentRevision()); EXPECT_FALSE(asset->ValidateContentRevision());
        const uint32_t original = 0; std::memcpy(bytes, &original, sizeof(original));
        EXPECT_TRUE(asset->ValidateContentRevision()); EXPECT_EQ(asset->GetContentRevision(), baseline);
    }

    TEST_F(ModelContentRevisionTests, CompressedCookAndHandlerDecodedSnapshotHaveTheSameSemanticRevision)
    {
        auto asset = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        const auto baseline = asset->GetContentRevision();
        auto buffer = asset->GetLodAssets()[0]->GetMeshes()[0].GetStreamBufferInfoList()[0].m_bufferAssetView.GetBufferAsset();
        const auto raw = buffer->GetBuffer();
        // Fixed literal meshopt vertex-v0 layout (one <=16-record block); no compressor or production-code substitute.
        RevisionRpi::BufferAsset::BufferData encoded{0xa0};
        constexpr size_t stride = 12;
        const size_t count = raw.size() / stride;
        ASSERT_LE(count, 16);
        for (size_t channel = 0; channel != stride; ++channel)
        {
            encoded.push_back(3); // Sixteen literal zigzag byte deltas.
            uint8_t previous = raw[channel];
            for (size_t i = 0; i != 16; ++i)
            {
                const uint8_t value = i < count ? raw[i * stride + channel] : previous;
                const uint8_t delta = static_cast<uint8_t>(value - previous);
                encoded.push_back(static_cast<uint8_t>((delta << 1) ^ (0u - (delta >> 7))));
                previous = value;
            }
        }
        encoded.resize(encoded.size() + 32, 0);
        std::memcpy(encoded.data() + encoded.size() - stride, raw.data(), stride);
        // Seed the enum through the public producer; its AzTypeInfo specialization lives only in BufferAsset.cpp.
        const auto compression = RevisionRpi::BufferAsset::CompressionFormat::Vertex;
        RevisionRpi::BufferAssetCreator seedCreator;
        seedCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom())); seedCreator.SetCompressionFormat(compression);
        seedCreator.SetBuffer(raw.data(), raw.size(), buffer->GetBufferDescriptor());
        seedCreator.SetBufferViewDescriptor(buffer->GetBufferViewDescriptor());
        seedCreator.SetUseCommonPool(buffer->GetCommonPoolType()); seedCreator.SetPoolAsset(buffer->GetPoolAsset());
        seedCreator.SetBufferName(buffer->GetName());
        AZ::Data::Asset<RevisionRpi::BufferAsset> serializationSeed; ASSERT_TRUE(seedCreator.End(serializationSeed));
        AZ::SerializeContext::DataElementNode node;
        ASSERT_TRUE(node.Convert<RevisionRpi::BufferAsset>(*GetSerializeContext()));
        ASSERT_TRUE(node.SetData(*GetSerializeContext(), *serializationSeed.Get()));
        const int dataIndex = node.FindElement(AZ_CRC_CE("Buffer"));
        const int formatIndex = node.FindElement(AZ_CRC_CE("CompressionFormat"));
        ASSERT_GE(dataIndex, 0); ASSERT_GE(formatIndex, 0);
        const auto& formatNode = node.GetSubElement(formatIndex);
        const AZ::Uuid enumTypeId("{D6624140-8972-4356-9A88-56BA8DD24D0A}");
        ASSERT_EQ(formatNode.GetId(), enumTypeId);
        ASSERT_EQ(GetSerializeContext()->GetUnderlyingTypeId(enumTypeId), azrtti_typeid<uint8_t>());
        ASSERT_EQ(formatNode.GetRawDataElement().m_dataSize, sizeof(compression));
        ASSERT_TRUE(node.GetSubElement(dataIndex).SetData(*GetSerializeContext(), encoded));
        ASSERT_TRUE(node.GetData(*buffer.Get())); // Real serialized input, including encoded size > descriptor.
        ASSERT_EQ(buffer->GetCompressionFormat(), compression);
        ASSERT_GT(buffer->GetBuffer().size(), buffer->GetBufferDescriptor().m_byteCount);
        EXPECT_EQ(asset->ComputeContentRevision(), AZStd::optional<RevisionRpi::ModelContentRevision>(baseline));
        EXPECT_EQ(buffer->GetCompressionFormat(), RevisionRpi::BufferAsset::CompressionFormat::Vertex);
        RevisionRpi::ModelAssetCreator cookCreator;
        cookCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        auto cookLod = asset->GetLodAssets()[0]; cookCreator.AddLodAsset(AZStd::move(cookLod));
        AZ::Data::Asset<RevisionRpi::ModelAsset> cookModel; ASSERT_TRUE(cookCreator.End(cookModel));
        EXPECT_EQ(cookModel->GetContentRevision(), baseline); // Creator stamps the encoded cook snapshot via the const helper.
        AZStd::vector<AZ::u8> serialized;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> output(&serialized);
        auto* handler = AZ::Data::AssetManager::Instance().GetHandler(RevisionRpi::BufferAsset::RTTI_Type());
        ASSERT_TRUE(handler); ASSERT_TRUE(handler->SaveAssetData(buffer, &output));
        auto input = AZStd::make_shared<AZ::Data::AssetDataStream>(); input->Open(AZStd::move(serialized));
        ASSERT_EQ(handler->LoadAssetDataFromStream(buffer, input, {}), AZ::Data::AssetHandler::LoadResult::LoadComplete);
        EXPECT_EQ(buffer->GetCompressionFormat(), RevisionRpi::BufferAsset::CompressionFormat::Uncompressed);
        EXPECT_TRUE(asset->ValidateContentRevision()); EXPECT_EQ(asset->GetContentRevision(), baseline);
    }

    class ModelSurfaceIdentityTests : public ModelContentRevisionTests
    {
    protected:
        using Identity = RevisionRpi::ModelSurfaceIdentity;
        using Table = RevisionRpi::ModelSurfaceIdentityTable;
        static AZ::Uuid SurfaceA() { return AZ::Uuid("{10000000-0000-0000-0000-000000000001}"); }
        static AZ::Uuid SurfaceB() { return AZ::Uuid("{10000000-0000-0000-0000-000000000002}"); }
        static AZ::Uuid FaceA() { return AZ::Uuid("{20000000-0000-0000-0000-000000000002}"); }
        static AZ::Uuid FaceB() { return AZ::Uuid("{20000000-0000-0000-0000-000000000004}"); }
        static AZ::Uuid RegionA() { return AZ::Uuid("{30000000-0000-0000-0000-000000000003}"); }
        static Table Polygon()
        {
            Table table; table.m_schema = Table::Schema;
            table.m_entries = {{SurfaceA(), FaceA(), RegionA()}, {SurfaceA(), FaceA(), AZStd::nullopt}};
            return table; // Two triangles of one authored face, with per-triangle region membership.
        }
        bool IdentityLod(AZStd::initializer_list<Table> tables, AZ::Data::Asset<RevisionRpi::ModelLodAsset>& result)
        {
            const AZ::Data::AssetId id(AZ::Uuid::CreateRandom());
            const AZStd::array<float, 9> positions{{-1, -1, 0, 1, -1, 0, 0, 1, 0}};
            const AZStd::array<uint32_t, 6> indices{{0, 1, 2, 0, 1, 2}};
            const auto bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1, -1, 0), AZ::Vector3(1, 1, 0));
            RevisionRpi::ModelLodAssetCreator creator; creator.Begin(id);
            uint32_t subId = 1;
            for (const auto& table : tables)
            {
                auto position = RevisionBuffer({id.m_guid, subId++}, positions.data(), 3, 12, 0, false, AZ::RHI::Format::R32G32B32_FLOAT);
                auto index = RevisionBuffer({id.m_guid, subId++}, indices.data(), 6, 4, 0, false, AZ::RHI::Format::R32_UINT);
                creator.BeginMesh(); creator.SetMeshAabb(bounds); creator.SetMeshMaterialSlot(7);
                creator.SetMeshIndexBuffer({index, index->GetBufferViewDescriptor()});
                creator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                    {position, position->GetBufferViewDescriptor()});
                creator.SetMeshSurfaceIdentityTable(table); creator.EndMesh();
            }
            return creator.End(result);
        }
        AZ::Data::Asset<RevisionRpi::ModelAsset> IdentityModel(const Table& table, float uv = 0.25f)
        {
            // The standard fixture has one triangle and nonempty position/normal/UV streams.
            return RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), uv, 0, false, 7, 1, false, table);
        }
    };

    TEST_F(ModelSurfaceIdentityTests, RepeatedAuthoredFaceAcrossTrianglesAndMaterialSubmeshesKeepsExplicitIdentity)
    {
        const auto table = Polygon(); ASSERT_TRUE(table.Validate(2));
        AZ::Data::Asset<RevisionRpi::ModelLodAsset> lod; ASSERT_TRUE(IdentityLod({table, table}, lod));
        ASSERT_TRUE(lod->ValidateSurfaceIdentityTables());
        RevisionRpi::ModelAssetCreator creator; creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        auto reference = lod; creator.AddLodAsset(AZStd::move(reference));
        AZ::Data::Asset<RevisionRpi::ModelAsset> model; ASSERT_TRUE(creator.End(model));
        ASSERT_TRUE(model->ValidateContentRevision());
        AZStd::array<RevisionRpi::ModelRayHit, 4> hits;
        const auto query = model->LocalRayIntersectionsAgainstModel({0, 0, 1}, {0, 0, -2}, true,
            RevisionRpi::ModelRaySidedness::FrontFace, hits);
        ASSERT_EQ(query.m_status, RevisionRpi::ModelRayQueryStatus::Complete); ASSERT_EQ(query.m_hitCount, 4);
        for (const auto& hit : hits)
        {
            const auto& mesh = lod->GetMeshes()[hit.m_meshIndex];
            const auto* identity = mesh.GetSurfaceIdentity(hit.m_primitiveIndex); ASSERT_NE(identity, nullptr);
            EXPECT_EQ(*identity, table.m_entries[hit.m_primitiveIndex]);
            EXPECT_EQ(identity->m_surfaceId, SurfaceA()); EXPECT_EQ(identity->m_faceId, FaceA());
        }
        EXPECT_EQ(lod->GetMeshes()[0].GetSurfaceIdentity(2), nullptr);
    }

    TEST_F(ModelSurfaceIdentityTests, SerializedAndClonedProductsPreserveAuthoredReferencesWithoutRegeneration)
    {
        const auto table = Polygon(); AZ::Data::Asset<RevisionRpi::ModelLodAsset> lod;
        ASSERT_TRUE(IdentityLod({table}, lod));
        SerializeTester<RevisionRpi::ModelLodAsset> tester(GetSerializeContext());
        tester.SerializeOut(lod.Get(), AZ::DataStream::ST_BINARY);
        auto loaded = tester.SerializeIn(AZ::Data::AssetId(AZ::Uuid::CreateRandom()),
            AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
        ASSERT_TRUE(loaded); ASSERT_TRUE(loaded->ValidateSurfaceIdentityTables());
        EXPECT_EQ(loaded->GetMeshes()[0].GetSurfaceIdentityTable(), table);
        auto generic = AZStd::unique_ptr<RevisionRpi::ModelLodAsset>(GetSerializeContext()->CloneObject(lod.Get()));
        ASSERT_TRUE(generic); EXPECT_EQ(generic->GetMeshes()[0].GetSurfaceIdentityTable(), table);
        AZ::Data::Asset<RevisionRpi::ModelLodAsset> clone; AZ::Data::AssetId next(AZ::Uuid::CreateRandom());
        ASSERT_TRUE(RevisionRpi::ModelLodAssetCreator::Clone(lod, clone, next));
        EXPECT_NE(clone.GetId(), lod.GetId()); EXPECT_EQ(clone->GetMeshes()[0].GetSurfaceIdentityTable(), table);
        ASSERT_TRUE(clone->ValidateSurfaceIdentityTables());
    }

    TEST_F(ModelSurfaceIdentityTests, InvalidTablesRejectUnknownNullCardinalityAndContradictoryFaceOwnership)
    {
        const auto valid = Polygon(); AZ::Data::Asset<RevisionRpi::ModelLodAsset> original;
        ASSERT_TRUE(IdentityLod({valid}, original));
        for (int kind = 0; kind != 8; ++kind)
        {
            auto invalid = valid;
            if (kind == 0) { invalid.m_schema = 999; }
            if (kind == 1) { invalid.m_schema = 0; }
            if (kind == 2) { invalid.m_entries[0].m_surfaceId = AZ::Uuid::CreateNull(); }
            if (kind == 3) { invalid.m_entries[0].m_faceId = AZ::Uuid::CreateNull(); }
            if (kind == 4) { invalid.m_entries[0].m_regionId = AZ::Uuid::CreateNull(); }
            if (kind == 5) { invalid.m_entries.pop_back(); }
            if (kind == 6) { invalid.m_entries.push_back(valid.m_entries[0]); }
            if (kind == 7) { invalid.m_entries[1].m_surfaceId = SurfaceB(); }
            EXPECT_FALSE(invalid.Validate(2)) << kind;
            auto result = original;
            ErrorMessageFinder errors("Invalid surface identity table", 1);
            errors.AddIgnoredErrorMessage("Cannot continue building", true);
            EXPECT_FALSE(IdentityLod({invalid}, result)) << kind;
            EXPECT_EQ(result.Get(), original.Get()); // No partial product overwrites the caller's existing asset.
        }
    }

    TEST_F(ModelSurfaceIdentityTests, ConflictingFaceOwnershipAcrossSubmeshesRejectsLod)
    {
        auto a = Polygon(); auto b = a;
        for (auto& entry : b.m_entries) { entry.m_surfaceId = SurfaceB(); }
        ASSERT_TRUE(a.Validate(2)); ASSERT_TRUE(b.Validate(2));
        AZ::Data::Asset<RevisionRpi::ModelLodAsset> result;
        ErrorMessageFinder errors("Conflicting face ownership", 1);
        EXPECT_FALSE(IdentityLod({a, b}, result)); EXPECT_FALSE(result);
    }

    TEST_F(ModelSurfaceIdentityTests, IdentityEditsChangeContentRevisionAndUvEditsPreserveIdentity)
    {
        auto one = Polygon(); one.m_entries.resize(1);
        auto a = IdentityModel(one); auto uv = IdentityModel(one, 0.75f);
        ASSERT_TRUE(a); ASSERT_TRUE(uv); ASSERT_TRUE(a->ValidateContentRevision()); ASSERT_TRUE(uv->ValidateContentRevision());
        EXPECT_NE(a->GetContentRevision(), uv->GetContentRevision());
        EXPECT_EQ(a->GetLodAssets()[0]->GetMeshes()[0].GetSurfaceIdentityTable(), uv->GetLodAssets()[0]->GetMeshes()[0].GetSurfaceIdentityTable());
        auto changed = one; changed.m_entries[0].m_faceId = FaceB(); auto b = IdentityModel(changed);
        ASSERT_TRUE(b); EXPECT_NE(a->GetContentRevision(), b->GetContentRevision());
        auto region = one; region.m_entries[0].m_regionId.reset(); auto c = IdentityModel(region);
        ASSERT_TRUE(c); EXPECT_NE(a->GetContentRevision(), c->GetContentRevision());
        EXPECT_EQ(a->GetLodAssets()[0]->GetMeshes()[0].GetSurfaceIdentity(0)->m_faceId, FaceA());
    }
    TEST_F(ModelSurfaceIdentityTests, GeometryV1GoldenValidatesOnlyAbsentTablesAndClonePublishesV2)
    {
        auto model = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom())); ASSERT_TRUE(model);
        auto& claim = const_cast<RevisionRpi::ModelContentRevision&>(model->GetContentRevision());
        claim.m_schema = RevisionRpi::ModelContentRevision::GeometrySchema;
        claim.m_digest = {{0xE80D7DF6u, 0xEC97D8BBu, 0x64C83647u, 0xF1E4B811u, 0x2831E86Eu}};
        const auto oldClaim = claim;
        const auto validated = model->ValidateContentRevision(418); ASSERT_TRUE(validated); EXPECT_EQ(*validated, oldClaim);
        EXPECT_FALSE(model->ValidateContentRevision(417));
        const auto current = model->ComputeContentRevision(); ASSERT_TRUE(current); EXPECT_EQ(current->m_schema, 2);
        AZ::Data::Asset<RevisionRpi::ModelAsset> clone;
        ASSERT_TRUE(RevisionRpi::ModelAssetCreator::Clone(model, clone, AZ::Data::AssetId(AZ::Uuid::CreateRandom())));
        EXPECT_EQ(clone->GetContentRevision(), *current); EXPECT_EQ(claim, oldClaim);
        auto table = Polygon(); table.m_entries.resize(1);
        auto& storedTable = const_cast<Table&>(model->GetLodAssets()[0]->GetMeshes()[0].GetSurfaceIdentityTable());
        storedTable = table;
        EXPECT_FALSE(model->ValidateContentRevision()); // V1 cannot silently ignore newly introduced identity.
        EXPECT_TRUE(model->ComputeContentRevision()); EXPECT_EQ(claim, oldClaim);
    }

    TEST_F(ModelSurfaceIdentityTests, IdentityCodecHasIndependentUuidVectorAndExactBudget)
    {
        auto table = Polygon(); table.m_entries.resize(1); auto model = IdentityModel(table); ASSERT_TRUE(model);
        const AZStd::array<AZ::u32, 5> expected{{0x31E497B9u, 0x9E06EB5Cu, 0xF63C29A5u, 0xE1ACE255u, 0xFE2E2708u}};
        // Explicit UUID byte order and optional Region marker in codec-vectors-v2.json, independent of the C++ encoder.
        const auto exact = model->ComputeContentRevision(482); ASSERT_TRUE(exact); EXPECT_EQ(exact->m_digest, expected);
        EXPECT_EQ(model->GetContentRevision(), *exact); EXPECT_FALSE(model->ComputeContentRevision(481));
        EXPECT_FALSE(model->ValidateContentRevision(481)); EXPECT_EQ(model->GetContentRevision(), *exact);
        AZStd::array<RevisionRpi::ModelRayHit, 1> output;
        ASSERT_EQ(model->LocalRayIntersectionsAgainstModel({0, 0, 1}, {0, 0, -2}, true,
            RevisionRpi::ModelRaySidedness::FrontFace, output).m_hitCount, 1);
        EXPECT_EQ(model->GetContentRevision(), *exact); // Query/pose inputs do not become local authored identity.
    }

    TEST_F(ModelSurfaceIdentityTests, ConflictingFaceOwnershipAcrossLodsCannotAuthenticateOneModelSnapshot)
    {
        auto table = Polygon(); table.m_entries.resize(1);
        auto model = RevisionModel(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), 0.25f, 0, false, 7, 2, false, table);
        ASSERT_TRUE(model); ASSERT_TRUE(model->ValidateContentRevision()); const auto claim = model->GetContentRevision();
        auto& other = const_cast<Table&>(model->GetLodAssets()[1]->GetMeshes()[0].GetSurfaceIdentityTable());
        other.m_entries[0].m_surfaceId = SurfaceB(); ASSERT_TRUE(other.Validate(1));
        EXPECT_FALSE(model->ComputeContentRevision()); EXPECT_FALSE(model->ValidateContentRevision());
        EXPECT_EQ(model->GetContentRevision(), claim);
        other = table; EXPECT_TRUE(model->ValidateContentRevision());
    }

    TEST_F(ModelSurfaceIdentityTests, CorruptedTableInvalidatesClaimWithoutDestroyingLegacyGeometry)
    {
        auto table = Polygon(); table.m_entries.resize(1); auto model = IdentityModel(table); ASSERT_TRUE(model);
        const auto claim = model->GetContentRevision();
        auto& stored = const_cast<Table&>(model->GetLodAssets()[0]->GetMeshes()[0].GetSurfaceIdentityTable());
        for (int kind = 0; kind != 4; ++kind)
        {
            stored = table;
            if (kind == 0) { stored.m_schema = 999; }
            if (kind == 1) { stored.m_entries.clear(); }
            if (kind == 2) { stored.m_entries[0].m_faceId = AZ::Uuid::CreateNull(); }
            if (kind == 3) { stored.m_entries[0].m_regionId = AZ::Uuid::CreateNull(); }
            EXPECT_FALSE(model->ComputeContentRevision()) << kind; EXPECT_FALSE(model->ValidateContentRevision()) << kind;
            EXPECT_EQ(model->GetContentRevision(), claim);
            AZStd::array<RevisionRpi::ModelRayHit, 1> output;
            const auto result = model->LocalRayIntersectionsAgainstModel({0, 0, 1}, {0, 0, -2}, true,
                RevisionRpi::ModelRaySidedness::FrontFace, output);
            EXPECT_EQ(result.m_status, RevisionRpi::ModelRayQueryStatus::Complete); EXPECT_EQ(result.m_hitCount, 1);
        }
        stored = table; EXPECT_TRUE(model->ValidateContentRevision());
    }

    TEST_F(ModelSurfaceIdentityTests, VersionOneMeshWithoutTableReloadsGeometryAndDoesNotInventIdentity)
    {
        auto table = Polygon(); AZ::Data::Asset<RevisionRpi::ModelLodAsset> lod; ASSERT_TRUE(IdentityLod({table}, lod));
        const auto& source = lod->GetMeshes()[0];
        AZStd::vector<char> xml;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> out(&xml);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(out, AZ::DataStream::ST_XML, &source, GetSerializeContext()));
        xml.push_back('\0'); AZ::rapidxml::xml_document<char> document;
        ASSERT_TRUE(document.parse<0>(xml.data()));
        size_t removed = 0;
        const auto strip = [&removed](auto&& self, AZ::rapidxml::xml_node<char>* node) -> void
        {
            for (auto* child = node->first_node(); child;)
            {
                auto* next = child->next_sibling(); const auto* field = child->first_attribute("field");
                if (field && AZStd::string_view(field->value(), field->value_size()) == "SurfaceIdentityTable")
                {
                    if (auto* version = node->first_attribute("version")) { version->value("1"); }
                    node->remove_node(child); ++removed;
                }
                else { self(self, child); }
                child = next;
            }
        };
        strip(strip, &document); ASSERT_EQ(removed, 1);
        AZStd::string legacyXml; AZ::rapidxml::print(AZStd::back_inserter(legacyXml), document, 0);
        AZ::IO::MemoryStream in(static_cast<const void*>(legacyXml.data()), legacyXml.size());
        RevisionRpi::ModelLodAsset::Mesh legacy;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(in, legacy, GetSerializeContext(),
            AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading)));
        EXPECT_EQ(legacy.GetIndexCount(), 6); EXPECT_EQ(legacy.GetVertexCount(), 3);
        EXPECT_EQ(legacy.GetIndexBufferAssetView().GetBufferAsset().GetId(), source.GetIndexBufferAssetView().GetBufferAsset().GetId());
        EXPECT_EQ(legacy.GetIndexBufferAssetView().GetBufferAsset().Get(), source.GetIndexBufferAssetView().GetBufferAsset().Get());
        EXPECT_EQ(legacy.GetSurfaceIdentityTable().m_schema, 0); EXPECT_TRUE(legacy.GetSurfaceIdentityTable().m_entries.empty());
        EXPECT_TRUE(legacy.ValidateSurfaceIdentityTable()); EXPECT_EQ(legacy.GetSurfaceIdentity(0), nullptr);
    }
} // namespace UnitTest
