/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Material/ShaderCollection.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyCollection.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Tracks the runtime state for a material pipeline within a particular material
        struct MaterialPipelineState
        {
            ShaderCollection m_shaderCollection;
            MaterialPropertyCollection m_materialProperties;
        };

        using MaterialPipelineDataMap = AZStd::unordered_map<AZ::Name, MaterialPipelineState>;

    } // namespace RPI
} // namespace AZ
