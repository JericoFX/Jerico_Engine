/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include "ViewportCameraSelectorWindow_Internals.h"

class CameraEditorUITests
    : public UnitTest::ToolsApplicationFixture<>
{
};

TEST_F(CameraEditorUITests, TestCameraListModelAddAndRemove)
{
    QScopedPointer<JE::Editor::Camera::Internal::CameraListModel> model(
        new JE::Editor::Camera::Internal::CameraListModel(nullptr));

    int expectedAdds = 0;
    int expectedRemoves = 0;

    QObject::connect(model.get(), &QAbstractItemModel::rowsAboutToBeInserted, [&]() {
        EXPECT_TRUE(expectedAdds > 0);
        expectedAdds -= 1;
    });
    QObject::connect(model.get(), &QAbstractItemModel::rowsAboutToBeRemoved, [&]() {
        EXPECT_TRUE(expectedRemoves > 0);
        expectedRemoves -= 1;
    });

    const AZ::EntityId e1 = UnitTest::CreateDefaultEditorEntity("Camera One");
    const AZ::EntityId e2 = UnitTest::CreateDefaultEditorEntity("Camera Two");
    const AZ::EntityId e3 = UnitTest::CreateDefaultEditorEntity("Camera Not Added");
    ASSERT_TRUE(e1.IsValid());
    ASSERT_TRUE(e2.IsValid());
    ASSERT_TRUE(e3.IsValid());


    expectedAdds = 2;
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotificationBus::Events::OnCameraAdded, e1);
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotificationBus::Events::OnCameraAdded, e2);
    EXPECT_TRUE(expectedAdds == 0);
    // There should be three rows, two for our additions and one default editor camera entry
    EXPECT_TRUE(model->rowCount() == 3);

    expectedRemoves = 2;
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotificationBus::Events::OnCameraRemoved, e1);
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotificationBus::Events::OnCameraRemoved, e2);
    // We never added e3, so the model should just ignore this
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotificationBus::Events::OnCameraRemoved, e3);
    EXPECT_TRUE(expectedRemoves == 0);
    EXPECT_TRUE(model->rowCount() == 1);
}

AZ_TOOLS_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
