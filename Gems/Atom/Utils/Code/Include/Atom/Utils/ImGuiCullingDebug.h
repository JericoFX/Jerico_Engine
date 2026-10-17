/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class Scene;
    }

namespace AZ
{

    namespace Render
    {
        void ImGuiDrawCullingDebug(bool& draw, JE::Render::RPI::Scene* scene);
    }
}

#include "ImGuiCullingDebug.inl"
