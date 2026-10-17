/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderVariantListSourceData.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/function/function_template.h>

namespace JE::ShaderVariants
{
    // A snapshot of an existing material shader item. Ownership is supplied by MaterialOwnsShaderOption.
    struct MaterialVariant
    {
        JE::Render::RPI::ShaderOptionGroup m_options;
        AZStd::unordered_set<AZ::Name> m_materialOwnedOptions;
    };

    enum class FailureReason
    {
        InvalidInput,
        Cancelled
    };

    struct Failure
    {
        FailureReason m_reason;
        AZStd::string m_message;
    };

    // No document or file is changed. An error/cancellation never returns a partial source data set.
    AZ::Outcome<JE::Render::RPI::ShaderVariantListSourceData, Failure> Generate(
        const AZStd::string& shaderPath,
        const JE::Render::RPI::ShaderOptionGroupLayout& layout,
        const AZStd::vector<MaterialVariant>& materials,
        const JE::Render::RPI::ShaderOptionValuesSourceData& systemOptions,
        const AZStd::function<bool()>& isCancelled = {});
}
