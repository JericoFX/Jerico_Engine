/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "WhiteBoxTestFixtures.h"
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Console/IConsole.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAssetCreator.h>
#include <JE/Render/RPI/Reflect/Material/MaterialTypeAssetCreator.h>
#include "Editor/JEWhiteBoxSurfaceMaterials.h"
#include "Rendering/Atom/WhiteBoxMeshAtomData.h"
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzCore/std/sort.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <JE/Render/Materials/MaterialComponentController.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/SurfaceDataProviderRequestBus.h>
#include <JE/Geometry/SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include "WhiteBoxSystemComponent.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"


namespace UnitTest
{
    namespace Api = JE::Geometry::Api;
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
        JE::Geometry::WhiteBoxFaces faces(3);
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
        const JE::Geometry::WhiteBoxMeshAtomData data(faces);
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
        JE::Render::MaterialAssignmentMap sourceMaterials;
        auto& assignment = sourceMaterials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)];
        assignment.m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        assignment.m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV0");
        SurfaceMaterialMerge result;
        AZStd::string error;
        ASSERT_TRUE(MergeSurfaceMaterials(*m_whiteBox, {}, {}, {}, *source, {}, sourceMaterials, {}, true, result, error));
        const auto aSlot = FindSurfaceMaterialSlot(result.m_bindings, a);
        const auto bSlot = FindSurfaceMaterialSlot(result.m_bindings, b);
        EXPECT_NE(aSlot, bSlot);
        const auto& imported = result.m_materials.at(JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(bSlot));
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
        JE::Render::MaterialAssignmentMap materials;
        materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.2f;
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
        JE::Render::MaterialAssignmentMap materials;
        materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.4f;
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
        auto* reopened = AZ::Utils::LoadObjectFromStream<JE::Geometry::EditorWhiteBoxComponent>(stream, context);
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
        JE::Render::MaterialAssignmentMap materials;
        materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.65f;
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
        struct WarningRecorder : AZ::Debug::TraceMessageBus::Handler
        {
            WarningRecorder() { BusConnect(); }
            ~WarningRecorder() { BusDisconnect(); }
            bool OnPreWarning(const char* window, const char*, int, const char*, const char* message) override
            {
                ++m_count;
                m_window = window;
                m_message = message;
                return true;
            }
            size_t m_count = 0;
            AZStd::string m_window;
            AZStd::string m_message;
        };
        {
            WarningRecorder warning;
            m_whiteBoxComponent->SetVoxelCell(AZ::Vector3(5, 5, 5), true);
            EXPECT_EQ(warning.m_count, 1);
            EXPECT_EQ(warning.m_window, "WhiteBox");
            EXPECT_EQ(warning.m_message,
                "Geometry edit rejected to preserve authored UVs. Use the White Box Inspector's Reset All UVs to Planar action before geometry edits that reproject UVs.");
        }
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, NativeMaterialAssignmentRoundtripKeepsAssetsPropertiesAndUVReferences)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        if (!context->FindClassData(azrtti_typeid<AZ::RHI::ShaderSemantic>()))
        {
            AZ::RHI::ShaderSemantic::Reflect(context);
        }
        if (!context->FindClassData(azrtti_typeid<JE::Render::MaterialAssignment>()))
        {
            JE::Render::MaterialAssignment::Reflect(context);
        }
        JE::Render::MaterialAssignmentMap materials;
        const auto key = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(12);
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 47);
        materials[key] = JE::Render::MaterialAssignment(assetId);
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
        const auto importedKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(importedSlot);
        EXPECT_EQ(merged.m_materials.at(importedKey).m_materialAsset.GetId(), assetId);
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &merged.m_materials, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        JE::Render::MaterialAssignmentMap reopened;
        // This binary reference roundtrip preserves dummy asset IDs without loading asset files.
        const AZ::ObjectStream::FilterDescriptor referenceOnly{AZ::Data::AssetFilterNoAssetLoading};
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, reopened, context, referenceOnly));
        ASSERT_EQ(reopened.size(), 1);
        EXPECT_EQ(reopened.at(importedKey).m_materialAsset.GetId(), assetId);
        EXPECT_EQ(*AZStd::any_cast<float>(&reopened.at(importedKey).m_propertyOverrides.at(AZ::Name("roughness.factor"))), 0.875f);
        EXPECT_EQ(reopened.at(importedKey).m_matModUvOverrides.at(AZ::RHI::ShaderSemantic("UV", 0)), AZ::Name("UV0"));
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, LegacyFaceSchemaDefaultsToSlotZeroWithoutChangingCornerUVs)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        JE::Geometry::WhiteBoxFace authored;
        authored.m_v1 = {AZ::Vector3(0, 0, 0), AZ::Vector2(3, 7)};
        authored.m_v2 = {AZ::Vector3(1, 0, 0), AZ::Vector2(9, 11)};
        authored.m_v3 = {AZ::Vector3(0, 1, 0), AZ::Vector2(13, 17)};
        authored.m_normal = AZ::Vector3::CreateAxisZ();
        authored.m_surfaceId = AZ::Uuid::CreateRandom();
        authored.m_materialSlot = 23;
        AZ::SerializeContext::DataElementNode legacy;
        ASSERT_TRUE(legacy.Convert<JE::Geometry::WhiteBoxFace>(*context));
        ASSERT_TRUE(legacy.SetData(*context, authored));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("SurfaceId")));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("MaterialSlot")));
        legacy.SetVersion(1);
        JE::Geometry::WhiteBoxFace reopened;
        ASSERT_TRUE(legacy.GetData(reopened));
        EXPECT_EQ(reopened.m_materialSlot, 0);
        EXPECT_TRUE(reopened.m_surfaceId.IsNull());
        EXPECT_EQ(reopened.m_v1.m_uv, authored.m_v1.m_uv);
        EXPECT_EQ(reopened.m_v2.m_uv, authored.m_v2.m_uv);
        EXPECT_EQ(reopened.m_v3.m_uv, authored.m_v3.m_uv);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, InvalidMaterialPartitionPreservesInputOrderAndCornerData)
    {
        JE::Geometry::WhiteBoxFaces faces(2);
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
        const JE::Geometry::WhiteBoxMaterial legacy;
        JE::Render::MaterialAssignmentMap materials;
        auto& assignment = materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0)];
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
        const JE::Geometry::WhiteBoxMaterial targetLegacy;
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
        const auto& imported = output.m_materials.at(JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot));
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
        const auto key = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0);
        JE::Render::MaterialAssignmentMap targetMaterials, sourceMaterials;
        targetMaterials[key] = JE::Render::MaterialAssignment(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 7));
        sourceMaterials[key] = JE::Render::MaterialAssignment(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 7));
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

    class NativeFaceMaterialPickerTestFixture : public EditorWhiteBoxComponentTestFixture
    {
        AZStd::unique_ptr<JE::Render::RPI::MaterialAssetHandler> m_materialHandler;
        AZStd::unique_ptr<JE::Render::RPI::MaterialTypeAssetHandler> m_typeHandler;
        void SetUpEditorFixtureImpl() override
        {
            EditorWhiteBoxComponentTestFixture::SetUpEditorFixtureImpl();
            auto& manager = AZ::Data::AssetManager::Instance();
            if (!manager.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialAsset>()))
            {
                m_materialHandler = JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialAssetHandler>();
            }
            if (!manager.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialTypeAsset>()))
            {
                m_typeHandler = JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialTypeAssetHandler>();
            }
        }
        void TearDownEditorFixtureImpl() override
        {
            EditorWhiteBoxComponentTestFixture::TearDownEditorFixtureImpl();
            m_materialHandler.reset();
            m_typeHandler.reset();
        }
    };

    namespace
    {
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> CreatePickerMaterial(bool reversedEnum = false)
        {
            JE::Render::RPI::MaterialTypeAssetCreator typeCreator;
            typeCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));
            typeCreator.SetVersion(1);
            typeCreator.BeginMaterialProperty(AZ::Name("roughness.factor"), JE::Render::RPI::MaterialPropertyDataType::Float);
            typeCreator.EndMaterialProperty();
            typeCreator.SetPropertyValue(AZ::Name("roughness.factor"), 0.5f);
            typeCreator.BeginMaterialProperty(AZ::Name("surface.mode"), JE::Render::RPI::MaterialPropertyDataType::Enum);
            typeCreator.SetMaterialPropertyEnumNames(reversedEnum ? AZStd::vector<AZStd::string>{"masked", "opaque"} : AZStd::vector<AZStd::string>{"opaque", "masked"});
            typeCreator.EndMaterialProperty();
            typeCreator.SetPropertyValue(AZ::Name("surface.mode"), uint32_t{0});
            typeCreator.AddUvName(AZ::RHI::ShaderSemantic("UV", 0), AZ::Name("uv"));
            AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset> type;
            EXPECT_TRUE(typeCreator.End(type));
            if (!type.IsReady()) { return {}; }
            JE::Render::RPI::MaterialAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0), type);
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> result;
            EXPECT_TRUE(creator.End(result));
            return result;
        }
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, MaterialPickerSeparatesSharedSlotsAndPreservesFullUUIDCornersAndOverrides)
    {
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        ASSERT_GE(polygons.size(), 2);
        const auto a = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555555}");
        const auto b = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555556}");
        for (const auto face : polygons[0].m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, a); }
        for (const auto face : polygons[1].m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, b); }
        SurfaceMaterialBindings bindings{{a, 7}, {b, 7}};
        const auto oldKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(7);
        JE::Render::MaterialAssignmentMap materials;
        materials[oldKey] = JE::Render::MaterialAssignment(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));
        materials[oldKey].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        materials[oldKey].m_propertyOverrides[AZ::Name("surface.mode")] = AZ::Name("masked");
        materials[oldKey].m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV0");
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(mesh, before));
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, bindings, materials, material,
            {AZ::Name("UV0")}, true, output, error)) << error.c_str();
        const auto slot = FindSurfaceMaterialSlot(output.m_bindings, a);
        ASSERT_NE(slot, 7);
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, b), 7);
        EXPECT_TRUE(MaterialAssignmentsEquivalent({{oldKey, output.m_materials.at(oldKey)}}, materials));
        const auto& assigned = output.m_materials.at(JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot));
        EXPECT_EQ(assigned.m_materialAsset.GetId(), material.GetId());
        EXPECT_EQ(*AZStd::any_cast<float>(&assigned.m_propertyOverrides.at(AZ::Name("roughness.factor"))), 0.375f);
        EXPECT_EQ(*AZStd::any_cast<AZ::Name>(&assigned.m_propertyOverrides.at(AZ::Name("surface.mode"))), AZ::Name("masked"));
        EXPECT_EQ(assigned.m_matModUvOverrides.at(AZ::RHI::ShaderSemantic("UV", 0)), AZ::Name("UV0"));
        ASSERT_TRUE(Api::WriteMesh(mesh, after));
        EXPECT_EQ(before, after);
        auto clone = Api::CloneMesh(mesh);
        EXPECT_EQ(Api::FaceSurfaceId(*clone, polygons[0].m_faceHandles.front()), a);
        EXPECT_EQ(Api::FaceSurfaceId(*clone, polygons[1].m_faceHandles.front()), b);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, MaterialPickerKeepsDifferentOverridesInDistinctNewSlots)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        ASSERT_GE(polygons.size(), 2);
        const auto a = Api::FaceSurfaceId(mesh, polygons[0].m_faceHandles.front());
        const auto b = Api::FaceSurfaceId(mesh, polygons[1].m_faceHandles.front());
        SurfaceMaterialBindings bindings{{a, 2}, {b, 3}};
        JE::Render::MaterialAssignmentMap materials;
        for (const auto slot : {2u, 3u})
        {
            materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot)].m_propertyOverrides[AZ::Name("roughness.factor")] =
                slot == 2 ? 0.25f : 0.75f;
        }
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0], polygons[1]}, bindings, materials,
            material, {AZ::Name("UV0")}, true, output, error));
        const auto aSlot = FindSurfaceMaterialSlot(output.m_bindings, a);
        const auto bSlot = FindSurfaceMaterialSlot(output.m_bindings, b);
        EXPECT_NE(aSlot, bSlot);
        EXPECT_NE(aSlot, 2);
        EXPECT_NE(bSlot, 3);
        for (const auto slot : {aSlot, bSlot})
        {
            EXPECT_EQ(output.m_materials.at(JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot)).m_materialAsset.GetId(), material.GetId());
        }
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, MaterialPickerRejectsIncompatibleOverridesWithoutPartialPublication)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        const auto key = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0);
        AZStd::vector<JE::Render::MaterialAssignmentMap> invalid(6);
        invalid[0][key].m_propertyOverrides[AZ::Name("absent.property")] = 0.5f;
        invalid[1][key].m_propertyOverrides[AZ::Name("roughness.factor")] = true;
        invalid[2][key].m_propertyOverrides[AZ::Name("surface.mode")] = AZ::Name("absent");
        invalid[3][key].m_propertyOverrides[AZ::Name("surface.mode")] = uint32_t{7};
        invalid[4][key].m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 1)] = AZ::Name("UV0");
        invalid[5][key].m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 0)] = AZ::Name("UV1");
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(mesh, before));
        for (const auto& input : invalid)
        {
            SurfaceMaterialMerge output{{{AZ::Uuid::CreateRandom(), 91}}, input};
            const auto sentinel = output.m_bindings;
            AZStd::string error;
            EXPECT_FALSE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, {}, input, material,
                {AZ::Name("UV0")}, true, output, error));
            EXPECT_FALSE(error.empty());
            EXPECT_EQ(output.m_bindings, sentinel);
            EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, input));
        }
        ASSERT_TRUE(Api::WriteMesh(mesh, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, MaterialPickerMissingOwnerAssetOrSelectionKeepsCandidateAndComponentUnchanged)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        SurfaceMaterialMerge output{{{AZ::Uuid::CreateRandom(), 91}}, {}};
        const auto sentinel = output.m_bindings;
        AZStd::string error;
        EXPECT_FALSE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, {}, {}, material, {}, false, output, error));
        EXPECT_NE(error.find("Material component"), AZStd::string::npos);
        EXPECT_FALSE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, {}, {}, {}, {}, true, output, error));
        EXPECT_FALSE(PreparePolygonMaterialAssignment(mesh, {}, {}, {}, material, {}, true, output, error));
        EXPECT_EQ(output.m_bindings, sentinel);
        EXPECT_TRUE(output.m_materials.empty());
        const auto componentBefore = m_whiteBoxComponent->GetSurfaceMaterialBindings();
        EXPECT_FALSE(m_whiteBoxComponent->AssignPolygonMaterialAsset({polygons[0]}, material, error));
        EXPECT_EQ(m_whiteBoxComponent->GetSurfaceMaterialBindings(), componentBefore);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, MaterialPickerAssignmentRoundtripKeepsNativeReferencesAndSurfaceBindings)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {Api::MeshPolygonHandles(mesh).front()}, {}, {},
            material, {AZ::Name("UV0")}, true, output, error));
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        if (!context->FindClassData(azrtti_typeid<AZ::RHI::ShaderSemantic>()))
        {
            AZ::RHI::ShaderSemantic::Reflect(context);
        }
        if (!context->FindClassData(azrtti_typeid<JE::Render::MaterialAssignment>())) { JE::Render::MaterialAssignment::Reflect(context); }
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &output.m_materials, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        JE::Render::MaterialAssignmentMap reopened;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, reopened, context));
        EXPECT_TRUE(MaterialAssignmentsEquivalent(reopened, output.m_materials));
        AZStd::vector<AZ::u8> bindingBytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> bindingStream(&bindingBytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(bindingStream, AZ::DataStream::ST_BINARY, &output.m_bindings, context));
        bindingStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        SurfaceMaterialBindings reopenedBindings;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(bindingStream, reopenedBindings, context));
        EXPECT_EQ(reopenedBindings, output.m_bindings);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, LegacySlotFieldRemainsSerializableAndPickerDefaultsEmpty)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        AZ::SerializeContext::DataElementNode legacy;
        ASSERT_TRUE(legacy.Convert<JE::Geometry::EditorWhiteBoxComponent>(*context));
        ASSERT_TRUE(legacy.SetData(*context, *m_whiteBoxComponent));
        const auto slotIndex = legacy.FindElement(AZ_CRC_CE("SelectedMaterialSlot"));
        ASSERT_NE(slotIndex, -1);
        ASSERT_TRUE(legacy.GetSubElement(slotIndex).SetData(*context, uint32_t{37}));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("SelectedFaceMaterial")));
        legacy.SetVersion(3);
        JE::Geometry::EditorWhiteBoxComponent reopened;
        ASSERT_TRUE(legacy.GetData(reopened));
        AZ::SerializeContext::DataElementNode current;
        ASSERT_TRUE(current.Convert<JE::Geometry::EditorWhiteBoxComponent>(*context));
        ASSERT_TRUE(current.SetData(*context, reopened));
        uint32_t slot = 0;
        ASSERT_TRUE(current.GetSubElement(current.FindElement(AZ_CRC_CE("SelectedMaterialSlot"))).GetData(slot));
        EXPECT_EQ(slot, 37);
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> picker;
        ASSERT_TRUE(current.GetSubElement(current.FindElement(AZ_CRC_CE("SelectedFaceMaterial"))).GetData(picker));
        EXPECT_FALSE(picker.GetId().IsValid());
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, NumericEnumOverridesRetainTheirOriginalMeaningOrRejectAtomically)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygon = Api::MeshPolygonHandles(mesh).front();
        const auto oldMaterial = CreatePickerMaterial();
        const auto compatible = CreatePickerMaterial();
        const auto incompatible = CreatePickerMaterial(true);
        ASSERT_TRUE(oldMaterial.IsReady());
        ASSERT_TRUE(compatible.IsReady());
        ASSERT_TRUE(incompatible.IsReady());
        JE::Render::MaterialAssignmentMap materials;
        const auto key = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(0);
        materials[key] = JE::Render::MaterialAssignment(oldMaterial);
        materials[key].m_propertyOverrides[AZ::Name("surface.mode")] = uint32_t{0};
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygon}, {}, materials, compatible, {}, true, output, error));
        const auto bindings = output.m_bindings;
        const auto assignments = output.m_materials;
        EXPECT_FALSE(PreparePolygonMaterialAssignment(mesh, {polygon}, {}, materials, incompatible, {}, true, output, error));
        EXPECT_EQ(output.m_bindings, bindings);
        EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, assignments));
        EXPECT_NE(error.find("retain its meaning"), AZStd::string::npos);
    }


    TEST_F(NativeFaceMaterialPickerTestFixture, RepeatedMaterialChoiceReusesEffectiveSlotAndKeepsMapAndIdsStable)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygon = Api::MeshPolygonHandles(mesh).front();
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygon}, {}, {}, material,
            {}, true, output, error));
        const auto bindings = output.m_bindings;
        const auto materials = output.m_materials;
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            SurfaceMaterialMerge next;
            ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygon}, output.m_bindings, output.m_materials,
                material, {}, true, next, error));
            EXPECT_EQ(next.m_bindings, bindings);
            EXPECT_EQ(next.m_materials.size(), materials.size());
            EXPECT_TRUE(MaterialAssignmentsEquivalent(next.m_materials, materials));
            output = AZStd::move(next);
        }
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, ExistingSlotReuseResolvesLodOverrideInsteadOfComparingStableEntryAlone)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        ASSERT_GE(polygons.size(), 2);
        const auto selected = Api::FaceSurfaceId(mesh, polygons[0].m_faceHandles.front());
        const auto unselected = Api::FaceSurfaceId(mesh, polygons[1].m_faceHandles.front());
        const auto material = CreatePickerMaterial();
        const auto oldMaterial = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        ASSERT_TRUE(oldMaterial.IsReady());
        SurfaceMaterialBindings bindings{{selected, 2}, {unselected, 7}};
        JE::Render::MaterialAssignmentMap materials;
        const auto oldKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(2);
        const auto stableKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(7);
        const auto lodKey = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, 7);
        materials[oldKey] = JE::Render::MaterialAssignment(oldMaterial);
        materials[oldKey].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        materials[stableKey] = JE::Render::MaterialAssignment(material);
        materials[stableKey].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        materials[lodKey] = JE::Render::MaterialAssignment(material);
        materials[lodKey].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.875f;
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, bindings, materials,
            material, {}, true, output, error));
        EXPECT_NE(FindSurfaceMaterialSlot(output.m_bindings, selected), 7);
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, unselected), 7);
        EXPECT_EQ(*AZStd::any_cast<float>(&output.m_materials.at(lodKey).m_propertyOverrides.at(AZ::Name("roughness.factor"))), 0.875f);
        EXPECT_EQ(output.m_materials.size(), materials.size() + 1);
        // A compatible stable slot may be reused only once its LOD0 override no longer disagrees.
        materials[lodKey] = materials[stableKey];
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, bindings, materials,
            material, {}, true, output, error));
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, selected), 7);
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, unselected), 7);
        EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, materials));
        // With no slot-specific entry, default is the effective association and may be reused.
        materials.erase(stableKey);
        materials.erase(lodKey);
        materials[JE::Render::MaterialAssignmentId::CreateDefault()] = JE::Render::MaterialAssignment(material);
        materials[JE::Render::MaterialAssignmentId::CreateDefault()].m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0]}, bindings, materials,
            material, {}, true, output, error));
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, selected), 0);
        EXPECT_EQ(FindSurfaceMaterialSlot(output.m_bindings, unselected), 7);
        EXPECT_TRUE(MaterialAssignmentsEquivalent(output.m_materials, materials));
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, UniqueInstancesStayInDistinctSlotsWhileSameSlotReassignmentIsIdempotent)
    {
        const auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto polygons = Api::MeshPolygonHandles(mesh);
        ASSERT_GE(polygons.size(), 2);
        const auto a = Api::FaceSurfaceId(mesh, polygons[0].m_faceHandles.front());
        const auto b = Api::FaceSurfaceId(mesh, polygons[1].m_faceHandles.front());
        const auto oldMaterial = CreatePickerMaterial();
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(oldMaterial.IsReady());
        ASSERT_TRUE(material.IsReady());
        SurfaceMaterialBindings bindings{{a, 2}, {b, 3}};
        JE::Render::MaterialAssignmentMap materials;
        for (const auto slot : {2u, 3u, 7u})
        {
            auto& assignment = materials[JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot)];
            assignment = JE::Render::MaterialAssignment(slot == 7 ? material : oldMaterial);
            assignment.m_materialInstanceMustBeUnique = true;
        }
        SurfaceMaterialMerge output;
        AZStd::string error;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0], polygons[1]}, bindings, materials,
            material, {}, true, output, error));
        const auto aSlot = FindSurfaceMaterialSlot(output.m_bindings, a);
        const auto bSlot = FindSurfaceMaterialSlot(output.m_bindings, b);
        EXPECT_NE(aSlot, bSlot);
        EXPECT_NE(aSlot, 7);
        EXPECT_NE(bSlot, 7);
        EXPECT_EQ(output.m_materials.size(), materials.size() + 2);
        for (const auto slot : {aSlot, bSlot})
        {
            EXPECT_TRUE(output.m_materials.at(JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot)).m_materialInstanceMustBeUnique);
        }
        SurfaceMaterialMerge repeated;
        ASSERT_TRUE(PreparePolygonMaterialAssignment(mesh, {polygons[0], polygons[1]}, output.m_bindings, output.m_materials,
            material, {}, true, repeated, error));
        EXPECT_EQ(repeated.m_bindings, output.m_bindings);
        EXPECT_EQ(repeated.m_materials.size(), output.m_materials.size());
        EXPECT_TRUE(MaterialAssignmentsEquivalent(repeated.m_materials, output.m_materials));
    }


    namespace
    {
        const AZ::Uuid SurfaceDropTopId = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555555}");
        const AZ::Uuid SurfaceDropSideId = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555556}");

        struct SurfaceDropPolygons
        {
            Api::PolygonHandle m_top;
            Api::PolygonHandle m_side;
        };

        SurfaceDropPolygons PrepareSurfaceDropCube(JE::Geometry::EditorWhiteBoxComponent& component)
        {
            auto& mesh = *component.GetWhiteBoxMesh();
            SurfaceDropPolygons selected;
            for (const auto& polygon : Api::MeshPolygonHandles(mesh))
            {
                EXPECT_FALSE(polygon.m_faceHandles.empty());
                if (polygon.m_faceHandles.empty()) { continue; }
                const auto nativeId = Api::FaceSurfaceId(mesh, polygon.m_faceHandles.front());
                EXPECT_FALSE(nativeId.IsNull());
                for (const auto face : polygon.m_faceHandles)
                {
                    // Characterize the real cube fixture before replacing either test identity.
                    EXPECT_EQ(Api::FaceSurfaceId(mesh, face), nativeId);
                }
                const auto normal = Api::FaceNormal(mesh, polygon.m_faceHandles.front());
                if (normal.IsClose(AZ::Vector3::CreateAxisZ())) { selected.m_top = polygon; }
                if (normal.IsClose(AZ::Vector3::CreateAxisX())) { selected.m_side = polygon; }
            }
            for (const auto face : selected.m_top.m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, SurfaceDropTopId); }
            for (const auto face : selected.m_side.m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, SurfaceDropSideId); }
            component.RebuildWhiteBox();
            return selected;
        }

        AzFramework::RenderGeometry::RayRequest SurfaceDropRay(AZ::EntityId entityId, float worldX = 0.0f)
        {
            AzFramework::RenderGeometry::RayRequest ray;
            ray.m_startWorldPosition = AZ::Vector3(worldX + 0.125f, 0.2f, 2.0f);
            ray.m_endWorldPosition = AZ::Vector3(worldX + 0.125f, 0.2f, -2.0f);
            ray.m_entityFilter.m_filterFunction = [entityId](AZ::EntityId candidate) { return candidate == entityId; };
            return ray;
        }

        AzFramework::EntityContextId SurfaceDropContext(AZ::EntityId entityId)
        {
            auto contextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::EntityIdContextQueryBus::EventResult(
                contextId, entityId, &AzFramework::EntityIdContextQueries::GetOwningContextId);
            return contextId;
        }

        AzFramework::RenderGeometry::RayResult QuerySurfaceDropHit(
            AzFramework::EntityContextId contextId, const AzFramework::RenderGeometry::RayRequest& ray)
        {
            AzFramework::RenderGeometry::RayResult result;
            AzFramework::RenderGeometry::IntersectorBus::EventResult(
                result, contextId, &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, ray);
            return result;
        }

        struct ScopedSurfaceDropMaterialOwner
        {
            explicit ScopedSurfaceDropMaterialOwner(AZ::EntityId entityId) { m_controller.Activate(entityId); }
            ~ScopedSurfaceDropMaterialOwner() { m_controller.Deactivate(); }
            JE::Render::MaterialComponentController m_controller;
        };

        struct SurfaceDropState
        {
            Api::WhiteBoxMeshStream m_mesh;
            SurfaceMaterialBindings m_bindings;
            JE::Render::MaterialAssignmentMap m_materials;
        };

        SurfaceDropState CaptureSurfaceDropState(
            JE::Geometry::EditorWhiteBoxComponent& component, const JE::Render::MaterialComponentController& owner)
        {
            SurfaceDropState state;
            EXPECT_TRUE(Api::WriteMesh(*component.GetWhiteBoxMesh(), state.m_mesh));
            state.m_bindings = component.GetSurfaceMaterialBindings();
            state.m_materials = owner.GetMaterialMapCopy();
            return state;
        }

        void ExpectSurfaceDropStateUnchanged(JE::Geometry::EditorWhiteBoxComponent& component,
            const JE::Render::MaterialComponentController& owner, const SurfaceDropState& before)
        {
            Api::WhiteBoxMeshStream after;
            ASSERT_TRUE(Api::WriteMesh(*component.GetWhiteBoxMesh(), after));
            EXPECT_EQ(after, before.m_mesh);
            EXPECT_EQ(component.GetSurfaceMaterialBindings(), before.m_bindings);
            EXPECT_TRUE(MaterialAssignmentsEquivalent(owner.GetMaterialMapCopy(), before.m_materials));
        }

        bool AssignSurfaceDropMaterial(JE::Geometry::EditorWhiteBoxComponent& component, const AZ::Uuid& surfaceId,
            const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& material, AZStd::string& error)
        {
            bool result = false;
            JE::Geometry::EditorWhiteBoxComponentRequestBus::EventResult(
                result, AZ::EntityComponentIdPair(component.GetEntityId(), component.GetId()),
                &JE::Geometry::EditorWhiteBoxComponentRequests::AssignSurfaceMaterialAsset, surfaceId, material, error);
            return result;
        }
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, SurfaceDropIntersectorRetainsFullUuidAndExactComponent)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_EQ(polygons.m_top.m_faceHandles.size(), 2);
        ASSERT_EQ(polygons.m_side.m_faceHandles.size(), 2);
        const auto contextId = SurfaceDropContext(m_whiteBoxEntityId);
        ASSERT_FALSE(contextId.IsNull());
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(contextId));
        const auto ray = SurfaceDropRay(m_whiteBoxEntityId);
        const auto hit = QuerySurfaceDropHit(contextId, ray);
        ASSERT_TRUE(static_cast<bool>(hit));
        EXPECT_EQ(hit.m_entityAndComponent, AZ::EntityComponentIdPair(m_whiteBoxEntityId, m_whiteBoxComponent->GetId()));
        ASSERT_TRUE(hit.m_surfaceId.has_value());
        EXPECT_EQ(*hit.m_surfaceId, SurfaceDropTopId);
        EXPECT_FALSE(hit.m_materialSlotId.has_value());
        EXPECT_NEAR(hit.m_distance, 1.5f, 1e-5f);
        EXPECT_THAT(hit.m_worldPosition, IsClose(AZ::Vector3(0.125f, 0.2f, 0.5f)));
        EXPECT_THAT(hit.m_worldNormal, IsClose(AZ::Vector3::CreateAxisZ()));

        float legacyDistance = 1000.0f;
        ASSERT_TRUE(m_whiteBoxComponent->EditorSelectionIntersectRayViewport(
            AzFramework::ViewportInfo{0}, ray.m_startWorldPosition, AZ::Vector3(0.0f, 0.0f, -2.0f), legacyDistance));
        EXPECT_NEAR(legacyDistance, 0.75f, 1e-5f);

        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);
        EXPECT_FALSE(static_cast<bool>(QuerySurfaceDropHit(contextId, ray)));
        auto includingHidden = ray;
        includingHidden.m_onlyVisible = false;
        EXPECT_TRUE(static_cast<bool>(QuerySurfaceDropHit(contextId, includingHidden)));
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, true);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, SurfaceDropIntersectorRefreshesBoundsGeometryAndActivation)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        const auto contextId = SurfaceDropContext(m_whiteBoxEntityId);
        ASSERT_FALSE(contextId.IsNull());
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(contextId));
        const auto originalRay = SurfaceDropRay(m_whiteBoxEntityId);
        ASSERT_TRUE(static_cast<bool>(QuerySurfaceDropHit(contextId, originalRay))); // Populate broadphase and triangle caches.

        AZ::TransformBus::Event(m_whiteBoxEntityId, &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)));
        EXPECT_FALSE(static_cast<bool>(QuerySurfaceDropHit(contextId, originalRay)));
        const auto transformedRay = SurfaceDropRay(m_whiteBoxEntityId, 10.0f);
        ASSERT_TRUE(static_cast<bool>(QuerySurfaceDropHit(contextId, transformedRay)));

        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        for (const auto vertex : Api::MeshVertexHandles(mesh))
        {
            Api::SetVertexPosition(mesh, vertex, Api::VertexPosition(mesh, vertex) + AZ::Vector3(4.0f, 0.0f, 0.0f));
        }
        m_whiteBoxComponent->RebuildWhiteBox();
        EXPECT_FALSE(static_cast<bool>(QuerySurfaceDropHit(contextId, transformedRay)));
        const auto rebuiltRay = SurfaceDropRay(m_whiteBoxEntityId, 14.0f);
        const auto rebuiltHit = QuerySurfaceDropHit(contextId, rebuiltRay);
        ASSERT_TRUE(static_cast<bool>(rebuiltHit));
        ASSERT_TRUE(rebuiltHit.m_surfaceId.has_value());
        EXPECT_EQ(*rebuiltHit.m_surfaceId, SurfaceDropTopId);
        EXPECT_NEAR(rebuiltHit.m_distance, 1.5f, 1e-5f);

        // Keep the actual edited mesh across normal component deactivation/reactivation.
        m_whiteBoxComponent->SerializeWhiteBox();
        auto* entity = m_whiteBoxComponent->GetEntity();
        entity->Deactivate();
        EXPECT_FALSE(static_cast<bool>(QuerySurfaceDropHit(contextId, rebuiltRay)));
        entity->Activate();
        const auto reactivatedHit = QuerySurfaceDropHit(contextId, rebuiltRay);
        ASSERT_TRUE(static_cast<bool>(reactivatedHit));
        ASSERT_TRUE(reactivatedHit.m_surfaceId.has_value());
        EXPECT_EQ(*reactivatedHit.m_surfaceId, SurfaceDropTopId);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropBusSeparatesFullUuidsSharingOneMaterialSlot)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_FALSE(polygons.m_side.m_faceHandles.empty());
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygons.m_top, polygons.m_side}, 7));
        const auto oldMaterial = CreatePickerMaterial();
        const auto newMaterial = CreatePickerMaterial();
        ASSERT_TRUE(oldMaterial.IsReady());
        ASSERT_TRUE(newMaterial.IsReady());
        const auto oldKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(7);
        JE::Render::MaterialAssignment assignment;
        assignment.m_materialAsset = oldMaterial;
        assignment.m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
        owner.m_controller.SetMaterialMap({{oldKey, assignment}});
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);

        AZStd::string error;
        ASSERT_TRUE(AssignSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, newMaterial, error)) << error.c_str();
        const auto& bindings = m_whiteBoxComponent->GetSurfaceMaterialBindings();
        const auto topSlot = FindSurfaceMaterialSlot(bindings, SurfaceDropTopId);
        EXPECT_NE(topSlot, 7);
        EXPECT_EQ(FindSurfaceMaterialSlot(bindings, SurfaceDropSideId), 7);
        const auto materials = owner.m_controller.GetMaterialMapCopy();
        ASSERT_NE(materials.find(oldKey), materials.end());
        EXPECT_TRUE(MaterialAssignmentsEquivalent({{oldKey, materials.at(oldKey)}}, before.m_materials));
        const auto topKey = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(topSlot);
        ASSERT_NE(materials.find(topKey), materials.end());
        const auto& applied = materials.at(topKey);
        EXPECT_EQ(applied.m_materialAsset.GetId(), newMaterial.GetId());
        const auto value = applied.m_propertyOverrides.find(AZ::Name("roughness.factor"));
        ASSERT_NE(value, applied.m_propertyOverrides.end());
        const auto* factor = AZStd::any_cast<float>(&value->second);
        ASSERT_NE(factor, nullptr);
        EXPECT_FLOAT_EQ(*factor, 0.375f);
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBoxComponent->GetWhiteBoxMesh(), after));
        EXPECT_EQ(after, before.m_mesh); // Includes every surface UUID and corner UV.

        // Repeating the same explicit assignment is a no-op on both bindings and native owner storage.
        const auto assigned = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        ASSERT_TRUE(AssignSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, newMaterial, error)) << error.c_str();
        ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, assigned);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropBusRejectsNullAndStaleUuidWithoutMutation)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        const auto contextId = SurfaceDropContext(m_whiteBoxEntityId);
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(contextId));
        const auto hit = QuerySurfaceDropHit(contextId, SurfaceDropRay(m_whiteBoxEntityId));
        ASSERT_TRUE(static_cast<bool>(hit));
        ASSERT_TRUE(hit.m_surfaceId.has_value());
        ASSERT_EQ(*hit.m_surfaceId, SurfaceDropTopId);
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygons.m_top, polygons.m_side}, 7));
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());

        // Invalidate the real picked identity before dispatch, without reusing an old PolygonHandle as a target.
        const auto replacement = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555557}");
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        for (const auto face : polygons.m_top.m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, replacement); }
        m_whiteBoxComponent->RebuildWhiteBox();
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        for (const auto& id : {AZ::Uuid::CreateNull(), *hit.m_surfaceId})
        {
            AZStd::string error;
            EXPECT_FALSE(AssignSurfaceDropMaterial(*m_whiteBoxComponent, id, material, error));
            EXPECT_FALSE(error.empty());
            ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, before);
        }
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropBusRejectsMixedUuidPolygonWithoutMutation)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_EQ(polygons.m_top.m_faceHandles.size(), 2);
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        Api::SetFaceSurfaceId(mesh, polygons.m_top.m_faceHandles.back(), SurfaceDropSideId);
        m_whiteBoxComponent->RebuildWhiteBox();
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        AZStd::string error;
        EXPECT_FALSE(AssignSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, material, error));
        EXPECT_FALSE(error.empty());
        ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, before);
    }


    namespace
    {
        bool RequestSurfaceDropMaterial(JE::Geometry::EditorWhiteBoxComponent& component,
            const AZ::Uuid& surfaceId, const AZ::Data::AssetId& assetId, AZStd::string& error)
        {
            bool result = false;
            JE::Geometry::EditorWhiteBoxComponentRequestBus::EventResult(
                result, AZ::EntityComponentIdPair(component.GetEntityId(), component.GetId()),
                &JE::Geometry::EditorWhiteBoxComponentRequests::RequestSurfaceMaterialAsset, surfaceId, assetId, error);
            return result;
        }

        void FinishSurfaceDropFixtureSetup(AZ::EntityId entityId, JE::Geometry::EditorWhiteBoxComponent*& component)
        {
            const auto componentId = component->GetId();
            component->SerializeWhiteBox();
            auto* prefab = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
            ASSERT_NE(prefab, nullptr);
            {
                AzToolsFramework::ScopedUndoBatch setup("Prepare surface drop fixture");
                const auto updated = prefab->GenerateUndoNodesForEntityChangeAndUpdateCache(entityId, setup.GetUndoBatch());
                ASSERT_TRUE(updated.IsSuccess());
            }
            // Finish setup propagation before requests; later external propagation must still cancel them.
            auto* updates = AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get();
            ASSERT_NE(updates, nullptr);
            ASSERT_TRUE(updates->UpdateTemplateInstancesInQueue(true));
            auto* entity = AzToolsFramework::GetEntityById(entityId);
            ASSERT_NE(entity, nullptr);
            component = entity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>();
            ASSERT_NE(component, nullptr);
            ASSERT_EQ(component->GetId(), componentId);
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::FlushUndo);
        }

        struct SurfaceDropWarningRecorder : AZ::Debug::TraceMessageBus::Handler
        {
            SurfaceDropWarningRecorder() { BusConnect(); }
            ~SurfaceDropWarningRecorder() { BusDisconnect(); }
            bool OnPreWarning(const char* window, const char*, int, const char*, const char* message) override
            {
                ++m_count;
                m_window = window;
                m_message = message;
                return true;
            }
            void ExpectOne(const char* expectedMessage) const
            {
                EXPECT_EQ(m_count, 1);
                EXPECT_EQ(m_window, "WhiteBoxSurfaceMaterial");
                EXPECT_EQ(m_message, expectedMessage);
            }
            size_t m_count = 0;
            AZStd::string m_window;
            AZStd::string m_message;
        };

        void DrainSurfaceDropMaterialCallbacks()
        {
            // Real asset delivery, then loader callback, then a separate queued owner completion.
            AZ::Data::AssetManager::Instance().DispatchEvents();
            AZ::SystemTickBus::ExecuteQueuedEvents();
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickEvents::OnSystemTick);
            AZ::SystemTickBus::ExecuteQueuedEvents();
        }

        struct UnownedSurfaceDropState
        {
            Api::WhiteBoxMeshStream m_mesh;
            SurfaceMaterialBindings m_bindings;
            AZStd::vector<AZ::ComponentId> m_components;
        };

        UnownedSurfaceDropState CaptureUnownedSurfaceDropState(JE::Geometry::EditorWhiteBoxComponent& component)
        {
            UnownedSurfaceDropState state;
            EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(component.GetEntityId()));
            EXPECT_TRUE(Api::WriteMesh(*component.GetWhiteBoxMesh(), state.m_mesh));
            state.m_bindings = component.GetSurfaceMaterialBindings();
            AZ::Entity::ComponentArrayType components;
            AzToolsFramework::GetAllComponentsForEntity(component.GetEntityId(), components);
            for (const auto* entry : components) { state.m_components.push_back(entry->GetId()); }
            AZStd::sort(state.m_components.begin(), state.m_components.end());
            return state;
        }

        void ExpectUnownedSurfaceDropStateUnchanged(
            JE::Geometry::EditorWhiteBoxComponent& component, const UnownedSurfaceDropState& before)
        {
            const auto after = CaptureUnownedSurfaceDropState(component);
            EXPECT_EQ(after.m_mesh, before.m_mesh);
            EXPECT_EQ(after.m_bindings, before.m_bindings);
            EXPECT_EQ(after.m_components, before.m_components);
        }

        AZ::Data::AssetId SurfaceDropAssignedAsset(JE::Geometry::EditorWhiteBoxComponent& component,
            const JE::Render::MaterialComponentController& owner, const AZ::Uuid& surfaceId)
        {
            const auto slot = FindSurfaceMaterialSlot(component.GetSurfaceMaterialBindings(), surfaceId);
            const auto materials = owner.GetMaterialMapCopy();
            const auto key = JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot);
            const auto entry = materials.find(key);
            return entry == materials.end() ? AZ::Data::AssetId{} : entry->second.m_materialAsset.GetId();
        }

        class SurfaceDropReadOnlyQuery final : public AzToolsFramework::ReadOnlyEntityQueryRequestBus::Handler
        {
        public:
            SurfaceDropReadOnlyQuery(AzFramework::EntityContextId contextId, AZ::EntityId entityId) : m_entityId(entityId)
            {
                BusConnect(contextId);
            }
            ~SurfaceDropReadOnlyQuery() { BusDisconnect(); }
            void SetReadOnly(bool value)
            {
                m_readOnly = value;
                auto* queries = AZ::Interface<AzToolsFramework::ReadOnlyEntityQueryInterface>::Get();
                EXPECT_NE(queries, nullptr);
                if (queries) { queries->RefreshReadOnlyState({m_entityId}); }
            }
            void IsReadOnly(const AZ::EntityId& entityId, bool& readOnly) override
            {
                if (entityId == m_entityId && m_readOnly) { readOnly = true; }
            }
        private:
            AZ::EntityId m_entityId;
            bool m_readOnly = false;
        };
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropAsyncReadyKeepsPickedUuidAfterEntitySelectionClears)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_FALSE(polygons.m_side.m_faceHandles.empty());
        const auto contextId = SurfaceDropContext(m_whiteBoxEntityId);
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(contextId));
        const auto hit = QuerySurfaceDropHit(contextId, SurfaceDropRay(m_whiteBoxEntityId));
        ASSERT_TRUE(static_cast<bool>(hit));
        ASSERT_TRUE(hit.m_surfaceId.has_value());
        ASSERT_EQ(*hit.m_surfaceId, SurfaceDropTopId);
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygons.m_top, polygons.m_side}, 7));
        ASSERT_NO_FATAL_FAILURE(FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent));
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady()); // Tests deferred delivery, not filesystem I/O or a synthetic Loading status.
        AzToolsFramework::SelectEntity(m_whiteBoxEntityId);
        AZStd::string error;
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, *hit.m_surfaceId, material.GetId(), error)) << error.c_str();
        ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, before);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{});
        DrainSurfaceDropMaterialCallbacks();
        EXPECT_EQ(SurfaceDropAssignedAsset(*m_whiteBoxComponent, owner.m_controller, SurfaceDropTopId), material.GetId());
        EXPECT_EQ(FindSurfaceMaterialSlot(m_whiteBoxComponent->GetSurfaceMaterialBindings(), SurfaceDropSideId), 7);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropAsyncReplacementKeepsOtherSurfaceIntent)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_FALSE(polygons.m_side.m_faceHandles.empty());
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygons.m_top, polygons.m_side}, 7));
        ASSERT_NO_FATAL_FAILURE(FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent));
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        const auto superseded = CreatePickerMaterial();
        const auto side = CreatePickerMaterial();
        const auto latest = CreatePickerMaterial();
        ASSERT_TRUE(superseded.IsReady());
        ASSERT_TRUE(side.IsReady());
        ASSERT_TRUE(latest.IsReady());
        AZStd::string error;
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, superseded.GetId(), error)) << error.c_str();
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropSideId, side.GetId(), error)) << error.c_str();
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, latest.GetId(), error)) << error.c_str();
        ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, before);
        DrainSurfaceDropMaterialCallbacks();
        EXPECT_EQ(SurfaceDropAssignedAsset(*m_whiteBoxComponent, owner.m_controller, SurfaceDropTopId), latest.GetId());
        EXPECT_EQ(SurfaceDropAssignedAsset(*m_whiteBoxComponent, owner.m_controller, SurfaceDropSideId), side.GetId());
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropAsyncExternalDeactivateCancelsEveryOldIntent)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_FALSE(polygons.m_side.m_faceHandles.empty());
        ScopedSurfaceDropMaterialOwner owner(m_whiteBoxEntityId);
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots({polygons.m_top, polygons.m_side}, 7));
        m_whiteBoxComponent->SerializeWhiteBox();
        ASSERT_NO_FATAL_FAILURE(FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent));
        const auto before = CaptureSurfaceDropState(*m_whiteBoxComponent, owner.m_controller);
        const auto top = CreatePickerMaterial();
        const auto side = CreatePickerMaterial();
        ASSERT_TRUE(top.IsReady());
        ASSERT_TRUE(side.IsReady());
        AZStd::string error;
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, top.GetId(), error));
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropSideId, side.GetId(), error));
        const auto originalComponentId = m_whiteBoxComponent->GetId();
        auto* entity = m_whiteBoxComponent->GetEntity();
        entity->Deactivate();
        entity->Activate();
        DrainSurfaceDropMaterialCallbacks();
        entity = AzToolsFramework::GetEntityById(m_whiteBoxEntityId);
        ASSERT_NE(entity, nullptr);
        m_whiteBoxComponent = entity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>();
        ASSERT_NE(m_whiteBoxComponent, nullptr);
        ASSERT_EQ(m_whiteBoxComponent->GetId(), originalComponentId);
        ExpectSurfaceDropStateUnchanged(*m_whiteBoxComponent, owner.m_controller, before);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropAsyncErrorAndStaleUuidNeverCreateMaterialOwner)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_NO_FATAL_FAILURE(FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent));
        const auto failed = CreatePickerMaterial();
        ASSERT_TRUE(failed.IsReady());
        auto before = CaptureUnownedSurfaceDropState(*m_whiteBoxComponent);
        AZStd::string error;
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, failed.GetId(), error));
        ExpectUnownedSurfaceDropStateUnchanged(*m_whiteBoxComponent, before);
        // Use the actual manager error transition also used by AssetHandler::InitAsset(false), not private status writes.
        const AZ::Data::Asset<AZ::Data::AssetData> failedData(failed);
        AZ::Data::AssetManagerBus::Broadcast(&AZ::Data::AssetManagerEvents::OnAssetError, failedData);
        EXPECT_TRUE(failed.IsError());
        {
            SurfaceDropWarningRecorder warning;
            DrainSurfaceDropMaterialCallbacks();
            warning.ExpectOne("The material failed or was canceled; no surface was edited.");
        }
        ExpectUnownedSurfaceDropStateUnchanged(*m_whiteBoxComponent, before);

        const auto material = CreatePickerMaterial();
        ASSERT_TRUE(material.IsReady());
        ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, material.GetId(), error));
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto replacement = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555557}");
        for (const auto face : polygons.m_top.m_faceHandles) { Api::SetFaceSurfaceId(mesh, face, replacement); }
        m_whiteBoxComponent->RebuildWhiteBox();
        before = CaptureUnownedSurfaceDropState(*m_whiteBoxComponent);
        {
            SurfaceDropWarningRecorder warning;
            DrainSurfaceDropMaterialCallbacks();
            warning.ExpectOne("The picked surface is no longer present in the editable White Box mesh.");
        }
        ExpectUnownedSurfaceDropStateUnchanged(*m_whiteBoxComponent, before);
    }

    TEST_F(NativeFaceMaterialPickerTestFixture, SurfaceDropAsyncRechecksLateLockAndReadOnlyBeforeComposition)
    {
        const auto polygons = PrepareSurfaceDropCube(*m_whiteBoxComponent);
        ASSERT_FALSE(polygons.m_top.m_faceHandles.empty());
        ASSERT_NO_FATAL_FAILURE(FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent));
        const auto contextId = SurfaceDropContext(m_whiteBoxEntityId);
        ASSERT_FALSE(contextId.IsNull());
        ASSERT_NE(AZ::Interface<AzToolsFramework::ReadOnlyEntityQueryInterface>::Get(), nullptr);
        ASSERT_NE(AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get(), nullptr);
        SurfaceDropReadOnlyQuery readOnly(contextId, m_whiteBoxEntityId);
        const auto before = CaptureUnownedSurfaceDropState(*m_whiteBoxComponent);
        for (const bool useReadOnly : {false, true})
        {
            const auto material = CreatePickerMaterial();
            ASSERT_TRUE(material.IsReady());
            AZStd::string error;
            ASSERT_TRUE(RequestSurfaceDropMaterial(*m_whiteBoxComponent, SurfaceDropTopId, material.GetId(), error));
            if (useReadOnly) { readOnly.SetReadOnly(true); }
            else { AzToolsFramework::SetEntityLockState(m_whiteBoxEntityId, true); }
            {
                SurfaceDropWarningRecorder warning;
                DrainSurfaceDropMaterialCallbacks();
                warning.ExpectOne("The surface owner is no longer editable; no surface was edited.");
            }
            ExpectUnownedSurfaceDropStateUnchanged(*m_whiteBoxComponent, before);
            if (useReadOnly) { readOnly.SetReadOnly(false); }
            else { AzToolsFramework::SetEntityLockState(m_whiteBoxEntityId, false); }
        }
    }

    namespace
    {
        using JE::Geometry::IntersectWhiteBoxRenderData;
        using JE::Geometry::WhiteBoxRayHit;
        using JE::Geometry::WhiteBoxRayQueryStatus;
        using JE::Geometry::WhiteBoxRaySidedness;

        constexpr const char* SurfaceRayFloorId = "{AB123456-1234-5678-9ABC-DEF012345601}";
        constexpr const char* SurfaceRayUpperFloorId = "{AB123456-1234-5678-9ABC-DEF012345602}";
        constexpr const char* SurfaceRayCeilingId = "{AB123456-1234-5678-9ABC-DEF012345603}";
        constexpr const char* SurfaceRayWallId = "{AB123456-1234-5678-9ABC-DEF012345604}";

        JE::Geometry::WhiteBoxFace MakeSurfaceRayTriangle(
            const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c, const AZ::Uuid& surfaceId, uint32_t slot)
        {
            JE::Geometry::WhiteBoxFace face;
            face.m_v1 = {a, AZ::Vector2::CreateZero()};
            face.m_v2 = {b, AZ::Vector2::CreateZero()};
            face.m_v3 = {c, AZ::Vector2::CreateZero()};
            face.m_normal = (b - a).Cross(c - a).GetNormalizedSafe();
            face.m_surfaceId = surfaceId;
            face.m_materialSlot = slot;
            return face;
        }

        JE::Geometry::WhiteBoxRenderData MakeSurfaceRayLayers()
        {
            JE::Geometry::WhiteBoxRenderData data;
            // Deliberately store far/near/middle, so enumeration order cannot satisfy the distance oracle.
            data.m_faces.push_back(MakeSurfaceRayTriangle(
                {-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, AZ::Uuid::CreateString(SurfaceRayFloorId), 7));
            data.m_faces.push_back(MakeSurfaceRayTriangle(
                {-2, -2, 4}, {0, 2, 4}, {2, -2, 4}, AZ::Uuid::CreateString(SurfaceRayCeilingId), 13));
            data.m_faces.push_back(MakeSurfaceRayTriangle(
                {-2, -2, 2}, {2, -2, 2}, {0, 2, 2}, AZ::Uuid::CreateString(SurfaceRayUpperFloorId), 7));
            data.m_faces.push_back(MakeSurfaceRayTriangle(
                {2, -2, -1}, {2, 2, -1}, {2, 0, 3}, AZ::Uuid::CreateString(SurfaceRayWallId), 11));
            return data;
        }

        void AppendSurfaceRayQuad(JE::Geometry::WhiteBoxRenderData& data, const AZ::Vector3& a,
            AZ::Vector3 b, const AZ::Vector3& c, AZ::Vector3 d, const AZ::Uuid& id, uint32_t slot, bool inward)
        {
            if (inward) { AZStd::swap(b, d); }
            data.m_faces.push_back(MakeSurfaceRayTriangle(a, b, c, id, slot));
            data.m_faces.push_back(MakeSurfaceRayTriangle(a, c, d, id, slot));
        }

        void AppendSurfaceRayBox(JE::Geometry::WhiteBoxRenderData& data, const AZ::Vector3& halfExtents,
            const AZ::Uuid& id, uint32_t slot, bool inward)
        {
            const float x = halfExtents.GetX();
            const float y = halfExtents.GetY();
            const float z = halfExtents.GetZ();
            AppendSurfaceRayQuad(data, {-x, -y, -z}, {-x, -y, z}, {-x, y, z}, {-x, y, -z}, id, slot, inward);
            AppendSurfaceRayQuad(data, {x, -y, -z}, {x, y, -z}, {x, y, z}, {x, -y, z}, id, slot, inward);
            AppendSurfaceRayQuad(data, {-x, -y, -z}, {x, -y, -z}, {x, -y, z}, {-x, -y, z}, id, slot, inward);
            AppendSurfaceRayQuad(data, {-x, y, -z}, {-x, y, z}, {x, y, z}, {x, y, -z}, id, slot, inward);
            AppendSurfaceRayQuad(data, {-x, -y, -z}, {-x, y, -z}, {x, y, -z}, {x, -y, -z}, id, slot, inward);
            AppendSurfaceRayQuad(data, {-x, -y, z}, {x, -y, z}, {x, y, z}, {-x, y, z}, id, slot, inward);
        }
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayCollectsTwoFloorsCeilingAndWallWithFullIdentity)
    {
        const auto data = MakeSurfaceRayLayers();
        AZStd::array<WhiteBoxRayHit, 4> hits{};
        const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
        ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 3);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        EXPECT_FLOAT_EQ(hits[1].m_distance, 3.0f);
        EXPECT_FLOAT_EQ(hits[2].m_distance, 5.0f);
        EXPECT_TRUE(hits[0].m_worldPosition.IsClose({0, -0.5f, 4}, 1e-5f));
        EXPECT_TRUE(hits[1].m_worldPosition.IsClose({0, -0.5f, 2}, 1e-5f));
        EXPECT_TRUE(hits[2].m_worldPosition.IsClose({0, -0.5f, 0}, 1e-5f));
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({0, 0, -1}, 1e-5f));
        EXPECT_TRUE(hits[1].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
        EXPECT_TRUE(hits[2].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
        EXPECT_EQ(hits[1].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
        EXPECT_EQ(hits[2].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayFloorId));
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 13);
        ASSERT_TRUE(hits[1].m_materialSlot.has_value());
        EXPECT_EQ(*hits[1].m_materialSlot, 7);
        ASSERT_TRUE(hits[2].m_materialSlot.has_value());
        EXPECT_EQ(*hits[2].m_materialSlot, 7);
        EXPECT_EQ(hits[0].m_primitiveIndex, 1);
        EXPECT_EQ(hits[1].m_primitiveIndex, 2);
        EXPECT_EQ(hits[2].m_primitiveIndex, 0);
        const auto wall = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {4, 0, 1}, {0, 0, 1}, WhiteBoxRaySidedness::FrontFace, hits);
        ASSERT_EQ(wall.m_hitCount, 1);
        EXPECT_EQ(wall.m_status, WhiteBoxRayQueryStatus::Complete);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 2.0f);
        EXPECT_TRUE(hits[0].m_worldPosition.IsClose({2, 0, 1}, 1e-5f));
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({1, 0, 0}, 1e-5f));
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 11);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayCollectsCavityEntryInteriorAndExitFromSameMesh)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto outerId = AZ::Uuid::CreateString(SurfaceRayFloorId);
        const auto innerId = AZ::Uuid::CreateString(SurfaceRayUpperFloorId);
        // One triangle mesh bounds a solid shell and its inward-facing cavity. The ray misses quad diagonals.
        AppendSurfaceRayBox(data, {3, 2, 2}, outerId, 3, false);
        AppendSurfaceRayBox(data, {1, 1, 1}, innerId, 9, true);
        AZStd::array<WhiteBoxRayHit, 8> hits{};
        const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {4, 0.25f, 0.5f}, {-4, 0.25f, 0.5f}, WhiteBoxRaySidedness::DoubleSided, hits);
        ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 4);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        const AZStd::array<float, 4> distances{1, 3, 5, 7};
        const AZStd::array<float, 4> positions{3, 1, -1, -3};
        const AZStd::array<float, 4> normals{1, -1, 1, -1};
        for (size_t i = 0; i < distances.size(); ++i)
        {
            EXPECT_FLOAT_EQ(hits[i].m_distance, distances[i]);
            EXPECT_TRUE(hits[i].m_worldPosition.IsClose({positions[i], 0.25f, 0.5f}, 1e-5f));
            EXPECT_TRUE(hits[i].m_worldNormal.IsClose({normals[i], 0, 0}, 1e-5f));
        }
        EXPECT_EQ(hits[0].m_surfaceId, outerId);
        EXPECT_EQ(hits[1].m_surfaceId, innerId);
        EXPECT_EQ(hits[2].m_surfaceId, innerId);
        EXPECT_EQ(hits[3].m_surfaceId, outerId);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 3);
        ASSERT_TRUE(hits[1].m_materialSlot.has_value());
        EXPECT_EQ(*hits[1].m_materialSlot, 9);
        ASSERT_TRUE(hits[2].m_materialSlot.has_value());
        EXPECT_EQ(*hits[2].m_materialSlot, 9);
        ASSERT_TRUE(hits[3].m_materialSlot.has_value());
        EXPECT_EQ(*hits[3].m_materialSlot, 3);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayTransformsPositionWindingNormalAndWorldDistance)
    {
        JE::Geometry::WhiteBoxRenderData data;
        data.m_faces.push_back(MakeSurfaceRayTriangle(
            {-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, AZ::Uuid::CreateString(SurfaceRayFloorId), 7));
        auto transform = AZ::Transform::CreateRotationY(AZ::Constants::HalfPi);
        transform.SetTranslation({10, -3, 7});
        transform.SetUniformScale(2.0f);
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        const auto result = IntersectWhiteBoxRenderData(
            data, transform, {16, -3, 7}, {4, -3, 7}, WhiteBoxRaySidedness::FrontFace, hits);
        ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_NEAR(hits[0].m_distance, 6.0f, 1e-5f);
        EXPECT_TRUE(hits[0].m_worldPosition.IsClose({10, -3, 7}, 1e-5f));
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({1, 0, 0}, 1e-5f));
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayDoubleSidedPreservesWindingNormalOnBackHit)
    {
        JE::Geometry::WhiteBoxRenderData data;
        data.m_faces.push_back(MakeSurfaceRayTriangle(
            {-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, AZ::Uuid::CreateString(SurfaceRayFloorId), 7));
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, 0, -1}, {0, 0, 1}, WhiteBoxRaySidedness::FrontFace, hits);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, 0, -1}, {0, 0, 1}, WhiteBoxRaySidedness::DoubleSided, hits);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayNearestCapacityReportsEveryDiscardWithoutHoles)
    {
        const auto data = MakeSurfaceRayLayers();
        AZStd::array<WhiteBoxRayHit, 2> hits{};
        auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Truncated);
        ASSERT_EQ(result.m_hitCount, 2);
        EXPECT_EQ(result.m_discardedHitCount, 1);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
        EXPECT_EQ(hits[1].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        EXPECT_FLOAT_EQ(hits[1].m_distance, 3.0f);
        result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided,
            AZStd::span<WhiteBoxRayHit>(hits.data(), 1));
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_discardedHitCount, 2);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Truncated);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, {});
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_discardedHitCount, 3);
        result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {9, 9, 5}, {9, 9, -1}, WhiteBoxRaySidedness::DoubleSided, {});
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_discardedHitCount, 0);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayRejectsNonFiniteAndZeroSegmentsBeforeWritingOutput)
    {
        const auto data = MakeSurfaceRayLayers();
        const float nan = AZStd::numeric_limits<float>::quiet_NaN();
        const float inf = AZStd::numeric_limits<float>::infinity();
        const float max = AZStd::numeric_limits<float>::max();
        struct InvalidSegment
        {
            AZ::Vector3 m_start;
            AZ::Vector3 m_end;
        };
        const AZStd::array<InvalidSegment, 4> invalidSegments{{
            {{0, 0, 1}, {0, 0, 1}}, {{nan, 0, 1}, {0, 0, -1}},
            {{0, 0, 1}, {0, 0, inf}}, {{-max, 0, 0}, {max, 0, 0}}}};
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        hits[0].m_distance = 123.0f;
        hits[0].m_surfaceId = AZ::Uuid::CreateString(SurfaceRayWallId);
        for (const auto& segment : invalidSegments)
        {
            const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
                segment.m_start, segment.m_end, WhiteBoxRaySidedness::DoubleSided, hits);
            EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::InvalidInput);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
            EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
        }
        auto transform = AZ::Transform::CreateIdentity();
        transform.SetTranslation({nan, 0, 0});
        const auto result = IntersectWhiteBoxRenderData(data, transform,
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::InvalidInput);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayKeepsCoincidentPrimitivesAndStableTieOrder)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto a = AZ::Uuid::CreateString(SurfaceRayFloorId);
        const auto b = AZ::Uuid::CreateString(SurfaceRayUpperFloorId);
        data.m_faces.push_back(MakeSurfaceRayTriangle({-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, a, 7));
        data.m_faces.push_back(MakeSurfaceRayTriangle({-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, b, 7));
        data.m_faces.push_back(data.m_faces[0]); // Distinct primitive, even with the same position/UUID/slot.
        AZStd::array<WhiteBoxRayHit, 3> hits{};
        for (int repeat = 0; repeat < 2; ++repeat)
        {
            const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
                {0, 0, 1}, {0, 0, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
            ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 3);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            EXPECT_EQ(hits[0].m_primitiveIndex, 0);
            EXPECT_EQ(hits[1].m_primitiveIndex, 1);
            EXPECT_EQ(hits[2].m_primitiveIndex, 2);
            EXPECT_EQ(hits[0].m_surfaceId, a);
            EXPECT_EQ(hits[1].m_surfaceId, b);
            EXPECT_EQ(hits[2].m_surfaceId, a);
        }
        const auto limited = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, 0, 1}, {0, 0, -1}, WhiteBoxRaySidedness::DoubleSided,
            AZStd::span<WhiteBoxRayHit>(hits.data(), 1));
        EXPECT_EQ(limited.m_status, WhiteBoxRayQueryStatus::Truncated);
        EXPECT_EQ(limited.m_hitCount, 1);
        EXPECT_EQ(limited.m_discardedHitCount, 2);
        EXPECT_EQ(hits[0].m_primitiveIndex, 0);
        EXPECT_EQ(hits[0].m_surfaceId, a);
        EXPECT_EQ(data.m_faces[0].m_surfaceId, a);
        EXPECT_EQ(data.m_faces[1].m_surfaceId, b);
        EXPECT_EQ(data.m_faces[0].m_materialSlot, 7);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayDoesNotReturnGeometryBeyondSegmentEndpoints)
    {
        const auto data = MakeSurfaceRayLayers();
        AZStd::array<WhiteBoxRayHit, 4> hits{};
        const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, -0.5f, 3}, {0, -0.5f, 1}, WhiteBoxRaySidedness::DoubleSided, hits);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayRejectsLaterNonFiniteGeometryWithoutPartialOutput)
    {
        const float nan = AZStd::numeric_limits<float>::quiet_NaN();
        const float inf = AZStd::numeric_limits<float>::infinity();
        for (const float badVertex : {nan, inf})
        {
            auto data = MakeSurfaceRayLayers();
            // The first primitive is a valid hit. A later bad primitive must reject the whole query before writing it.
            data.m_faces[1].m_v2.m_position.SetX(badVertex);
            AZStd::array<WhiteBoxRayHit, 2> hits{};
            for (auto& hit : hits)
            {
                hit.m_worldPosition = {9, 8, 7};
                hit.m_worldNormal = {1, 0, 0};
                hit.m_distance = 123.0f;
                hit.m_surfaceId = AZ::Uuid::CreateString(SurfaceRayWallId);
                hit.m_materialSlot = 77;
                hit.m_primitiveIndex = 999;
            }
            const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
                {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
            EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::InvalidInput);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(result.m_discardedHitCount, 0);
            for (const auto& hit : hits)
            {
                EXPECT_EQ(hit.m_worldPosition, AZ::Vector3(9, 8, 7));
                EXPECT_EQ(hit.m_worldNormal, AZ::Vector3(1, 0, 0));
                EXPECT_FLOAT_EQ(hit.m_distance, 123.0f);
                EXPECT_EQ(hit.m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
                ASSERT_TRUE(hit.m_materialSlot.has_value());
                EXPECT_EQ(*hit.m_materialSlot, 77);
                EXPECT_EQ(hit.m_primitiveIndex, 999);
            }
        }
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayIgnoresFiniteDegenerateTrianglesWithoutDiscardedHits)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto id = AZ::Uuid::CreateString(SurfaceRayFloorId);
        data.m_faces.push_back(MakeSurfaceRayTriangle({0, 0, 1}, {1, 0, 1}, {2, 0, 1}, id, 99));
        data.m_faces.push_back(MakeSurfaceRayTriangle({-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, id, 7));
        data.m_faces.push_back(MakeSurfaceRayTriangle({0, 0, -1}, {0, 0, -1}, {0, 0, -1}, id, 99));
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        const auto result = IntersectWhiteBoxRenderData(data, AZ::Transform::CreateIdentity(),
            {0, 0, 2}, {0, 0, -2}, WhiteBoxRaySidedness::DoubleSided, hits);
        ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 2.0f);
        EXPECT_EQ(hits[0].m_primitiveIndex, 1);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 7);
        EXPECT_EQ(hits[0].m_surfaceId, id);
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayPreservesLocallyValidTriangleAtUniformScalePointZeroZeroOne)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto id = AZ::Uuid::CreateString(SurfaceRayFloorId);
        data.m_faces.push_back(MakeSurfaceRayTriangle({-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, id, 7));
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        const auto identity = AZ::Transform::CreateIdentity();
        const auto baseline = IntersectWhiteBoxRenderData(
            data, identity, {0, 0, 1}, {0, 0, -1}, WhiteBoxRaySidedness::FrontFace, hits);
        ASSERT_EQ(baseline.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(baseline.m_hitCount, 1);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 1.0f);
        auto small = AZ::Transform::CreateIdentity();
        small.SetUniformScale(0.001f);
        const auto result = IntersectWhiteBoxRenderData(data, small,
            small.TransformPoint({0, 0, 1}), small.TransformPoint({0, 0, -1}),
            WhiteBoxRaySidedness::FrontFace, hits);
        ASSERT_EQ(result.m_status, WhiteBoxRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 0.001f);
        EXPECT_TRUE(hits[0].m_worldPosition.IsClose({0, 0, 0}, 1e-7f));
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
        EXPECT_EQ(hits[0].m_surfaceId, id);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 7);
        EXPECT_EQ(hits[0].m_primitiveIndex, 0);
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayRejectsCollapsedZeroScaleBeforeWritingOutput)
    {
        const auto data = MakeSurfaceRayLayers();
        auto collapsed = AZ::Transform::CreateIdentity();
        collapsed.SetUniformScale(0.0f);
        AZStd::array<WhiteBoxRayHit, 1> hits{};
        hits[0].m_distance = 123.0f;
        hits[0].m_surfaceId = AZ::Uuid::CreateString(SurfaceRayWallId);
        const auto result = IntersectWhiteBoxRenderData(data, collapsed,
            {0, -0.5f, 5}, {0, -0.5f, -1}, WhiteBoxRaySidedness::DoubleSided, hits);
        EXPECT_EQ(result.m_status, WhiteBoxRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
    }

    namespace SurfaceQueries = JE::Geometry::SurfaceData;

    class WhiteBoxSurfaceQueryFixture : public EditorWhiteBoxComponentTestFixture
    {
    protected:
        template<class Component>
        void EnsureQueryDescriptor()
        {
            if (!AZ::ComponentDescriptorBus::FindFirstHandler(azrtti_typeid<Component>()))
            {
                AZStd::unique_ptr<AZ::ComponentDescriptor> descriptor(Component::CreateDescriptor());
                AZ::SerializeContext* context = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (context && !context->FindClassData(azrtti_typeid<Component>())) { descriptor->Reflect(context); }
                m_queryDescriptors.push_back(AZStd::move(descriptor));
            }
        }

        void SetUpEditorFixtureImpl() override
        {
            // No getter can restore an arbitrary pre-existing factory. Reject that setup before changing it.
            ASSERT_FALSE(JE::Geometry::WhiteBoxRequestBus::HasHandlers())
                << "Surface query tests require a fixture-owned WhiteBoxSystem; an external factory is left unchanged.";
            EditorWhiteBoxComponentTestFixture::SetUpEditorFixtureImpl();
            m_editorFixtureCreated = m_whiteBoxEntityId.IsValid();
            ASSERT_TRUE(m_editorFixtureCreated);
            ASSERT_NE(m_whiteBoxComponent, nullptr);
            if (!AZ::Interface<SurfaceQueries::SurfaceDataSystem>::Get())
            {
                EnsureQueryDescriptor<SurfaceQueries::SurfaceDataSystemComponent>();
                m_ownedSurfaceSystem = AZStd::make_unique<AZ::Entity>("Surface query test system");
                m_ownedSurfaceSystem->CreateComponent<SurfaceQueries::SurfaceDataSystemComponent>();
                m_ownedSurfaceSystem->Init();
                m_ownedSurfaceSystem->Activate();
            }
            m_querySystem = AZ::Interface<SurfaceQueries::SurfaceDataSystem>::Get();
            ASSERT_NE(m_querySystem, nullptr);
            EnsureQueryDescriptor<JE::Geometry::WhiteBoxSystemComponent>();
            m_ownedWhiteBoxSystem = AZStd::make_unique<AZ::Entity>("WhiteBox query render seam");
            auto* renderSystem = m_ownedWhiteBoxSystem->CreateComponent<JE::Geometry::WhiteBoxSystemComponent>();
            ASSERT_NE(renderSystem, nullptr);
            m_ownedWhiteBoxSystem->Init();
            m_ownedWhiteBoxSystem->Activate();
            ASSERT_EQ(m_ownedWhiteBoxSystem->GetState(), AZ::Entity::State::Active);
            // Modify the owned component directly, never whichever System happens to be on the global bus.
            renderSystem->SetRenderMeshInterfaceBuilder(
                [](AZ::EntityId entityId) -> AZStd::unique_ptr<JE::Geometry::RenderMeshInterface>
                { return AZStd::make_unique<JE::Geometry::WhiteBoxNullRenderMesh>(entityId); });
            m_whiteBoxComponent->RebuildWhiteBox();
        }

        void TearDownEditorFixtureImpl() override
        {
            if (m_editorFixtureCreated)
            {
                EditorWhiteBoxComponentTestFixture::TearDownEditorFixtureImpl();
            }
            else
            {
                // Base setup may have produced descriptors before failing to create its editor entity.
                m_whiteBoxComponentDescriptor.reset();
                m_editorWhiteBoxComponentDescriptor.reset();
            }
            m_editorFixtureCreated = false;
            m_whiteBoxComponent = nullptr;
            m_whiteBoxEntityId.SetInvalid();
            m_ownedWhiteBoxSystem.reset();
            m_ownedSurfaceSystem.reset();
            m_queryDescriptors.clear();
            m_querySystem = nullptr;
        }

        AZStd::unique_ptr<AZ::Entity> CreateRuntimeQuerySource(
            const JE::Geometry::WhiteBoxRenderData& data, const AZ::Transform& world, AZ::EntityId id)
        {
            EnsureQueryDescriptor<AzFramework::TransformComponent>();
            auto entity = AZStd::make_unique<AZ::Entity>("Runtime WhiteBox surface query");
            entity->SetId(id);
            entity->CreateComponent<AzFramework::TransformComponent>();
            auto* component = entity->CreateComponent<JE::Geometry::WhiteBoxComponent>();
            component->GenerateWhiteBoxMesh(data);
            entity->Init();
            entity->Activate();
            AZ::TransformBus::Event(id, &AZ::TransformBus::Events::SetWorldTM, world);
            return entity;
        }

        SurfaceQueries::SurfaceRayRequest QueryAt(float x, float startZ = 5.0f, float endZ = -1.0f)
        {
            SurfaceQueries::SurfaceRayRequest request;
            request.m_startWorldPosition = {x, -0.5f, startZ};
            request.m_endWorldPosition = {x, -0.5f, endZ};
            request.m_sidedness = SurfaceQueries::SurfaceRaySidedness::DoubleSided;
            return request;
        }

        struct LegacyQueryEntry
        {
            explicit LegacyQueryEntry(SurfaceQueries::SurfaceDataSystem& system, const AZ::Aabb& bounds)
                : m_system(system)
            {
                SurfaceQueries::SurfaceDataRegistryEntry entry;
                entry.m_entityId = AZ::EntityId(701000);
                entry.m_bounds = bounds;
                entry.m_maxPointsCreatedPerInput = 1; // Real legacy registration contract; no invented ray response.
                m_handle = m_system.RegisterSurfaceDataProvider(entry);
            }
            ~LegacyQueryEntry() { m_system.UnregisterSurfaceDataProvider(m_handle); }
            SurfaceQueries::SurfaceDataSystem& m_system;
            SurfaceQueries::SurfaceDataRegistryHandle m_handle;
        };

        bool m_editorFixtureCreated = false;
        SurfaceQueries::SurfaceDataSystem* m_querySystem = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_ownedSurfaceSystem;
        AZStd::unique_ptr<AZ::Entity> m_ownedWhiteBoxSystem;
        AZStd::vector<AZStd::unique_ptr<AZ::ComponentDescriptor>> m_queryDescriptors;
    };


    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionNonemptyCodecHasIndependentCornerUvNormalUuidAndSlotVector)
    {
        JE::Geometry::WhiteBoxRenderData data;
        data.m_faces.resize(1);
        auto& face = data.m_faces.front();
        face.m_v1.m_position = {1.0f, 2.0f, 3.0f};
        face.m_v1.m_uv = AZ::Vector2(0.25f, 0.5f);
        face.m_v2.m_position = {-4.0f, 5.5f, -6.25f};
        face.m_v2.m_uv = AZ::Vector2(-0.125f, 2.0f);
        face.m_v3.m_position = {0.125f, -0.25f, 8.0f};
        face.m_v3.m_uv = AZ::Vector2(0.75f, -0.5f);
        face.m_normal = {0.0f, 0.5f, -1.0f};
        face.m_surfaceId = AZ::Uuid::CreateString("{00112233-4455-6677-8899-AABBCCDDEEFF}");
        face.m_materialSlot = 0x01020304u;
        data.m_material.m_tint = {0.25f, 0.5f, 0.75f};
        data.m_material.m_useTexture = false;
        data.m_material.m_visible = true;
        const auto token = JE::Geometry::ComputeWhiteBoxSourceRevision(data);
        ASSERT_TRUE(token);
        EXPECT_EQ(token->m_schema, 1);
        // SHA-1 of the independently enumerated 146-byte schema-1 vector in codec-nonempty-vector.json.
        const AZStd::array<AZ::u32, 5> expected{{0x9DEAA9D0u, 0x4996C594u, 0xC10651DEu, 0x964DF5DEu, 0xC50FFB4Fu}};
        EXPECT_EQ(token->m_digest, expected);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionMaterialChangeWithoutRenderMeshPublishesNewTokenAndPreservesGeometry)
    {
        PrepareSurfaceDropCube(*m_whiteBoxComponent);
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);
        ASSERT_FALSE(m_whiteBoxComponent->HasRenderMesh());
        auto request = QueryAt(0.125f, 2.0f, -2.0f);
        request.m_startWorldPosition.SetY(0.2f); request.m_endWorldPosition.SetY(0.2f);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {m_whiteBoxEntityId, m_whiteBoxComponent->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> beforeHits{};
        const auto before = m_querySystem->QueryRay(request, beforeHits);
        ASSERT_EQ(before.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(before.m_hitCount, 2);
        ASSERT_EQ(beforeHits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        Api::WhiteBoxMeshStream beforeMesh;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBoxComponent->GetWhiteBoxMesh(), beforeMesh));

        // Public editor property API writes the reflected field and invokes the real ancestor ChangeNotify.
        AzToolsFramework::PropertyTreeEditor properties(m_whiteBoxComponent, azrtti_typeid<JE::Geometry::EditorWhiteBoxComponent>());
        const AZ::Vector3 tint(0.25f, 0.5f, 0.75f);
        const AZStd::string tintSuffix = "White Box Material|Tint";
        AZStd::string tintPath;
        for (const auto& path : properties.BuildPathsList())
        {
            if (path.size() >= tintSuffix.size() &&
                path.compare(path.size() - tintSuffix.size(), tintSuffix.size(), tintSuffix) == 0)
            {
                ASSERT_TRUE(tintPath.empty()) << "Ambiguous reflected material tint path";
                tintPath = path;
            }
        }
        ASSERT_FALSE(tintPath.empty()); // Empty editor groups may prefix the public property path.
        const auto original = properties.GetProperty(tintPath);
        ASSERT_TRUE(original.IsSuccess());
        ASSERT_NE(AZStd::any_cast<AZ::Vector3>(original.GetValue()), tint);
        const auto changed = properties.SetProperty(tintPath, AZStd::any(tint));
        ASSERT_TRUE(changed.IsSuccess());
        EXPECT_TRUE(properties.CompareProperty(tintPath, AZStd::any(tint)));
        EXPECT_FALSE(m_whiteBoxComponent->HasRenderMesh());

        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> afterHits{};
        const auto after = m_querySystem->QueryRay(request, afterHits);
        ASSERT_EQ(after.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(after.m_hitCount, before.m_hitCount);
        EXPECT_NE(afterHits[0].m_sourceRevision, beforeHits[0].m_sourceRevision);
        for (size_t index = 0; index != after.m_hitCount; ++index)
        {
            EXPECT_EQ(afterHits[index].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
            EXPECT_EQ(afterHits[index].m_sourceRevision, afterHits[0].m_sourceRevision);
            EXPECT_EQ(afterHits[index].m_worldPosition, beforeHits[index].m_worldPosition);
            EXPECT_EQ(afterHits[index].m_worldNormal, beforeHits[index].m_worldNormal);
            EXPECT_EQ(afterHits[index].m_distance, beforeHits[index].m_distance);
            EXPECT_EQ(afterHits[index].m_surfaceId, beforeHits[index].m_surfaceId);
            EXPECT_EQ(afterHits[index].m_materialSlot, beforeHits[index].m_materialSlot);
            EXPECT_EQ(afterHits[index].m_primitiveIndex, beforeHits[index].m_primitiveIndex);
            EXPECT_EQ(afterHits[index].m_entityAndComponent, beforeHits[index].m_entityAndComponent);
        }
        Api::WhiteBoxMeshStream afterMesh;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBoxComponent->GetWhiteBoxMesh(), afterMesh));
        EXPECT_EQ(afterMesh, beforeMesh);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionCodecHasIndependentVectorAndCanonicalSignedZero)
    {
        JE::Geometry::WhiteBoxRenderData data;
        data.m_material.m_tint = AZ::Vector3::CreateZero();
        data.m_material.m_useTexture = false;
        data.m_material.m_visible = false;
        const auto token = JE::Geometry::ComputeWhiteBoxSourceRevision(data);
        ASSERT_TRUE(token);
        EXPECT_EQ(token->m_schema, 1);
        const AZStd::array<AZ::u32, 5> expected{{0x3E5707B1u, 0x63C29EE1u, 0x0E9F1526u, 0xDB1FC2FFu, 0xF9D9E02Au}};
        EXPECT_EQ(token->m_digest, expected); // Independent, explicit little-endian schema-1 byte stream.
        data.m_material.m_tint.SetX(-0.0f);
        EXPECT_EQ(JE::Geometry::ComputeWhiteBoxSourceRevision(data), token);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionDistinguishesPublishedFieldsAndRestoredContent)
    {
        auto data = MakeSurfaceRayLayers();
        const auto baseline = JE::Geometry::ComputeWhiteBoxSourceRevision(data);
        ASSERT_TRUE(baseline);
        for (int field = 0; field != 9; ++field)
        {
            auto changed = data;
            switch (field)
            {
            case 0: changed.m_faces[0].m_v1.m_position.SetX(0.125f); break;
            case 1: changed.m_faces[0].m_v1.m_uv.SetX(2.0f); break;
            case 2: changed.m_faces[0].m_normal.SetX(0.5f); break;
            case 3: changed.m_faces[0].m_surfaceId = SurfaceDropSideId; break;
            case 4: ++changed.m_faces[0].m_materialSlot; break;
            case 5: changed.m_material.m_tint.SetX(0.125f); break;
            case 6: changed.m_material.m_useTexture = !changed.m_material.m_useTexture; break;
            case 7: changed.m_material.m_visible = !changed.m_material.m_visible; break;
            case 8: AZStd::swap(changed.m_faces[0], changed.m_faces[1]); break;
            }
            const auto revision = JE::Geometry::ComputeWhiteBoxSourceRevision(changed);
            ASSERT_TRUE(revision);
            EXPECT_NE(revision, baseline) << field;
        }
        EXPECT_EQ(JE::Geometry::ComputeWhiteBoxSourceRevision(data), baseline);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionRejectsNonfiniteGeometryUvNormalAndMaterial)
    {
        for (int field = 0; field != 4; ++field)
        {
            auto data = MakeSurfaceRayLayers();
            ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
            const auto invalid = AZStd::numeric_limits<float>::quiet_NaN();
            switch (field)
            {
            case 0: data.m_faces.back().m_v3.m_position.SetZ(invalid); break;
            case 1: data.m_faces.back().m_v3.m_uv.SetY(invalid); break;
            case 2: data.m_faces.back().m_normal.SetX(invalid); break;
            case 3: data.m_material.m_tint.SetX(invalid); break;
            }
            EXPECT_FALSE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
            EXPECT_EQ(data.m_sourceRevision.m_schema, 0);
            EXPECT_FALSE(JE::Geometry::ComputeWhiteBoxSourceRevision(data));
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionBinaryRoundtripPersistsTokenAndLegacyIsUnavailable)
    {
        auto data = MakeSurfaceRayLayers();
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &data, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        JE::Geometry::WhiteBoxRenderData reopened;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, reopened, context));
        EXPECT_EQ(reopened.m_sourceRevision, data.m_sourceRevision);
        EXPECT_EQ(JE::Geometry::ComputeWhiteBoxSourceRevision(reopened), JE::Geometry::ComputeWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(reopened, AZ::Transform::CreateIdentity(), AZ::EntityId(709020));
        auto request = QueryAt(0);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        auto* component = source->FindComponent<JE::Geometry::WhiteBoxComponent>();
        request.m_owner = {source->GetId(), component->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hits[0].m_sourceRevision, data.m_sourceRevision);
        AZ::SerializeContext::DataElementNode legacy;
        ASSERT_TRUE(legacy.Convert<JE::Geometry::WhiteBoxRenderData>(*context));
        ASSERT_TRUE(legacy.SetData(*context, data));
        ASSERT_TRUE(legacy.RemoveElementByName(AZ_CRC_CE("SourceRevision")));
        legacy.SetVersion(3);
        JE::Geometry::WhiteBoxRenderData oldCooked;
        ASSERT_TRUE(legacy.GetData(oldCooked));
        EXPECT_EQ(oldCooked.m_sourceRevision.m_schema, 0);
        component->GenerateWhiteBoxMesh(oldCooked);
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
        EXPECT_EQ(hits[0].m_sourceRevision.m_schema, 0);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionRuntimeRejectsStaleOrUnknownClaimWithoutRetainingOldSnapshot)
    {
        auto data = MakeSurfaceRayLayers();
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateIdentity(), AZ::EntityId(709021));
        auto* component = source->FindComponent<JE::Geometry::WhiteBoxComponent>();
        auto request = QueryAt(0);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), component->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        for (auto& face : data.m_faces)
        {
            face.m_v1.m_position.SetZ(face.m_v1.m_position.GetZ() + 0.25f);
            face.m_v2.m_position.SetZ(face.m_v2.m_position.GetZ() + 0.25f);
            face.m_v3.m_position.SetZ(face.m_v3.m_position.GetZ() + 0.25f);
        }
        component->GenerateWhiteBoxMesh(data); // Geometry changed; serialized claim is stale.
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 4.25f);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        component->GenerateWhiteBoxMesh(data);
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_EQ(hits[0].m_sourceRevision, data.m_sourceRevision);
        ++data.m_sourceRevision.m_schema;
        component->GenerateWhiteBoxMesh(data);
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionStaysLocalWhileTransformUpdatesWorldQuery)
    {
        auto data = MakeSurfaceRayLayers();
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateIdentity(), AZ::EntityId(709022));
        auto world = AZ::Transform::CreateTranslation({20, 0, 0});
        world.SetUniformScale(2.0f);
        AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM, world);
        auto request = QueryAt(20, 10, -2);
        request.m_startWorldPosition.SetY(-1); request.m_endWorldPosition.SetY(-1);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 8.0f);
        EXPECT_EQ(hits[0].m_sourceRevision, data.m_sourceRevision);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ContentRevisionRealUvUndoRedoAndSavedAuthoringBuildGameEntity)
    {
        PrepareSurfaceDropCube(*m_whiteBoxComponent);
        FinishSurfaceDropFixtureSetup(m_whiteBoxEntityId, m_whiteBoxComponent);
        ASSERT_NE(m_whiteBoxComponent, nullptr);
        auto* updates = AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get();
        ASSERT_NE(updates, nullptr);
        auto request = QueryAt(0.125f, 2, -2);
        request.m_startWorldPosition.SetY(0.2f); request.m_endWorldPosition.SetY(0.2f);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {m_whiteBoxEntityId, m_whiteBoxComponent->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 2);
        ASSERT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        const auto baseline = hits[0].m_sourceRevision;
        const auto polygons = Api::MeshPolygonHandles(*m_whiteBoxComponent->GetWhiteBoxMesh());
        ASSERT_FALSE(polygons.empty());
        ASSERT_TRUE(m_whiteBoxComponent->ApplyPolygonUVTransform({polygons.front()}, AZ::Vector2(2.0f, 2.0f), AZ::Vector2(3.0f, 5.0f), 0.0f));
        ASSERT_TRUE(updates->UpdateTemplateInstancesInQueue(true));
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 2);
        const auto changed = hits[0].m_sourceRevision;
        EXPECT_NE(changed, baseline);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        ASSERT_TRUE(updates->UpdateTemplateInstancesInQueue(true));
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 2);
        EXPECT_EQ(hits[0].m_sourceRevision, baseline);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        ASSERT_TRUE(updates->UpdateTemplateInstancesInQueue(true));
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 2);
        EXPECT_EQ(hits[0].m_sourceRevision, changed);
        auto* entity = AzToolsFramework::GetEntityById(m_whiteBoxEntityId);
        ASSERT_NE(entity, nullptr);
        m_whiteBoxComponent = entity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>();
        ASSERT_NE(m_whiteBoxComponent, nullptr);
        m_whiteBoxComponent->SerializeWhiteBox();
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, m_whiteBoxComponent, context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::unique_ptr<JE::Geometry::EditorWhiteBoxComponent> reopened(
            AZ::Utils::LoadObjectFromStream<JE::Geometry::EditorWhiteBoxComponent>(stream, context));
        ASSERT_TRUE(reopened);
        EnsureQueryDescriptor<AzFramework::TransformComponent>();
        auto runtime = AZStd::make_unique<AZ::Entity>("Saved revision runtime");
        runtime->CreateComponent<AzFramework::TransformComponent>();
        static_cast<AzToolsFramework::Components::EditorComponentBase*>(reopened.get())->BuildGameEntity(runtime.get());
        runtime->Init(); runtime->Activate();
        request.m_owner = {runtime->GetId(), runtime->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 2);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hits[0].m_sourceRevision, changed);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ConsoleRayReportsAuthoredIdentityFromRuntimeGeometry)
    {
        class Capture final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            Capture() { BusConnect(); }
            ~Capture() override { BusDisconnect(); }
            bool OnPrintf(const char* window, const char* message) override
            {
                if (AZStd::string_view(window) == "JE.SurfaceData") { m_output += message; }
                return false;
            }
            AZStd::string m_output;
        } capture;
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(709030));
        auto* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(console, nullptr);
        ASSERT_TRUE(console->HasCommand("je_surfaceRay"));
        ASSERT_TRUE(console->PerformCommand("je_surfaceRay 100 -0.5 5 100 -0.5 -1").IsSuccess());
        EXPECT_NE(capture.m_output.find("status=Complete hits=3 discarded=0"), AZStd::string::npos);
        EXPECT_NE(capture.m_output.find("revision=Content"), AZStd::string::npos);
        EXPECT_NE(capture.m_output.find("surface=" + data.m_faces[0].m_surfaceId.ToString<AZStd::string>()), AZStd::string::npos);
        EXPECT_EQ(capture.m_output.find("model="), AZStd::string::npos); // WhiteBox has no model-product provenance.
        source->Deactivate();
        capture.m_output.clear();
        ASSERT_TRUE(console->PerformCommand("je_surfaceRay 100 -0.5 5 100 -0.5 -1").IsSuccess());
        EXPECT_NE(capture.m_output.find("status=Complete hits=0"), AZStd::string::npos);
        EXPECT_EQ(capture.m_output.find("surface="), AZStd::string::npos);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayRealRuntimeSourcesMergeNearestAndOwnerTiesDeterministically)
    {
        auto nearData = MakeSurfaceRayLayers();
        nearData.m_faces.resize(3); // Two floors and a ceiling in the actual runtime mesh.
        JE::Geometry::WhiteBoxRenderData otherData;
        otherData.m_faces.push_back(MakeSurfaceRayTriangle(
            {-2, -2, 3}, {2, -2, 3}, {0, 2, 3}, AZ::Uuid::CreateString(SurfaceRayWallId), 19));
        const auto world = AZ::Transform::CreateTranslation({100, 0, 0});
        // Register the larger owner first; insertion order must not decide equal-distance results.
        auto other = CreateRuntimeQuerySource(otherData, world, AZ::EntityId(702002));
        auto nearSource = CreateRuntimeQuerySource(nearData, world, AZ::EntityId(702001));
        AZStd::array<SurfaceQueries::SurfaceRayHit, 2> hits{};
        auto request = QueryAt(100);
        SurfaceQueries::SurfaceRayQueryResult result;
        SurfaceQueries::SurfaceDataSystemRequestBus::BroadcastResult(result,
            &SurfaceQueries::SurfaceDataSystem::QueryRay, request, AZStd::span<SurfaceQueries::SurfaceRayHit>(hits));
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        ASSERT_EQ(result.m_hitCount, 2);
        EXPECT_EQ(result.m_discardedHitCount, 2);
        EXPECT_EQ(result.m_supportedProviderCount, 2);
        EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 4.0f);
        EXPECT_FLOAT_EQ(hits[1].m_worldPosition.GetZ(), 3.0f);
        EXPECT_EQ(hits[0].m_entityAndComponent.GetEntityId(), nearSource->GetId());
        EXPECT_EQ(hits[1].m_entityAndComponent.GetEntityId(), other->GetId());
        EXPECT_EQ(hits[1].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
        ASSERT_TRUE(hits[1].m_materialSlot.has_value());
        EXPECT_EQ(*hits[1].m_materialSlot, 19);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
        const auto zeroCapacity = m_querySystem->QueryRay(request, {});
        EXPECT_EQ(zeroCapacity.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(zeroCapacity.m_hitCount, 0);
        EXPECT_EQ(zeroCapacity.m_discardedHitCount, 4);
        EXPECT_EQ(zeroCapacity.m_supportedProviderCount, 2);
        otherData.m_faces[0] = MakeSurfaceRayTriangle(
            {-2, -2, 4}, {2, -2, 4}, {0, 2, 4}, AZ::Uuid::CreateString(SurfaceRayWallId), 19);
        other->FindComponent<JE::Geometry::WhiteBoxComponent>()->GenerateWhiteBoxMesh(otherData);
        result = m_querySystem->QueryRay(request, AZStd::span<SurfaceQueries::SurfaceRayHit>(hits.data(), 1));
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_discardedHitCount, 3);
        EXPECT_EQ(hits[0].m_entityAndComponent.GetEntityId(), nearSource->GetId());
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayPartialPreservesKnownTruncationAndExactOwnerDoesNotHideScope)
    {
        auto source = CreateRuntimeQuerySource(MakeSurfaceRayLayers(),
            AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(702003));
        LegacyQueryEntry legacy(*m_querySystem, AZ::Aabb::CreateCenterRadius({100, 0, 2}, 4));
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits{};
        auto request = QueryAt(100);
        auto result = m_querySystem->QueryRay(request, hits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Partial);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(result.m_discardedHitCount, 2);
        EXPECT_EQ(result.m_supportedProviderCount, 1);
        EXPECT_EQ(result.m_unsupportedProviderCount, 1);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> fullHits{};
        const auto partialWithoutTruncation = m_querySystem->QueryRay(request, fullHits);
        EXPECT_EQ(partialWithoutTruncation.m_status, SurfaceQueries::SurfaceRayQueryStatus::Partial);
        EXPECT_EQ(partialWithoutTruncation.m_hitCount, 3);
        EXPECT_EQ(partialWithoutTruncation.m_discardedHitCount, 0);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_supportedProviderCount, 1);
        EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        source->Deactivate();
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::AllRegisteredProviders;
        result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0);
        EXPECT_EQ(result.m_unsupportedProviderCount, 1);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayInvalidLaterRealProviderDoesNotPublishPartialOutput)
    {
        JE::Geometry::WhiteBoxRenderData good;
        good.m_faces.push_back(MakeSurfaceRayTriangle(
            {-2, -2, 0}, {2, -2, 0}, {0, 2, 0}, AZ::Uuid::CreateString(SurfaceRayFloorId), 7));
        auto first = CreateRuntimeQuerySource(good, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(702010));
        auto request = QueryAt(100);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {first->GetId(), first->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits{};
        ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 1);
        good.m_faces[0].m_v2.m_position.SetX(AZStd::numeric_limits<float>::quiet_NaN());
        auto later = CreateRuntimeQuerySource(good, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(702011));
        LegacyQueryEntry legacy(*m_querySystem, AZ::Aabb::CreateCenterRadius({100, 0, 2}, 4));
        // Deterministic owner order observes unsupported, valid geometry, then invalid geometry; InvalidInput dominates all.
        hits[0].m_distance = 123.0f;
        hits[0].m_surfaceId = AZ::Uuid::CreateString(SurfaceRayWallId);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::AllRegisteredProviders;
        const auto result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0);
        EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayRuntimeMoveEmptyDeactivateAndTwoCyclesLeaveNoOldPoints)
    {
        const auto data = MakeSurfaceRayLayers();
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(702020));
        auto* component = source->FindComponent<JE::Geometry::WhiteBoxComponent>();
        auto request = QueryAt(100);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), component->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> hits{};
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            if (cycle != 0) { component->GenerateWhiteBoxMesh(data); source->Activate(); }
            AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM,
                AZ::Transform::CreateTranslation({100, 0, 0}));
            request.m_startWorldPosition.SetX(100); request.m_endWorldPosition.SetX(100);
            ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
            AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM,
                AZ::Transform::CreateTranslation({110, 0, 0}));
            EXPECT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 0);
            request.m_startWorldPosition.SetX(110); request.m_endWorldPosition.SetX(110);
            ASSERT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 3);
            auto collapsed = AZ::Transform::CreateTranslation({110, 0, 0});
            collapsed.SetUniformScale(0.0f);
            AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM, collapsed);
            const auto invalidSource = m_querySystem->QueryRay(request, hits);
            EXPECT_EQ(invalidSource.m_status, SurfaceQueries::SurfaceRayQueryStatus::InvalidInput);
            EXPECT_EQ(invalidSource.m_hitCount, 0);
            component->GenerateWhiteBoxMesh({});
            auto empty = m_querySystem->QueryRay(request, hits);
            EXPECT_EQ(empty.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
            EXPECT_EQ(empty.m_hitCount, 0);
            source->Deactivate();
            EXPECT_EQ(m_querySystem->GetSurfaceDataProviderHandle(source->GetId()), SurfaceQueries::InvalidSurfaceDataRegistryHandle);
            EXPECT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 0);
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayAuthoringHiddenRebuildAndBuildGameEntityUseRealSurfaceIds)
    {
        PrepareSurfaceDropCube(*m_whiteBoxComponent);
        auto request = QueryAt(0, 2, -2);
        request.m_startWorldPosition.SetY(0.2f); request.m_endWorldPosition.SetY(0.2f);
        request.m_startWorldPosition.SetX(0.125f); request.m_endWorldPosition.SetX(0.125f);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {m_whiteBoxEntityId, m_whiteBoxComponent->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        auto result = m_querySystem->QueryRay(request, hits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 2);
        EXPECT_EQ(hits[0].m_surfaceId, SurfaceDropTopId);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 0.5f);
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        for (const auto vertex : Api::MeshVertexHandles(mesh))
        {
            Api::SetVertexPosition(mesh, vertex, Api::VertexPosition(mesh, vertex) + AZ::Vector3(0, 0, 4));
        }
        m_whiteBoxComponent->RebuildWhiteBox();
        EXPECT_EQ(m_querySystem->QueryRay(request, hits).m_hitCount, 0);
        request.m_startWorldPosition.SetZ(6); request.m_endWorldPosition.SetZ(2);
        result = m_querySystem->QueryRay(request, hits);
        ASSERT_EQ(result.m_hitCount, 2);
        EXPECT_EQ(hits[0].m_surfaceId, SurfaceDropTopId);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 4.5f);
        EXPECT_EQ(hits[0].m_entityAndComponent, request.m_owner);
        const auto authoredRevision = hits[0].m_sourceRevision;
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        EnsureQueryDescriptor<AzFramework::TransformComponent>();
        auto runtime = AZStd::make_unique<AZ::Entity>("Hidden authored surface runtime");
        runtime->CreateComponent<AzFramework::TransformComponent>();
        static_cast<AzToolsFramework::Components::EditorComponentBase*>(m_whiteBoxComponent)->BuildGameEntity(runtime.get());
        runtime->Init(); runtime->Activate();
        auto* runtimeWhiteBox = runtime->FindComponent<JE::Geometry::WhiteBoxComponent>();
        ASSERT_NE(runtimeWhiteBox, nullptr);
        request.m_owner = {runtime->GetId(), runtimeWhiteBox->GetId()};
        result = m_querySystem->QueryRay(request, hits);
        ASSERT_EQ(result.m_hitCount, 2);
        EXPECT_EQ(hits[0].m_surfaceId, SurfaceDropTopId);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 4.5f);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hits[0].m_sourceRevision, authoredRevision);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayRaysOnlyDoesNotInjectXyPointsAndInvalidOwnerDoesNotWrite)
    {
        SurfaceQueries::SurfacePointList points;
        m_querySystem->GetSurfacePoints({0.125f, 0.2f, 0}, {}, points);
        EXPECT_TRUE(points.IsEmpty());
        auto request = QueryAt(0);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits{};
        hits[0].m_distance = 123.0f;
        const auto result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayEmptyGlobalAndUnregisteredExactOwnerHaveDifferentStatuses)
    {
        auto request = QueryAt(1000);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits{};
        hits[0].m_distance = 123.0f;
        auto result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0);
        EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {AZ::EntityId(799999), AZ::ComponentId(12)};
        result = m_querySystem->QueryRay(request, hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0);
        EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
    }

    // Unit-level protocol inputs only: this does NOT stand in for the pending Scene/ModelAsset producer or cook identity.
    class ContractRayMetadataProvider
        : public JE::Geometry::SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        ContractRayMetadataProvider(SurfaceQueries::SurfaceDataSystem& system, const SurfaceQueries::SurfaceRayHit& hit,
            AZStd::optional<AZ::Aabb> registeredBounds = AZStd::nullopt,
            SurfaceQueries::SurfaceDataQueryCapabilities capabilities = SurfaceQueries::SurfaceDataQueryCapabilities::Rays)
            : m_system(system), m_hit(hit)
        {
            SurfaceQueries::SurfaceDataRegistryEntry entry;
            entry.m_entityId = hit.m_entityAndComponent.GetEntityId();
            entry.m_componentId = hit.m_entityAndComponent.GetComponentId();
            entry.m_bounds = registeredBounds ? *registeredBounds : AZ::Aabb::CreateCenterRadius(hit.m_worldPosition, 0.1f);
            entry.m_queryCapabilities = capabilities;
            entry.m_maxPointsCreatedPerInput = SurfaceQueries::HasSurfaceQueryCapability(
                capabilities, SurfaceQueries::SurfaceDataQueryCapabilities::Points) ? 1 : 0;
            m_handle = m_system.RegisterSurfaceDataProvider(entry);
            EXPECT_NE(m_handle, SurfaceQueries::InvalidSurfaceDataRegistryHandle);
            if (m_handle != SurfaceQueries::InvalidSurfaceDataRegistryHandle) { BusConnect(m_handle); }
        }
        ~ContractRayMetadataProvider()
        {
            BusDisconnect();
            if (m_handle != SurfaceQueries::InvalidSurfaceDataRegistryHandle) { m_system.UnregisterSurfaceDataProvider(m_handle); }
        }
        SurfaceQueries::SurfaceRayQueryResult QueryRay(
            const SurfaceQueries::SurfaceRayRequest&, AZStd::span<SurfaceQueries::SurfaceRayHit> output) const override
        {
            ++m_queryCalls;
            SurfaceQueries::SurfaceRayQueryResult result;
            if (m_pending)
            {
                result.m_status = SurfaceQueries::SurfaceRayQueryStatus::Unsupported;
                return result;
            }
            if (output.empty())
            {
                result.m_status = SurfaceQueries::SurfaceRayQueryStatus::Truncated;
                result.m_discardedHitCount = 1;
            }
            else
            {
                output[0] = m_hit;
                result.m_hitCount = 1;
            }
            return result;
        }
        void GetSurfacePoints(const AZ::Vector3&, SurfaceQueries::SurfacePointList&) const override {}
        AZStd::optional<AZ::Aabb> GetSurfaceRayBounds() const override
        {
            ++m_boundsCalls;
            return m_rayBounds ? m_rayBounds : SurfaceQueries::SurfaceDataProviderRequests::GetSurfaceRayBounds();
        }
        void SetRayBounds(AZStd::optional<AZ::Aabb> bounds) { m_rayBounds = bounds; }
        void SetPending(bool pending) { m_pending = pending; }
        size_t BoundsCalls() const { return m_boundsCalls; }
        size_t QueryCalls() const { return m_queryCalls; }

    private:
        SurfaceQueries::SurfaceDataSystem& m_system;
        SurfaceQueries::SurfaceDataRegistryHandle m_handle;
        SurfaceQueries::SurfaceRayHit m_hit;
        AZStd::optional<AZ::Aabb> m_rayBounds;
        bool m_pending = false;
        mutable size_t m_boundsCalls = 0;
        mutable size_t m_queryCalls = 0;
    };

    SurfaceQueries::SurfaceRayHit MakeContractRayHit(uint32_t lod, uint32_t submesh, size_t primitive)
    {
        SurfaceQueries::SurfaceRayHit hit;
        hit.m_worldPosition = {100, -0.5f, 2};
        hit.m_worldNormal = AZ::Vector3::CreateAxisZ();
        hit.m_distance = 3;
        hit.m_entityAndComponent = {AZ::EntityId(709001), AZ::ComponentId(709002)};
        hit.m_primitiveIndex = primitive;
        SurfaceQueries::SurfaceModelProvenance source;
        // An explicit test AssetId, not an invented production SurfaceId or a claim about an installed cooked asset.
        source.m_modelAssetId = AZ::Data::AssetId(AZ::Uuid::CreateString("{38D94778-87D3-4F00-8332-AB6826B634C9}"), 17);
        source.m_lodIndex = lod;
        source.m_submeshIndex = submesh;
        hit.m_modelProvenance = source;
        return hit;
    }

    TEST_F(WhiteBoxSurfaceMaterialTestFixture, SurfaceRayHitOptionalMaterialSlotDistinguishesUnavailableFromZero)
    {
        SurfaceQueries::SurfaceRayHit hit;
        EXPECT_FALSE(hit.m_materialSlot.has_value());
        EXPECT_FALSE(hit.m_modelProvenance.has_value());
        EXPECT_TRUE(hit.m_surfaceId.IsNull());
        EXPECT_EQ(hit.m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
        hit.m_materialSlot = 0;
        ASSERT_TRUE(hit.m_materialSlot.has_value());
        EXPECT_EQ(*hit.m_materialSlot, 0);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayWhiteBoxKeepsRealZeroSlotAndAuthoredSurfaceWithoutModelProvenance)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto surface = AZ::Uuid::CreateString(SurfaceRayWallId);
        data.m_faces.push_back(MakeSurfaceRayTriangle({-2, -2, 2}, {2, -2, 2}, {0, 2, 2}, surface, 0));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(709003));
        auto* whiteBox = source->FindComponent<JE::Geometry::WhiteBoxComponent>();
        ASSERT_NE(whiteBox, nullptr);
        auto request = QueryAt(100);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), whiteBox->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
        const auto result = m_querySystem->QueryRay(request, hits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(hits[0].m_surfaceId, surface);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 0);
        EXPECT_FALSE(hits[0].m_modelProvenance.has_value());
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRaySubmeshPrecedesPrimitiveBeforeGlobalTruncation)
    {
        for (bool reverse : {false, true})
        {
            auto lowerSubmesh = MakeContractRayHit(0, 2, 9);
            lowerSubmesh.m_materialSlot = 0;
            const auto higherSubmesh = MakeContractRayHit(0, 5, 0); // Material genuinely unavailable in this protocol input.
            ContractRayMetadataProvider first(*m_querySystem, reverse ? lowerSubmesh : higherSubmesh);
            ContractRayMetadataProvider second(*m_querySystem, reverse ? higherSubmesh : lowerSubmesh);
            AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
            const auto result = m_querySystem->QueryRay(QueryAt(100), hits);
            ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_EQ(result.m_discardedHitCount, 1);
            ASSERT_TRUE(hits[0].m_modelProvenance.has_value());
            EXPECT_EQ(hits[0].m_modelProvenance->m_modelAssetId, lowerSubmesh.m_modelProvenance->m_modelAssetId);
            EXPECT_EQ(hits[0].m_modelProvenance->m_lodIndex, 0);
            EXPECT_EQ(hits[0].m_modelProvenance->m_submeshIndex, 2);
            EXPECT_EQ(hits[0].m_primitiveIndex, 9);
            ASSERT_TRUE(hits[0].m_materialSlot.has_value());
            EXPECT_EQ(*hits[0].m_materialSlot, 0);
            EXPECT_TRUE(hits[0].m_surfaceId.IsNull());
            EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Unavailable);

            AZStd::array<SurfaceQueries::SurfaceRayHit, 2> all;
            const auto full = m_querySystem->QueryRay(QueryAt(100), all);
            ASSERT_EQ(full.m_hitCount, 2);
            EXPECT_EQ(full.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
            EXPECT_FALSE(all[1].m_materialSlot.has_value());
            const auto empty = m_querySystem->QueryRay(QueryAt(100), {});
            EXPECT_EQ(empty.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
            EXPECT_EQ(empty.m_discardedHitCount, 2);
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayLodOrderAndAbsentModelAddressAreExplicit)
    {
        const auto lod0 = MakeContractRayHit(0, 9, 8);
        const auto lod1 = MakeContractRayHit(1, 0, 0); // Protocol ordering only; current ModelAsset producer still queries real LOD0.
        {
            ContractRayMetadataProvider first(*m_querySystem, lod1);
            ContractRayMetadataProvider second(*m_querySystem, lod0);
            AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
            const auto result = m_querySystem->QueryRay(QueryAt(100), hits);
            ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
            ASSERT_EQ(result.m_hitCount, 1);
            ASSERT_TRUE(hits[0].m_modelProvenance.has_value());
            EXPECT_EQ(hits[0].m_modelProvenance->m_lodIndex, 0);
            EXPECT_EQ(hits[0].m_modelProvenance->m_submeshIndex, 9);
            EXPECT_EQ(hits[0].m_primitiveIndex, 8);
        }
        auto noModel = lod0;
        noModel.m_modelProvenance.reset();
        noModel.m_primitiveIndex = 99;
        {
            ContractRayMetadataProvider first(*m_querySystem, lod0);
            ContractRayMetadataProvider second(*m_querySystem, noModel);
            AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
            const auto result = m_querySystem->QueryRay(QueryAt(100), hits);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_FALSE(hits[0].m_modelProvenance.has_value());
            EXPECT_EQ(hits[0].m_primitiveIndex, 99);
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayInvalidPresentProductAddressDoesNotPublishPartialOutput)
    {
        const auto valid = MakeContractRayHit(0, 0, 0);
        auto invalid = valid;
        invalid.m_entityAndComponent = {AZ::EntityId(709004), AZ::ComponentId(709005)};
        invalid.m_modelProvenance->m_modelAssetId = AZ::Data::AssetId();
        ContractRayMetadataProvider first(*m_querySystem, valid);
        ContractRayMetadataProvider second(*m_querySystem, invalid);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 2> hits;
        for (auto& hit : hits) { hit.m_distance = 77; hit.m_materialSlot = 77; }
        const auto result = m_querySystem->QueryRay(QueryAt(100), hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0);
        for (const auto& hit : hits)
        {
            EXPECT_FLOAT_EQ(hit.m_distance, 77);
            ASSERT_TRUE(hit.m_materialSlot.has_value());
            EXPECT_EQ(*hit.m_materialSlot, 77);
            EXPECT_FALSE(hit.m_modelProvenance.has_value());
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayUnknownPendingBoundsOutsideOldRegistryAreNotHiddenAsMiss)
    {
        const auto hit = MakeContractRayHit(0, 0, 0);
        const auto oldBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1);
        ContractRayMetadataProvider pending(*m_querySystem, hit, oldBounds);
        pending.SetRayBounds(AZ::Aabb::CreateNull()); // Present but unknown; it must not fall back to the stale registry box.
        pending.SetPending(true);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
        hits[0].m_distance = 77;
        auto result = m_querySystem->QueryRay(QueryAt(100), hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_unsupportedProviderCount, 1);
        EXPECT_EQ(pending.BoundsCalls(), 1);
        EXPECT_EQ(pending.QueryCalls(), 1);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 77);
        {
            auto otherHit = hit;
            otherHit.m_entityAndComponent = {AZ::EntityId(709006), AZ::ComponentId(709007)};
            ContractRayMetadataProvider ready(*m_querySystem, otherHit);
            result = m_querySystem->QueryRay(QueryAt(100), hits);
            EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Partial);
            EXPECT_EQ(result.m_supportedProviderCount, 1);
            EXPECT_EQ(result.m_unsupportedProviderCount, 1);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_EQ(hits[0].m_entityAndComponent, otherHit.m_entityAndComponent);
        }
        pending.SetPending(false);
        pending.SetRayBounds(AZ::Aabb::CreateCenterRadius(hit.m_worldPosition, 0.1f));
        result = m_querySystem->QueryRay(QueryAt(100), hits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 1);
        EXPECT_EQ(hits[0].m_entityAndComponent, hit.m_entityAndComponent);
        pending.SetRayBounds(oldBounds); // A valid steady box still culls the same query.
        const auto queriesBefore = pending.QueryCalls();
        result = m_querySystem->QueryRay(QueryAt(100), hits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0);
        EXPECT_EQ(pending.QueryCalls(), queriesBefore);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryRayBoundsFallbackAndCapabilityGuardsRemainCompatible)
    {
        const auto hit = MakeContractRayHit(0, 0, 0);
        const auto oldBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1);
        {
            ContractRayMetadataProvider provider(*m_querySystem, hit, oldBounds);
            AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
            auto result = m_querySystem->QueryRay(QueryAt(100), hits);
            EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(provider.BoundsCalls(), 1); // The inherited default nullopt uses the old registry box.
            EXPECT_EQ(provider.QueryCalls(), 0);
            provider.SetRayBounds(AZ::Aabb::CreateCenterRadius(hit.m_worldPosition, 0.1f));
            result = m_querySystem->QueryRay(QueryAt(100), hits);
            ASSERT_EQ(result.m_hitCount, 1); // A current steady box can also replace an obsolete registry miss.
            EXPECT_EQ(provider.QueryCalls(), 1);
        }
        for (bool missingComponent : {false, true})
        {
            auto legacyHit = hit;
            if (missingComponent) { legacyHit.m_entityAndComponent = {AZ::EntityId(709008), AZ::InvalidComponentId}; }
            const auto capability = missingComponent ? SurfaceQueries::SurfaceDataQueryCapabilities::Rays
                                                     : SurfaceQueries::SurfaceDataQueryCapabilities::Points;
            ContractRayMetadataProvider legacy(*m_querySystem, legacyHit, AZStd::nullopt, capability);
            AZStd::array<SurfaceQueries::SurfaceRayHit, 1> hits;
            const auto result = m_querySystem->QueryRay(QueryAt(100), hits);
            EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
            EXPECT_EQ(result.m_hitCount, 0);
            EXPECT_EQ(legacy.BoundsCalls(), 0);
            EXPECT_EQ(legacy.QueryCalls(), 0);
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportSelectsRealFloorsCeilingAndWallWithoutFlippingWinding)
    {
        auto data = MakeSurfaceRayLayers();
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720101));
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> candidates{};
        const SurfaceQueries::SurfaceRayBatchLimits limits{1, 256, candidates.size(), nullptr};
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        request.m_requiredWorldNormal = {0, 0, 7}; // Non-unit input is normalized, not used as an arbitrary weight.
        request.m_minNormalDot = 1.0f; // Inclusive boundary.
        auto result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_query.m_hitCount, 3);
        EXPECT_TRUE(result.m_hit->m_worldPosition.IsClose({100, -0.5f, 2}, 1e-5f));
        EXPECT_FLOAT_EQ(result.m_hit->m_distance, 3.0f);
        EXPECT_EQ(result.m_hit->m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
        EXPECT_EQ(result.m_hit->m_entityAndComponent.GetEntityId(), source->GetId());
        ASSERT_TRUE(result.m_hit->m_materialSlot.has_value());
        EXPECT_EQ(*result.m_hit->m_materialSlot, 7);
        EXPECT_EQ(result.m_hit->m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(result.m_hit->m_sourceRevision, data.m_sourceRevision);
        EXPECT_TRUE(result.m_hit->m_faceId.IsNull()); // No FaceId is manufactured for direct WhiteBox geometry.
        EXPECT_FALSE(result.m_hit->m_modelProvenance.has_value());
        for (float magnitude : {AZStd::numeric_limits<float>::max(), AZStd::numeric_limits<float>::min()})
        {
            request.m_requiredWorldNormal = {0, 0, magnitude};
            const auto scaled = m_querySystem->QuerySupport(request, candidates, limits);
            ASSERT_EQ(scaled.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
            ASSERT_TRUE(scaled.m_hit.has_value());
            EXPECT_EQ(scaled.m_hit->m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
        }
        request.m_requiredWorldNormal = {0, 0, -1};
        result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_hit->m_surfaceId, AZ::Uuid::CreateString(SurfaceRayCeilingId));
        EXPECT_TRUE(result.m_hit->m_worldNormal.IsClose({0, 0, -1}, 1e-5f));
        EXPECT_FLOAT_EQ(result.m_hit->m_distance, 1.0f);
        request.m_ray.m_startWorldPosition = {104, 0, 1};
        request.m_ray.m_endWorldPosition = {100, 0, 1};
        request.m_requiredWorldNormal = {1, 0, 0};
        result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_hit->m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
        EXPECT_TRUE(result.m_hit->m_worldPosition.IsClose({102, 0, 1}, 1e-5f));
        EXPECT_FLOAT_EQ(result.m_hit->m_distance, 2.0f);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportRejectsCompleteHitsOutsideConeWithoutInventingSupport)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720102));
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> candidates{};
        const SurfaceQueries::SurfaceRayBatchLimits limits{1, 256, candidates.size(), nullptr};
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        request.m_requiredWorldNormal = {1, 0, 0};
        request.m_minNormalDot = 0.5f;
        const auto result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_query.m_hitCount, 3);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::NoSupport);
        EXPECT_FALSE(result.m_hit.has_value());
        request.m_requiredWorldNormal = {0, 1, 1};
        request.m_minNormalDot = 0.7f;
        const auto withinCone = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(withinCone.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(withinCone.m_hit.has_value());
        EXPECT_EQ(withinCone.m_hit->m_surfaceId, AZ::Uuid::CreateString(SurfaceRayUpperFloorId));
        request.m_minNormalDot = 0.8f;
        EXPECT_EQ(m_querySystem->QuerySupport(request, candidates, limits).m_status, SurfaceQueries::SurfaceSupportStatus::NoSupport);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportTruncationCannotPublishCandidateOrClaimNoSupport)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720103));
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 1> candidates{};
        SurfaceQueries::SurfaceRayBatchLimits limits{1, 256, 1, nullptr};
        const auto result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_query.m_discardedHitCount, 2);
        EXPECT_FALSE(result.m_hit.has_value()); // Retained ceiling is not evidence that upward support is absent.
        const auto noCapacity = m_querySystem->QuerySupport(request, {}, limits);
        EXPECT_EQ(noCapacity.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(noCapacity.m_query.m_discardedHitCount, 3);
        EXPECT_FALSE(noCapacity.m_hit.has_value());
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportPartialRequiresExplicitOwnerAndPreservesZeroMaterialSlot)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        data.m_faces[2].m_materialSlot = 0;
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720104));
        LegacyQueryEntry legacy(*m_querySystem, AZ::Aabb::CreateCenterRadius({100, 0, 2}, 10));
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> candidates{};
        const SurfaceQueries::SurfaceRayBatchLimits limits{1, 256, candidates.size(), nullptr};
        auto result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Partial);
        EXPECT_FALSE(result.m_hit.has_value());
        request.m_ray.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_ray.m_owner = {source->GetId(), source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        ASSERT_TRUE(result.m_hit->m_materialSlot.has_value());
        EXPECT_EQ(*result.m_hit->m_materialSlot, 0);
        EXPECT_EQ(result.m_hit->m_sourceRevision, data.m_sourceRevision);
        request.m_ray.m_owner = {AZ::EntityId(799999), AZ::ComponentId(1)};
        result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_FALSE(result.m_hit.has_value());
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportBudgetAndCancellationLeaveScratchUncommitted)
    {
        auto data = MakeSurfaceRayLayers();
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720105));
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> candidates{};
        candidates[0].m_distance = 123.0f;
        SurfaceQueries::SurfaceRayBatchLimits limits{0, 256, candidates.size(), nullptr};
        auto result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::BudgetExhausted);
        EXPECT_FALSE(result.m_hit.has_value());
        EXPECT_FLOAT_EQ(candidates[0].m_distance, 123.0f);
        AZStd::atomic_bool cancel{true};
        limits.m_maxRayCount = 1;
        limits.m_cancellationFlag = &cancel;
        result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Indeterminate);
        EXPECT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::Cancelled);
        EXPECT_FALSE(result.m_hit.has_value());
        EXPECT_FLOAT_EQ(candidates[0].m_distance, 123.0f);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, SupportUsesCurrentOwnersAndPoseWithoutRetainedHitCache)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        auto larger = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720107));
        auto smaller = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720106));
        SurfaceQueries::SurfaceSupportRequest request;
        request.m_ray = QueryAt(100);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> candidates{};
        const SurfaceQueries::SurfaceRayBatchLimits limits{1, 256, candidates.size(), nullptr};
        auto result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_hit->m_entityAndComponent.GetEntityId(), smaller->GetId());
        AZ::TransformBus::Event(smaller->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation({200, 0, 0}));
        result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_hit->m_entityAndComponent.GetEntityId(), larger->GetId());
        larger->Deactivate();
        result = m_querySystem->QuerySupport(request, candidates, limits);
        EXPECT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::NoSupport);
        EXPECT_FALSE(result.m_hit.has_value());
        larger->Activate();
        result = m_querySystem->QuerySupport(request, candidates, limits);
        ASSERT_EQ(result.m_status, SurfaceQueries::SurfaceSupportStatus::Found);
        ASSERT_TRUE(result.m_hit.has_value());
        EXPECT_EQ(result.m_hit->m_entityAndComponent.GetEntityId(), larger->GetId());
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, ConsoleSupportConsumesRealGeometryAndReportsSelectedIdentity)
    {
        class Capture final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            Capture() { BusConnect(); }
            ~Capture() override { BusDisconnect(); }
            bool OnPrintf(const char* window, const char* message) override
            {
                if (AZStd::string_view(window) == "JE.SurfaceData") { m_output += message; }
                return false;
            }
            AZStd::string m_output;
        } capture;
        auto data = MakeSurfaceRayLayers();
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(720108));
        auto* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(console, nullptr);
        ASSERT_TRUE(console->HasCommand("je_surfaceSupport"));
        ASSERT_TRUE(console->PerformCommand("je_surfaceSupport 100 -0.5 5 100 -0.5 -1 0 0 1 0.9").IsSuccess());
        EXPECT_NE(capture.m_output.find("support=Found"), AZStd::string::npos);
        EXPECT_NE(capture.m_output.find("surface=" + AZ::Uuid::CreateString(SurfaceRayUpperFloorId).ToString<AZStd::string>()), AZStd::string::npos);
        EXPECT_NE(capture.m_output.find("revision=Content"), AZStd::string::npos);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeFindsTriangleInteriorAndBoundaryWithoutAcceptingCandidateBounds)
    {
        JE::Geometry::WhiteBoxRenderData data;
        const auto id = AZ::Uuid::CreateString(SurfaceRayFloorId);
        data.m_faces.push_back(MakeSurfaceRayTriangle({0, 0, 0}, {4, 0, 0}, {0, 4, 0}, id, 0));
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730001));
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({101, 1, -0.25f}, {101.25f, 1.25f, 0.25f});
        AZStd::array<SurfaceQueries::SurfaceRayHit, 4> hits{};
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::Finished);
        ASSERT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_query.m_hitCount, 1); // No triangle vertex or triangle edge is inside this box.
        EXPECT_TRUE(request.m_worldBounds.Contains(hits[0].m_worldPosition));
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 0.0f);
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({0, 0, 1}, 1e-5f));
        EXPECT_EQ(hits[0].m_surfaceId, id);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 0);
        EXPECT_EQ(hits[0].m_sourceRevision, data.m_sourceRevision);
        EXPECT_NEAR(hits[0].m_distance, (hits[0].m_worldPosition - request.m_worldBounds.GetCenter()).GetLength(), 1e-5f);
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({101, 1, 0}, {101.25f, 1.25f, 0.5f});
        ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 1); // Closed boundary.
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 0.0f);
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({103, 3, -0.25f}, {103.5f, 3.5f, 0.25f});
        result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_query.m_hitCount, 0); // Triangle AABB overlaps, actual triangle does not.
        // A thin triangle crosses the box, but none of the twelve box edges intersects the triangle.
        data.m_faces[0] = MakeSurfaceRayTriangle({-2, -0.05f, 0}, {2, -0.05f, 0}, {2, 0.05f, 0}, id, 0);
        source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GenerateWhiteBoxMesh(data);
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({99.75f, -0.25f, -0.25f}, {100.25f, 0.25f, 0.25f});
        ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 1);
        EXPECT_TRUE(request.m_worldBounds.Contains(hits[0].m_worldPosition));
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 0.0f);
        // World edge components 4e-6 are below the native clipper's absolute 1e-4 parallel epsilon.
        // Both triangle vertices and box-edge/triangle intersections remain absent, so the edge path must work.
        auto tinyWorld = AZ::Transform::CreateIdentity(); tinyWorld.SetUniformScale(1e-6f);
        AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM, tinyWorld);
        request.m_worldBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.25e-6f);
        ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 1);
        EXPECT_TRUE(request.m_worldBounds.Contains(hits[0].m_worldPosition));
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeLayersWindingCoincidentPrimitivesAndCapacityAreExplicit)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.push_back(data.m_faces[0]); // Distinct original primitive, identical geometry and SurfaceId.
        auto degenerate = data.m_faces[0];
        degenerate.m_v2.m_position = degenerate.m_v1.m_position; degenerate.m_v3.m_position = degenerate.m_v1.m_position;
        data.m_faces.push_back(degenerate); // Finite renderer-degenerate geometry never becomes a volume contact.
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730002));
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({97, -3, -0.5f}, {103, 3, 4.5f});
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_query.m_hitCount, 5);
        for (size_t index = 0; index < 5; ++index)
        {
            EXPECT_EQ(hits[index].m_primitiveIndex, index);
            EXPECT_EQ(hits[index].m_entityAndComponent.GetEntityId(), source->GetId());
            EXPECT_TRUE(request.m_worldBounds.Contains(hits[index].m_worldPosition));
            EXPECT_EQ(hits[index].m_sourceRevision, data.m_sourceRevision);
            EXPECT_TRUE(hits[index].m_faceId.IsNull());
        }
        EXPECT_TRUE(hits[1].m_worldNormal.IsClose({0, 0, -1}, 1e-5f));
        EXPECT_TRUE(hits[3].m_worldNormal.IsClose({1, 0, 0}, 1e-5f));
        EXPECT_EQ(hits[0].m_surfaceId, hits[4].m_surfaceId);
        result = m_querySystem->QueryVolume(request, AZStd::span<SurfaceQueries::SurfaceRayHit>(hits).first(1), limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_query.m_hitCount, 1);
        EXPECT_EQ(result.m_query.m_discardedHitCount, 4);
        EXPECT_EQ(hits[0].m_primitiveIndex, 0);
        result = m_querySystem->QueryVolume(request, {}, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_query.m_hitCount, 0);
        EXPECT_EQ(result.m_query.m_discardedHitCount, 5);
        SurfaceQueries::SurfaceDataRegistryEntry stale;
        stale.m_entityId = source->GetId();
        stale.m_componentId = source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId();
        stale.m_bounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1);
        stale.m_queryCapabilities = SurfaceQueries::SurfaceDataQueryCapabilities::RaysAndVolumes;
        stale.m_maxPointsCreatedPerInput = 0;
        const auto handle = m_querySystem->GetSurfaceDataProviderHandle(source->GetId());
        m_querySystem->UpdateSurfaceDataProvider(handle, stale);
        EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 5); // Current snapshot bounds precede rejection.
        stale.m_queryCapabilities = SurfaceQueries::SurfaceDataQueryCapabilities::Rays;
        m_querySystem->UpdateSurfaceDataProvider(handle, stale);
        result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_query.m_hitCount, 0); // Real handler implements volume, but capability was explicitly withdrawn.
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeUsesSupportedSmallScalePoseAndCurrentContentRevision)
    {
        JE::Geometry::WhiteBoxRenderData data;
        data.m_faces.push_back(MakeSurfaceRayTriangle({0, 0, 0}, {4, 0, 0}, {0, 4, 0}, AZ::Uuid::CreateString(SurfaceRayFloorId), 7));
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        const auto originalRevision = data.m_sourceRevision;
        auto world = AZ::Transform::CreateFromQuaternion(AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi));
        world.SetUniformScale(0.001f);
        world.SetTranslation({100, 0, 0});
        auto source = CreateRuntimeQuerySource(data, world, AZ::EntityId(730003));
        const auto center = world.TransformPoint({1, 1, 0});
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateCenterRadius(center, 0.00025f);
        AZStd::array<SurfaceQueries::SurfaceRayHit, 2> hits{};
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_query.m_hitCount, 1);
        EXPECT_TRUE(hits[0].m_worldNormal.IsClose({1, 0, 0}, 1e-4f));
        EXPECT_TRUE(request.m_worldBounds.Contains(hits[0].m_worldPosition));
        EXPECT_EQ(hits[0].m_sourceRevision, originalRevision); // World pose never changes local content identity.
        data.m_faces[0].m_materialSlot = 13;
        ASSERT_TRUE(JE::Geometry::UpdateWhiteBoxSourceRevision(data));
        ASSERT_NE(data.m_sourceRevision, originalRevision);
        source->FindComponent<JE::Geometry::WhiteBoxComponent>()->GenerateWhiteBoxMesh(data);
        ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 1);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value());
        EXPECT_EQ(*hits[0].m_materialSlot, 13);
        EXPECT_EQ(hits[0].m_sourceRevision, data.m_sourceRevision);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeRuntimeMoveEmptyAndTwoCyclesLeaveNoOldSurface)
    {
        const auto data = MakeSurfaceRayLayers();
        auto source = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730004));
        auto* component = source->FindComponent<JE::Geometry::WhiteBoxComponent>();
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {source->GetId(), component->GetId()};
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            if (cycle != 0) { component->GenerateWhiteBoxMesh(data); source->Activate(); }
            AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation({100, 0, 0}));
            request.m_worldBounds = AZ::Aabb::CreateFromMinMax({97, -3, -0.5f}, {103, 3, 4.5f});
            ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 4);
            AZ::TransformBus::Event(source->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation({110, 0, 0}));
            EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 0);
            request.m_worldBounds = AZ::Aabb::CreateFromMinMax({107, -3, -0.5f}, {113, 3, 4.5f});
            ASSERT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 4);
            component->GenerateWhiteBoxMesh({});
            EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
            source->Deactivate();
            EXPECT_EQ(m_querySystem->GetSurfaceDataProviderHandle(source->GetId()), SurfaceQueries::InvalidSurfaceDataRegistryHandle);
            EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 0);
        }
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeHiddenAuthoringRebuildAndBuildGameEntityPreserveSurfaceAndRevision)
    {
        PrepareSurfaceDropCube(*m_whiteBoxComponent);
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {m_whiteBoxEntityId, m_whiteBoxComponent->GetId()};
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({-1, -1, -1}, {1, 1, 1});
        AZStd::array<SurfaceQueries::SurfaceRayHit, 16> hits{};
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_query.m_hitCount, 12);
        const auto beforeRevision = hits[0].m_sourceRevision;
        request.m_worldBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.125f);
        EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 0); // Surface query is not solid occupancy.
        AzToolsFramework::SetEntityVisibility(m_whiteBoxEntityId, false);
        auto& mesh = *m_whiteBoxComponent->GetWhiteBoxMesh();
        for (const auto vertex : Api::MeshVertexHandles(mesh))
        { Api::SetVertexPosition(mesh, vertex, Api::VertexPosition(mesh, vertex) + AZ::Vector3(0, 0, 4)); }
        m_whiteBoxComponent->RebuildWhiteBox();
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({-1, -1, -1}, {1, 1, 1});
        EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_query.m_hitCount, 0);
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({-1, -1, 3}, {1, 1, 5});
        result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_query.m_hitCount, 12);
        const auto revision = hits[0].m_sourceRevision;
        EXPECT_NE(revision, beforeRevision);
        EXPECT_NE(AZStd::find_if(hits.begin(), hits.begin() + 12, [](const auto& hit) { return hit.m_surfaceId == SurfaceDropTopId; }), hits.begin() + 12);
        EnsureQueryDescriptor<AzFramework::TransformComponent>();
        auto runtime = AZStd::make_unique<AZ::Entity>("Hidden authored volume runtime");
        runtime->CreateComponent<AzFramework::TransformComponent>();
        static_cast<AzToolsFramework::Components::EditorComponentBase*>(m_whiteBoxComponent)->BuildGameEntity(runtime.get());
        runtime->Init(); runtime->Activate();
        request.m_owner = {runtime->GetId(), runtime->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        result = m_querySystem->QueryVolume(request, hits, limits);
        ASSERT_EQ(result.m_query.m_hitCount, 12);
        EXPECT_EQ(hits[0].m_sourceRevision, revision);
        EXPECT_EQ(hits[0].m_revisionState, SurfaceQueries::SurfaceSourceRevisionState::Content);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeOwnerOrderingPartialTruncationAndAbortsStayExplicit)
    {
        auto data = MakeSurfaceRayLayers();
        data.m_faces.resize(3);
        auto larger = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730006));
        auto smaller = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730005));
        LegacyQueryEntry legacy(*m_querySystem, AZ::Aabb::CreateCenterRadius({100, 0, 2}, 5));
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({97, -3, -0.5f}, {103, 3, 4.5f});
        AZStd::array<SurfaceQueries::SurfaceRayHit, 2> hits{};
        SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Partial);
        ASSERT_EQ(result.m_query.m_hitCount, 2);
        EXPECT_EQ(result.m_query.m_discardedHitCount, 4);
        EXPECT_EQ(hits[0].m_entityAndComponent.GetEntityId(), smaller->GetId());
        EXPECT_EQ(hits[0].m_primitiveIndex, 0);
        EXPECT_EQ(hits[1].m_primitiveIndex, 1);
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::ExactOwner;
        request.m_owner = {smaller->GetId(), smaller->FindComponent<JE::Geometry::WhiteBoxComponent>()->GetId()};
        result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_query.m_discardedHitCount, 1);
        hits[0].m_distance = 123.0f;
        limits.m_maxProviderVisits = 0;
        EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_completion, SurfaceQueries::SurfaceRayBatchCompletion::BudgetExhausted);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
        limits.m_maxProviderVisits = 256;
        AZStd::atomic_bool cancelled{true};
        limits.m_cancellationFlag = &cancelled;
        EXPECT_EQ(m_querySystem->QueryVolume(request, hits, limits).m_completion, SurfaceQueries::SurfaceRayBatchCompletion::Cancelled);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
        limits.m_cancellationFlag = nullptr;
        larger->Deactivate(); smaller->Deactivate();
        request.m_filter = SurfaceQueries::SurfaceRayQueryFilter::AllRegisteredProviders;
        result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_query.m_hitCount, 0);
    }

    TEST_F(WhiteBoxSurfaceQueryFixture, QueryVolumeInvalidLaterGeometryNeverPublishesEarlierStagedHit)
    {
        auto data = MakeSurfaceRayLayers();
        auto good = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730010));
        data.m_faces.back().m_v3.m_position.SetX(AZStd::numeric_limits<float>::quiet_NaN());
        auto invalid = CreateRuntimeQuerySource(data, AZ::Transform::CreateTranslation({100, 0, 0}), AZ::EntityId(730011));
        SurfaceQueries::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({97, -3, -0.5f}, {103, 3, 4.5f});
        AZStd::array<SurfaceQueries::SurfaceRayHit, 8> hits{};
        hits[0].m_distance = 123.0f;
        hits[0].m_surfaceId = AZ::Uuid::CreateString(SurfaceRayWallId);
        const SurfaceQueries::SurfaceVolumeLimits limits{256, hits.size(), nullptr};
        auto result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::InvalidInput);
        EXPECT_EQ(result.m_query.m_status, SurfaceQueries::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
        EXPECT_EQ(hits[0].m_surfaceId, AZ::Uuid::CreateString(SurfaceRayWallId));
        invalid->Deactivate();
        auto zeroScale = AZ::Transform::CreateTranslation({100, 0, 0}); zeroScale.SetUniformScale(0.0f);
        AZ::TransformBus::Event(good->GetId(), &AZ::TransformBus::Events::SetWorldTM, zeroScale);
        result = m_querySystem->QueryVolume(request, hits, limits);
        EXPECT_EQ(result.m_completion, SurfaceQueries::SurfaceRayBatchCompletion::InvalidInput);
        EXPECT_FLOAT_EQ(hits[0].m_distance, 123.0f);
    }

}
