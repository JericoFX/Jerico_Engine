/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportInputController.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/IdleBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateCameraBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.h>
#include <AzFramework/Viewport/ViewportControllerList.h>

namespace JE::Editor::Tools
{
    EntityPreviewViewportInputController::EntityPreviewViewportInputController(
        const AZ::Crc32& toolId, QWidget* widget, AZStd::shared_ptr<EntityPreviewViewportContent> viewportContent)
        : ViewportInputBehaviorController(
              widget, viewportContent->GetCameraEntityId(), viewportContent->GetObjectEntityId(), viewportContent->GetEnvironmentEntityId())
        , m_toolId(toolId)
        , m_viewportContent(viewportContent)
    {
        AddBehavior(ViewportInputBehaviorController::None, AZStd::make_shared<IdleBehavior>(this));
        AddBehavior(ViewportInputBehaviorController::Lmb, AZStd::make_shared<RotateCameraBehavior>(this));
        AddBehavior(ViewportInputBehaviorController::Mmb, AZStd::make_shared<PanCameraBehavior>(this));
        AddBehavior(ViewportInputBehaviorController::Rmb, AZStd::make_shared<OrbitCameraBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<OrbitCameraBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Mmb,
            AZStd::make_shared<PanCameraBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Rmb,
            AZStd::make_shared<DollyCameraBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Lmb ^ ViewportInputBehaviorController::Rmb,
            AZStd::make_shared<DollyCameraBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Ctrl ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<RotateObjectBehavior>(this));
        AddBehavior(
            ViewportInputBehaviorController::Shift ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<RotateEnvironmentBehavior>(this));
    }
} // namespace JE::Editor::Tools
