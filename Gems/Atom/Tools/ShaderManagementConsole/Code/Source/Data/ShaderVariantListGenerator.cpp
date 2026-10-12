/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Data/ShaderVariantListGenerator.h>

namespace JE::ShaderVariants
{
    AZ::Outcome<AZ::RPI::ShaderVariantListSourceData, Failure> Generate(
        const AZStd::string& shaderPath,
        const AZ::RPI::ShaderOptionGroupLayout& layout,
        const AZStd::vector<MaterialVariant>& materials,
        const AZ::RPI::ShaderOptionValuesSourceData& systemOptions,
        const AZStd::function<bool()>& isCancelled)
    {
        using namespace AZ::RPI;
        const auto invalid = [](const char* message) { return AZ::Failure(Failure{FailureReason::InvalidInput, message}); };
        const auto cancelled = [] { return AZ::Failure(Failure{FailureReason::Cancelled, {}}); };
        const auto poll = [&isCancelled] { return isCancelled && isCancelled(); };
        if (poll())
        {
            return cancelled();
        }
        if (shaderPath.empty() || !layout.IsFinalized())
        {
            return invalid("The shader path and finalized option layout are required.");
        }

        const auto& descriptors = layout.GetShaderOptions();
        for (const auto& descriptor : descriptors)
        {
            if (descriptor.GetMinValue().GetIndex() > descriptor.GetMaxValue().GetIndex() ||
                !descriptor.FindValue(descriptor.GetDefaultValue()).IsValid())
            {
                return invalid("A shader option has an invalid range or default value.");
            }
        }
        for (const auto& [name, value] : systemOptions)
        {
            const auto index = layout.FindShaderOptionIndex(name);
            if (!index.IsValid() || (!value.IsEmpty() && !layout.FindValue(index, value).IsValid()))
            {
                return invalid("A system option name or value is not defined by the selected shader.");
            }
        }

        // ShaderVariantAsset::MakeAssetProductSubId reserves bits [0,16] for stable IDs; zero is the root variant.
        constexpr size_t MaxVariants = (1u << 17) - 1;
        ShaderVariantListSourceData result;
        result.m_shaderFilePath = shaderPath;
        AZStd::vector<ShaderOptionGroup> groups;
        AZStd::unordered_set<ShaderVariantId> uniqueGroups;
        const ConstPtr<ShaderOptionGroupLayout> layoutPtr(&layout);

        for (const auto& material : materials)
        {
            if (poll())
            {
                return cancelled();
            }
            const auto* materialLayout = material.m_options.GetShaderOptionLayout();
            if (!materialLayout || !materialLayout->IsFinalized() || materialLayout->GetShaderOptions() != descriptors)
            {
                return invalid("A material shader option group does not match the selected shader layout.");
            }
            for (const auto& name : material.m_materialOwnedOptions)
            {
                if (!layout.FindShaderOptionIndex(name).IsValid())
                {
                    return invalid("Material ownership refers to an unknown shader option.");
                }
            }

            const auto& materialId = material.m_options.GetShaderVariantId();
            if ((materialId.m_mask & ~layout.GetBitMask()).any())
            {
                return invalid("A material option mask includes bits outside the selected shader layout.");
            }

            ShaderOptionGroup group(layoutPtr);
            for (const auto& descriptor : descriptors)
            {
                const auto& name = descriptor.GetName();
                const auto optionMask = materialId.m_mask & descriptor.GetBitMask();
                if (optionMask.any() && optionMask != descriptor.GetBitMask())
                {
                    return invalid("A material option mask contains an incomplete descriptor.");
                }
                auto value = material.m_options.GetValue(name);
                if (!value.IsValid())
                {
                    continue;
                }
                if (value.GetIndex() < descriptor.GetMinValue().GetIndex() ||
                    value.GetIndex() > descriptor.GetMaxValue().GetIndex() || descriptor.GetValueName(value).IsEmpty())
                {
                    return invalid("A material shader option value is outside its descriptor range.");
                }
                if (material.m_materialOwnedOptions.count(name) == 0)
                {
                    continue;
                }
                const auto system = systemOptions.find(name);
                if (system != systemOptions.end())
                {
                    value = descriptor.FindValue(system->second.IsEmpty() ? descriptor.GetDefaultValue() : system->second);
                }
                else
                {
                    result.m_materialOptionsHint.insert(name);
                }
                if (!group.SetValue(name, value))
                {
                    return invalid("A material option could not be projected into the shader layout.");
                }
            }
            if (!group.IsEmpty() && uniqueGroups.insert(group.GetShaderVariantId()).second)
            {
                if (groups.size() == MaxVariants)
                {
                    return invalid("The variant count exceeds the shader asset stable ID range.");
                }
                groups.push_back(AZStd::move(group));
            }
        }

        // Match the document's existing empty-list initialization, without publishing unchecked system options.
        if (groups.empty() && !systemOptions.empty())
        {
            ShaderOptionGroup group(layoutPtr);
            for (const auto& [name, value] : systemOptions)
            {
                const auto& descriptor = layout.GetShaderOption(layout.FindShaderOptionIndex(name));
                if (!group.SetValue(name, value.IsEmpty() ? descriptor.GetDefaultValue() : value))
                {
                    return invalid("A system option could not be set in the selected shader layout.");
                }
            }
            uniqueGroups.insert(group.GetShaderVariantId());
            groups.push_back(AZStd::move(group));
        }

        // Check the full Cartesian size before allocating expansion buffers or assigning stable IDs.
        size_t total = 0;
        for (const auto& group : groups)
        {
            size_t count = 1;
            for (const auto& descriptor : descriptors)
            {
                const auto system = systemOptions.find(descriptor.GetName());
                if (system != systemOptions.end() && system->second.IsEmpty() && group.GetValue(descriptor.GetName()).IsValid())
                {
                    const uint64_t width = uint64_t(descriptor.GetMaxValue().GetIndex()) - descriptor.GetMinValue().GetIndex() + 1;
                    if (width > MaxVariants || count > MaxVariants / width)
                    {
                        return invalid("System option expansion exceeds the shader asset stable ID range.");
                    }
                    count *= static_cast<size_t>(width);
                }
            }
            if (count > MaxVariants - total)
            {
                return invalid("The expanded variant count exceeds the shader asset stable ID range.");
            }
            total += count;
        }
        groups.reserve(total);

        // Descriptor order, then default-first value order, gives reproducible IDs for a fixed material input order.
        for (const auto& descriptor : descriptors)
        {
            const auto system = systemOptions.find(descriptor.GetName());
            if (system == systemOptions.end() || !system->second.IsEmpty())
            {
                continue;
            }
            const size_t originalSize = groups.size();
            for (uint64_t valueIndex = descriptor.GetMinValue().GetIndex(); valueIndex <= descriptor.GetMaxValue().GetIndex(); ++valueIndex)
            {
                if (poll())
                {
                    return cancelled();
                }
                const AZ::Name valueName = descriptor.GetValueName(static_cast<uint32_t>(valueIndex));
                if (valueName.IsEmpty())
                {
                    return invalid("A system option range contains an undefined value.");
                }
                if (valueName == descriptor.GetDefaultValue())
                {
                    continue;
                }
                for (size_t index = 0; index < originalSize; ++index)
                {
                    if (poll())
                    {
                        return cancelled();
                    }
                    if (!groups[index].GetValue(descriptor.GetName()).IsValid())
                    {
                        continue;
                    }
                    ShaderOptionGroup copy(groups[index]);
                    if (!copy.SetValue(descriptor.GetName(), valueName))
                    {
                        return invalid("A system option value could not be expanded.");
                    }
                    if (uniqueGroups.insert(copy.GetShaderVariantId()).second)
                    {
                        groups.push_back(AZStd::move(copy));
                    }
                }
            }
        }

        result.m_shaderVariants.reserve(groups.size());
        for (const auto& group : groups)
        {
            if (poll())
            {
                return cancelled();
            }
            ShaderOptionValuesSourceData values;
            for (const auto& descriptor : descriptors)
            {
                const auto value = group.GetValue(descriptor.GetName());
                if (value.IsValid())
                {
                    values.emplace(descriptor.GetName(), descriptor.GetValueName(value));
                }
            }
            result.m_shaderVariants.emplace_back(static_cast<AZ::u32>(result.m_shaderVariants.size() + 1), AZStd::move(values));
        }
        return AZ::Success(AZStd::move(result));
    }
}
