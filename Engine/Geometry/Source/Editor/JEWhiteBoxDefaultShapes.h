/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <JE/Geometry/EditorWhiteBoxDefaultShapeTypes.h>
#include <JE/Geometry/WhiteBoxToolApi.h>

namespace JE::BSP::Editor
{
    //! Native equivalent of the five original WhiteBox default_shapes.py primitives.
    //! Asset/invalid types leave the destination unchanged and return false.
    JE_GEOMETRY_AUTHORING_API bool InitializeDefaultShape(JE::Geometry::WhiteBoxMesh& mesh, JE::Geometry::DefaultShapeType shape);
}
