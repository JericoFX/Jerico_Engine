/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Configuration.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Name/Name.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        using ShaderOptionIndex = AZ::RHI::Handle<uint32_t, class ShaderOptionIndexNamespace>;  //!< ShaderOption index in the group layout
        using ShaderOptionValue = AZ::RHI::Handle<uint32_t, class ShaderOptionValueNamespace>;  //!< Numerical representation for a single value in the ShaderOption
        using ShaderOptionValuePair = AZStd::pair<AZ::Name/*valueName*/, ShaderOptionValue>;  //!< Provides a string representation for a ShaderOptionValue
        using ShaderOptionValues = AZStd::vector<ShaderOptionValuePair>; //!< List of possible values for a shader option

        enum class ShaderOptionType : uint32_t
        {
            Unknown,
            Boolean,
            Enumeration,
            IntegerRange,
        };

        JE_RENDER_INTEGRATION_API const char* ToString(ShaderOptionType shaderOptionType);

    }

namespace AZ
{ // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(JE::Render::RPI::ShaderOptionIndexNamespace, "{CE66656A-CDC3-4B62-9B50-3B9CC014DCE7}", "RPI::ShaderOptionIndexNamespace");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(JE::Render::RPI::ShaderOptionValueNamespace, "{154874D8-D9D0-4D57-A22E-55174FFC003F}", "RPI::ShaderOptionValueNamespace");
} // namespace AZ
