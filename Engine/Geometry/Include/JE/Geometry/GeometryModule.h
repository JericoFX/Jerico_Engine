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

namespace JE::Geometry
{
    class JE_GEOMETRY_API GeometryModule : public AZ::Module
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(GeometryModule, "{7B6D6056-1C3C-4B0B-B7CF-B1D18956A069}", AZ::Module);

        GeometryModule();
        ~GeometryModule();

        // AZ::Module ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
    //! Ownership transfers to the host's existing static-module lifecycle.
    JE_GEOMETRY_API AZ::Module* CreateRuntimeModule();
    //! Tools only: includes runtime descriptors, requiring the authoring
    //! system component which owns the runtime service lifecycle.
    JE_GEOMETRY_AUTHORING_API AZ::Module* CreateAuthoringModule();
} // namespace JE::Geometry
