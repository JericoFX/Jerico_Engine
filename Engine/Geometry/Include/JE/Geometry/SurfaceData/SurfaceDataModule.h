/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Module/Module.h>

namespace JE::Geometry::SurfaceData
{
    class JE_GEOMETRY_API SurfaceDataModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(SurfaceDataModule, "{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SurfaceDataModule, AZ::SystemAllocator);

        SurfaceDataModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
    //! Ownership transfers to the host's existing module lifecycle.
    JE_GEOMETRY_API AZ::Module* CreateRuntimeModule();
    JE_GEOMETRY_AUTHORING_API AZ::Module* CreateAuthoringModule();
}
