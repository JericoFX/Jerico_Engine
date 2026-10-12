/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <JE/Geometry/GeometryModule.h>

namespace JE::Geometry
{
    class JE_GEOMETRY_AUTHORING_API GeometryAuthoringModule : public GeometryModule
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(GeometryAuthoringModule, "{DAB2F46E-29A1-4898-9D4B-EB0EA41BDA32}", GeometryModule);

        GeometryAuthoringModule();
        ~GeometryAuthoringModule();

        // GeometryModule ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace JE::Geometry
