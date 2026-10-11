/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Editor/JEEditorFixtureAuthoring.h"
#include "Editor/JEWhiteBoxDefaultShapes.h"
#include "WhiteBoxTestFixtures.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathUtils.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <cmath>

namespace UnitTest
{
    namespace Api = WhiteBox::Api;

    namespace
    {
        double Volume(const WhiteBox::WhiteBoxMesh& mesh)
        {
            double volume = 0.0;
            for (const auto& face : Api::MeshFaces(mesh))
            {
                volume += static_cast<double>(face[0].Dot(face[1].Cross(face[2]))) / 6.0;
            }
            return volume;
        }

        // A segment crossing a triangle's interior, independent of mesh triangulation.
        bool CrossesMesh(const WhiteBox::WhiteBoxMesh& mesh, const AZ::Vector3& start, const AZ::Vector3& end)
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
        const WhiteBox::DefaultShapeType shapes[] = {
            WhiteBox::DefaultShapeType::Cube, WhiteBox::DefaultShapeType::Tetrahedron,
            WhiteBox::DefaultShapeType::Icosahedron, WhiteBox::DefaultShapeType::Cylinder, WhiteBox::DefaultShapeType::Sphere};
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
                if (shapes[index] == WhiteBox::DefaultShapeType::Icosahedron)
                {
                    EXPECT_NEAR(position.GetLength(), 0.6f, 1e-5f);
                }
                if (shapes[index] == WhiteBox::DefaultShapeType::Sphere)
                {
                    EXPECT_NEAR(position.GetLength(), 0.55f, 1e-5f);
                }
                if (shapes[index] == WhiteBox::DefaultShapeType::Cube)
                {
                    EXPECT_NEAR(std::abs(position.GetX()), 0.5f, 1e-6f);
                    EXPECT_NEAR(std::abs(position.GetY()), 0.5f, 1e-6f);
                    EXPECT_NEAR(std::abs(position.GetZ()), 0.5f, 1e-6f);
                }
                if (shapes[index] == WhiteBox::DefaultShapeType::Tetrahedron)
                {
                    const float halfHeight = 0.375f * (1.0f - std::cos(AZ::DegToRad(109.5f)));
                    EXPECT_NEAR(std::abs(position.GetZ()), halfHeight, 1e-5f);
                }
                if (shapes[index] == WhiteBox::DefaultShapeType::Cylinder)
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
        EXPECT_FALSE(JE::BSP::Editor::InitializeDefaultShape(*m_whiteBox, WhiteBox::DefaultShapeType::Asset));
        EXPECT_FALSE(JE::BSP::Editor::InitializeDefaultShape(*m_whiteBox, static_cast<WhiteBox::DefaultShapeType>(-1)));
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
        m_whiteBoxComponent->SetDefaultShape(WhiteBox::DefaultShapeType::Sphere);
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
            if (path == "Tint" || (path.size() >= 5 && path.substr(path.size() - 5) == "|Tint"))
            {
                Bus::BroadcastResult(value, &Bus::Events::GetComponentProperty, component, path);
                ASSERT_TRUE(value.IsSuccess());
                const auto* tint = AZStd::any_cast<AZ::Vector3>(&value.GetValue());
                ASSERT_NE(tint, nullptr);
                EXPECT_TRUE(tint->IsClose(AZ::Vector3(0.72f)));
                readTint = true;
            }
            if (path == "Use Texture" || (path.size() >= 12 && path.substr(path.size() - 12) == "|Use Texture"))
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
}
