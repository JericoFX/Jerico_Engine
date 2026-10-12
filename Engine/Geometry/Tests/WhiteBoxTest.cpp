/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"
#include "Core/WhiteBoxCsg.h"
#include "Util/WhiteBoxTextureUtil.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <QApplication>
#include <limits>
#include <JE/Geometry/WhiteBoxToolApi.h>

namespace UnitTest
{
    namespace
    {
        AZ::Vector2 AuthoredUV(const AZ::Vector3& p, float offset)
        {
            return AZ::Vector2(offset + 11.0f + 2.0f * p.GetX() + 3.0f * p.GetY() + 5.0f * p.GetZ(),
                offset - 9.0f - 4.0f * p.GetX() + 7.0f * p.GetY() - p.GetZ());
        }

        void AuthorUVs(JE::Geometry::WhiteBoxMesh& mesh, float offset)
        {
            namespace Api = JE::Geometry::Api;
            for (const auto face : Api::MeshFaceHandles(mesh))
            {
                for (const auto halfedge : Api::FaceHalfedgeHandles(mesh, face))
                {
                    Api::SetHalfedgeUV(mesh, halfedge, AuthoredUV(Api::HalfedgeVertexPositionAtTip(mesh, halfedge), offset));
                }
            }
        }

        // Remove a named custom-property chunk from a real OpenMesh 2.2 stream.
        // OMWriter writes header16, name8/string, type8/string, block-size32 and data.
        // This exercises the production legacy reader without adding a legacy writer API.
        bool RemoveSurfaceProperty(JE::Geometry::Api::WhiteBoxMeshStream& bytes, const char* name)
        {
            const AZStd::string propertyName(name);
            const auto found = AZStd::search(bytes.begin(), bytes.end(), propertyName.begin(), propertyName.end());
            if (found == bytes.end())
            {
                return false;
            }
            const size_t nameOffset = static_cast<size_t>(found - bytes.begin());
            if (nameOffset < 3 || static_cast<size_t>(bytes[nameOffset - 1]) != propertyName.size())
            {
                return false;
            }
            size_t cursor = nameOffset + propertyName.size();
            if (cursor >= bytes.size())
            {
                return false;
            }
            cursor += 1 + bytes[cursor]; // storage type
            if (cursor + 4 > bytes.size())
            {
                return false;
            }
            uint32_t dataSize = 0;
            for (unsigned int i = 0; i < 4; ++i)
            {
                dataSize |= static_cast<uint32_t>(bytes[cursor + i]) << (i * 8);
            }
            const size_t end = cursor + 4 + dataSize;
            if (end > bytes.size())
            {
                return false;
            }
            bytes.erase(bytes.begin() + nameOffset - 3, bytes.begin() + end);
            return true;
        }
    }

    TEST_F(WhiteBoxTestFixture, DamagedSurfacePropertyChunksPreserveDestination)
    {
        namespace Api = JE::Geometry::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::WhiteBoxMeshStream snapshot;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, snapshot));
        for (const char* name : {"je-surface-id-high", "je-surface-id-low"})
        {
            // Mutate a known-good writer fixture; production validation walks chunk headers.
            const AZStd::string propertyName(name);
            const auto found = AZStd::search(snapshot.begin(), snapshot.end(), propertyName.begin(), propertyName.end());
            ASSERT_NE(found, snapshot.end());
            const size_t nameOffset = static_cast<size_t>(found - snapshot.begin());
            ASSERT_GE(nameOffset, 3);
            ASSERT_EQ(snapshot[nameOffset - 1], propertyName.size());
            const size_t typeOffset = nameOffset + propertyName.size();
            const size_t sizeOffset = typeOffset + 1 + snapshot[typeOffset];
            ASSERT_LE(sizeOffset + 4, snapshot.size());
            uint32_t dataSize = 0;
            for (unsigned int i = 0; i < 4; ++i) { dataSize |= static_cast<uint32_t>(snapshot[sizeOffset + i]) << (i * 8); }
            ASSERT_EQ(dataSize, Api::MeshFaceCount(*m_whiteBox) * sizeof(uint64_t));
            const size_t payloadEnd = sizeOffset + 4 + dataSize;
            ASSERT_LE(payloadEnd, snapshot.size());
            auto expectRejected = [&](const Api::WhiteBoxMeshStream& damaged)
            {
                EXPECT_EQ(Api::ReadMesh(*m_whiteBox, damaged), Api::ReadResult::Error);
                Api::WhiteBoxMeshStream after;
                EXPECT_TRUE(Api::WriteMesh(*m_whiteBox, after));
                EXPECT_EQ(after, snapshot);
            };
            auto missingPayload = snapshot;
            missingPayload.erase(missingPayload.begin() + sizeOffset + 4, missingPayload.begin() + payloadEnd);
            for (size_t i = 0; i < 4; ++i) { missingPayload[sizeOffset + i] = 0; }
            expectRejected(missingPayload); // OpenMesh otherwise warns, skips and accepts a zero half.
            auto wrongType = snapshot;
            ASSERT_EQ(wrongType[typeOffset], 8);
            wrongType[typeOffset + 5] = '3';
            wrongType[typeOffset + 6] = '2'; // A registered, but incorrect uint32_t storage type.
            expectRejected(wrongType);
            auto duplicate = snapshot;
            const Api::WhiteBoxMeshStream chunk(snapshot.begin() + nameOffset - 3, snapshot.begin() + payloadEnd);
            duplicate.insert(duplicate.begin() + payloadEnd, chunk.begin(), chunk.end());
            expectRejected(duplicate);
            auto oversized = snapshot;
            for (size_t i = 0; i < 4; ++i) { oversized[sizeOffset + i] = 0xff; }
            expectRejected(oversized);
            auto truncated = snapshot;
            truncated.resize(payloadEnd - 1);
            expectRejected(truncated);
        }
    }

    TEST_F(WhiteBoxTestFixture, EmptyInputsPreserveDestinationButSerializedEmptyMeshReplacesIt)
    {
        namespace Api = JE::Geometry::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::WhiteBoxMeshStream snapshot;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, snapshot));
        EXPECT_EQ(Api::ReadMesh(*m_whiteBox, Api::WhiteBoxMeshStream{}), Api::ReadResult::Empty);
        std::stringstream empty;
        empty >> std::noskipws;
        EXPECT_EQ(Api::ReadMesh(*m_whiteBox, empty), Api::ReadResult::Empty);
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(after, snapshot);
        auto emptyMesh = Api::CreateWhiteBoxMesh();
        Api::WhiteBoxMeshStream serializedEmpty;
        ASSERT_TRUE(Api::WriteMesh(*emptyMesh, serializedEmpty));
        ASSERT_FALSE(serializedEmpty.empty());
        EXPECT_EQ(Api::ReadMesh(*m_whiteBox, serializedEmpty), Api::ReadResult::Full);
        EXPECT_EQ(Api::MeshFaceCount(*m_whiteBox), 0);
    }

    TEST_F(WhiteBoxTestFixture, SurfaceIdentityAllowsAnEntireZeroHalf)
    {
        namespace Api = JE::Geometry::Api;
        const auto polygons = Api::InitializeAsUnitCube(*m_whiteBox);
        const auto id = AZ::Uuid::CreateString("{00000000-0000-0000-0123-456789ABCDEF}");
        ASSERT_FALSE(id.IsNull());
        for (const auto face : polygons.front().m_faceHandles) { Api::SetFaceSurfaceId(*m_whiteBox, face, id); }
        Api::WhiteBoxMeshStream bytes;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, bytes));
        auto restored = Api::CreateWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*restored, bytes), Api::ReadResult::Full);
        for (const auto face : polygons.front().m_faceHandles) { EXPECT_EQ(Api::FaceSurfaceId(*restored, face), id); }
    }

    TEST_F(WhiteBoxTestFixture, SurfaceIdentityAndCornerUVsSurviveCodecAndClone)
    {
        namespace Api = JE::Geometry::Api;
        const auto polygons = Api::InitializeAsUnitCube(*m_whiteBox);
        AuthorUVs(*m_whiteBox, 23.0f);
        const auto authoredId = AZ::Uuid::CreateString("{01234567-89AB-CDEF-FEDC-BA9876543210}");
        for (const auto face : polygons[0].m_faceHandles)
        {
            Api::SetFaceSurfaceId(*m_whiteBox, face, authoredId);
        }
        Api::WhiteBoxMeshStream stream;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, stream));
        auto restored = Api::CreateWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*restored, stream), Api::ReadResult::Full);
        auto clone = Api::CloneMesh(*restored);
        ASSERT_NE(clone, nullptr);
        for (const auto* mesh : {restored.get(), clone.get()})
        {
            for (const auto face : Api::MeshFaceHandles(*mesh))
            {
                EXPECT_EQ(Api::FaceSurfaceId(*mesh, face), Api::FaceSurfaceId(*m_whiteBox, face));
                EXPECT_FALSE(Api::FaceSurfaceId(*mesh, face).IsNull());
                for (const auto halfedge : Api::FaceHalfedgeHandles(*mesh, face))
                {
                    EXPECT_TRUE(Api::HalfedgeUV(*mesh, halfedge).IsClose(
                        AuthoredUV(Api::HalfedgeVertexPositionAtTip(*mesh, halfedge), 23.0f), 1e-5f));
                }
            }
        }
        EXPECT_EQ(Api::FaceSurfaceId(*restored, polygons[0].m_faceHandles[0]), authoredId);
        EXPECT_NE(Api::FaceSurfaceId(*restored, polygons[0].m_faceHandles[0]),
            Api::FaceSurfaceId(*restored, polygons[1].m_faceHandles[0]));
    }

    TEST_F(WhiteBoxTestFixture, LegacySurfaceIdentityIsDeterministicAndThenPersisted)
    {
        namespace Api = JE::Geometry::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        AuthorUVs(*m_whiteBox, 17.0f);
        Api::WhiteBoxMeshStream legacy;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, legacy));
        ASSERT_TRUE(RemoveSurfaceProperty(legacy, "je-surface-id-high"));
        ASSERT_TRUE(RemoveSurfaceProperty(legacy, "je-surface-id-low"));
        const auto original = legacy;
        auto first = Api::CreateWhiteBoxMesh();
        auto second = Api::CreateWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*first, legacy), Api::ReadResult::Full);
        ASSERT_EQ(Api::ReadMesh(*second, legacy), Api::ReadResult::Full);
        EXPECT_EQ(legacy, original);
        Api::WhiteBoxMeshStream upgraded;
        ASSERT_TRUE(Api::WriteMesh(*first, upgraded));
        auto reopened = Api::CreateWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*reopened, upgraded), Api::ReadResult::Full);
        for (const auto face : Api::MeshFaceHandles(*first))
        {
            const auto id = Api::FaceSurfaceId(*first, face);
            EXPECT_FALSE(id.IsNull());
            EXPECT_EQ(id, Api::FaceSurfaceId(*second, face));
            EXPECT_EQ(id, Api::FaceSurfaceId(*reopened, face));
            for (const auto halfedge : Api::FaceHalfedgeHandles(*reopened, face))
            {
                EXPECT_TRUE(Api::HalfedgeUV(*reopened, halfedge).IsClose(
                    AuthoredUV(Api::HalfedgeVertexPositionAtTip(*reopened, halfedge), 17.0f), 1e-5f));
            }
        }
        AuthorUVs(*m_whiteBox, 18.0f);
        Api::WhiteBoxMeshStream otherLegacy;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, otherLegacy));
        ASSERT_TRUE(RemoveSurfaceProperty(otherLegacy, "je-surface-id-high"));
        ASSERT_TRUE(RemoveSurfaceProperty(otherLegacy, "je-surface-id-low"));
        auto otherContent = Api::CreateWhiteBoxMesh();
        ASSERT_EQ(Api::ReadMesh(*otherContent, otherLegacy), Api::ReadResult::Full);
        EXPECT_NE(Api::FaceSurfaceId(*first, Api::FaceHandle(0)), Api::FaceSurfaceId(*otherContent, Api::FaceHandle(0)));

        const auto polygons = Api::MeshPolygonHandles(*first);
        ASSERT_EQ(polygons.size(), 6);
        AZStd::vector<AZ::Uuid> polygonIds;
        for (const auto& polygon : polygons)
        {
            ASSERT_EQ(polygon.m_faceHandles.size(), 2);
            const auto id = Api::FaceSurfaceId(*first, polygon.m_faceHandles[0]);
            EXPECT_EQ(AZStd::find(polygonIds.begin(), polygonIds.end(), id), polygonIds.end());
            polygonIds.push_back(id);
            EXPECT_EQ(Api::FaceSurfaceId(*first, polygon.m_faceHandles[0]), Api::FaceSurfaceId(*first, polygon.m_faceHandles[1]));
        }
    }

    TEST_F(WhiteBoxTestFixture, NewPolygonsInRelatedClonesHaveDistinctIdentity)
    {
        namespace Api = JE::Geometry::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        auto clone = Api::CloneMesh(*m_whiteBox);
        ASSERT_NE(clone, nullptr);
        AZStd::vector<AZ::Uuid> newIds;
        for (auto* mesh : {m_whiteBox.get(), clone.get()})
        {
            const auto a = Api::AddVertex(*mesh, AZ::Vector3(2, 0, 0));
            const auto b = Api::AddVertex(*mesh, AZ::Vector3(3, 0, 0));
            const auto c = Api::AddVertex(*mesh, AZ::Vector3(2, 1, 0));
            const auto polygon = Api::AddTriPolygon(*mesh, a, b, c);
            ASSERT_EQ(polygon.m_faceHandles.size(), 1);
            newIds.push_back(Api::FaceSurfaceId(*mesh, polygon.m_faceHandles[0]));
        }
        EXPECT_FALSE(newIds[0].IsNull());
        EXPECT_FALSE(newIds[1].IsNull());
        EXPECT_NE(newIds[0], newIds[1]);
        EXPECT_EQ(Api::FaceSurfaceId(*m_whiteBox, Api::FaceHandle(0)), Api::FaceSurfaceId(*clone, Api::FaceHandle(0)));
    }

    TEST_F(WhiteBoxTestFixture, AttributedBooleansKeepSourceChartsAndIdentityThroughChain)
    {
        namespace Api = JE::Geometry::Api;
        for (const auto operation : {Api::BooleanOperation::Union, Api::BooleanOperation::Intersection, Api::BooleanOperation::Subtraction})
        {
            Api::Clear(*m_whiteBox);
            Api::InitializeAsUnitCube(*m_whiteBox);
            auto operand = Api::CreateWhiteBoxMesh();
            Api::InitializeAsUnitCube(*operand);
            AuthorUVs(*m_whiteBox, 0.0f);
            AuthorUVs(*operand, 100.0f);
            AZStd::vector<AZ::Uuid> targetIds;
            AZStd::vector<AZ::Uuid> operandIds;
            for (const auto face : Api::MeshFaceHandles(*m_whiteBox)) { targetIds.push_back(Api::FaceSurfaceId(*m_whiteBox, face)); }
            for (const auto face : Api::MeshFaceHandles(*operand)) { operandIds.push_back(Api::FaceSurfaceId(*operand, face)); }
            Api::WhiteBoxMeshStream operandBefore;
            ASSERT_TRUE(Api::WriteMesh(*operand, operandBefore));
            const AZ::Vector3 shift(0.5f, 0.0f, 0.0f);
            ASSERT_TRUE(Api::ApplyMeshBoolean(*m_whiteBox, *operand, AZ::Transform::CreateTranslation(shift), operation));
            const auto checkResult = [&]()
            {
                bool sawTarget = false;
                bool sawOperand = false;
                for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
                {
                    const auto id = Api::FaceSurfaceId(*m_whiteBox, face);
                    const bool fromTarget = AZStd::find(targetIds.begin(), targetIds.end(), id) != targetIds.end();
                    const bool fromOperand = AZStd::find(operandIds.begin(), operandIds.end(), id) != operandIds.end();
                    ASSERT_NE(fromTarget, fromOperand);
                    sawTarget |= fromTarget;
                    sawOperand |= fromOperand;
                    for (const auto halfedge : Api::FaceHalfedgeHandles(*m_whiteBox, face))
                    {
                        const auto local = Api::HalfedgeVertexPositionAtTip(*m_whiteBox, halfedge) - (fromOperand ? shift : AZ::Vector3::CreateZero());
                        EXPECT_TRUE(Api::HalfedgeUV(*m_whiteBox, halfedge).IsClose(AuthoredUV(local, fromOperand ? 100.0f : 0.0f), 1e-4f));
                    }
                }
                EXPECT_TRUE(sawTarget);
                EXPECT_TRUE(sawOperand);
            };
            checkResult();
            if (operation == Api::BooleanOperation::Union)
            {
                auto roundtrip = Api::CloneMesh(*m_whiteBox);
                ASSERT_NE(roundtrip, nullptr);
                Api::ReplaceMesh(*m_whiteBox, AZStd::move(roundtrip));
                ASSERT_TRUE(Api::ApplyMeshBoolean(*m_whiteBox, *operand, AZ::Transform::CreateTranslation(shift), Api::BooleanOperation::Union));
                checkResult();
            }
            Api::WhiteBoxMeshStream operandAfter;
            ASSERT_TRUE(Api::WriteMesh(*operand, operandAfter));
            EXPECT_EQ(operandBefore, operandAfter);
        }
    }

    TEST_F(WhiteBoxTestFixture, CoplanarGroupingPreservesSurfaceAndUVSeams)
    {
        namespace Api = JE::Geometry::Api;
        JE::BSP::Csg::TriangleMesh triangles;
        triangles.m_positions = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
        triangles.m_indices = {0, 1, 2, 0, 2, 3};
        triangles.m_cornerUVs = {0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1};
        triangles.m_surfaceKeys = {{0x0123456789abcdefULL, 0xfedcba9876543210ULL}, {0x0123456789abcdefULL, 0xfedcba9876543210ULL}};
        EXPECT_EQ(Api::Detail::GroupCoplanarTriangles(triangles).size(), 1);
        triangles.m_cornerUVs[6] = 500;
        EXPECT_EQ(Api::Detail::GroupCoplanarTriangles(triangles).size(), 2);
        ASSERT_TRUE(Api::Detail::RebuildFromTriangleMesh(*m_whiteBox, triangles));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 2);
        const auto converted = Api::Detail::ToTriangleMesh(*m_whiteBox, AZ::Transform::CreateIdentity());
        EXPECT_EQ(converted.m_cornerUVs.size(), 12);
        bool foundSeam = false;
        for (const double value : converted.m_cornerUVs) { foundSeam |= value == 500.0; }
        EXPECT_TRUE(foundSeam);
        triangles.m_cornerUVs[6] = 0;
        triangles.m_surfaceKeys[1].m_surface = 42;
        EXPECT_EQ(Api::Detail::GroupCoplanarTriangles(triangles).size(), 2);
    }

    TEST_F(WhiteBoxTestFixture, FailedAttributedOperationsAndReadsPreserveSnapshot)
    {
        namespace Api = JE::Geometry::Api;
        Api::InitializeAsUnitCube(*m_whiteBox);
        AuthorUVs(*m_whiteBox, 19.0f);
        Api::WhiteBoxMeshStream before;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, before));
        auto partial = before;
        ASSERT_TRUE(RemoveSurfaceProperty(partial, "je-surface-id-low"));
        EXPECT_EQ(Api::ReadMesh(*m_whiteBox, partial), Api::ReadResult::Error);
        JE::BSP::Csg::TriangleMesh invalid;
        invalid.m_positions = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        invalid.m_indices = {0, 1, 99};
        invalid.m_cornerUVs = {0, 0, 1, 0, 0, 1};
        invalid.m_surfaceKeys = {{1, 2}};
        EXPECT_FALSE(Api::Detail::RebuildFromTriangleMesh(*m_whiteBox, invalid));
        invalid.m_indices[2] = 2;
        invalid.m_cornerUVs.pop_back();
        EXPECT_FALSE(Api::Detail::RebuildFromTriangleMesh(*m_whiteBox, invalid));
        invalid.m_cornerUVs.push_back(1);
        invalid.m_cornerUVs[0] = std::numeric_limits<double>::infinity();
        EXPECT_FALSE(Api::Detail::RebuildFromTriangleMesh(*m_whiteBox, invalid));
        invalid.m_cornerUVs[0] = 0;
        invalid.m_surfaceKeys.clear();
        EXPECT_FALSE(Api::Detail::RebuildFromTriangleMesh(*m_whiteBox, invalid));
        auto badOperand = Api::CloneMesh(*m_whiteBox);
        ASSERT_NE(badOperand, nullptr);
        const auto halfedge = Api::FaceHalfedgeHandles(*badOperand, Api::FaceHandle(0))[0];
        Api::SetHalfedgeUV(*badOperand, halfedge, AZ::Vector2(std::numeric_limits<float>::quiet_NaN(), 0.0f));
        EXPECT_FALSE(Api::ApplyMeshBoolean(*m_whiteBox, *badOperand, AZ::Transform::CreateIdentity(), Api::BooleanOperation::Union));
        Api::WhiteBoxMeshStream after;
        ASSERT_TRUE(Api::WriteMesh(*m_whiteBox, after));
        EXPECT_EQ(before, after);
    }

    TEST_F(WhiteBoxTestFixture, ExtrusionAndFaceSplitRetainInheritedSurfaceIdentity)
    {
        namespace Api = JE::Geometry::Api;
        const auto polygons = Api::InitializeAsUnitCube(*m_whiteBox);
        const auto topId = Api::FaceSurfaceId(*m_whiteBox, polygons[0].m_faceHandles[0]);
        AZStd::vector<AZ::Uuid> originalIds;
        for (const auto& polygon : polygons) { originalIds.push_back(Api::FaceSurfaceId(*m_whiteBox, polygon.m_faceHandles[0])); }
        const auto result = Api::TranslatePolygonAppendAdvanced(*m_whiteBox, polygons[0], 0.5f);
        for (const auto face : result.m_appendedPolygonHandle.m_faceHandles) { EXPECT_EQ(Api::FaceSurfaceId(*m_whiteBox, face), topId); }
        bool sawNewWall = false;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            const auto id = Api::FaceSurfaceId(*m_whiteBox, face);
            EXPECT_FALSE(id.IsNull());
            sawNewWall |= AZStd::find(originalIds.begin(), originalIds.end(), id) == originalIds.end();
        }
        EXPECT_TRUE(sawNewWall);
        const auto splitFace = result.m_appendedPolygonHandle.m_faceHandles[0];
        const auto faces = Api::FaceVertexHandles(*m_whiteBox, splitFace);
        const auto point = (Api::VertexPosition(*m_whiteBox, faces[0]) + Api::VertexPosition(*m_whiteBox, faces[1]) + Api::VertexPosition(*m_whiteBox, faces[2])) / 3.0f;
        const auto splitVertex = Api::SplitFace(*m_whiteBox, splitFace, point);
        for (const auto halfedge : Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, splitVertex))
        {
            const auto face = Api::HalfedgeFaceHandle(*m_whiteBox, halfedge);
            ASSERT_TRUE(face.IsValid());
            EXPECT_EQ(Api::FaceSurfaceId(*m_whiteBox, face), topId);
        }
    }
    TEST(WhiteBoxTest, HandlesInitializedInvalid)
    {
        namespace Api = JE::Geometry::Api;

        Api::VertexHandle vertexHandle;
        Api::FaceHandle faceHandle;
        Api::HalfedgeHandle halfedgeHandle;
        Api::EdgeHandle edgeHandle;

        EXPECT_FALSE(vertexHandle.IsValid());
        EXPECT_FALSE(faceHandle.IsValid());
        EXPECT_FALSE(halfedgeHandle.IsValid());
        EXPECT_FALSE(edgeHandle.IsValid());
    }

    TEST(WhiteBoxTest, VertexHandlesNotEqual)
    {
        namespace Api = JE::Geometry::Api;

        Api::VertexHandle firstVertexHandle{1};
        Api::VertexHandle secondVertexHandle{2};

        EXPECT_TRUE(firstVertexHandle != secondVertexHandle);
        EXPECT_FALSE(firstVertexHandle == secondVertexHandle);
    }

    TEST(WhiteBoxTest, FaceHandlesNotEqual)
    {
        namespace Api = JE::Geometry::Api;

        Api::FaceHandle firstFaceHandle{1};
        Api::FaceHandle secondFaceHandle{2};

        EXPECT_TRUE(firstFaceHandle != secondFaceHandle);
        EXPECT_FALSE(firstFaceHandle == secondFaceHandle);
    }

    TEST(WhiteBoxTest, HalfedgeHandlesNotEqual)
    {
        namespace Api = JE::Geometry::Api;

        Api::HalfedgeHandle firstHalfedgeHandle{1};
        Api::HalfedgeHandle secondHalfedgeHandle{2};

        EXPECT_TRUE(firstHalfedgeHandle != secondHalfedgeHandle);
        EXPECT_FALSE(firstHalfedgeHandle == secondHalfedgeHandle);
    }

    TEST(WhiteBoxTest, EdgeHandlesNotEqual)
    {
        namespace Api = JE::Geometry::Api;

        Api::EdgeHandle firstEdgeHandle{1};
        Api::EdgeHandle secondEdgeHandle{2};

        EXPECT_TRUE(firstEdgeHandle != secondEdgeHandle);
        EXPECT_FALSE(firstEdgeHandle == secondEdgeHandle);
    }

    TEST_F(WhiteBoxTestFixture, ClearRemovesMeshData)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::Clear(*m_whiteBox);

        const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
        const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
        const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);
        const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
        const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(faceCount, 0);
        EXPECT_EQ(faceHandles.size(), 0);
        EXPECT_EQ(vertexCount, 0);
        EXPECT_EQ(vertexHandles.size(), 0);
        EXPECT_EQ(halfedgeHandleCount, 0);
        EXPECT_EQ(polygonHandles.size(), 0);
    }

    TEST_F(WhiteBoxTestFixture, BooleanOperationsPreserveZeroCornerUVs)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        const AZStd::array<Api::BooleanOperation, 3> operations{
            Api::BooleanOperation::Union, Api::BooleanOperation::Intersection, Api::BooleanOperation::Subtraction};
        for (const auto operation : operations)
        {
            SCOPED_TRACE(operation == Api::BooleanOperation::Union ? "Union"
                : operation == Api::BooleanOperation::Intersection ? "Intersection" : "Subtraction");
            Api::Clear(*m_whiteBox);
            Api::InitializeAsUnitCube(*m_whiteBox);
            auto operand = Api::CreateWhiteBoxMesh();
            Api::InitializeAsUnitCube(*operand);
            Api::ZeroUVs(*m_whiteBox);
            Api::ZeroUVs(*operand);
            const auto originalOperandFaces = Api::MeshFaces(*operand);

            // The authored inputs have one constant chart, including every face corner.
            for (const auto* input : {m_whiteBox.get(), operand.get()})
            {
                for (const auto faceHandle : Api::MeshFaceHandles(*input))
                {
                    for (const auto halfedgeHandle : Api::FaceHalfedgeHandles(*input, faceHandle))
                    {
                        const auto uv = Api::HalfedgeUV(*input, halfedgeHandle);
                        ASSERT_FLOAT_EQ(uv.GetX(), 0.0f);
                        ASSERT_FLOAT_EQ(uv.GetY(), 0.0f);
                    }
                }
            }

            ASSERT_TRUE(Api::ApplyMeshBoolean(
                *m_whiteBox, *operand, AZ::Transform::CreateTranslation(AZ::Vector3(0.5f, 0.0f, 0.0f)), operation));
            const auto resultFaces = Api::MeshFaceHandles(*m_whiteBox);
            ASSERT_FALSE(resultFaces.empty());
            EXPECT_THAT(Api::MeshFaces(*operand), Eq(originalOperandFaces));

            // Unit cubes overlap by half their volume; a no-op result must also fail.
            double signedVolume = 0.0;
            for (const auto& face : Api::MeshFaces(*m_whiteBox))
            {
                signedVolume += static_cast<double>(face[0].Dot(face[1].Cross(face[2]))) / 6.0;
            }
            const double expectedVolume = operation == Api::BooleanOperation::Union ? 1.5 : 0.5;
            EXPECT_NEAR(signedVolume, expectedVolume, 1e-6);

            for (const auto faceHandle : resultFaces)
            {
                const auto halfedgeHandles = Api::FaceHalfedgeHandles(*m_whiteBox, faceHandle);
                ASSERT_EQ(halfedgeHandles.size(), 3);
                for (const auto halfedgeHandle : halfedgeHandles)
                {
                    const auto uv = Api::HalfedgeUV(*m_whiteBox, halfedgeHandle);
                    EXPECT_FLOAT_EQ(uv.GetX(), 0.0f) << halfedgeHandle;
                    EXPECT_FLOAT_EQ(uv.GetY(), 0.0f) << halfedgeHandle;
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, FirstFaceOfCubeIsTop)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const AZ::Vector3 normal = Api::FaceNormal(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(WhiteBoxTestFixture, FaceEdgeHandlesEmptyEdgeHandlesReturnedWithInvalidInput)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const Api::EdgeHandles edgeHandles = Api::FaceEdgeHandles(*m_whiteBox, Api::FaceHandle{});

        EXPECT_THAT(edgeHandles.empty(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, FaceVertexHandlesEmptyVertexHandlesReturnedWithInvalidInput)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const Api::VertexHandles vertexHandles = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{});

        EXPECT_THAT(vertexHandles.empty(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, ConnectedPolyFacesWithSameNormalReturned)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;

        AZStd::array<Api::VertexHandle, 8> vhandles;
        // verts must be added in CCW order
        vhandles[0] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-1, 1, 0));
        vhandles[1] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-2, 0, 0));
        vhandles[2] = Api::AddVertex(*m_whiteBox, AZ::Vector3(-1, -1, 0));
        vhandles[3] = Api::AddVertex(*m_whiteBox, AZ::Vector3(0, -3, 0));
        vhandles[4] = Api::AddVertex(*m_whiteBox, AZ::Vector3(1, -1, 0));
        vhandles[5] = Api::AddVertex(*m_whiteBox, AZ::Vector3(2, 0, 0));
        vhandles[6] = Api::AddVertex(*m_whiteBox, AZ::Vector3(1, 1, 0));
        vhandles[7] = Api::AddVertex(*m_whiteBox, AZ::Vector3(0, 3, 0));

        AZStd::vector<Api::FaceHandle> fhandles;
        for (size_t i = 1; i < vhandles.size() - 1; ++i)
        {
            // triangle fan topology setup
            fhandles.push_back(Api::AddFace(*m_whiteBox, vhandles[0], vhandles[i], vhandles[i + 1]));
        }

        Api::CalculateNormals(*m_whiteBox);
        Api::ZeroUVs(*m_whiteBox);

        const auto sideFaceHandles = Api::SideFaceHandles(*m_whiteBox, Api::FaceHandle{0});
        const auto sideVertexHandles = Api::SideVertexHandles(*m_whiteBox, Api::FaceHandle{0});
        const auto faceNormal = Api::FaceNormal(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(sideFaceHandles, ElementsAreArray(fhandles));
        EXPECT_THAT(sideVertexHandles, ElementsAreArray(vhandles));
        EXPECT_THAT(faceNormal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(WhiteBoxTestFixture, OutgoingHalfedgesFromVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // Note to future maintainers, a half-edge is an edge between two vertices, with a direction.
        // Think of it like an arrow starting at one vertex, and then pointing to another at its tip.
        // The other vertex then has a corresponding half edge, from itself, pointing back to the first vertex.

        // Given a vertex handle, you can ask Open Mesh to iterate over half-edges from that vertex.
        // If you ask it for "outgoing" half-edges, you'll get all of the ones which start at the given vertex and point at others.
        // If you ask it for "incoming" half-edges, you'll get all of the ones which point at the given vertex and start at others.

        // Most importantly, the "handle" of a half edge is an opaque integer that is implementation version specific and does not
        // impart meaning, similar to how the actual integer file handle returned when you open a user file using 'open' does not
        // impart meaning.  You can only use it to refer to the half-edge in functions which query other aspects, such as which faces
        // it is beside or which vertices it points at or comes from.  The acutal integer number is not guarinteed to remain in order
        // or to be stable.

        // Users, however to specify vertices, and in that case, the order matters. For this unit cube,
        // vertex 0 is at the corner of the top of the cube.
        // Vertex 0 has 5 outgoing half-edges, one for each of the 5 edges that connect from vertex 0 to other vertices as the model
        // is actually made of triangles (not quads), so each quad face is actually 2 triangles.
        // It has no 6th outgoing edge because the triangle-strip like zig-zag pattern which
        // the triangles are constructed from means that vertex 0 connects to 5 other vertices via edges but isn't the nexus of all
        // 3 cube sides it touches, as that would form a triangle fan around it instead of a strip.

        AZStd::vector<Api::HalfedgeHandle> outgoingHalfedgeHandles =
            Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        ASSERT_EQ(outgoingHalfedgeHandles.size(), 5); // we are going to assume this going forward, so stop here if its not.

        for (const auto& halfedgeHandle : outgoingHalfedgeHandles)
        {
            // when we are iterating over half-edges, the "tail" is the vertex that the half-edge is coming from, so in this case,
            // because we asked it to get all the outgoing half-edges 0, that means 0 should be the tail of every such 'arrow'.
            auto tailHandle = Api::VertexHandle{ 0 };
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTail(*m_whiteBox, halfedgeHandle), tailHandle);
        }

        // On the other hand, when we ask it for which vertex is at the tip of the edge, its the vertex on the other side of the
        // half-edge, from vertex 0 - We expect it to connect to exactly 5 other vertices since there are 5 half-edges and no duplicates.
        // Specifically the closest 5 from that corner in the cube.  Because the iterator always runs counter clockwise, this order is FIXED.
        const Api::VertexHandle expectedOtherSideVertexHandles[] = {
             Api::VertexHandle{ 7 },
             Api::VertexHandle{ 4 },
             Api::VertexHandle{ 1 },
             Api::VertexHandle{ 2 },
             Api::VertexHandle{ 3 }
        };

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedOtherSideVertexHandles), outgoingHalfedgeHandles.size());
        for (size_t halfEdgeIndex = 0; halfEdgeIndex < outgoingHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, outgoingHalfedgeHandles[halfEdgeIndex]), expectedOtherSideVertexHandles[halfEdgeIndex]);
        }
    }

    TEST_F(WhiteBoxTestFixture, IncomingHalfedgesFromVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        AZStd::vector<Api::HalfedgeHandle> incomingHalfedgeHandles =
            Api::VertexIncomingHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        // see the note in the above check.
        ASSERT_EQ(incomingHalfedgeHandles.size(), 5); // we are going to assume this going forward, so stop here if its not.
        for (const auto& halfedgeHandle : incomingHalfedgeHandles)
        {
            // when we are iterating over half-edges, the "tip" is the vertex that the half-edge is pointing at,
            // because we asked it to get all the incoming half-edges pointing at vertex 0, we expect that all of them return vertex 0
            auto tailHandle = Api::VertexHandle{ 0 };
            EXPECT_EQ(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, halfedgeHandle), tailHandle);
        }

        // On the other hand, when we ask it for which vertex is at the tail of the edge, its the one on the opposite side to vertex 0
        // this should be exactly 5 other vertices, in counter clockwise order (so it should be a fixed order).
        const Api::VertexHandle expectedOtherSideVertexHandles[] = {
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 }
        };

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedOtherSideVertexHandles), incomingHalfedgeHandles.size());
        for (size_t halfEdgeIndex = 0; halfEdgeIndex < incomingHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            EXPECT_EQ(
                Api::HalfedgeVertexHandleAtTail(*m_whiteBox, incomingHalfedgeHandles[halfEdgeIndex]),
                expectedOtherSideVertexHandles[halfEdgeIndex]);
        }
    }

    TEST_F(WhiteBoxTestFixture, AllHalfedgesFromVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // see the note on the previous 2 functions.  This one gets ALL of the half edges related to a vertex
        // both incoming and outgiong.  Handles are opaque data types, so its unreliable to expect the handles
        // to be any particular value.  However, each handle should point at vertex 0 at its head or tail
        // and there is always a fixed number of them (5 incoming, 5 outgoing = 10 total).
        // However, the iterator returns them in counter clockwise order, so the verts they connect to on each end are predictable.
        AZStd::vector<Api::HalfedgeHandle> allHalfedgeHandles =
            Api::VertexHalfedgeHandles(*m_whiteBox, Api::VertexHandle{0});

        ASSERT_EQ(allHalfedgeHandles.size(), 10);

        // notice that when we ask for all halfedges, we get 5 incoming and 5 outgoing
        // and it gets the outgoing first (so the tip will be all the other verts)
        // then the incoming, so the tip will all be the vert we asked for, which is zero.
        const Api::VertexHandle expectedTipVertexHandles[] = {
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 } };

        // Similarly when we ask for all halfedges, the tail of each one is going be the outgoing
        // first, so all 0's (since we asked for 0's halfedges), and then the incoming, so the other verts.
        const Api::VertexHandle expectedTailVertexHandles[] = {
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 0 },
            Api::VertexHandle{ 7 },
            Api::VertexHandle{ 4 },
            Api::VertexHandle{ 1 },
            Api::VertexHandle{ 2 },
            Api::VertexHandle{ 3 } };

        AZStd::vector<Api::VertexHandle> actualTipVertexHandles;
        AZStd::vector<Api::VertexHandle> actualTailVertexHandles;

        ASSERT_EQ(AZ_ARRAY_SIZE(expectedTipVertexHandles),  allHalfedgeHandles.size());
        ASSERT_EQ(AZ_ARRAY_SIZE(expectedTailVertexHandles), allHalfedgeHandles.size());

        for (size_t halfEdgeIndex = 0; halfEdgeIndex < allHalfedgeHandles.size(); ++halfEdgeIndex)
        {
            actualTipVertexHandles.push_back(Api::HalfedgeVertexHandleAtTip(*m_whiteBox, allHalfedgeHandles[halfEdgeIndex]));
            actualTailVertexHandles.push_back(Api::HalfedgeVertexHandleAtTail(*m_whiteBox, allHalfedgeHandles[halfEdgeIndex]));
        }

        EXPECT_THAT(actualTipVertexHandles, ElementsAreArray(expectedTipVertexHandles, std::size(expectedTipVertexHandles)));
        EXPECT_THAT(actualTailVertexHandles, ElementsAreArray(expectedTailVertexHandles, std::size(expectedTailVertexHandles)));
    }

    TEST_F(WhiteBoxTestFixture, VerticesForFace)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexHandlesForFace = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        const Api::VertexHandle expectedVertexHandles[] = {
            Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};

        EXPECT_THAT(vertexHandlesForFace, ElementsAreArray(expectedVertexHandles, std::size(expectedVertexHandles)));
    }

    TEST_F(WhiteBoxTestFixture, SideHalfedgesForFace)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto sideHalfEdgeHandlesCollection = Api::SideBorderHalfedgeHandles(*m_whiteBox, Api::FaceHandle{1});

        const Api::HalfedgeHandle expectedHalfedgeHandles[] = {
            Api::HalfedgeHandle{2}, Api::HalfedgeHandle{6}, Api::HalfedgeHandle{8}, Api::HalfedgeHandle{0}};

        EXPECT_THAT(sideHalfEdgeHandlesCollection.size(), Eq(1));
        EXPECT_THAT(
            sideHalfEdgeHandlesCollection.front(),
            ElementsAreArray(expectedHalfedgeHandles, std::size(expectedHalfedgeHandles)));
    }

    TEST_F(WhiteBoxTestFixture, VerticesOrderedForSide)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexHandlesCollection = Api::SideBorderVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        const Api::VertexHandle vhs[] = {
            Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}};

        EXPECT_THAT(vertexHandlesCollection.size(), Eq(1));
        EXPECT_THAT(vertexHandlesCollection.front(), ElementsAreArray(vhs, std::size(vhs)));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromFaceHandle)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 0.0f)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));

        auto faceHandle = Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]);

        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), Api::FaceVertexPositions(*m_whiteBox, faceHandle)));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromPolygonHandle)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[3]));

        Api::PolygonHandle polygonHandle{Api::FaceHandles{
            Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]), Api::AddFace(*m_whiteBox, vhs[0], vhs[2], vhs[3])}};

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        EXPECT_THAT(vertices.size(), Eq(vertexPositions.size()));
        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), vertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, VertexPositionsFromVertexHandles)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        AZStd::vector<AZ::Vector3> vertices = {AZ::Vector3(0, 0, 0), AZ::Vector3(1, 0, 0), AZ::Vector3(1, 1, 0)};

        Api::VertexHandles vhs;
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[0]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[1]));
        vhs.push_back(Api::AddVertex(*m_whiteBox, vertices[2]));

        Api::AddFace(*m_whiteBox, vhs[0], vhs[1], vhs[2]);

        EXPECT_THAT(vertices, Pointwise(ContainerIsClose(), Api::VertexPositions(*m_whiteBox, vhs)));
    }

    TEST_F(WhiteBoxTestFixture, PolygonHandlesFromUnitQuad)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 1);
    }

    TEST_F(WhiteBoxTestFixture, PolygonHandlesFromUnitCube)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 6);
    }

    TEST_F(WhiteBoxTestFixture, UniqueVertexPositionsFromUnitQuad)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto vertexPositions =
            Api::PolygonVertexPositions(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}));

        EXPECT_EQ(vertexPositions.size(), 4);
    }

    TEST_F(WhiteBoxTestFixture, MultiplePolygonExtrusions)
    {
        namespace Api = JE::Geometry::Api;
        using Vh = Api::VertexHandle;
        using ::testing::ElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{14}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17}), 1.0f);

        const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
        const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
        const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
        const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
        const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

        EXPECT_EQ(polygonHandles.size(), 22);
        EXPECT_EQ(faceCount, 44);
        EXPECT_EQ(faceHandles.size(), 44);
        EXPECT_EQ(vertexCount, 24);
        EXPECT_EQ(vertexHandles.size(), 24);
        EXPECT_THAT(vertexHandles, ElementsAreArray({Vh{0},  Vh{1},  Vh{2},  Vh{3},  Vh{4},  Vh{5},  Vh{6},  Vh{7},
                                                     Vh{8},  Vh{9},  Vh{10}, Vh{11}, Vh{12}, Vh{13}, Vh{14}, Vh{15},
                                                     Vh{16}, Vh{17}, Vh{18}, Vh{19}, Vh{20}, Vh{21}, Vh{22}, Vh{23}}));
        EXPECT_EQ(halfedgeHandleCount, 132);
    }

    TEST_F(WhiteBoxTestFixture, PolygonExtrusionEmptyWithEmptyMesh)
    {
        namespace Api = JE::Geometry::Api;

        const auto polygon = Api::TranslatePolygonAppend(*m_whiteBox, Api::PolygonHandle{}, 1.0f);

        EXPECT_EQ(polygon.m_faceHandles.size(), 0);
    }

    TEST_F(WhiteBoxTestFixture, MeshSerializedAndDeserialized)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{15}), 1.0f);

        {
            const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
            const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
            const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
            const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
            const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
            const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

            EXPECT_EQ(polygonHandles.size(), 14);
            EXPECT_EQ(faceCount, 28);
            EXPECT_EQ(faceHandles.size(), 28);
            EXPECT_EQ(vertexCount, 16);
            EXPECT_EQ(vertexHandles.size(), 16);
            EXPECT_EQ(halfedgeHandleCount, 84);
        }

        AZStd::vector<AZ::u8> whiteBoxMeshData;
        Api::WriteMesh(*m_whiteBox, whiteBoxMeshData);

        m_whiteBox.reset();
        m_whiteBox = Api::CreateWhiteBoxMesh();

        Api::ReadMesh(*m_whiteBox, whiteBoxMeshData);

        {
            const auto polygonHandles = Api::MeshPolygonHandles(*m_whiteBox);
            const auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);
            const auto faceCount = Api::MeshFaceCount(*m_whiteBox);
            const auto vertexCount = Api::MeshVertexCount(*m_whiteBox);
            const auto halfedgeHandleCount = Api::MeshHalfedgeCount(*m_whiteBox);
            const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);

            EXPECT_EQ(polygonHandles.size(), 14);
            EXPECT_EQ(faceCount, 28);
            EXPECT_EQ(faceHandles.size(), 28);
            EXPECT_EQ(vertexCount, 16);
            EXPECT_EQ(vertexHandles.size(), 16);
            EXPECT_EQ(halfedgeHandleCount, 84);
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshNotDeserializedWithSkipWhiteSpaceStream)
    {
        namespace Api = JE::Geometry::Api;
        using testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        AZStd::vector<AZ::u8> serializedWhiteBox;
        Api::WriteMesh(*m_whiteBox, serializedWhiteBox);

        std::string serializedWhiteBoxStr;
        serializedWhiteBoxStr.reserve(serializedWhiteBox.size());
        AZStd::copy(
            serializedWhiteBox.cbegin(), serializedWhiteBox.cend(), AZStd::back_inserter(serializedWhiteBoxStr));

        std::stringstream whiteBoxStream;
        whiteBoxStream.str(serializedWhiteBoxStr);
        // note: std::stringstream will default to skip white space characters

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_THAT(Api::ReadMesh(*m_whiteBox, whiteBoxStream), Eq(Api::ReadResult::Error));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(WhiteBoxTestFixture, InitialiseAsUnitQuad)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        AZStd::vector<AZ::Vector3> vertexPositions = Api::MeshVertexPositions(*m_whiteBox);
        AZStd::vector<AZ::Vector3> expectedVertexPositions = {
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),
            AZ::Vector3(-0.5f, 0.0f, 0.5f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), expectedVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, MeshScalePolygon)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto midpoint = Api::PolygonMidpoint(*m_whiteBox, polygonHandle);

        Api::ScalePolygonRelative(*m_whiteBox, polygonHandle, midpoint, 0.5f);

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // result of scaling each vertex by 0.5 towards the midpoint of the quad
        const AZStd::vector<AZ::Vector3> scaledUnitQuad = {
            AZ::Vector3(-0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, 0.75f),
            AZ::Vector3(-0.75f, 0.0f, 0.75f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), scaledUnitQuad));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 1);
    }

    TEST_F(WhiteBoxTestFixture, MeshScalePolygonAppend)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        polygonHandle = Api::ScalePolygonAppendRelative(*m_whiteBox, polygonHandle, 0.5f);

        const auto vertexPositions = Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // result of scaling each vertex by 0.5 towards the midpoint of the quad
        const AZStd::vector<AZ::Vector3> scaledUnitQuad = {
            AZ::Vector3(-0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, -0.75f), AZ::Vector3(0.75f, 0.0f, 0.75f),
            AZ::Vector3(-0.75f, 0.0f, 0.75f)};

        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), scaledUnitQuad));
        EXPECT_EQ(Api::MeshPolygonHandles(*m_whiteBox).size(), 5);
    }

    TEST_F(WhiteBoxTestFixture, MeshPolygonUniqueVertexHandles)
    {
        namespace Api = JE::Geometry::Api;
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto vertexHandles = Api::PolygonVertexHandles(*m_whiteBox, polygonHandle);
        EXPECT_EQ(vertexHandles.size(), 4);
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfPolygon)
    {
        namespace Api = JE::Geometry::Api;
        auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        EXPECT_THAT(AZ::Vector3::CreateZero(), IsClose(Api::PolygonMidpoint(*m_whiteBox, polygonHandle)));
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfEdge)
    {
        namespace Api = JE::Geometry::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandle);

        for (const auto& edgeHandle : edgeHandles)
        {
            // when
            const auto tail = Api::HalfedgeVertexPositionAtTail(
                *m_whiteBox, Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First));
            const auto tip = Api::HalfedgeVertexPositionAtTip(
                *m_whiteBox, Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First));

            // computed differently to EdgeMidpoint
            const auto midpoint = tail + (tip - tail) * 0.5f;

            // then
            EXPECT_THAT(midpoint, IsClose(Api::EdgeMidpoint(*m_whiteBox, edgeHandle)));
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshMidPointOfFace)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto faceMidpoint = Api::FaceMidpoint(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(faceMidpoint, IsClose(AZ::Vector3(0.1666f, -0.1666f, 0.5f)));
    }

    TEST_F(WhiteBoxTestFixture, MeshFacesReturned)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto faces = Api::MeshFaces(*m_whiteBox);

        EXPECT_EQ(faces.size(), 2);

        const AZStd::vector<AZ::Vector3> vertexPositions = {
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),
            AZ::Vector3(-0.5f, 0.0f, -0.5f), AZ::Vector3(0.5f, 0.0f, 0.5f),  AZ::Vector3(-0.5f, 0.0f, 0.5f),
        };

        AZStd::vector<AZ::Vector3> faceVertexPositions;
        faceVertexPositions.insert(faceVertexPositions.end(), faces[0].begin(), faces[0].end());
        faceVertexPositions.insert(faceVertexPositions.end(), faces[1].begin(), faces[1].end());

        EXPECT_THAT(faceVertexPositions, Pointwise(ContainerIsClose(), vertexPositions));
    }

    // note: here we sum and then normalize unit normals of each face (the normals are not weighted)
    TEST_F(WhiteBoxTestFixture, PolygonNormalIsAverageOfFaces)
    {
        namespace Api = JE::Geometry::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto vertexHandles = Api::PolygonVertexHandles(*m_whiteBox, polygonHandle);
        const auto vertexPositions = Api::VertexPositions(*m_whiteBox, vertexHandles);

        // update the position of a single vertex to make the faces in the polygon not co-planar
        Api::SetVertexPosition(*m_whiteBox, vertexHandles[0], vertexPositions[0] + AZ::Vector3::CreateAxisY());

        // ensure we refresh normals after modifications
        Api::CalculateNormals(*m_whiteBox);

        // when
        const auto polygonNormal = PolygonNormal(*m_whiteBox, polygonHandle);

        // then
        const auto faceNormal22 = FaceNormal(*m_whiteBox, polygonHandle.m_faceHandles[0]);
        const auto faceNormal23 = FaceNormal(*m_whiteBox, polygonHandle.m_faceHandles[1]);
        const auto averageNormal = (faceNormal22 + faceNormal23).GetNormalized();

        EXPECT_THAT(polygonNormal, IsClose(averageNormal));
    }

    TEST_F(WhiteBoxTestFixture, PolygonTranslateAlongNormal)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        // given
        // use default position (Y is at origin)
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        // when
        Api::TranslatePolygon(*m_whiteBox, polygonHandle, 1.0f);

        AZStd::vector<AZ::Vector3> expectedVertexPositions = {
            AZ::Vector3(-0.5f, -1.0f, -0.5f), AZ::Vector3(0.5f, -1.0f, -0.5f), AZ::Vector3(0.5f, -1.0f, 0.5f),
            AZ::Vector3(-0.5f, -1.0f, 0.5f)};

        const AZStd::vector<AZ::Vector3> polygonVertexPositions =
            Api::PolygonVertexPositions(*m_whiteBox, polygonHandle);

        // then
        EXPECT_THAT(expectedVertexPositions, Pointwise(ContainerIsClose(), polygonVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, SpaceCreatedForPolygonIsOrthogonal)
    {
        namespace Api = JE::Geometry::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        // when
        const AZ::Vector3 polygonMidpoint = Api::PolygonMidpoint(*m_whiteBox, polygonHandle);
        const AZ::Transform polygonSpace = Api::PolygonSpace(*m_whiteBox, polygonHandle, polygonMidpoint);

        // then
        EXPECT_TRUE(polygonSpace.IsOrthogonal());
    }

    TEST_F(WhiteBoxTestFixture, SpaceCreatedForEdgeIsOrthogonal)
    {
        namespace Api = JE::Geometry::Api;

        // given
        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);
        const auto edgeHandles = Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandle);

        // when
        const AZ::Vector3 edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, edgeHandles[0]);
        const AZ::Transform edgeSpace = Api::EdgeSpace(*m_whiteBox, edgeHandles[0], edgeMidpoint);

        // then
        EXPECT_TRUE(edgeSpace.IsOrthogonal());
    }

    TEST_F(WhiteBoxTestFixture, EdgeToHalfEdgeConversionsMapCorrectly)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        // given
        const auto edgeHandles = Api::MeshEdgeHandles(*m_whiteBox);
        for (const auto& edgeHandle : edgeHandles)
        {
            const auto firstHalfedgeHandle = Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First);
            const auto secondHalfedgeHandle =
                Api::EdgeHalfedgeHandle(*m_whiteBox, edgeHandle, Api::EdgeHalfedge::First);

            // when
            const auto edgeHandleFromFirst = Api::HalfedgeEdgeHandle(*m_whiteBox, firstHalfedgeHandle);
            const auto edgeHandleFromSecond = Api::HalfedgeEdgeHandle(*m_whiteBox, secondHalfedgeHandle);

            // then
            EXPECT_EQ(edgeHandle, edgeHandleFromFirst);
            EXPECT_EQ(edgeHandle, edgeHandleFromSecond);
        }
    }

    class WhiteBoxTestUpdateVerticesFixture : public WhiteBoxTestFixture
    {
    public:
        WhiteBoxTestUpdateVerticesFixture()
            : WhiteBoxTestFixture()
        {
        }

        void SetUpEditorFixtureImpl() override
        {
            namespace Api = JE::Geometry::Api;

            Api::InitializeAsUnitCube(*m_whiteBox);

            // triangle A of the unit cube's left face tri pair
            JE::Geometry::Api::FaceHandle leftFaceHandle = JE::Geometry::Api::FaceHandle{0};

            // triangle A of the unit cube's top face tri pair
            JE::Geometry::Api::FaceHandle topFaceHandle = JE::Geometry::Api::FaceHandle{10};

            // top face polygon comprised of triangles A and B
            JE::Geometry::Api::PolygonHandle topFacePolyHandle =
                JE::Geometry::Api::FacePolygonHandle(*m_whiteBox, topFaceHandle);

            Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, leftFaceHandle), 1.0f);

            m_polygonVertexHandles = Api::PolygonVertexHandles(*m_whiteBox, topFacePolyHandle);

            const auto vertexPositions = Api::VertexPositions(*m_whiteBox, m_polygonVertexHandles);

            // translate top face upwards one unit
            size_t index = 0;
            for (auto vertexHandle : m_polygonVertexHandles)
            {
                Api::SetVertexPositionAndUpdateUVs(
                    *m_whiteBox, vertexHandle, vertexPositions[index++] + AZ::Vector3::CreateAxisZ());
            }
        }

        void TearDownEditorFixtureImpl()
        {
            // ensure we deallocate memory for the vector before the allocator is destroyed
            m_polygonVertexHandles = {};
        }

        JE::Geometry::Api::VertexHandles m_polygonVertexHandles;
    };

    TEST_F(WhiteBoxTestUpdateVerticesFixture, MeshCanUpdateVertexPositions)
    {
        namespace Api = JE::Geometry::Api;

        const AZ::Vector3 updatedVertexPositions[] = {
            AZ::Vector3{-0.5f, -0.5f, 2.5f}, AZ::Vector3{0.5f, -0.5f, 2.5f}, AZ::Vector3{0.5f, 0.5f, 2.5f},
            AZ::Vector3{-0.5f, 0.5f, 2.5f}};

        size_t index = 0;
        for (auto vertexHandle : m_polygonVertexHandles)
        {
            const auto vertexPosition = Api::VertexPosition(*m_whiteBox, vertexHandle);
            EXPECT_THAT(vertexPosition, IsClose(updatedVertexPositions[index++]));
        }
    }

    TEST_F(WhiteBoxTestUpdateVerticesFixture, MeshCanUpdateVertexUVs)
    {
        namespace Api = JE::Geometry::Api;

        // iterate over all vertex handles associated with a polygon and get all outgoing
        // half edges - check the uvs at each halfedge at the outer edge of the extruded
        // face (halfedges are on the lateral faces, opposite of halfedges on the extruded
        // polygon/face) - we lookup the uv from the halfedge handle and verify the tiling
        using Heh = Api::HalfedgeHandle;
        const Heh topHalfedgeHandles[] = {Heh{35}, Heh{37}, Heh{41}, Heh{43}};

        // expected uv coordinates given the z-axis translation applied to the top face
        const AZ::Vector2 expectedUVs[] = {
            AZ::Vector2(-2.0f, 0.0f), AZ::Vector2(1.0f, -2.0f), AZ::Vector2(-2.0f, 1.0f), AZ::Vector2(0.0f, -2.0f)};

        unsigned i = 0;
        for (auto vertexHandle : m_polygonVertexHandles)
        {
            const auto outgoingHalfedges = Api::VertexOutgoingHalfedgeHandles(*m_whiteBox, vertexHandle);
            for (auto halfedgeHandle : outgoingHalfedges)
            {
                auto halfedgeHandleIt =
                    AZStd::find(std::begin(topHalfedgeHandles), std::end(topHalfedgeHandles), halfedgeHandle);

                if (halfedgeHandleIt != std::end(topHalfedgeHandles))
                {
                    const AZ::Vector2 uv = Api::HalfedgeUV(*m_whiteBox, halfedgeHandle);
                    EXPECT_THAT(uv, IsClose(expectedUVs[i++]));
                }
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeAppendedToWhiteBoxCubeConnectedByQuadPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountBefore = Api::MeshFaceCount(*m_whiteBox);

        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{1}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, nextEdgeHandle);
        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountAfter = Api::MeshFaceCount(*m_whiteBox);

        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{19}));
        EXPECT_THAT(edgeMidpoint, IsClose(AZ::Vector3{0.0f, 0.0f, 1.0f}));
        EXPECT_THAT(polygonCountAfter - polygonCountBefore, Eq(3));
        EXPECT_THAT(faceCountAfter - faceCountBefore, Eq(4));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeAppendedToWhiteBoxCubeConnectedByTriPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // quad edge extrusion - same as EdgeCanBeAppendedToWhiteBoxCubeConnectedByTwoQuadPolygons
        Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{1}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountBefore = Api::MeshFaceCount(*m_whiteBox);

        // triangle edge extrusion
        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{0}, AZ::Vector3{0.0f, -0.25f, 0.25f});

        const auto edgeMidpoint = Api::EdgeMidpoint(*m_whiteBox, nextEdgeHandle);
        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto faceCountAfter = Api::MeshFaceCount(*m_whiteBox);

        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{26}));
        EXPECT_THAT(edgeMidpoint, IsClose(AZ::Vector3{0.0f, -0.75f, 0.75f}));
        EXPECT_THAT(polygonCountAfter - polygonCountBefore, Eq(3));
        EXPECT_THAT(faceCountAfter - faceCountBefore, Eq(4));
    }

    TEST_F(WhiteBoxTestFixture, HidingEdgeCreatesNewPolygonHandleWithCombinedFaceHandles)
    {
        namespace Api = JE::Geometry::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);

        const auto beforeHidePolygonHandleFromFaceHandle_0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto beforeHidePolygonHandleFromFaceHandle_1 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1});
        const auto beforeHidePolygonHandleFromFaceHandle_16 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16});
        const auto beforeHidePolygonHandleFromFaceHandle_17 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17});

        // hide top edge
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{1});

        const auto afterHidePolygonHandleFromFaceHandle_0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto afterHidePolygonHandleFromFaceHandle_1 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1});
        const auto afterHidePolygonHandleFromFaceHandle_16 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16});
        const auto afterHidePolygonHandleFromFaceHandle_17 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{17});

        const auto beforeHidePolygonHandlesFaceHandle_0_1_Expected =
            Api::PolygonHandle{Api::FaceHandles{Api::FaceHandle{0}, Api::FaceHandle{1}}};
        const auto beforeHidePolygonHandlesFaceHandle_16_17_Expected =
            Api::PolygonHandle{Api::FaceHandles{Api::FaceHandle{16}, Api::FaceHandle{17}}};

        // two separate top polygons after append/extrusion
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_0, beforeHidePolygonHandlesFaceHandle_0_1_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_1, beforeHidePolygonHandlesFaceHandle_0_1_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_16, beforeHidePolygonHandlesFaceHandle_16_17_Expected);
        EXPECT_EQ(beforeHidePolygonHandleFromFaceHandle_17, beforeHidePolygonHandlesFaceHandle_16_17_Expected);

        const auto afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected = Api::PolygonHandle{
            Api::FaceHandles{{Api::FaceHandle{0}, Api::FaceHandle{1}, Api::FaceHandle{16}, Api::FaceHandle{17}}}};

        // single top polygon after hiding edge
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_0, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_1, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_16, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
        EXPECT_EQ(afterHidePolygonHandleFromFaceHandle_17, afterHidePolygonHandlesFaceHandle_1_0_16_17_Expected);
    }

    TEST_F(WhiteBoxTestFixture, FlipInternalEdgeOfQuadSucceeds)
    {
        namespace Api = JE::Geometry::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto beforeFlipVertexHandles =
            AZStd::array<Api::VertexHandle, 2>{Api::VertexHandle{2}, Api::VertexHandle{0}};
        const auto afterFlipVertexHandles =
            AZStd::array<Api::VertexHandle, 2>{Api::VertexHandle{3}, Api::VertexHandle{1}};

        const auto beforeFlipExpectedVertexHandles = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2});
        EXPECT_EQ(beforeFlipExpectedVertexHandles, beforeFlipVertexHandles);

        // flip diagonal edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_TRUE(result);

        const auto afterFlipExpectedVertexHandles = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2});
        EXPECT_EQ(afterFlipExpectedVertexHandles, afterFlipVertexHandles);
    }

    TEST_F(WhiteBoxTestFixture, FlipOuterEdgeOfQuadReturnsFalse)
    {
        namespace Api = JE::Geometry::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        // attempt to flip outer edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{0});

        EXPECT_EQ(result, false);
    }

    TEST_F(WhiteBoxTestFixture, FlipVisibleEdgeReturnsFalse)
    {
        namespace Api = JE::Geometry::Api;

        [[maybe_unused]] auto polygonHandles = Api::InitializeAsUnitQuad(*m_whiteBox);

        {
            Api::EdgeHandles restoringEdgeHandles;
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{2}, restoringEdgeHandles);
        }

        // attempt to flip outer edge
        bool result = Api::FlipEdge(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_EQ(result, false);
    }

    TEST_F(WhiteBoxTestFixture, EdgeCannotBeAppendedWhenPolygonHasMoreThanTwoFaces)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // quad face extrusion
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{7}), 1.0f);
        // hide top edge
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{1});

        const auto polygonCountBefore = Api::MeshPolygonHandles(*m_whiteBox).size();

        // attempt to perform an edge append
        const auto nextEdgeHandle =
            Api::TranslateEdgeAppend(*m_whiteBox, Api::EdgeHandle{20}, AZ::Vector3{-0.5f, 0.0f, 0.5f});

        const auto polygonCountAfter = Api::MeshPolygonHandles(*m_whiteBox).size();

        // same edge handle is returned, no append/extrusion is performed
        EXPECT_THAT(nextEdgeHandle, Eq(Api::EdgeHandle{20}));
        EXPECT_THAT(polygonCountBefore, Eq(polygonCountAfter));
    }

    TEST_F(WhiteBoxTestFixture, PolygonCannotBeAppendedWithNoEdges)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all 'logical'/'visible' edges (those that define the bounds of a polygon)
        for (const auto& edgeHandle :
             {Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{6}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto polygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto polygonCount = Api::MeshPolygonHandles(*m_whiteBox).size();
        const auto borderPolygonVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, polygonHandle);

        // attempt appending a polygon
        const auto nextPolygonHandle =
            Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);

        // mesh is unchanged, polygon count is as before, same polygon handle is returned
        EXPECT_THAT(polygonCount, Eq(1));
        EXPECT_THAT(nextPolygonHandle, Eq(polygonHandle));
        EXPECT_THAT(borderPolygonVertexHandlesCollection, Eq(Api::VertexHandlesCollection{}));
    }

    TEST_F(WhiteBoxTestFixture, MeshReturnsBothBordersOfPolygonWithHole)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAreArray;

        const auto polygonHandles = Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonHandle = Api::ScalePolygonAppendRelative(
            *m_whiteBox, Api::PolygonHandle{Api::FaceHandles{{Api::FaceHandle{4}, Api::FaceHandle{5}}}}, -0.25f);

        // hide all 'logical'/'visible' edges for scale appended face
        for (const auto& edgeHandle : {Api::EdgeHandle{25}, Api::EdgeHandle{27}, Api::EdgeHandle{24}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto expectedLoopFaceHandles =
            Api::FaceHandles{Api::FaceHandle{16}, Api::FaceHandle{17}, Api::FaceHandle{14}, Api::FaceHandle{15},
                             Api::FaceHandle{12}, Api::FaceHandle{13}, Api::FaceHandle{19}, Api::FaceHandle{18}};

        const auto expectedFirstBorderVertexHandles = Api::VertexHandles{
            Api::VertexHandle{11}, Api::VertexHandle{10}, Api::VertexHandle{9}, Api::VertexHandle{8}};

        const auto expectedSecondBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{5}, Api::VertexHandle{4}};

        const auto loopPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{13});
        const auto borderVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, loopPolygonHandle);

        EXPECT_THAT(borderVertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(
            loopPolygonHandle.m_faceHandles,
            UnorderedElementsAreArray(expectedLoopFaceHandles.cbegin(), expectedLoopFaceHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[0],
            UnorderedElementsAreArray(
                expectedFirstBorderVertexHandles.cbegin(), expectedFirstBorderVertexHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[1],
            UnorderedElementsAreArray(
                expectedSecondBorderVertexHandles.cbegin(), expectedSecondBorderVertexHandles.cend()));
    }

    TEST_F(WhiteBoxTestFixture, MeshReturnsMultipleBordersOfHollowCylinderPolygon)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all vertical 'logical'/'visible' edges
        for (const auto& edgeHandle : {Api::EdgeHandle{13}, Api::EdgeHandle{15}, Api::EdgeHandle{12}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        const auto expectedLoopFaceHandles =
            Api::FaceHandles{Api::FaceHandle{9}, Api::FaceHandle{8}, Api::FaceHandle{7},  Api::FaceHandle{6},
                             Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{11}, Api::FaceHandle{10}};

        const auto expectedFirstBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}};
        const auto expectedSecondBorderVertexHandles =
            Api::VertexHandles{Api::VertexHandle{4}, Api::VertexHandle{5}, Api::VertexHandle{6}, Api::VertexHandle{7}};

        const auto loopPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11});
        const auto borderVertexHandlesCollection = Api::PolygonBorderVertexHandles(*m_whiteBox, loopPolygonHandle);

        EXPECT_THAT(borderVertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(
            loopPolygonHandle.m_faceHandles,
            UnorderedElementsAreArray(expectedLoopFaceHandles.cbegin(), expectedLoopFaceHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[0],
            UnorderedElementsAreArray(
                expectedFirstBorderVertexHandles.cbegin(), expectedFirstBorderVertexHandles.cend()));
        EXPECT_THAT(
            borderVertexHandlesCollection[1],
            UnorderedElementsAreArray(
                expectedSecondBorderVertexHandles.cbegin(), expectedSecondBorderVertexHandles.cend()));
    }

    TEST_F(WhiteBoxTestFixture, SingleEdgeCanBeRestored)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{2}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        // split polygon was returned
        EXPECT_THAT(splitPolygons.has_value(), Eq(true));
        // each polygon has a single face
        EXPECT_THAT((*splitPolygons)[0].m_faceHandles.size(), Eq(1));
        EXPECT_THAT((*splitPolygons)[1].m_faceHandles.size(), Eq(1));
        // each polygon has 3 edges
        EXPECT_THAT(
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, (*splitPolygons)[0]),
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{2}));
        EXPECT_THAT(
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, (*splitPolygons)[1]),
            UnorderedElementsAre(Api::EdgeHandle{2}, Api::EdgeHandle{3}, Api::EdgeHandle{4}));
    }

    TEST_F(WhiteBoxTestFixture, MultipleEdgesCanBeRestored)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const auto edgeHandlesToRestore = {
            Api::EdgeHandle{48}, Api::EdgeHandle{47}, Api::EdgeHandle{27}, Api::EdgeHandle{59}, Api::EdgeHandle{41}};

        int restoreCount = 0;
        Api::EdgeHandles restoringEdgeHandles; // inout param
        AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons;
        for (const Api::EdgeHandle& edgeHandleToRestore : edgeHandlesToRestore)
        {
            splitPolygons = Api::RestoreEdge(*m_whiteBox, edgeHandleToRestore, restoringEdgeHandles);
            restoreCount++;

            if (splitPolygons.has_value())
            {
                break;
            }
        }

        EXPECT_THAT((*splitPolygons)[0].m_faceHandles.size(), Eq(8));
        EXPECT_THAT((*splitPolygons)[1].m_faceHandles.size(), Eq(10));
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        EXPECT_THAT(restoreCount, Eq(edgeHandlesToRestore.size()));
    }

    TEST_F(WhiteBoxTestFixture, RestoreExistingUserEdgeHasNoEffect)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> splitPolygons =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{12}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(0));
        // split polygon was not returned
        EXPECT_THAT(splitPolygons.has_value(), Eq(false));
    }

    TEST_F(WhiteBoxTestFixture, RestoreInnerOuterBorderSplitsPolygonLoop)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);

        // hide edges to form a polygon with a hole (inner and outer edge list - two borders)
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{41});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{12});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{20});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{4});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{48});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{85});

        Api::EdgeHandles restoringEdgeHandles;
        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> firstSplitPolygonsAttempt =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{88}, restoringEdgeHandles);

        // one left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.size(), Eq(1));
        // split polygon was not returned
        EXPECT_THAT(firstSplitPolygonsAttempt.has_value(), Eq(false));

        const AZStd::optional<AZStd::array<Api::PolygonHandle, 2>> secondSplitPolygonsAttempt =
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{28}, restoringEdgeHandles);

        // no left over restoring edge handles
        EXPECT_THAT(restoringEdgeHandles.empty(), Eq(true));
        // split polygon was returned
        EXPECT_THAT(secondSplitPolygonsAttempt.has_value(), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, PolygonWithMultipleFacesHalfedgeHandles)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{24});
        const Api::HalfedgeHandles polygonHalfedgeHandles = Api::PolygonHalfedgeHandles(*m_whiteBox, topPolygonHandle);

        // halfedge handles
        EXPECT_THAT(polygonHalfedgeHandles.size(), Eq(54));
    }

    TEST_F(WhiteBoxTestFixture, PolygonWithTwoFacesHalfedgeHandles)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const Api::HalfedgeHandles polygonHalfedgeHandles = Api::PolygonHalfedgeHandles(*m_whiteBox, topPolygonHandle);

        // halfedge handles
        EXPECT_THAT(polygonHalfedgeHandles.size(), Eq(6));
        EXPECT_THAT(
            polygonHalfedgeHandles,
            UnorderedElementsAre(
                Api::HalfedgeHandle{2}, Api::HalfedgeHandle{0}, Api::HalfedgeHandle{8}, Api::HalfedgeHandle{6},
                Api::HalfedgeHandle{5}, Api::HalfedgeHandle{4}));
    }

    TEST_F(WhiteBoxTestFixture, PolygonMultipleFacesBorderVertexPositionsInOrder)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::PolygonHandle topPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{24});
        Api::VertexPositionsCollection polygonBorderVertexPositionsCollection =
            Api::PolygonBorderVertexPositions(*m_whiteBox, topPolygonHandle);

        const AZStd::vector<AZ::Vector3> expectedBorderVertexPositions = {
            AZ::Vector3{0.5f, 0.5f, 0.5f},   AZ::Vector3{-0.5f, 0.5f, 0.5f},  AZ::Vector3{-1.5f, 0.5f, 0.5f},
            AZ::Vector3{-2.5f, 0.5f, 0.5f},  AZ::Vector3{-2.5f, -0.5f, 0.5f}, AZ::Vector3{-2.5f, -1.5f, 0.5f},
            AZ::Vector3{-2.5f, -2.5f, 0.5f}, AZ::Vector3{-1.5f, -2.5f, 0.5f}, AZ::Vector3{-0.5f, -2.5f, 0.5f},
            AZ::Vector3{0.5f, -2.5f, 0.5f},  AZ::Vector3{0.5f, -1.5f, 0.5f},  AZ::Vector3{0.5f, -0.5f, 0.5f}};

        // find the bottom corner to start from (used as the pivot position)
        auto vertexPositionIt = AZStd::find_if(
            polygonBorderVertexPositionsCollection.front().begin(),
            polygonBorderVertexPositionsCollection.front().end(),
            [](const AZ::Vector3& vertexPosition)
            {
                return vertexPosition.IsClose(AZ::Vector3{0.5f, 0.5f, 0.5f});
            });

        // rotate about the pivot to make the ordering of the vertices a little easier to understand
        AZStd::rotate(
            polygonBorderVertexPositionsCollection.front().begin(), vertexPositionIt,
            polygonBorderVertexPositionsCollection.front().end());

        // check the vertex positions are what we expect
        EXPECT_THAT(polygonBorderVertexPositionsCollection.size(), Eq(1));
        EXPECT_THAT(polygonBorderVertexPositionsCollection.front().size(), Eq(12));
        EXPECT_THAT(
            expectedBorderVertexPositions,
            Pointwise(ContainerIsClose(), polygonBorderVertexPositionsCollection.front()));
    }

    TEST_F(WhiteBoxTestFixture, PolygonFacePositionsForMultiFacePolygon)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::Pointwise;

        Create2x2CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor2x2Grid(*m_whiteBox);

        AZStd::vector<AZ::Vector3> polygonTriangles =
            Api::PolygonFacesPositions(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{29}));

        const AZStd::vector<AZ::Vector3> expectedPolygonFacePositions = {
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{0.5, 0.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{0.5, 0.5, 0.5},   AZ::Vector3{-0.5, 0.5, 0.5},
            AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-0.5, -1.5, 0.5},
            AZ::Vector3{0.5, -0.5, 0.5},  AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{0.5, -1.5, 0.5},
            AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-1.5, -0.5, 0.5},
            AZ::Vector3{-0.5, -1.5, 0.5}, AZ::Vector3{-1.5, -0.5, 0.5}, AZ::Vector3{-1.5, -1.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-0.5, 0.5, 0.5},  AZ::Vector3{-1.5, 0.5, 0.5},
            AZ::Vector3{-0.5, -0.5, 0.5}, AZ::Vector3{-1.5, 0.5, 0.5},  AZ::Vector3{-1.5, -0.5, 0.5}};

        EXPECT_THAT(polygonTriangles.size(), Eq(24));
        EXPECT_THAT(expectedPolygonFacePositions, Pointwise(ContainerIsClose(), polygonTriangles));
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandleNext)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // next - CCW order
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{5}), Eq(Api::HalfedgeHandle{6}));
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{34}), Eq(Api::HalfedgeHandle{19}));
        EXPECT_THAT(Api::HalfedgeHandleNext(*m_whiteBox, Api::HalfedgeHandle{30}), Eq(Api::HalfedgeHandle{32}));
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandlePrevious)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // previous - CW order
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{6}), Eq(Api::HalfedgeHandle{5}));
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{19}), Eq(Api::HalfedgeHandle{34}));
        EXPECT_THAT(Api::HalfedgeHandlePrevious(*m_whiteBox, Api::HalfedgeHandle{32}), Eq(Api::HalfedgeHandle{30}));
    }

    TEST_F(WhiteBoxTestFixture, CloneOperationProducesIdenticalResults)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::NotNull;

        Create3x3CubeGrid(*m_whiteBox);

        Api::WhiteBoxMeshPtr whiteBoxClone = Api::CloneMesh(*m_whiteBox);

        // ensure all important data is identical
        EXPECT_THAT(whiteBoxClone, NotNull());
        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(Api::MeshVertexCount(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshVertexHandles(*m_whiteBox), Eq(Api::MeshVertexHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshFaceHandles(*m_whiteBox), Eq(Api::MeshFaceHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshEdgeHandles(*m_whiteBox), Eq(Api::MeshEdgeHandles(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshHalfedgeCount(*m_whiteBox), Eq(Api::MeshHalfedgeCount(*whiteBoxClone)));
        EXPECT_THAT(Api::MeshFaces(*m_whiteBox), Eq(Api::MeshFaces(*whiteBoxClone)));
    }

    TEST_F(WhiteBoxTestFixture, EdgeVertexHandlesTailTipAreExpected)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const AZStd::array<Api::VertexHandle, 2> edge0VertexHandles = {Api::VertexHandle{0}, Api::VertexHandle{1}};

        const AZStd::array<Api::VertexHandle, 2> edge1VertexHandles = {Api::VertexHandle{1}, Api::VertexHandle{2}};

        const AZStd::array<Api::VertexHandle, 2> edge2VertexHandles = {Api::VertexHandle{2}, Api::VertexHandle{0}};

        const AZStd::array<Api::VertexHandle, 2> edge3VertexHandles = {Api::VertexHandle{2}, Api::VertexHandle{3}};

        const AZStd::array<Api::VertexHandle, 2> edge4VertexHandles = {Api::VertexHandle{3}, Api::VertexHandle{0}};

        // note: currently 'first' halfedge handle is always returned internally as
        // it will be the CCW direction of the halfedge
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0}), UnorderedElementsAreArray(edge0VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{1}), UnorderedElementsAreArray(edge1VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{2}), UnorderedElementsAreArray(edge2VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{3}), UnorderedElementsAreArray(edge3VertexHandles));
        EXPECT_THAT(
            Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{4}), UnorderedElementsAreArray(edge4VertexHandles));
    }

    TEST_F(WhiteBoxTestFixture, MultipleLoops)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAreArray;
        using ::testing::Eq;

        const auto polygonHandle = Api::InitializeAsUnitQuad(*m_whiteBox);

        Api::ScalePolygonAppendRelative(*m_whiteBox, polygonHandle, -0.25f);
        // hide edges to create a polygon loop (two vertex lists)
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{13});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{10});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{6});

        // retrieve the halfedge and vertex handles for the polygon loop
        const auto halfedgeHandlesCollection =
            PolygonBorderHalfedgeHandles(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{2}));
        const auto vertexHandlesCollection =
            PolygonBorderVertexHandles(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{2}));

        const AZStd::array<Api::VertexHandle, 4> vertexHandlesFirstBorder = {
            Api::VertexHandle{6}, Api::VertexHandle{5}, Api::VertexHandle{4}, Api::VertexHandle{7}};
        const AZStd::array<Api::VertexHandle, 4> vertexHandlesSecondBorder = {
            Api::VertexHandle{3}, Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};
        const AZStd::array<Api::HalfedgeHandle, 4> halfedgeHandlesFirstBorder = {
            Api::HalfedgeHandle{7}, Api::HalfedgeHandle{3}, Api::HalfedgeHandle{1}, Api::HalfedgeHandle{9}};
        const AZStd::array<Api::HalfedgeHandle, 4> halfedgeHandlesSecondBorder = {
            Api::HalfedgeHandle{24}, Api::HalfedgeHandle{30}, Api::HalfedgeHandle{10}, Api::HalfedgeHandle{18}};

        EXPECT_THAT(vertexHandlesCollection.size(), Eq(2));
        EXPECT_THAT(vertexHandlesCollection[0], ElementsAreArray(vertexHandlesFirstBorder));
        EXPECT_THAT(vertexHandlesCollection[1], ElementsAreArray(vertexHandlesSecondBorder));
        EXPECT_THAT(halfedgeHandlesCollection.size(), Eq(2));
        EXPECT_THAT(halfedgeHandlesCollection[0], ElementsAreArray(halfedgeHandlesFirstBorder));
        EXPECT_THAT(halfedgeHandlesCollection[1], ElementsAreArray(halfedgeHandlesSecondBorder));
    }

    TEST_F(WhiteBoxTestFixture, ExtrusionFromQuadWithBoundaryEdges)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitQuad(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), 1.0f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(8));
        // note: MeshFaceCount should be 12 when 2D extrusion case is correctly handled
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(10));
    }

    TEST_F(WhiteBoxTestFixture, ImpressionOneConnectedEdge)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{9}), 1.0f);
        // use impression to squash one of the cubes down
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(14)); // 2 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(24)); // 4 faces added (2 for side polygon, 2 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(13)); // 3 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionTwoConnectedEdges)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{9}), 1.0f);
        // append another cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        // use impression to squash center cube down
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{16}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(20)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(36)); // 8 faces added (4 for side polygons, 4 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(20)); // 6 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionFourConnectedEdges)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Create3x3CubeGrid(*m_whiteBox);

        // use impression to squash center cube
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{27}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(36)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(68)); // 16 faces added (8 for side polygons, 8 for linking)
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(32)); // 12 polygons added
    }

    TEST_F(WhiteBoxTestFixture, ImpressionInsideLoop)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // scale append polygon in
        Api::ScalePolygonAppendRelative(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), -0.25f);

        // hide connecting edges (make loop)
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{45}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{50}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{54}));

        // use impression to squash center polygon
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(16)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(28)); // 28 faces added
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(11)); // 11 polygons added (should be 7 before?)
    }

    TEST_F(WhiteBoxTestFixture, ImpressionOutsideLoop)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // scale append polygon in
        Api::ScalePolygonAppendRelative(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0}), -0.25f);

        // hide connecting edges (make loop)
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{45}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{50}));
        Api::HideEdge(*m_whiteBox, Api::HalfedgeEdgeHandle(*m_whiteBox, Api::HalfedgeHandle{54}));

        // use impression to squash outer polygon loop
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{19}), -0.5f);

        EXPECT_THAT(Api::MeshVertexCount(*m_whiteBox), Eq(16)); // 4 vertices added
        EXPECT_THAT(Api::MeshFaceCount(*m_whiteBox), Eq(28)); // 28 faces added
        EXPECT_THAT(Api::MeshPolygonHandles(*m_whiteBox).size(), Eq(11)); // 11 polygons added (should be 7 before?)
    }

    TEST_F(WhiteBoxTestFixture, AdvancedPolygonAppendReturnsExpectedRestoredPolygonHandles)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;
        using ::testing::UnorderedElementsAreArray;
        using fh = Api::FaceHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const auto appendedPolygonHandles = Api::TranslatePolygonAppendAdvanced(
            *m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{34}), -1.0f);

        const fh expectedFirstRestoredBefore[] = {fh(54), fh(55), fh(16), fh(17), fh(56), fh(57),
                                                  fh(0),  fh(1),  fh(5),  fh(4),  fh(52), fh(53),
                                                  fh(36), fh(37), fh(27), fh(26), fh(25), fh(24)};
        const fh expectedFirstRestoredAfter[] = {fh(38), fh(39), fh(40), fh(41), fh(42), fh(43),
                                                 fh(44), fh(45), fh(46), fh(47), fh(48), fh(49),
                                                 fh(50), fh(51), fh(52), fh(53), fh(54), fh(55)};

        const fh expectedSecondRestoredBefore[] = {fh(32), fh(33)};
        const fh expectedSecondRestoredAfter[] = {fh(56), fh(57)};

        EXPECT_THAT(
            appendedPolygonHandles.m_appendedPolygonHandle.m_faceHandles,
            UnorderedElementsAre(Api::FaceHandle{62}, Api::FaceHandle{63}));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[0].m_before.m_faceHandles,
            UnorderedElementsAreArray(expectedFirstRestoredBefore));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[0].m_after.m_faceHandles,
            UnorderedElementsAreArray(expectedFirstRestoredAfter));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[1].m_before.m_faceHandles,
            UnorderedElementsAreArray(expectedSecondRestoredBefore));
        EXPECT_THAT(
            appendedPolygonHandles.m_restoredPolygonHandles[1].m_after.m_faceHandles,
            UnorderedElementsAreArray(expectedSecondRestoredAfter));
    }

    TEST_F(WhiteBoxTestFixture, PolygonAppendKeepsGeneratedGeometryPlanar)
    {
        namespace Api = JE::Geometry::Api;
        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);
        const auto appended = Api::TranslatePolygonAppendAdvanced(
            *m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{34}), -1.0f);
        ASSERT_FALSE(appended.m_appendedPolygonHandle.m_faceHandles.empty());
        ASSERT_EQ(appended.m_restoredPolygonHandles.size(), 2);
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            for (const auto corner : Api::FaceHalfedgeHandles(*m_whiteBox, face))
            {
                EXPECT_EQ(Api::HalfedgeUV(*m_whiteBox, corner),
                    JE::Geometry::CreatePlanarUVFromVertex(Api::FaceNormal(*m_whiteBox, face),
                        Api::HalfedgeVertexPositionAtTip(*m_whiteBox, corner)));
            }
        }
    }

    TEST_F(WhiteBoxTestFixture, PolygonAppendPreservesInheritedAndUntouchedManualCorners)
    {
        namespace Api = JE::Geometry::Api;
        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);
        struct SourceFace
        {
            AZ::Uuid m_id;
            AZStd::array<AZ::Vector3, 3> m_positions;
            AZStd::array<AZ::Vector2, 3> m_uvs;
        };
        AZStd::vector<SourceFace> sources;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            SourceFace source;
            source.m_id = AZ::Uuid::CreateRandom();
            Api::SetFaceSurfaceId(*m_whiteBox, face, source.m_id);
            const auto corners = Api::FaceHalfedgeHandles(*m_whiteBox, face);
            ASSERT_EQ(corners.size(), 3);
            for (size_t corner = 0; corner < corners.size(); ++corner)
            {
                source.m_positions[corner] = Api::HalfedgeVertexPositionAtTip(*m_whiteBox, corners[corner]);
                source.m_uvs[corner] = AuthoredUV(source.m_positions[corner], 1000.0f * static_cast<float>(face.Index()));
                Api::SetHalfedgeUV(*m_whiteBox, corners[corner], source.m_uvs[corner]);
            }
            sources.push_back(source);
        }
        const auto polygon = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{34});
        const auto delta = Api::PolygonNormal(*m_whiteBox, polygon) * -1.0f;
        const auto appended = Api::TranslatePolygonAppendAdvanced(*m_whiteBox, polygon, -1.0f);
        ASSERT_FALSE(appended.m_appendedPolygonHandle.m_faceHandles.empty());
        ASSERT_EQ(appended.m_restoredPolygonHandles.size(), 2);
        size_t inheritedFaces = 0;
        size_t newFaces = 0;
        for (const auto face : Api::MeshFaceHandles(*m_whiteBox))
        {
            const auto id = Api::FaceSurfaceId(*m_whiteBox, face);
            const auto source = AZStd::find_if(sources.begin(), sources.end(),
                [&id](const SourceFace& value) { return value.m_id == id; });
            for (const auto corner : Api::FaceHalfedgeHandles(*m_whiteBox, face))
            {
                const auto position = Api::HalfedgeVertexPositionAtTip(*m_whiteBox, corner);
                const auto uv = Api::HalfedgeUV(*m_whiteBox, corner);
                if (source != sources.end())
                {
                    bool matched = false;
                    for (size_t original = 0; original < source->m_uvs.size(); ++original)
                    {
                        matched |= uv == source->m_uvs[original] &&
                            (position == source->m_positions[original] || position == source->m_positions[original] + delta);
                    }
                    EXPECT_TRUE(matched); // Same source corner, including the translated top and restored sides.
                }
                else
                {
                    EXPECT_EQ(uv, JE::Geometry::CreatePlanarUVFromVertex(Api::FaceNormal(*m_whiteBox, face), position));
                }
            }
            source != sources.end() ? ++inheritedFaces : ++newFaces;
        }
        EXPECT_EQ(inheritedFaces, sources.size());
        EXPECT_GT(newFaces, 0);
    }

    TEST_F(WhiteBoxTestFixture, EdgeHandlesConnectedToVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_THAT(
            edgeHandles,
            UnorderedElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{17}, Api::EdgeHandle{2}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeHandlesConnectedToVertexAfterPolygonAppend)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::TranslatePolygonAppend(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{10}), 1.0f);

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_THAT(
            edgeHandles,
            UnorderedElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{25}, Api::EdgeHandle{2},
                Api::EdgeHandle{28}));
    }

    TEST_F(WhiteBoxTestFixture, VertexCanBeHidden)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_TRUE(Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, VertexCanBeRestored)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        // verify precondition
        EXPECT_TRUE(Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));

        Api::RestoreVertex(*m_whiteBox, Api::VertexHandle{0});

        EXPECT_TRUE(!Api::VertexIsHidden(*m_whiteBox, Api::VertexHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeHidden)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        EXPECT_FALSE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));

        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});

        EXPECT_TRUE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));
    }

    TEST_F(WhiteBoxTestFixture, EdgeCanBeRestored)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{0});

        // verify precondition
        EXPECT_TRUE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));

        {
            Api::EdgeHandles restoringEdgeHandles;
            Api::RestoreEdge(*m_whiteBox, Api::EdgeHandle{0}, restoringEdgeHandles);
        }

        EXPECT_FALSE(Api::EdgeIsHidden(*m_whiteBox, Api::EdgeHandle{0}));
    }

    // note: no boundaries implies the mesh is closed (like a cube) as opposed
    // to having unconnected halfedges in the case of a quad
    TEST_F(WhiteBoxTestFixture, HalfedgeHandlesOfEdgeHandleWithoutBoundaries)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{0}, Api::HalfedgeHandle{1}));
        }

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{17});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{35}, Api::HalfedgeHandle{34}));
        }
    }

    TEST_F(WhiteBoxTestFixture, HalfedgeHandlesOfEdgeHandleWithBoundaries)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        {
            // edge will only have a single halfedge (one connected face)
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{1});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{2}));
        }

        {
            const auto halfedgeHandles = Api::EdgeHalfedgeHandles(*m_whiteBox, Api::EdgeHandle{2});
            EXPECT_THAT(halfedgeHandles, UnorderedElementsAre(Api::HalfedgeHandle{5}, Api::HalfedgeHandle{4}));
        }
    }

    TEST_F(WhiteBoxTestFixture, MeshUserEdgeHandlesForDefaultQuad)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        const auto userMeshEdgeHandles = Api::MeshUserEdgeHandles(*m_whiteBox);

        EXPECT_THAT(userMeshEdgeHandles.m_mesh, UnorderedElementsAre(Api::EdgeHandle{2}));
        EXPECT_THAT(
            userMeshEdgeHandles.m_user,
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}));
    }

    // no hidden vertices means only a single edge (the one passed in) will be returned
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithoutHiddenVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(edgeGrouping, ElementsAre(Api::EdgeHandle{0}));
        }

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{15});
            EXPECT_THAT(edgeGrouping, ElementsAre(Api::EdgeHandle{15}));
        }
    }

    // requesting an edge grouping for a 'mesh' edge (not selectable by
    // the user) will return an empty grouping
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfMeshEdgeWithoutHiddenVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;

        Api::InitializeAsUnitCube(*m_whiteBox);

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{2});
            EXPECT_THAT(edgeGrouping.size(), Eq(0));
        }
    }

    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithHiddenVertex)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{0});

        const auto expectedEdgeGrouping = Api::EdgeHandles{Api::EdgeHandle{0}, Api::EdgeHandle{4}, Api::EdgeHandle{12}};

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{0});
            EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
        }

        {
            const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{12});
            EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
        }
    }

    // here verify hidden connected edges will not be added to the grouping
    TEST_F(WhiteBoxTestFixture, EdgeGroupingOfUserEdgeWithHiddenVertexAndConnectedHiddenEdge)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);
        Api::HideVertex(*m_whiteBox, Api::VertexHandle{3});
        Api::HideEdge(*m_whiteBox, Api::EdgeHandle{15});

        const auto expectedEdgeGrouping = Api::EdgeHandles{Api::EdgeHandle{3}, Api::EdgeHandle{4}};

        const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{4});
        EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
    }

    TEST_F(WhiteBoxTestFixture, EdgeGroupingForTopLoopOfCube)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // hide all top vertices
        for (const auto& vertexHandle :
             {Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}, Api::VertexHandle{3}})
        {
            Api::HideVertex(*m_whiteBox, vertexHandle);
        }

        // hide all vertical edges
        for (const auto& edgeHandle :
             {Api::EdgeHandle{15}, Api::EdgeHandle{13}, Api::EdgeHandle{12}, Api::EdgeHandle{10}})
        {
            Api::HideEdge(*m_whiteBox, edgeHandle);
        }

        // edge grouping is the top loop
        const auto expectedEdgeGrouping =
            Api::EdgeHandles{Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{4}};

        const Api::EdgeHandles edgeGrouping = Api::EdgeGrouping(*m_whiteBox, Api::EdgeHandle{3});
        EXPECT_THAT(edgeGrouping, UnorderedElementsAreArray(expectedEdgeGrouping));
    }

    TEST_F(WhiteBoxTestFixture, TriPolygonCreated)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;
        using ::testing::UnorderedElementsAreArray;

        Api::InitializeAsUnitTriangle(*m_whiteBox);

        const auto vertexHandles = Api::MeshVertexHandles(*m_whiteBox);
        const auto vertexPositions = Api::MeshVertexPositions(*m_whiteBox);

        const auto expectedVertexHandles =
            Api::VertexHandles{Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}};

        const auto expectedVertexPositions = AZStd::vector<AZ::Vector3>{
            AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{-0.866f, -0.5f, 0.0f}, AZ::Vector3{0.866f, -0.5f, 0.0f}};

        EXPECT_THAT(vertexHandles, UnorderedElementsAreArray(expectedVertexHandles));
        EXPECT_THAT(vertexPositions, Pointwise(ContainerIsClose(), expectedVertexPositions));
    }

    TEST_F(WhiteBoxTestFixture, SplitUserEdgeCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAre;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // verify preconditions
        const auto edgeFaceHandlesBefore = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVertexHandlesBefore = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto firstConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[0]);
        const auto secondConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[1]);

        // given
        EXPECT_THAT(edgeFaceHandlesBefore, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{0}));
        EXPECT_THAT(
            firstConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            secondConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(edgeVertexHandlesBefore, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}));

        // when
        const Api::VertexHandle splitVertexHandle =
            Api::SplitEdge(*m_whiteBox, Api::EdgeHandle{0}, Api::EdgeMidpoint(*m_whiteBox, Api::EdgeHandle{3}));

        // then
        const auto splitVertexEdgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto faceHandlesForEdge20 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{20});
        const auto faceHandlesForEdge19 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{19});
        const auto faceHandlesForEdge18 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{18});
        const auto faceHandlesForEdge0 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto polygonHandleForFace0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto polygonHandleForFace5 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5});
        const auto bordedEdgeHandlesPolygon0 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace0);
        const auto bordedEdgeHandlesPolygon5 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace5);

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(false));
        EXPECT_THAT(
            splitVertexEdgeHandles,
            UnorderedElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{18}, Api::EdgeHandle{20}, Api::EdgeHandle{19}));
        EXPECT_THAT(faceHandlesForEdge0, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{5}));
        EXPECT_THAT(faceHandlesForEdge18, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(faceHandlesForEdge19, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{0}));
        EXPECT_THAT(faceHandlesForEdge20, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{13}));
        EXPECT_THAT(
            polygonHandleForFace0.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{12}, Api::FaceHandle{1}));
        EXPECT_THAT(
            polygonHandleForFace5.m_faceHandles, // near (side) face
            UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{13}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon0,
            ElementsAre(
                Api::EdgeHandle{4}, Api::EdgeHandle{18}, Api::EdgeHandle{0}, Api::EdgeHandle{1}, Api::EdgeHandle{3}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon5,
            ElementsAre(
                Api::EdgeHandle{12}, Api::EdgeHandle{8}, Api::EdgeHandle{10}, Api::EdgeHandle{0}, Api::EdgeHandle{18}));
    }

    TEST_F(WhiteBoxTestFixture, SplitMeshEdgeCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::ElementsAre;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // verify preconditions
        const auto edgeFaceHandlesBefore = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVertexHandlesBefore = Api::EdgeVertexHandles(*m_whiteBox, Api::EdgeHandle{0});
        const auto firstConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[0]);
        const auto secondConnectedPolygonHandle = Api::FacePolygonHandle(*m_whiteBox, edgeFaceHandlesBefore[1]);

        // given
        EXPECT_THAT(edgeFaceHandlesBefore, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{0}));
        EXPECT_THAT(
            firstConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            secondConnectedPolygonHandle.m_faceHandles, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(edgeVertexHandlesBefore, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}));

        // when
        const Api::VertexHandle splitVertexHandle =
            Api::SplitEdge(*m_whiteBox, Api::EdgeHandle{11}, Api::EdgeMidpoint(*m_whiteBox, Api::EdgeHandle{11}));

        // then
        const auto splitVertexEdgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto faceHandlesForEdge20 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{20});
        const auto faceHandlesForEdge19 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{19});
        const auto faceHandlesForEdge18 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{18});
        const auto faceHandlesForEdge11 = Api::EdgeFaceHandles(*m_whiteBox, Api::EdgeHandle{11});
        const auto polygonHandleForFace5 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5});
        const auto bordedEdgeHandlesPolygon5 =
            Api::PolygonBorderEdgeHandlesFlattened(*m_whiteBox, polygonHandleForFace5);

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(true));
        EXPECT_THAT(
            splitVertexEdgeHandles,
            UnorderedElementsAre(Api::EdgeHandle{20}, Api::EdgeHandle{18}, Api::EdgeHandle{11}, Api::EdgeHandle{19}));
        EXPECT_THAT(faceHandlesForEdge11, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}));
        EXPECT_THAT(faceHandlesForEdge18, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(faceHandlesForEdge19, UnorderedElementsAre(Api::FaceHandle{12}, Api::FaceHandle{4}));
        EXPECT_THAT(faceHandlesForEdge20, UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{13}));
        EXPECT_THAT(
            polygonHandleForFace5.m_faceHandles, // near (side) face
            UnorderedElementsAre(Api::FaceHandle{5}, Api::FaceHandle{4}, Api::FaceHandle{13}, Api::FaceHandle{12}));
        EXPECT_THAT(
            bordedEdgeHandlesPolygon5,
            ElementsAre(Api::EdgeHandle{0}, Api::EdgeHandle{12}, Api::EdgeHandle{8}, Api::EdgeHandle{10}));
    }

    TEST_F(WhiteBoxTestFixture, SplitFaceCausesNewlyFormedFacesToBeAddedToCorrespondingPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Eq;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto polygonHandleForFace0 = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto faceVertexHandles = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(
            polygonHandleForFace0.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}));
        EXPECT_THAT(
            faceVertexHandles, UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{1}, Api::VertexHandle{2}));

        const auto splitVertexHandle =
            Api::SplitFace(*m_whiteBox, Api::FaceHandle{0}, Api::FaceMidpoint(*m_whiteBox, Api::FaceHandle{0}));

        const auto edgeHandles = Api::VertexEdgeHandles(*m_whiteBox, splitVertexHandle);
        const auto polygonHandleForFace0After = Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{0});
        const auto faceVertexHandlesAfter = Api::FaceVertexHandles(*m_whiteBox, Api::FaceHandle{0});

        EXPECT_THAT(splitVertexHandle, Eq(Api::VertexHandle{8}));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, splitVertexHandle), Eq(true));
        EXPECT_THAT(edgeHandles, UnorderedElementsAre(Api::EdgeHandle{18}, Api::EdgeHandle{19}, Api::EdgeHandle{20}));
        EXPECT_THAT(
            polygonHandleForFace0After.m_faceHandles, // top face
            UnorderedElementsAre(Api::FaceHandle{0}, Api::FaceHandle{1}, Api::FaceHandle{12}, Api::FaceHandle{13}));
        EXPECT_THAT(
            faceVertexHandlesAfter,
            UnorderedElementsAre(Api::VertexHandle{0}, Api::VertexHandle{8}, Api::VertexHandle{2}));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgeHandlesReturnedForVertexHandle)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::UnorderedElementsAre;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexUserEdgeHandles = Api::VertexUserEdgeHandles(*m_whiteBox, Api::VertexHandle{2});

        // in the unit cube, vertex handle 2 has 5 connected edge handles but only two of these
        // are user edges (two are internal edges of a cube face)
        EXPECT_THAT(
            vertexUserEdgeHandles, UnorderedElementsAre(Api::EdgeHandle{1}, Api::EdgeHandle{3}, Api::EdgeHandle{13}));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgeAxesReturnedForVertexHandle)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitCube(*m_whiteBox);

        const auto vertexUserEdgeVectors = Api::VertexUserEdgeVectors(*m_whiteBox, Api::VertexHandle{2});

        // The order of these can change between versions of OM3D but the values should be the same (one for each basis)
        // Winding is counter clockwise so it tends to go -X ... -Y ... -Z in latest OpenMesh 11.x
        const auto expectedUserEdgeVectors = AZStd::vector<AZ::Vector3>{
            -AZ::Vector3::CreateAxisX(), -AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisZ()};

        EXPECT_THAT(vertexUserEdgeVectors, Pointwise(ContainerIsClose(), expectedUserEdgeVectors));
    }

    TEST_F(WhiteBoxTestFixture, EdgeAxisReturnedForEdgeHandle)
    {
        namespace Api = JE::Geometry::Api;

        Api::InitializeAsUnitCube(*m_whiteBox);

        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1}), 0.5f);

        const auto edgeVector_4 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{4});
        const auto edgeVector_17 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{17});
        const auto edgeVector_12 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{12});
        const auto edgeVector_0 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{0});
        const auto edgeVector_2 = Api::EdgeVector(*m_whiteBox, Api::EdgeHandle{2});

        EXPECT_THAT(edgeVector_4, IsClose(AZ::Vector3(0.0f, -1.0f, 0.0f)));
        EXPECT_THAT(edgeVector_17, IsClose(AZ::Vector3(0.0f, 1.0f, -1.5f)));
        EXPECT_THAT(edgeVector_12, IsClose(AZ::Vector3(0.0f, 0.0f, -1.5f)));
        EXPECT_THAT(edgeVector_0, IsClose(AZ::Vector3(2.0f, 0.0f, 0.0f)));
        EXPECT_THAT(edgeVector_2, IsClose(AZ::Vector3(-2.0f, -1.0f, 0.0f)));
    }

    TEST_F(WhiteBoxTestFixture, UserEdgesWithZeroLengthNotReturned)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Pointwise;

        Api::InitializeAsUnitCube(*m_whiteBox);

        // squash a cube to be flat (where certain edges will have zero length)
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{1}), 1.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{5}), 2.0f);
        Api::TranslatePolygon(*m_whiteBox, Api::FacePolygonHandle(*m_whiteBox, Api::FaceHandle{11}), -1.0f);

        const auto vertexUserEdgeVectors = Api::VertexUserEdgeVectors(*m_whiteBox, Api::VertexHandle{2});
        const auto vertexUserEdgeAxes = Api::VertexUserEdgeAxes(*m_whiteBox, Api::VertexHandle{2});

        // Again, these could change in order between versions of OpenMesh.  But the values will be present.
        // In the OpenMesh 11.0 it tends to go (-x ... -y ... -z) so in this case its (-y, -z) becuase we have 
        // collapsed along x, which will have a zero length and should no be returned.

        const auto expectedUserEdgeVectors = AZStd::vector<AZ::Vector3>{ -AZ::Vector3::CreateAxisY(3.0f), -AZ::Vector3::CreateAxisZ(2.0f) };
        const auto expectedUserEdgeAxes    = AZStd::vector<AZ::Vector3>{ -AZ::Vector3::CreateAxisY(), -AZ::Vector3::CreateAxisZ() };

        EXPECT_THAT(vertexUserEdgeVectors, Pointwise(ContainerIsClose(), expectedUserEdgeVectors));
        EXPECT_THAT(vertexUserEdgeAxes, Pointwise(ContainerIsClose(), expectedUserEdgeAxes));
    }

    TEST_F(WhiteBoxTestFixture, IsolatedVerticesAreHiddenWhenCreatingNewPolygons)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::VertexHandle internalVertexHandles[] = {vh{0}, vh{11}, vh{20}, vh{16}};

        bool internalVertexHandlesHidden[std::size(internalVertexHandles)];
        bool internalVertexHandlesIsolated[std::size(internalVertexHandles)];

        AZStd::transform(
            AZStd::cbegin(internalVertexHandles), AZStd::cend(internalVertexHandles),
            AZStd::begin(internalVertexHandlesHidden),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsHidden(*whiteBox, vertexHandle);
            });

        AZStd::transform(
            AZStd::cbegin(internalVertexHandles), AZStd::cend(internalVertexHandles),
            AZStd::begin(internalVertexHandlesIsolated),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsIsolated(*whiteBox, vertexHandle);
            });

        EXPECT_THAT(internalVertexHandlesHidden, Each(Eq(true)));
        EXPECT_THAT(internalVertexHandlesIsolated, Each(Eq(true)));
    }

    TEST_F(WhiteBoxTestFixture, HiddenVerticesConnectedToRestoredEdgesAreRestored)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        const Api::VertexHandle reconnectedInternalVertexHandles[] = {vh{11}, vh{20}, vh{16}};

        const auto edgeHandlesToRestore = {
            Api::EdgeHandle{85}, Api::EdgeHandle{45}, Api::EdgeHandle{59}, Api::EdgeHandle{12}};

        bool edgeRestored = false;
        Api::EdgeHandles restoringEdgeHandles; // inout param
        for (const Api::EdgeHandle& edgeHandleToRestore : edgeHandlesToRestore)
        {
            if (Api::RestoreEdge(*m_whiteBox, edgeHandleToRestore, restoringEdgeHandles))
            {
                edgeRestored = true;
                break;
            }
        }

        bool internalVertexHandlesHidden[std::size(reconnectedInternalVertexHandles)];
        bool internalVertexHandlesIsolated[std::size(reconnectedInternalVertexHandles)];

        AZStd::transform(
            AZStd::cbegin(reconnectedInternalVertexHandles), AZStd::cend(reconnectedInternalVertexHandles),
            AZStd::begin(internalVertexHandlesHidden),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsHidden(*whiteBox, vertexHandle);
            });

        AZStd::transform(
            AZStd::cbegin(reconnectedInternalVertexHandles), AZStd::cend(reconnectedInternalVertexHandles),
            AZStd::begin(internalVertexHandlesIsolated),
            [&whiteBox = m_whiteBox](const vh vertexHandle)
            {
                return Api::VertexIsIsolated(*whiteBox, vertexHandle);
            });

        // ensure the edge was correctly restored
        EXPECT_THAT(edgeRestored, Eq(true));

        // vertex handles connected to restored edges will be no longer be hidden or isolated
        EXPECT_THAT(internalVertexHandlesHidden, Each(Eq(false)));
        EXPECT_THAT(internalVertexHandlesIsolated, Each(Eq(false)));

        // unaffected vertex will remain hidden and isolated
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, TryingToRestoreIsolatedHidddenVerticesFails)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);
        HideAllTopUserEdgesFor3x3Grid(*m_whiteBox);

        // precondition check
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));

        const bool vertexRestored = Api::TryRestoreVertex(*m_whiteBox, vh{0});

        // postcondition check - values remain the same
        EXPECT_THAT(vertexRestored, Eq(false));
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(true));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));
    }

    TEST_F(WhiteBoxTestFixture, TryingToRestoreConnectedHidddenVerticesSucceeds)
    {
        namespace Api = JE::Geometry::Api;
        using ::testing::Each;
        using ::testing::Eq;
        using vh = Api::VertexHandle;

        Create3x3CubeGrid(*m_whiteBox);

        // precondition check
        Api::HideVertex(*m_whiteBox, vh{0});

        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(false));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(true));

        const bool vertexRestored = Api::TryRestoreVertex(*m_whiteBox, vh{0});

        // postcondition check - values have changed
        EXPECT_THAT(vertexRestored, Eq(true));
        EXPECT_THAT(Api::VertexIsIsolated(*m_whiteBox, vh{0}), Eq(false));
        EXPECT_THAT(Api::VertexIsHidden(*m_whiteBox, vh{0}), Eq(false));
    }

} // namespace UnitTest

// Required to support running integration tests with Qt
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
