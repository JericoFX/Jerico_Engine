/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h>

namespace JE::Editor::Tools
{
    class IdleBehavior final : public ViewportInputBehavior
    {
    public:
        IdleBehavior(ViewportInputBehaviorControllerInterface* controller);
        virtual ~IdleBehavior() = default;
    };
} // namespace JE::Editor::Tools
