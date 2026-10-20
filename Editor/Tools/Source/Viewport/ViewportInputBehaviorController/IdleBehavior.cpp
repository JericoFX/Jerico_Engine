/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/IdleBehavior.h>
#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>

namespace JE::Editor::Tools
{
    IdleBehavior::IdleBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }
} // namespace JE::Editor::Tools
