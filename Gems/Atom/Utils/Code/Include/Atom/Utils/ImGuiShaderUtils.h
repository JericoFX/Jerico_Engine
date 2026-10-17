/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/MeshDrawPacket.h>

namespace AZ
{
    namespace Render
    {
        namespace ImGuiShaderUtils
        {
            //! Draws an ImGui table that compares the shader options for a shader variant that was requested and the shader variant that was found.
            void DrawShaderVariantTable(const JE::Render::RPI::ShaderOptionGroupLayout* layout, JE::Render::RPI::ShaderVariantId requestedVariantId, JE::Render::RPI::ShaderVariantId selectedVariantId);

            //! Draws a variety of ImGui debug info about one shader variant that is being used by a MeshDrawPacket, including DrawShaderVariantTable above.
            void DrawShaderDetails(const JE::Render::RPI::MeshDrawPacket::ShaderData& shaderData);

            //! Returns the JSON representation of the shader option values for a specific shader variant. This can be copied and pasted into a .shadervariantlist file.
            AZStd::string GetShaderVariantIdJson(const JE::Render::RPI::ShaderOptionGroupLayout* layout, JE::Render::RPI::ShaderVariantId variantId);
        }
    }
}

#include "ImGuiShaderUtils.inl"
