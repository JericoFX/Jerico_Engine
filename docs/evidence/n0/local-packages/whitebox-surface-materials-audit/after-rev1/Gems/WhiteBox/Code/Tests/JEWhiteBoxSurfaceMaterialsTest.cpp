/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "WhiteBoxTestFixtures.h"
#include <AzCore/Name/NameDictionary.h>
#include "Editor/JEWhiteBoxSurfaceMaterials.h"
#include "Rendering/Atom/WhiteBoxMeshAtomData.h"
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/algorithm.h>

namespace UnitTest
{
    namespace Api = WhiteBox::Api;
    using namespace JE::BSP;

    class WhiteBoxSurfaceMaterialTestFixture : public WhiteBoxTestFixture
    {
        bool m_ownsNameDictionary = false;
        void SetUpEditorFixtureImpl() override
        {
            m_ownsNameDictionary = !AZ::NameDictionary::IsReady();
            if (m_ownsNameDictionary) { AZ::NameDictionary::Create(); }
        }
        void TearDownEditorFixtureImpl() override
        {
            if (m_ownsNameDictionary) { AZ::NameDictionary::Destroy(); }
        }
    };


    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialPartitionRetainsDistinctSlotsAndExactCornerUVs)
    {
        WhiteBox::WhiteBoxFaces faces(3);
        for (size_t i = 0; i < faces.size(); ++i)
        {
            faces[i].m_surfaceId = AZ::Uuid::CreateRandom();
            faces[i].m_materialSlot = i == 1 ? 7 : 0;
            faces[i].m_v1 = {AZ::Vector3(0, 0, static_cast<float>(i)), AZ::Vector2(10 + static_cast<float>(i), 20)};
            faces[i].m_v2 = {AZ::Vector3(1, 0, static_cast<float>(i)), AZ::Vector2(30, 40)};
            faces[i].m_v3 = {AZ::Vector3(0, 1, static_cast<float>(i)), AZ::Vector2(50, 60)};
        }
        auto ranges = PartitionFacesByMaterial(faces);
        ASSERT_EQ(ranges.size(), 2);
        EXPECT_EQ(ranges[0].m_materialSlot, 0);
        EXPECT_EQ(ranges[0].m_firstIndex, 0);
        EXPECT_EQ(ranges[0].m_indexCount, 6);
        EXPECT_EQ(ranges[1].m_materialSlot, 7);
        EXPECT_EQ(ranges[1].m_firstIndex, 6);
        EXPECT_EQ(ranges[1].m_indexCount, 3);
        EXPECT_EQ(faces[0].m_v1.m_uv, AZ::Vector2(10, 20));
        EXPECT_EQ(faces[1].m_v1.m_uv, AZ::Vector2(12, 20));
        EXPECT_EQ(faces[2].m_v1.m_uv, AZ::Vector2(11, 20));
        const WhiteBox::WhiteBoxMeshAtomData data(faces);
        EXPECT_EQ(data.GetUVs()[3][0], 12.0f);
        EXPECT_EQ(data.GetUVs()[6][0], 11.0f);
        EXPECT_EQ(data.GetIndices().size(), 9);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialMergeRemapsLocalSlotsWithoutTruncatingUUIDOrLosingOverrides)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CloneMesh(*m_whiteBox);
        const AZ::Uuid a = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555555}");
        const AZ::Uuid b = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555556}");
        for (auto face : Api::MeshFaceHandles(*m_whiteBox)) { Api::SetFaceSurfaceId(*m_whiteBox, face, a); }
        for (auto face : Api::MeshFaceHandles(*source)) { Api::SetFaceSurfaceId(*source, face, b); }
        AZ::Render::MaterialAssignmentMap sourceMaterials;
        auto& assignment = sourceMaterials[AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)];
        assignment.m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        assignment.m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV0");
        SurfaceMaterialMerge result;
        AZStd::string error;
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, {}, *source, {}, sourceMaterials, {}, true, result, error));
        const auto aSlot = FindSurfaceMaterialSlot(result.m_bindings, a);
        const auto bSlot = FindSurfaceMaterialSlot(result.m_bindings, b);
        EXPECT_NE(aSlot, bSlot);
        const auto& imported = result.m_materials.at(AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(bSlot));
        EXPECT_EQ(*AZStd::any_cast<float>(&imported.m_propertyOverrides.at(AZ::Name("roughness.factor"))), 0.375f);
        EXPECT_EQ(imported.m_matModUvOverrides.at(AZ::RHI::ShaderSemantic("UV", 0)), AZ::Name("UV0"));
        EXPECT_EQ(FindSurfaceMaterialSlot(result.m_bindings, AZ::Uuid::CreateRandom()), 0);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialConflictPreservesGeometryBindingsAndMaterialReferences)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CloneMesh(*m_whiteBox);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        AZ::Render::MaterialAssignmentMap materials;
        materials[AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.2f;
        SurfaceMaterialMerge output;
        const auto sentinel = AZ::Uuid::CreateRandom();
        output.m_bindings.push_back({sentinel, 99});
        output.m_materials = materials;
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, {}, *source, {}, materials, {}, true, output, error));
        EXPECT_NE(error.find("incompatible"), AZStd::string::npos);
        ASSERT_EQ(output.m_bindings.size(), 1);
        EXPECT_EQ(output.m_bindings[0].m_surfaceId, sentinel);
        EXPECT_EQ(output.m_bindings[0].m_materialSlot, 99);
        EXPECT_EQ(*AZStd::any_cast<float>(&output.m_materials.begin()->second.m_propertyOverrides.begin()->second), 0.2f);
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialOwnerGuardRejectsBeforeReplacingCandidate)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*source);
        AZ::Render::MaterialAssignmentMap materials;
        materials[AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.4f;
        SurfaceMaterialMerge result;
        result.m_bindings.push_back({AZ::Uuid::CreateRandom(), 8});
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, {}, *source, {}, materials, {}, false, result, error));
        EXPECT_NE(error.find("Material component"), AZStd::string::npos);
        ASSERT_EQ(result.m_bindings.size(), 1);
        EXPECT_EQ(result.m_bindings[0].m_materialSlot, 8);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SelectedPolygonUVTransformIsAtomicAndKeepsUnselectedCornersAndUUIDs)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto polygons = Api::MeshPolygonHandles(*m_whiteBox);
        const auto chosen = polygons.front();
        const auto untouched = Api::FaceHalfedgeHandles(*m_whiteBox, polygons.back().m_faceHandles.front()).front();
        const auto untouchedUV = Api::HalfedgeUV(*m_whiteBox, untouched);
        const auto corner = Api::FaceHalfedgeHandles(*m_whiteBox, chosen.m_faceHandles.front()).front();
        const auto oldUV = Api::HalfedgeUV(*m_whiteBox, corner);
        const auto id = Api::FaceSurfaceId(*m_whiteBox, chosen.m_faceHandles.front());
        EXPECT_TRUE(TransformPolygonUVs(*m_whiteBox, {chosen}, AZ::Vector2(2, 3), AZ::Vector2(5, 7), 0));
        EXPECT_EQ(Api::HalfedgeUV(*m_whiteBox, corner), AZ::Vector2(oldUV.GetX() * 2 + 5, oldUV.GetY() * 3 + 7));
        EXPECT_EQ(Api::HalfedgeUV(*m_whiteBox, untouched), untouchedUV);
        EXPECT_EQ(Api::FaceSurfaceId(*m_whiteBox, chosen.m_faceHandles.front()), id);
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        EXPECT_FALSE(TransformPolygonUVs(*m_whiteBox, {Api::PolygonHandle{{Api::FaceHandle{99999}}}}, AZ::Vector2::CreateOne(), AZ::Vector2::CreateZero(), 0));
        EXPECT_FALSE(TransformPolygonUVs(*m_whiteBox, {chosen}, AZ::Vector2::CreateOne(), AZ::Vector2::CreateZero(), AZStd::numeric_limits<float>::infinity()));
        EXPECT_FALSE(TransformPolygonUVs(*m_whiteBox, {chosen}, AZ::Vector2(AZStd::numeric_limits<float>::quiet_NaN(), 1), AZ::Vector2::CreateZero(), 0));
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialAssignmentChangesWholeInheritedSurfaceAndRejectsInvalidSelection)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto polygons = Api::MeshPolygonHandles(*m_whiteBox);
        SurfaceMaterialBindings bindings;
        EXPECT_TRUE(AssignPolygonMaterial(*m_whiteBox, {polygons.front()}, 12, bindings));
        for (auto face : polygons.front().m_faceHandles)
        {
            EXPECT_EQ(FindSurfaceMaterialSlot(bindings, Api::FaceSurfaceId(*m_whiteBox, face)), 12);
        }
        const auto before = bindings;
        EXPECT_FALSE(AssignPolygonMaterial(*m_whiteBox, {}, 9, bindings));
        EXPECT_EQ(bindings, before);
        EXPECT_FALSE(AssignPolygonMaterial(*m_whiteBox, {polygons.front()}, AZStd::numeric_limits<uint32_t>::max(), bindings));
        EXPECT_EQ(bindings, before);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, SceneSurfaceBindingsAndManualUVGuardSurviveSerializationClone)
    {
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygon = Api::MeshPolygonHandles(*mesh).front();
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygon}, 17));
        ASSERT_TRUE(m_whiteBoxComponent->ApplyPolygonUVTransform({polygon}, AZ::Vector2(2, 2), AZ::Vector2(3, 5), 0));
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        auto* clone = context->CloneObject(m_whiteBoxComponent);
        ASSERT_NE(clone, nullptr);
        EXPECT_TRUE(clone->HasManualUVs());
        EXPECT_EQ(clone->GetSurfaceMaterialBindings(), m_whiteBoxComponent->GetSurfaceMaterialBindings());
        delete clone;
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, m_whiteBoxComponent, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        auto* reopened = AZ::Utils::LoadObjectFromStream<WhiteBox::EditorWhiteBoxComponent>(stream, context);
        ASSERT_NE(reopened, nullptr);
        reopened->DeserializeWhiteBox();
        EXPECT_TRUE(reopened->HasManualUVs());
        EXPECT_EQ(reopened->GetSurfaceMaterialBindings(), m_whiteBoxComponent->GetSurfaceMaterialBindings());
        const auto corner = Api::FaceHalfedgeHandles(*mesh, polygon.m_faceHandles.front()).front();
        EXPECT_EQ(Api::HalfedgeUV(*reopened->GetWhiteBoxMesh(), corner), Api::HalfedgeUV(*mesh, corner));
        delete reopened;
    }
    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceMaterialBooleanChainRetainsBothOperandSlotsAndCornerUVs)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*source);
        AZStd::vector<AZ::Uuid> sourceIds;
        for (const auto face : Api::MeshFaceHandles(*source))
        {
            sourceIds.push_back(Api::FaceSurfaceId(*source, face));
            for (const auto corner : Api::FaceHalfedgeHandles(*source, face))
            {
                const auto p = Api::VertexPosition(*source, Api::HalfedgeVertexHandleAtTip(*source, corner));
                Api::SetHalfedgeUV(*source, corner, AZ::Vector2(2 * p.GetX() + 3 * p.GetY() + 5 * p.GetZ() + 17,
                    7 * p.GetX() + 11 * p.GetY() + 13 * p.GetZ() + 29));
            }
        }
        AZ::Render::MaterialAssignmentMap materials;
        materials[AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.65f;
        SurfaceMaterialMerge merged;
        AZStd::string error;
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, {}, *source, {}, materials, {}, true, merged, error));
        ASSERT_TRUE(Api::ApplyMeshBoolean(*m_whiteBox, *source, AZ::Transform::CreateTranslation(AZ::Vector3(0.5f, 0, 0)), Api::BooleanOperation::Union));
        bool targetFound = false, sourceFound = false;
        for (auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            const auto slot = FindSurfaceMaterialSlot(merged.m_bindings, Api::FaceSurfaceId(*m_whiteBox, face));
            targetFound |= slot == 0;
            sourceFound |= slot != 0;
        }
        EXPECT_TRUE(targetFound);
        EXPECT_TRUE(sourceFound);
        auto third = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*third);
        SurfaceMaterialMerge chain;
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBox, merged.m_bindings, merged.m_materials, {}, *third, {}, materials, {}, true, chain, error));
        const auto thirdId = Api::FaceSurfaceId(*third, Api::MeshFaceHandles(*third).front());
        EXPECT_NE(FindSurfaceMaterialSlot(chain.m_bindings, thirdId), 0);
        for (const auto& binding : merged.m_bindings)
        {
            EXPECT_EQ(FindSurfaceMaterialSlot(chain.m_bindings, binding.m_surfaceId), binding.m_materialSlot);
        }
        ASSERT_TRUE(Api::ApplyMeshBoolean(*m_whiteBox, *third, AZ::Transform::CreateTranslation(AZ::Vector3(0, 0.5f, 0)), Api::BooleanOperation::Union));
        size_t survivingSourceCorners = 0;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            const auto id = Api::FaceSurfaceId(*m_whiteBox, face);
            if (AZStd::find(sourceIds.begin(), sourceIds.end(), id) == sourceIds.end()) { continue; }
            EXPECT_NE(FindSurfaceMaterialSlot(chain.m_bindings, id), 0);
            for (const auto corner : Api::FaceHalfedgeHandles(*m_whiteBox, face))
            {
                const auto p = Api::VertexPosition(*m_whiteBox, Api::HalfedgeVertexHandleAtTip(*m_whiteBox, corner));
                const AZ::Vector2 expected(2 * (p.GetX() - 0.5f) + 3 * p.GetY() + 5 * p.GetZ() + 17,
                    7 * (p.GetX() - 0.5f) + 11 * p.GetY() + 13 * p.GetZ() + 29);
                EXPECT_EQ(Api::HalfedgeUV(*m_whiteBox, corner), expected);
                ++survivingSourceCorners;
            }
        }
        EXPECT_GT(survivingSourceCorners, 0);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, DuplicateSurfaceBindingsFailWithoutPublishingAnyPartialOutput)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CloneMesh(*m_whiteBox);
        const auto id = Api::FaceSurfaceId(*m_whiteBox, Api::MeshFaceHandles(*m_whiteBox).front());
        SurfaceMaterialMerge output;
        output.m_bindings.push_back({AZ::Uuid::CreateRandom(), 33});
        const auto before = output.m_bindings;
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {{id, 0}, {id, 1}}, {}, {}, *source, {}, {}, {}, true, output, error));
        EXPECT_EQ(output.m_bindings, before);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, ManualUVGuardRejectsVoxelGeometryBeforeMutation)
    {
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        auto polygon = Api::MeshPolygonHandles(*mesh).front();
        ASSERT_TRUE(m_whiteBoxComponent->ApplyPolygonUVTransform({polygon}, AZ::Vector2(2, 2), AZ::Vector2(3, 5), 0));
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_whiteBoxComponent->SetVoxelCell(AZ::Vector3(5, 5, 5), true);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, NativeMaterialAssignmentRoundtripKeepsAssetsPropertiesAndUVReferences)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        if (!context->FindClassData(azrtti_typeid<AZ::Render::MaterialAssignment>()))
        {
            AZ::Render::MaterialAssignment::Reflect(context);
        }
        AZ::Render::MaterialAssignmentMap materials;
        const auto key = AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(12);
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 47);
        materials[key] = AZ::Render::MaterialAssignment(assetId);
        materials[key].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.875f;
        materials[key].m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV0");
        auto sourceMesh = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*sourceMesh);
        SurfaceMaterialBindings sourceBindings;
        ASSERT_TRUE(AssignPolygonMaterial(*sourceMesh, Api::MeshPolygonHandles(*sourceMesh), 12, sourceBindings));
        SurfaceMaterialMerge merged;
        AZStd::string error;
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBoxComponent->GetWhiteBoxMesh(), {}, {}, {},
            *sourceMesh, sourceBindings, materials, {}, true, merged, error));
        const auto importedSlot = FindSurfaceMaterialSlot(merged.m_bindings,
            Api::FaceSurfaceId(*sourceMesh, Api::MeshFaceHandles(*sourceMesh).front()));
        const auto importedKey = AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(importedSlot);
        EXPECT_EQ(merged.m_materials.at(importedKey).m_materialAsset.GetId(), assetId);
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &merged.m_materials, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        auto* reopened = AZ::Utils::LoadObjectFromStream<AZ::Render::MaterialAssignmentMap>(stream, context);
        ASSERT_NE(reopened, nullptr);
        ASSERT_EQ(reopened->size(), 1);
        EXPECT_EQ(reopened->at(importedKey).m_materialAsset.GetId(), assetId);
        EXPECT_EQ(*AZStd::any_cast<float>(&reopened->at(importedKey).m_propertyOverrides.at(AZ::Name("roughness.factor"))), 0.875f);
        EXPECT_EQ(reopened->at(importedKey).m_matModUvOverrides.at(AZ::RHI::ShaderSemantic("UV", 0)), AZ::Name("UV0"));
        delete reopened;
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, LegacyFaceSchemaDefaultsToSlotZeroWithoutChangingCornerUVs)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        WhiteBox::WhiteBoxFace authored;
        authored.m_v1 = {AZ::Vector3(0, 0, 0), AZ::Vector2(3, 7)};
        authored.m_v2 = {AZ::Vector3(1, 0, 0), AZ::Vector2(9, 11)};
        authored.m_v3 = {AZ::Vector3(0, 1, 0), AZ::Vector2(13, 17)};
        authored.m_normal = AZ::Vector3::CreateAxisZ();
        authored.m_surfaceId = AZ::Uuid::CreateRandom();
        authored.m_materialSlot = 23;
        AZ::SerializeContext::DataElementNode legacy;
        ASSERT_TRUE(legacy.Convert<WhiteBox::WhiteBoxFace>(*context));
        ASSERT_TRUE(legacy.SetData(*context, authored));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("SurfaceId")));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("MaterialSlot")));
        legacy.SetVersion(1);
        WhiteBox::WhiteBoxFace reopened;
        ASSERT_TRUE(legacy.GetData(reopened));
        EXPECT_EQ(reopened.m_materialSlot, 0);
        EXPECT_TRUE(reopened.m_surfaceId.IsNull());
        EXPECT_EQ(reopened.m_v1.m_uv, authored.m_v1.m_uv);
        EXPECT_EQ(reopened.m_v2.m_uv, authored.m_v2.m_uv);
        EXPECT_EQ(reopened.m_v3.m_uv, authored.m_v3.m_uv);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, InvalidMaterialPartitionPreservesInputOrderAndCornerData)
    {
        WhiteBox::WhiteBoxFaces faces(2);
        faces[0].m_materialSlot = 7;
        faces[0].m_v1.m_uv = AZ::Vector2(2, 3);
        faces[1].m_materialSlot = AZStd::numeric_limits<uint32_t>::max();
        faces[1].m_v1.m_uv = AZ::Vector2(5, 7);
        EXPECT_TRUE(PartitionFacesByMaterial(faces).empty());
        EXPECT_EQ(faces[0].m_materialSlot, 7);
        EXPECT_EQ(faces[0].m_v1.m_uv, AZ::Vector2(2, 3));
        EXPECT_EQ(faces[1].m_v1.m_uv, AZ::Vector2(5, 7));
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, OwnerGuardAlsoPreservesExplicitOverridesEqualToLegacyFallback)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*source);
        const WhiteBox::WhiteBoxMaterial legacy;
        AZ::Render::MaterialAssignmentMap materials;
        auto& assignment = materials[AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)];
        assignment.m_propertyOverrides[AZ::Name("baseColor.color")] = AZ::Color(legacy.m_tint);
        assignment.m_propertyOverrides[AZ::Name("baseColor.useTexture")] = legacy.m_useTexture;
        SurfaceMaterialMerge output;
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, legacy, *source, {}, materials, legacy, false, output, error));
        EXPECT_NE(error.find("Material component"), AZStd::string::npos);
        EXPECT_TRUE(output.m_bindings.empty());
        EXPECT_TRUE(output.m_materials.empty());
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, DifferentLegacyTintAndTextureRequireOwnerAndImportExactFallback)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CreateWhiteBoxMesh();
        Api::InitializeAsUnitCube(*source);
        const WhiteBox::WhiteBoxMaterial targetLegacy;
        auto sourceLegacy = targetLegacy;
        sourceLegacy.m_tint = AZ::Vector3(0.2f, 0.4f, 0.8f);
        sourceLegacy.m_useTexture = !targetLegacy.m_useTexture;
        SurfaceMaterialMerge output;
        output.m_bindings.push_back({AZ::Uuid::CreateRandom(), 41});
        const auto before = output.m_bindings;
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, targetLegacy,
            *source, {}, {}, sourceLegacy, false, output, error));
        EXPECT_EQ(output.m_bindings, before);
        EXPECT_TRUE(output.m_materials.empty());
        EXPECT_NE(error.find("Material component"), AZStd::string::npos);
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, targetLegacy,
            *source, {}, {}, sourceLegacy, true, output, error));
        const auto slot = FindSurfaceMaterialSlot(output.m_bindings, Api::FaceSurfaceId(*source, Api::MeshFaceHandles(*source).front()));
        ASSERT_NE(slot, 0);
        ASSERT_EQ(output.m_materials.size(), 1);
        const auto& imported = output.m_materials.at(AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot));
        EXPECT_EQ(*AZStd::any_cast<AZ::Color>(&imported.m_propertyOverrides.at(AZ::Name("baseColor.color"))), AZ::Color(sourceLegacy.m_tint));
        EXPECT_EQ(*AZStd::any_cast<bool>(&imported.m_propertyOverrides.at(AZ::Name("baseColor.useTexture"))), sourceLegacy.m_useTexture);
        for (const auto face : Api::MeshFaceHandles(*source))
        {
            EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, Api::FaceSurfaceId(*source, face)), slot);
        }
        auto inherited = Api::CloneMesh(*m_whiteBox);
        output.m_bindings = before;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, targetLegacy,
            *inherited, {}, {}, sourceLegacy, true, output, error));
        EXPECT_EQ(output.m_bindings, before);
        EXPECT_NE(error.find("incompatible"), AZStd::string::npos);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SharedUUIDAssetOrUVOverrideConflictNeverReplacesCandidate)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto source = Api::CloneMesh(*m_whiteBox);
        const auto key = AZ::Render::MaterialAssignmentId::CreateFromStableIdOnly(0);
        AZ::Render::MaterialAssignmentMap targetMaterials, sourceMaterials;
        targetMaterials[key] = AZ::Render::MaterialAssignment(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 7));
        sourceMaterials[key] = AZ::Render::MaterialAssignment(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 7));
        SurfaceMaterialMerge output;
        output.m_bindings.push_back({AZ::Uuid::CreateRandom(), 41});
        output.m_materials = targetMaterials;
        const auto before = output.m_bindings;
        AZStd::string error;
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, targetMaterials, {},
            *source, {}, sourceMaterials, {}, true, output, error));
        EXPECT_EQ(output.m_bindings, before);
        EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, targetMaterials));
        EXPECT_NE(error.find("incompatible"), AZStd::string::npos);
        sourceMaterials = targetMaterials;
        sourceMaterials[key].m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV1");
        EXPECT_FALSE(MergeSurfaceMaterials(*m_whiteBox, {}, targetMaterials, {},
            *source, {}, sourceMaterials, {}, true, output, error));
        EXPECT_EQ(output.m_bindings, before);
        EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, targetMaterials));
        EXPECT_NE(error.find("incompatible"), AZStd::string::npos);
    }
}
