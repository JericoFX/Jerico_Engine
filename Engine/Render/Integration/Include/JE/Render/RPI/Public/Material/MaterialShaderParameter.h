/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Material/MaterialShaderParameterLayout.h>
#include <JE/Render/RPI/Public/Material/SharedSamplerState.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Image/Image.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Name/Name.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace JE::Render::RPI
{
    //! This is a layer between Material-Properties and Shader Parameters.
    //! The shader parameter values are written into a Structured Buffer and/or a ShaderResourceGroup
    //! Properties that can't be stored in a Structured Buffer (i.e. Textures and SamplerStates) are converte into indices
    class JE_RENDER_INTEGRATION_API MaterialShaderParameter : public AZStd::intrusive_base
    {
        // utility class to keep some templated functions out of the header
        friend class TypedParameterHelper;

    public:
        MaterialShaderParameter(
            const int materialTypeIndex,
            const int materialInstanceIndex,
            const MaterialShaderParameterLayout* layout,
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg);

        template<typename T>
        bool SetParameter(const AZ::Name& name, T value)
        {
            return SetParameter(name.GetStringView(), value);
        }

        template<typename T>
        bool SetParameter(const AZStd::string_view& name, T value)
        {
            auto index = m_layout->GetParameterIndex(name);
            if (index.IsValid())
            {
                return SetParameter(index, value);
            }
            AZ_Assert(index.IsValid(), "Member index %s not found in MaterialShaderParameter", AZStd::string(name).c_str());
            return false;
        }

        template<typename T>
        bool SetParameter(MaterialShaderParameterNameIndex& nameIndex, T value)
        {
            if (nameIndex.ValidateOrFindIndex(m_layout))
            {
                return SetParameter(nameIndex.GetIndex(), value);
            }
            return false;
        }

        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const int value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const uint32_t value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const float value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const bool value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Vector2& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Vector3& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Vector4& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Color& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Matrix3x3& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::Matrix4x4& value);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, AZ::Data::Instance<Image> image);
        bool SetParameter(const MaterialShaderParameterLayout::Index& index, const AZ::RHI::SamplerState& samplerState);

        AZStd::unordered_map<int, const void*> GetStructuredBufferData() const;

        // These differ only in the return value, so overloading doesn't work.
        template<typename T>
        T GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex = 0) const
        {
            T result{};
            auto desc = m_layout->GetDescriptor(index);
            if (desc)
            {
                auto rawData = GetRawBufferParameterData(desc, deviceIndex);
                if (sizeof(result) != rawData.size_bytes())
                {
                    AZ_Assert(
                        false,
                        "GetShaderParameterData for parameter %s expects wrong type size (expected: %llu, actual: %llu)",
                        desc->m_typeName.c_str(),
                        sizeof(result),
                        rawData.size());
                }
                else
                {
                    result = *reinterpret_cast<const T*>(rawData.data());
                }
            }
            return result;
        }

        template<>
        bool GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            auto result = GetShaderParameterData<uint32_t>(index, deviceIndex);
            AZ_Assert(result == 0 || result == 1, "GetShaderParameterData: GPU Boolean contains illegal value %d", result);
            if (result == 0)
            {
                return false;
            }
            return true;
        }

        template<>
        AZ::Matrix3x3 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            auto result = GetShaderParameterData<AZStd::array<float, 9>>(index, deviceIndex);
            return AZ::Matrix3x3::CreateFromRowMajorFloat9(result.data());
        }

        template<>
        AZ::Matrix4x4 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            auto result = GetShaderParameterData<AZStd::array<float, 16>>(index, deviceIndex);
            return AZ::Matrix4x4::CreateFromRowMajorFloat16(result.data());
        }

        template<typename T, unsigned int N, AZStd::enable_if_t<N <= 4, bool> = true>
        T GetVectorShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex = 0) const
        {
            AZStd::array<float, N> result = GetShaderParameterData<AZStd::array<float, N>>(index, deviceIndex);
            T vectorResult;
            for (int ele = 0; ele < N; ++ele)
            {
                vectorResult.SetElement(ele, result[ele]);
            }
            return vectorResult;
        }

        template<>
        AZ::Vector2 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            return GetVectorShaderParameterData<AZ::Vector2, 2>(index, deviceIndex);
        }

        template<>
        AZ::Vector3 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            return GetVectorShaderParameterData<AZ::Vector3, 3>(index, deviceIndex);
        }

        template<>
        AZ::Vector4 GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            return GetVectorShaderParameterData<AZ::Vector4, 4>(index, deviceIndex);
        }

        template<>
        AZ::Color GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            return GetVectorShaderParameterData<AZ::Color, 4>(index, deviceIndex);
        }

        template<>
        AZ::RHI::SamplerState GetShaderParameterData(const MaterialShaderParameterLayout::Index& index, const uint32_t deviceIndex) const
        {
            auto samplerIndex = GetShaderParameterData<uint32_t>(index, deviceIndex);
            return GetSharedSamplerState(samplerIndex);
        }

        AZStd::span<const uint8_t> GetRawBufferParameterData(
            const MaterialShaderParameterDescriptor* desc, const uint32_t deviceIndex = 0) const;

        uint64_t GetStructuredBufferDataSize() const
        {
            return m_structuredBufferData.at(AZ::RHI::MultiDevice::DefaultDeviceIndex).size();
        }

    private:
        AZ::RHI::SamplerState GetSharedSamplerState(const uint32_t samplerIndex) const;
        bool SetMaterialSrgDeviceReadIndex(
            const MaterialShaderParameterDescriptor* desc, [[maybe_unused]] const int deviceIndex, const int32_t readIndex);

        void SetStructuredBufferData(const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& data);
        void SetStructuredBufferData(
            const MaterialShaderParameterDescriptor* desc, const AZStd::span<const uint8_t>& deviceData, const int deviceIndex);

        const MaterialShaderParameterLayout* m_layout = nullptr;
        AZStd::unordered_map<uint32_t, AZStd::vector<uint8_t>> m_structuredBufferData = {};
        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_shaderResourceGroup = nullptr;
        int m_materialTypeIndex = 0;
        int m_materialInstanceIndex = 0;

        // keep a reference to the used sampler states
        AZStd::unordered_map<MaterialShaderParameterLayout::Index, AZStd::shared_ptr<SharedSamplerState>> m_sharedSamplerStates;

        // keep a reference to the registered non-bindless textures, if AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL is defined
        AZStd::unordered_map<MaterialShaderParameterLayout::Index, int32_t> m_materialTextureIndices;
    };

} // namespace AZ::RPI