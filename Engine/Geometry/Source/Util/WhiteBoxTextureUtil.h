/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

namespace JE::Geometry
{
    //! Returns the index of the largest element in the vector
    JE_GEOMETRY_AUTHORING_API int FindLargestElement(const AZ::Vector3& vector);

    //! Creates UV coordinates from vertex positioning on closest basis vector p
    JE_GEOMETRY_AUTHORING_API AZ::Vector2 CreatePlanarUVFromVertex(
        const AZ::Vector3& normal, const AZ::Vector3& position, const AZ::Vector2& offset = AZ::Vector2(0.5f, 0.5f),
        const AZ::Vector2& scale = AZ::Vector2(-1.0f, -1.0f));

} // namespace JE::Geometry
