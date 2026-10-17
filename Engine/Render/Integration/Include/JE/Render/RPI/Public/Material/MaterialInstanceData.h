/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Material/MaterialShaderParameter.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Instance/Instance.h>

namespace JE::Render::RPI
{
    struct MaterialInstanceData
    {
        int32_t m_materialTypeId{ -1 };
        int32_t m_materialInstanceId{ -1 };
        bool m_usesSceneMaterialSrg{ false };
        AZ::Data::Instance<ShaderResourceGroup> m_shaderResourceGroup{ nullptr };
        AZ::Data::Instance<MaterialShaderParameter> m_materialShaderParameter{ nullptr };
    };
} // namespace AZ::RPI
