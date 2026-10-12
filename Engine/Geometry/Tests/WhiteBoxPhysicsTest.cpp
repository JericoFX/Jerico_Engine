/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "Components/WhiteBoxColliderComponent.h"
#include "JE/Geometry/EditorWhiteBoxComponentBus.h"
#include "JE/Geometry/WhiteBoxToolApi.h"
#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    class EditorWhiteBoxPhysicsTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        // AZ::Test::GemTestEnvironment overrides ...
        void AddGemsAndComponents() override;
        AZ::ComponentApplication* CreateApplicationInstance() override;
        void PostSystemEntityActivate() override;

    public:
        EditorWhiteBoxPhysicsTestEnvironment() = default;
        ~EditorWhiteBoxPhysicsTestEnvironment() override = default;
    };

    void EditorWhiteBoxPhysicsTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({"PhysX5.Editor.Gem"});
        AddComponentDescriptors(
            {JE::Geometry::EditorWhiteBoxComponent::CreateDescriptor(),
             JE::Geometry::WhiteBoxComponent::CreateDescriptor(),
             JE::Geometry::WhiteBoxColliderComponent::CreateDescriptor(),
             JE::Geometry::EditorWhiteBoxColliderComponent::CreateDescriptor()});
    }

    AZ::ComponentApplication* EditorWhiteBoxPhysicsTestEnvironment::CreateApplicationInstance()
    {
        // Using ToolsTestApplication to have AzFramework and AzToolsFramework components.
        return aznew UnitTest::ToolsTestApplication("EditorWhiteBoxPhysics");
    }

    void EditorWhiteBoxPhysicsTestEnvironment::PostSystemEntityActivate()
    {
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    class WhiteBoxPhysicsFixture : public ::testing::Test
    {
    };

    TEST_F(WhiteBoxPhysicsFixture, EditorWhiteBoxColliderComponentCanBeAddedToAnEmptyWhiteBoxComponent)
    {
        // given
        // create an entity with a transform and editor white box component
        AZ::Entity entity;
        entity.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        auto editorWhiteBoxComponent = entity.CreateComponent<JE::Geometry::EditorWhiteBoxComponent>();

        entity.Init();
        entity.Activate();

        JE::Geometry::WhiteBoxMesh* whiteBox = nullptr;
        JE::Geometry::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(entity.GetId(), editorWhiteBoxComponent->GetId()),
            &JE::Geometry::EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // clear all data from the white box mesh
        JE::Geometry::Api::Clear(*whiteBox);

        // error messages present in EditorWhiteBoxComponent prior to fix for empty WhiteBoxMesh
        UnitTest::ErrorHandler physxCookFailed("Failed to cook triangle mesh. Please check the data is correct");
        UnitTest::ErrorHandler colliderCookFailed("Failed to cook mesh data");
        UnitTest::ErrorHandler invalidShape("Trying to add an invalid shape");
        UnitTest::ErrorHandler invalidConfiguration("Unable to create a shape from configuration");
        UnitTest::ErrorHandler physxError("TriangleMesh::loadFromDesc: desc.isValid() failed!");

        // when
        // add an editor white box collider component
        entity.Deactivate();
        entity.CreateComponent<JE::Geometry::EditorWhiteBoxColliderComponent>();
        entity.Activate();

        // then
        // ensure none of the previous error messages are reported
        EXPECT_EQ(physxCookFailed.GetWarningCount(), 0);
        EXPECT_EQ(colliderCookFailed.GetWarningCount(), 0);
        EXPECT_EQ(invalidShape.GetErrorCount(), 0);
        EXPECT_EQ(invalidConfiguration.GetErrorCount(), 0);
        EXPECT_EQ(physxError.GetErrorCount(), 0);
    }

} // namespace UnitTest

// required to support running integration tests with Qt and PhysX
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({new UnitTest::EditorWhiteBoxPhysicsTestEnvironment()});
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
