/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/function/function_template.h>

namespace JE::ShaderVariants
{
    // A snapshot of an existing material shader item. Ownership is supplied by MaterialOwnsShaderOption.
    struct MaterialVariant
    {
        AZ::RPI::ShaderOptionGroup m_options;
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
    AZ::Outcome<AZ::RPI::ShaderVariantListSourceData, Failure> Generate(
        const AZStd::string& shaderPath,
        const AZ::RPI::ShaderOptionGroupLayout& layout,
        const AZStd::vector<MaterialVariant>& materials,
        const AZ::RPI::ShaderOptionValuesSourceData& systemOptions,
        const AZStd::function<bool()>& isCancelled = {});
}
