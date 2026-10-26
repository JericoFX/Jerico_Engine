/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIBlendShapeData.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIMeshData.h>

#include <SceneProcessing/Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>
#include <InitSceneAPIFixture.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Generation
{
    class HasBlendShapesFixture
        : public JE::Assets::RenderBuilders::SceneProcessing::InitSceneAPIFixture
    {
    public:
        HasBlendShapesFixture()
            // NodeIndex's constructor is private. We can't initialize this value until SetUp when the allocators and
            // the graph are made. So just get some value in place to be filled in later.
            : m_meshIndex(*reinterpret_cast<AZ::SceneAPI::Containers::SceneGraph::NodeIndex*>(&m_meshIndex))
        {
        }

        void SetUp() override
        {
            JE::Assets::RenderBuilders::SceneProcessing::InitSceneAPIFixture::SetUp();
            m_graph = AZStd::make_unique<AZ::SceneAPI::Containers::SceneGraph>();
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex root = GetGraph().GetRoot();
            m_meshIndex = GetGraph().AddChild(root, "testMesh", AZStd::make_shared<AZ::SceneAPI::DataTypes::MockIMeshData>());
        }

        AZ::SceneAPI::Containers::SceneGraph& GetGraph() { return *m_graph; }
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex GetMeshNodeIndex() { return m_meshIndex; }

    private:
        AZStd::unique_ptr<AZ::SceneAPI::Containers::SceneGraph> m_graph;
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex m_meshIndex;
    };

    TEST_F(HasBlendShapesFixture, DoesNotHaveBlendShapes)
    {
        EXPECT_FALSE(JE::Assets::RenderBuilders::SceneProcessing::Generation::MeshOptimizerComponent::HasAnyBlendShapeChild(GetGraph(), GetMeshNodeIndex()));
    }

    TEST_F(HasBlendShapesFixture, HasBlendShapes)
    {
        GetGraph().AddChild(GetMeshNodeIndex(), "blendShape", AZStd::make_shared<AZ::SceneAPI::DataTypes::MockIBlendShapeData>());

        EXPECT_TRUE(JE::Assets::RenderBuilders::SceneProcessing::Generation::MeshOptimizerComponent::HasAnyBlendShapeChild(GetGraph(), GetMeshNodeIndex()));
    }
} // namespace JE::Assets::RenderBuilders::SceneProcessing::Generation
