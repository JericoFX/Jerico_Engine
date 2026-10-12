/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace JE::World::Camera::Debug
{
    void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

    float NormalizeAngle(float angle);
} // namespace JE::World::Camera::Debug
