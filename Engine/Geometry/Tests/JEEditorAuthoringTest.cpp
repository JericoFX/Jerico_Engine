/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Editor/JEEditorFixtureAuthoring.h"
#include "Editor/JEEditorFixtureVerification.h"
#include "Editor/JEWhiteBoxDefaultShapes.h"
#include "WhiteBoxTestFixtures.h"
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathUtils.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <cmath>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

namespace UnitTest
{
    namespace Api = JE::Geometry::Api;

    namespace
    {
        double Volume(const JE::Geometry::WhiteBoxMesh& mesh)
        {
            double volume = 0.0;
            for (const auto& face : Api::MeshFaces(mesh))
            {
                volume += static_cast<double>(face[0].Dot(face[1].Cross(face[2]))) / 6.0;
            }
            return volume;
        }

        // A segment crossing a triangle's interior, independent of mesh triangulation.
        bool CrossesMesh(const JE::Geometry::WhiteBoxMesh& mesh, const AZ::Vector3& start, const AZ::Vector3& end)
        {
            const auto direction = end - start;
            for (const auto& face : Api::MeshFaces(mesh))
            {
                const auto edge1 = face[1] - face[0];
                const auto edge2 = face[2] - face[0];
                const auto p = direction.Cross(edge2);
                const float determinant = edge1.Dot(p);
                if (std::abs(determinant) < 1e-7f)
                {
                    continue;
                }
                const auto offset = start - face[0];
                const float u = offset.Dot(p) / determinant;
                const auto q = offset.Cross(edge1);
                const float v = direction.Dot(q) / determinant;
                const float t = edge2.Dot(q) / determinant;
                if (u >= -1e-6f && v >= -1e-6f && u + v <= 1.000001f && t > 0.0f && t < 1.0f)
                {
                    return true;
                }
            }
            return false;
        }
    }

    TEST_F(WhiteBoxTestFixture, JENativeDefaultShapesPreserveTopologyWindingAndStream)
    {
        const JE::Geometry::DefaultShapeType shapes[] = {
            JE::Geometry::DefaultShapeType::Cube, JE::Geometry::DefaultShapeType::Tetrahedron,
            JE::Geometry::DefaultShapeType::Icosahedron, JE::Geometry::DefaultShapeType::Cylinder, JE::Geometry::DefaultShapeType::Sphere};
        const size_t vertices[] = {8, 4, 12, 34, 162};
        const size_t faces[] = {12, 4, 20, 64, 320};
        const size_t polygons[] = {6, 4, 20, 18, 320};
        for (size_t index = 0; index < 5; ++index)
        {
            SCOPED_TRACE(index);
            ASSERT_TRUE(JE::BSP::Editor::InitializeDefaultShape(*m_whiteBox, shapes[index]));
            EXPECT_EQ(Api::MeshVertexCount(*m_whiteBox), vertices[index]);
            EXPECT_EQ(Api::MeshFaceCount(*m_whiteBox), faces[index]);
            EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), polygons[index]);
            EXPECT_GT(Volume(*m_whiteBox), 0.0);
            for (const auto& face : Api::MeshFaces(*m_whiteBox))
            {
                const auto normal = (face[1] - face[0]).Cross(face[2] - face[0]);
                EXPECT_GT(normal.Dot((face[0] + face[1] + face[2]) / 3.0f), 0.0f);
            }
            for (const auto vertex : Api::MeshVertexHandles(*m_whiteBox))
            {
                const auto position = Api::VertexPosition(*m_whiteBox, vertex);
                if (shapes[index] == JE::Geometry::DefaultShapeType::Icosahedron)
                {
                    EXPECT_NEAR(position.GetLength(), 0.6f, 1e-5f);
                }
                if (shapes[index] == JE::Geometry::DefaultShapeType::Sphere)
                {
                    EXPECT_NEAR(position.GetLength(), 0.55f, 1e-5f);
                }
                if (shapes[index] == JE::Geometry::DefaultShapeType::Cube)
                {
                    EXPECT_NEAR(std::abs(position.GetX()), 0.5f, 1e-6f);
                    EXPECT_NEAR(std::abs(position.GetY()), 0.5f, 1e-6f);
                    EXPECT_NEAR(std::abs(position.GetZ()), 0.5f, 1e-6f);
                }
                if (shapes[index] == JE::Geometry::DefaultShapeType::Tetrahedron)
                {
                    const float halfHeight = 0.375f * (1.0f - std::cos(AZ::DegToRad(109.5f)));
                    EXPECT_NEAR(std::abs(position.GetZ()), halfHeight, 1e-5f);
                }
                if (shapes[index] == JE::Geometry::DefaultShapeType::Cylinder)
                {
                    // Keep the historical 1.414 factor rather than silently changing the primitive.
                    EXPECT_NEAR(std::abs(position.GetZ()), 0.5f, 1e-4f);
                    const float xyRadius = std::sqrt(position.GetX() * position.GetX() + position.GetY() * position.GetY());
                    EXPECT_TRUE(xyRadius < 1e-5f || std::abs(xyRadius - 0.4999245f) < 1e-5f);
                }
            }
            Api::WhiteBoxMeshStream stream;
            ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, stream));
            auto restored = Api::CreateWhiteBoxMesh();
            ASSERT_EQ(Api::ReadMesh(*restored, stream), Api::ReadResult::Full);
            EXPECT_EQ(Api::MeshFaces(*restored), Api::MeshFaces(*m_whiteBox));
            for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
            {
                for (const auto halfedge : Api::FaceHalfedgeHandles(*m_whiteBox, face))
                {
                    const auto uv = Api::HalfedgeUV(*m_whiteBox, halfedge);
                    EXPECT_TRUE(std::isfinite(uv.GetX()) && std::isfinite(uv.GetY()));
                    EXPECT_EQ(Api::HalfedgeUV(*restored, halfedge), uv);
                }
            }
        }
        const auto before = Api::MeshFaces(*m_whiteBox);
        EXPECT_FALSE(JE::BSP::Editor::InitializeDefaultShape(*m_whiteBox, JE::Geometry::DefaultShapeType::Asset));
        EXPECT_FALSE(JE::BSP::Editor::InitializeDefaultShape(*m_whiteBox, static_cast<JE::Geometry::DefaultShapeType>(-1)));
        EXPECT_EQ(Api::MeshFaces(*m_whiteBox), before);
    }

    TEST_F(WhiteBoxTestFixture, JENativeRoomHasRealDoorWindowAndPersistedGeometry)
    {
        auto result = JE::BSP::Editor::BuildRoomFixture();
        ASSERT_TRUE(result.IsSuccess());
        const auto& mesh = *result.GetValue();
        ASSERT_GT(Api::MeshFaceCount(mesh), 0);
        EXPECT_NEAR(Volume(mesh), 90.90625, 1e-4);
        AZ::Vector3 minimum(100.0f), maximum(-100.0f);
        for (const auto vertex : Api::MeshVertexHandles(mesh))
        {
            const auto position = Api::VertexPosition(mesh, vertex);
            minimum = minimum.GetMin(position);
            maximum = maximum.GetMax(position);
        }
        EXPECT_TRUE(minimum.IsClose(AZ::Vector3(-5, -10, 0), 1e-5f));
        EXPECT_TRUE(maximum.IsClose(AZ::Vector3(5, 4, 4), 1e-5f));
        // Rays through the openings must miss; nearby wall rays must hit.
        EXPECT_FALSE(CrossesMesh(mesh, {0.13f, -4.5f, 1.4f}, {0.13f, -3.0f, 1.4f}));
        EXPECT_TRUE(CrossesMesh(mesh, {1.0f, -4.5f, 1.4f}, {1.0f, -3.0f, 1.4f}));
        EXPECT_FALSE(CrossesMesh(mesh, {4.0f, 0.13f, 1.8f}, {5.5f, 0.13f, 1.8f}));
        EXPECT_TRUE(CrossesMesh(mesh, {4.0f, 1.3f, 1.8f}, {5.5f, 1.3f, 1.8f}));
        Api::WhiteBoxMeshStream stream;
        ASSERT_TRUE(Api::WriteMesh(mesh, stream));
        ASSERT_EQ(Api::ReadMesh(*m_whiteBox, stream), Api::ReadResult::Full);
        EXPECT_EQ(Api::MeshFaces(*m_whiteBox), Api::MeshFaces(mesh));
        EXPECT_NEAR(Volume(*m_whiteBox), 90.90625, 1e-4);
    }

    TEST_F(WhiteBoxTestFixture, JEFixtureLevelGuardRejectsOtherScenesAndRemotePaths)
    {
        using JE::BSP::Editor::IsFixtureLevelPath;
        const AZStd::string root = AZ::IO::Path().PreferredSeparator() == AZ::IO::WindowsPathSeparator ? "F:/project" : "/project";
        const auto level = root + "/Levels/JEEditorFixture";
        EXPECT_TRUE(IsFixtureLevelPath(root, level, "JEEditorFixture"));
        EXPECT_TRUE(IsFixtureLevelPath(root, level + "/JEEditorFixture.prefab", "JEEditorFixture.prefab"));
        EXPECT_FALSE(IsFixtureLevelPath(root, root + "/other/Levels/JEEditorFixture", "JEEditorFixture"));
        EXPECT_FALSE(IsFixtureLevelPath(root, level, "Other"));
        EXPECT_FALSE(IsFixtureLevelPath(root, "//server/project/Levels/JEEditorFixture", "JEEditorFixture"));
        EXPECT_FALSE(IsFixtureLevelPath(root, "https://host/JEEditorFixture", "JEEditorFixture"));
        EXPECT_FALSE(IsFixtureLevelPath(root, "Levels/JEEditorFixture", "JEEditorFixture"));
        EXPECT_FALSE(IsFixtureLevelPath("//server/project", "//server/project/Levels/JEEditorFixture", "JEEditorFixture"));
        EXPECT_FALSE(IsFixtureLevelPath(root, level + "/../Other", "JEEditorFixture"));
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, JENativeShapeSelectorSerializesWithoutScriptRunner)
    {
        m_whiteBoxComponent->SetDefaultShape(JE::Geometry::DefaultShapeType::Sphere);
        const auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        ASSERT_NE(mesh, nullptr);
        ASSERT_EQ(Api::MeshFaceCount(*mesh), 320);
        const auto before = Api::MeshFaces(*mesh);
        Api::Clear(*m_whiteBoxComponent->GetWhiteBoxMesh());
        m_whiteBoxComponent->DeserializeWhiteBox();
        EXPECT_EQ(Api::MeshFaces(*m_whiteBoxComponent->GetWhiteBoxMesh()), before);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, JEFixtureMaterialUsesNativeReflectedValues)
    {
        const AZ::EntityComponentIdPair component(m_whiteBoxEntityId, m_whiteBoxComponent->GetId());
        ASSERT_TRUE(JE::BSP::Editor::ConfigureFixtureMaterial(component).IsSuccess());
        using Bus = AzToolsFramework::EditorComponentAPIBus;
        AZStd::vector<AZStd::string> paths;
        Bus::BroadcastResult(paths, &Bus::Events::BuildComponentPropertyList, component);
        bool readTint = false, readTexture = false;
        for (const auto& path : paths)
        {
            AzToolsFramework::EditorComponentAPIRequests::PropertyOutcome value;
            if (path == "White Box Material|Tint")
            {
                Bus::BroadcastResult(value, &Bus::Events::GetComponentProperty, component, path);
                ASSERT_TRUE(value.IsSuccess());
                const auto* tint = AZStd::any_cast<AZ::Vector3>(&value.GetValue());
                ASSERT_NE(tint, nullptr);
                EXPECT_TRUE(tint->IsClose(AZ::Vector3(0.72f)));
                readTint = true;
            }
            if (path == "White Box Material|Use Texture")
            {
                Bus::BroadcastResult(value, &Bus::Events::GetComponentProperty, component, path);
                ASSERT_TRUE(value.IsSuccess());
                const auto* useTexture = AZStd::any_cast<bool>(&value.GetValue());
                ASSERT_NE(useTexture, nullptr);
                EXPECT_FALSE(*useTexture);
                readTexture = true;
            }
        }
        EXPECT_TRUE(readTint && readTexture);
    }
    TEST_F(WhiteBoxTestFixture, FixtureRuntimeIdentityUsesPrototypeIndicesWithoutLegacyEditorMapping)
    {
        using namespace JE::BSP::Editor;
        const auto context = AZ::Uuid::CreateRandom();
        AZStd::vector<FixtureEntityObservation> entities = {
            {AZ::EntityId(11), 3, FixtureCompleteEntityName, azrtti_typeid<JE::Geometry::WhiteBoxComponent>(), true, context, 42},
            {AZ::EntityId(12), 8, "Camera", CameraComponentTypeId, true, context, 42}
        };
        const auto source = ResolveFixtureSourceIndices(entities, FixtureCompleteEntityName, "Camera");
        ASSERT_TRUE(source.IsSuccess());
        const auto runtime = ResolveFixtureRuntimeIdentity(entities, source.GetValue(), FixtureCompleteEntityName, "Camera", context, 42);
        ASSERT_TRUE(runtime.IsSuccess()); // No MapEditorIdToRuntimeId result is required by the current prefab/spawn-ticket route.
        EXPECT_EQ(runtime.GetValue().m_room, AZ::EntityId(11));
        EXPECT_EQ(runtime.GetValue().m_camera, AZ::EntityId(12));
    }

    TEST_F(WhiteBoxTestFixture, FixtureRuntimeIdentityRejectsDuplicateSourceNamesAndDuplicateSpawnedIndices)
    {
        using namespace JE::BSP::Editor;
        const auto context = AZ::Uuid::CreateRandom();
        AZStd::vector<FixtureEntityObservation> entities = {
            {AZ::EntityId(11), 3, FixtureCompleteEntityName, azrtti_typeid<JE::Geometry::WhiteBoxComponent>(), true, context, 42},
            {AZ::EntityId(12), 8, "Camera", CameraComponentTypeId, true, context, 42}
        };
        const FixtureSourceIndices indices{3, 8};
        auto duplicate = entities.back();
        duplicate.m_id = AZ::EntityId(13);
        duplicate.m_sourceIndex = 9;
        entities.push_back(duplicate);
        EXPECT_FALSE(ResolveFixtureSourceIndices(entities, FixtureCompleteEntityName, "Camera").IsSuccess());
        entities.back().m_sourceIndex = 8;
        EXPECT_FALSE(ResolveFixtureRuntimeIdentity(entities, indices, FixtureCompleteEntityName, "Camera", context, 42).IsSuccess());
    }

    TEST_F(WhiteBoxTestFixture, FixtureRuntimeIdentityRejectsWrongContextTicketStateTypeAndName)
    {
        using namespace JE::BSP::Editor;
        const auto context = AZ::Uuid::CreateRandom();
        const AZStd::vector<FixtureEntityObservation> expected = {
            {AZ::EntityId(11), 3, FixtureCompleteEntityName, azrtti_typeid<JE::Geometry::WhiteBoxComponent>(), true, context, 42},
            {AZ::EntityId(12), 8, "Camera", CameraComponentTypeId, true, context, 42}
        };
        for (size_t entityIndex = 0; entityIndex < expected.size(); ++entityIndex)
        {
            for (int mismatch = 0; mismatch < 5; ++mismatch)
            {
                auto entities = expected;
                auto& changed = entities[entityIndex];
                switch (mismatch)
                {
                case 0: changed.m_context = AZ::Uuid::CreateRandom(); break;
                case 1: changed.m_spawnTicket = 43; break;
                case 2: changed.m_active = false; break;
                case 3: changed.m_componentType = AZ::Uuid::CreateRandom(); break;
                case 4: changed.m_name = "Another entity"; break;
                }
                EXPECT_FALSE(ResolveFixtureRuntimeIdentity(entities, {3, 8}, FixtureCompleteEntityName, "Camera", context, 42).IsSuccess());
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, FixtureRuntimeIdentityCannotSubstituteAnotherCameraWithMatchingNameOrMissingRoom)
    {
        using namespace JE::BSP::Editor;
        const auto context = AZ::Uuid::CreateRandom();
        AZStd::vector<FixtureEntityObservation> entities = {
            {AZ::EntityId(11), 3, FixtureCompleteEntityName, azrtti_typeid<JE::Geometry::WhiteBoxComponent>(), true, context, 42},
            {AZ::EntityId(12), 9, "Camera", CameraComponentTypeId, true, context, 42}
        };
        EXPECT_FALSE(ResolveFixtureRuntimeIdentity(entities, {3, 8}, FixtureCompleteEntityName, "Camera", context, 42).IsSuccess());
        entities[1].m_sourceIndex = 8;
        entities.erase(entities.begin());
        EXPECT_FALSE(ResolveFixtureRuntimeIdentity(entities, {3, 8}, FixtureCompleteEntityName, "Camera", context, 42).IsSuccess());
        EXPECT_FALSE(ResolveFixtureRuntimeIdentity(entities, {3, 8}, FixtureCompleteEntityName, "Camera", {}, 42).IsSuccess());
    }

    TEST_F(WhiteBoxTestFixture, FixtureVerificationRejectsRemoteRelativeAndEscapingEvidencePaths)
    {
        using JE::BSP::Editor::ValidateFixtureVerificationOutputPath;
        QTemporaryDir workspace;
        ASSERT_TRUE(workspace.isValid());
        ASSERT_TRUE(QDir(workspace.path()).mkpath("build/n0-validation"));
        const auto root = workspace.path().toUtf8();
        const auto output = workspace.path() + "/build/n0-validation/fresh-capture";
        EXPECT_TRUE(ValidateFixtureVerificationOutputPath(root.constData(), output.toUtf8().constData()).IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), "//server/capture").IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), "https://host/capture").IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), "build/n0-validation/capture").IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), (workspace.path() + "/build/n0-validation").toUtf8().constData()).IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), (workspace.path() + "/build/n0-validation/../capture").toUtf8().constData()).IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), (workspace.path() + "/other/capture").toUtf8().constData()).IsSuccess());
        EXPECT_FALSE(QFile::exists(output)); // Validation never creates an output.
    }

    TEST_F(WhiteBoxTestFixture, FixtureVerificationPreservesExistingEvidenceAndRejectsMissingParents)
    {
        using JE::BSP::Editor::ValidateFixtureVerificationOutputPath;
        QTemporaryDir workspace;
        ASSERT_TRUE(workspace.isValid());
        ASSERT_TRUE(QDir(workspace.path()).mkpath("build/n0-validation/existing"));
        const auto output = workspace.path() + "/build/n0-validation/existing";
        QFile evidence(output + "/editor.png");
        ASSERT_TRUE(evidence.open(QIODevice::WriteOnly | QIODevice::NewOnly));
        ASSERT_EQ(evidence.write("existing manual evidence"), 24);
        evidence.close();
        const auto root = workspace.path().toUtf8();
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), output.toUtf8().constData()).IsSuccess());
        EXPECT_FALSE(ValidateFixtureVerificationOutputPath(root.constData(), (workspace.path() + "/build/n0-validation/missing/child").toUtf8().constData()).IsSuccess());
        ASSERT_TRUE(evidence.open(QIODevice::ReadOnly));
        EXPECT_EQ(evidence.readAll(), QByteArray("existing manual evidence"));
    }
    TEST_F(WhiteBoxTestFixture, FixtureVerificationRejectsEmptyUniformAndUnreadableReadbacks)
    {
        using JE::BSP::Editor::HasFixtureCaptureContent;
        EXPECT_FALSE(HasFixtureCaptureContent(QImage()));
        QImage image(4, 3, QImage::Format_ARGB32);
        image.fill(0xff000000u);
        EXPECT_FALSE(HasFixtureCaptureContent(image));
        image.fill(0xffeeeeeeu);
        EXPECT_FALSE(HasFixtureCaptureContent(image));
        image.setPixel(2, 1, 0xff123456u);
        EXPECT_TRUE(HasFixtureCaptureContent(image)); // Necessary content guard, not proof that this is a room.
    }
    TEST_F(WhiteBoxTestFixture, FixtureVerificationAcceptsRunningGameWithRequestLatchedAndWaitsForCompleteExit)
    {
        using JE::BSP::Editor::IsFixtureGameModeReady;
        EXPECT_FALSE(IsFixtureGameModeReady(true, false, false)); // Editor cannot produce a runtime capture.
        EXPECT_FALSE(IsFixtureGameModeReady(true, false, true)); // Requested, but runtime entities are not active yet.
        EXPECT_TRUE(IsFixtureGameModeReady(true, true, true)); // Actual EntityContext state during normal play.
        EXPECT_TRUE(IsFixtureGameModeReady(true, true, false));
        EXPECT_FALSE(IsFixtureGameModeReady(false, true, true));
        EXPECT_FALSE(IsFixtureGameModeReady(false, true, false)); // Exit requested, runtime still active.
        EXPECT_FALSE(IsFixtureGameModeReady(false, false, true)); // Context stopped before request flag cleared.
        EXPECT_TRUE(IsFixtureGameModeReady(false, false, false));
    }
    TEST_F(WhiteBoxTestFixture, FixtureMaterialPlanCoversRealRoomWithTwoSlotsWithoutChangingGeometryOrUVs)
    {
        using namespace JE::BSP::Editor;
        auto room = BuildRoomFixture();
        ASSERT_TRUE(room.IsSuccess());
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(*room.GetValue(), before));
        const auto plan = BuildFixtureSurfaceBindings(*room.GetValue());
        ASSERT_TRUE(plan.IsSuccess());
        size_t upper = 0, other = 0;
        for (const auto face : Api::MeshFaceHandles(*room.GetValue()))
        {
            const bool positiveZ = Api::FaceNormal(*room.GetValue(), face).Dot(AZ::Vector3::CreateAxisZ()) > 0.999f;
            const auto slot = JE::BSP::FindSurfaceMaterialSlot(plan.GetValue(), Api::FaceSurfaceId(*room.GetValue(), face));
            EXPECT_EQ(slot, positiveZ ? FixtureGridSlot : FixtureMetalSlot);
            positiveZ ? ++upper : ++other;
        }
        EXPECT_GT(upper, 0);
        EXPECT_GT(other, 0);
        ASSERT_TRUE(Api::WriteMesh(*room.GetValue(), after));
        EXPECT_EQ(after, before);
        EXPECT_NEAR(Volume(*room.GetValue()), 90.90625, 1e-4);
    }

    TEST_F(WhiteBoxTestFixture, FixtureMaterialPlanRejectsOneSurfaceSharedByIncompatibleFaces)
    {
        Api::InitializeAsUnitCube(*m_whiteBox);
        const auto key = AZ::Uuid::CreateRandom();
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox)) { Api::SetFaceSurfaceId(*m_whiteBox, face, key); }
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        EXPECT_FALSE(JE::BSP::Editor::BuildFixtureSurfaceBindings(*m_whiteBox).IsSuccess());
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(after, before);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, FixtureMaterialContractRejectsMissingAssetsAndFallbackWithoutMutation)
    {
        using namespace JE::BSP::Editor;
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        const auto plan = BuildFixtureSurfaceBindings(*mesh);
        ASSERT_TRUE(plan.IsSuccess());
        const auto gridKey = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureGridSlot);
        const auto metalKey = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureMetalSlot);
        const AZ::Data::AssetId grid(AZ::Uuid::CreateRandom(), 1), metal(AZ::Uuid::CreateRandom(), 2);
        const JE::Render::MaterialAssignmentMap expected = {
            {gridKey, JE::Render::MaterialAssignment(grid)}, {metalKey, JE::Render::MaterialAssignment(metal)}};
        auto materials = expected;
        auto bindings = plan.GetValue();
        const auto originalBindings = bindings;
        Api::WhiteBoxMeshStream before, after;
        ASSERT_TRUE(Api::WriteMesh(*mesh, before));
        AZStd::string error;
        EXPECT_TRUE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, expected, error));
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, {}, error));
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, {}, materials, expected, error));
        bindings.pop_back();
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, expected, error));
        bindings = originalBindings;
        materials.erase(metalKey);
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, expected, error));
        materials[metalKey] = JE::Render::MaterialAssignment(); // Missing product, not fallback gray.
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, expected, error));
        materials[metalKey] = JE::Render::MaterialAssignment(grid); // Same asset in both slots is not two materials.
        EXPECT_FALSE(ValidateFixtureSurfaceMaterials(*mesh, bindings, materials, expected, error));
        EXPECT_EQ(bindings, originalBindings);
        EXPECT_TRUE(m_whiteBoxComponent->GetSurfaceMaterialBindings().empty());
        ASSERT_TRUE(Api::WriteMesh(*mesh, after));
        EXPECT_EQ(after, before);
    }

    TEST_F(EditorWhiteBoxComponentTestFixture, FixtureSceneRoundtripPreservesBothSurfaceSlotsAndNativeMaterialReferences)
    {
        using namespace JE::BSP::Editor;
        auto room = BuildRoomFixture();
        ASSERT_TRUE(room.IsSuccess());
        Api::WhiteBoxMeshStream roomBytes;
        ASSERT_TRUE(Api::WriteMesh(*room.GetValue(), roomBytes));
        auto* mesh = m_whiteBoxComponent->GetWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*mesh, roomBytes), Api::ReadResult::Full);
        const auto plan = BuildFixtureSurfaceBindings(*mesh);
        ASSERT_TRUE(plan.IsSuccess());
        Api::PolygonHandles upper, other;
        for (const auto& polygon : Api::MeshPolygonHandles(*mesh))
        {
            ASSERT_FALSE(polygon.m_faceHandles.empty());
            const auto slot = JE::BSP::FindSurfaceMaterialSlot(plan.GetValue(), Api::FaceSurfaceId(*mesh, polygon.m_faceHandles.front()));
            (slot == FixtureGridSlot ? upper : other).push_back(polygon);
        }
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots(upper, FixtureGridSlot));
        ASSERT_TRUE(m_whiteBoxComponent->AssignPolygonMaterialSlots(other, FixtureMetalSlot));
        Api::WhiteBoxMeshStream assigned;
        ASSERT_TRUE(Api::WriteMesh(*mesh, assigned));
        EXPECT_EQ(assigned, roomBytes); // Material authoring must not regenerate corner UVs or identity.
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        if (!context->FindClassData(azrtti_typeid<JE::Render::MaterialAssignment>())) { JE::Render::MaterialAssignment::Reflect(context); }
        const auto gridKey = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureGridSlot);
        const auto metalKey = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureMetalSlot);
        const AZ::Data::AssetId grid(AZ::Uuid::CreateRandom(), 1), metal(AZ::Uuid::CreateRandom(), 2);
        JE::Render::MaterialAssignmentMap materials = {
            {gridKey, JE::Render::MaterialAssignment(grid)}, {metalKey, JE::Render::MaterialAssignment(metal)}};
        AZStd::vector<AZ::u8> componentBytes, materialBytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> componentStream(&componentBytes), materialStream(&materialBytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(componentStream, AZ::DataStream::ST_BINARY, m_whiteBoxComponent, context));
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(materialStream, AZ::DataStream::ST_BINARY, &materials, context));
        componentStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        materialStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        auto* reopened = AZ::Utils::LoadObjectFromStream<JE::Geometry::EditorWhiteBoxComponent>(componentStream, context);
        JE::Render::MaterialAssignmentMap reopenedMaterials;
        ASSERT_NE(reopened, nullptr);
        // This binary reference roundtrip preserves dummy asset IDs without loading asset files.
        const AZ::ObjectStream::FilterDescriptor referenceOnly{AZ::Data::AssetFilterNoAssetLoading};
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(materialStream, reopenedMaterials, context, referenceOnly));
        reopened->DeserializeWhiteBox();
        EXPECT_EQ(reopened->GetSurfaceMaterialBindings(), m_whiteBoxComponent->GetSurfaceMaterialBindings());
        AZStd::string error;
        EXPECT_TRUE(ValidateFixtureSurfaceMaterials(*reopened->GetWhiteBoxMesh(), reopened->GetSurfaceMaterialBindings(),
            reopenedMaterials, materials, error));
        EXPECT_EQ(reopenedMaterials.at(gridKey).m_materialAsset.GetId(), grid);
        EXPECT_EQ(reopenedMaterials.at(metalKey).m_materialAsset.GetId(), metal);
        Api::WhiteBoxMeshStream reopenedMesh;
        ASSERT_TRUE(Api::WriteMesh(*reopened->GetWhiteBoxMesh(), reopenedMesh));
        EXPECT_EQ(reopenedMesh, roomBytes);
        delete reopened;
        // Binary component/reference roundtrip; prefab save/reopen and Atom readiness still require the real Editor.
    }
}
