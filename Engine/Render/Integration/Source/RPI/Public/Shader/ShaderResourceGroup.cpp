/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderDataMappings.h>

#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        const AZ::Data::Instance<Image> ShaderResourceGroup::s_nullImage;
        const AZ::Data::Instance<Buffer> ShaderResourceGroup::s_nullBuffer;

        AZ::Data::InstanceId ShaderResourceGroup::MakeSrgPoolInstanceId(
            const AZ::Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            AZ_Assert(!srgName.IsEmpty(), "Invalid ShaderResourceGroup name");

            // Let's find the srg layout with the given name, because it contains the azsl file path of origin
            // which is essential to uniquely identify an SRG and avoid redundant copies in memory.
            auto srgLayout = shaderAsset->FindShaderResourceGroupLayout(srgName, supervariantIndex);
            AZ_Assert(
                srgLayout != nullptr,
                "Failed to find SRG with name %s, using supervariantIndex %u from shaderAsset %s",
                srgName.GetCStr(),
                supervariantIndex.GetIndex(),
                shaderAsset.GetHint().c_str());

            // Create the InstanceId by combining data from the SRG name and layout. This value does not need to be unique between
            // asset IDs because the data can be shared as long as the names and layouts match.
            const AZ::Uuid instanceUuid = AZ::Uuid::CreateName(srgLayout->GetUniqueId()) + AZ::Uuid::CreateName(srgName.GetStringView());

            // Create a union to split the 64 bit layout hash into 32 bit unsigned integers for use as instance ID sub IDs 
            union {
                AZ::HashValue64 hash64;
                struct
                {
                    uint32_t x;
                    uint32_t y;
                };
            } hashUnion;
            hashUnion.hash64 = srgLayout->GetHash();

            // Use the supervariantIndex and layout hash as the subIds for the InstanceId
            return AZ::Data::InstanceId::CreateUuid(instanceUuid, { supervariantIndex.GetIndex(), hashUnion.x, hashUnion.y });
        }

        AZ::Data::Instance<ShaderResourceGroup> ShaderResourceGroup::Create(
            const AZ::Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& srgName)
        {
            // retrieve the supervariantIndex by searching for the default supervariant name, this will
            // allow the shader asset to properly handle the RPI::ShaderSystem supervariant
            SupervariantIndex supervariantIndex = shaderAsset->GetSupervariantIndex(AZ::Name(""));

            SrgInitParams initParams{ supervariantIndex, srgName };
            auto anyInitParams = AZStd::any(initParams);
            return AZ::Data::InstanceDatabase<ShaderResourceGroup>::Instance().Create(shaderAsset, &anyInitParams);
        }

        AZ::Data::Instance<ShaderResourceGroup> ShaderResourceGroup::Create(
            const AZ::Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            SrgInitParams initParams{ supervariantIndex, srgName };
            auto anyInitParams = AZStd::any(initParams);
            return AZ::Data::InstanceDatabase<ShaderResourceGroup>::Instance().Create(shaderAsset, &anyInitParams);
        }

        AZ::Data::Instance<ShaderResourceGroup> ShaderResourceGroup::CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* anySrgInitParams)
        {
            AZ_Assert(anySrgInitParams, "Invalid SrgInitParams");
            auto srgInitParams = AZStd::any_cast<SrgInitParams>(*anySrgInitParams);

            AZ::Data::Instance<ShaderResourceGroup> srg = aznew ShaderResourceGroup();
            const AZ::RHI::ResultCode resultCode = srg->Init(shaderAsset, srgInitParams.m_supervariantIndex, srgInitParams.m_srgName);
            if (resultCode != AZ::RHI::ResultCode::Success)
            {
                return nullptr;
            }

            return srg;
        }

        AZ::RHI::ResultCode ShaderResourceGroup::Init(ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            const auto& lay = shaderAsset.FindShaderResourceGroupLayout(srgName, supervariantIndex);
            m_layout = lay.get();

            if (!m_layout)
            {
                AZ_Assert(false, "ShaderResourceGroup cannot be initialized due to invalid ShaderResourceGroupLayout");
                return AZ::RHI::ResultCode::Fail;
            }

            m_pool = ShaderResourceGroupPool::FindOrCreate(
                AZ::Data::Asset<ShaderAsset>(&shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad), supervariantIndex, srgName);
            AZ_Assert(m_layout->GetHash() == m_pool->GetRHIPool()->GetLayout()->GetHash(), "This can happen if two shaders are including the same partial srg from different .azsl shader files and adding more custom entries to the srg. Recommendation is to just make a bigger SRG that can be shared between the two shaders.");
            
            if (!m_pool)
            {
                return AZ::RHI::ResultCode::Fail;
            }

            m_shaderResourceGroup = m_pool->CreateRHIShaderResourceGroup();
            if (!m_shaderResourceGroup)
            {
                return AZ::RHI::ResultCode::Fail;
            }
            m_shaderResourceGroup->SetName(m_pool->GetRHIPool()->GetName());
            m_data = AZ::RHI::ShaderResourceGroupData(AZ::RHI::MultiDevice::AllDevices, m_layout);
            m_asset = { &shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad };
            m_supervariantIndex = supervariantIndex;

            // The RPI groups match the same dimensions as the RHI group.
            m_imageGroup.resize(m_layout->GetGroupSizeForImages());
            m_bufferGroup.resize(m_layout->GetGroupSizeForBuffers());

            m_isInitialized = true;

            return AZ::RHI::ResultCode::Success;
        }

        void ShaderResourceGroup::Compile()
        {
            m_shaderResourceGroup->Compile(m_data);

            //Mask is passed to RHI in the Compile call so we can reset it here
            m_data.ResetUpdateMask();
        }

        bool ShaderResourceGroup::IsQueuedForCompile() const
        {
            return m_shaderResourceGroup->IsQueuedForCompile();
        }

        bool ShaderResourceGroup::IsInitialized() const
        {
            return m_isInitialized;
        }

        AZ::RHI::ShaderInputBufferIndex ShaderResourceGroup::FindShaderInputBufferIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputBufferIndex(name);
        }

        AZ::RHI::ShaderInputImageIndex ShaderResourceGroup::FindShaderInputImageIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputImageIndex(name);
        }

        AZ::RHI::ShaderInputSamplerIndex ShaderResourceGroup::FindShaderInputSamplerIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputSamplerIndex(name);
        }

        AZ::RHI::ShaderInputConstantIndex ShaderResourceGroup::FindShaderInputConstantIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputConstantIndex(name);
        }

        AZ::RHI::ShaderInputBufferUnboundedArrayIndex ShaderResourceGroup::FindShaderInputBufferUnboundedArrayIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputBufferUnboundedArrayIndex(name);
        }

        AZ::RHI::ShaderInputImageUnboundedArrayIndex  ShaderResourceGroup::FindShaderInputImageUnboundedArrayIndex(const AZ::Name& name) const
        {
            return m_layout->FindShaderInputImageUnboundedArrayIndex(name);
        }

        const AZ::RHI::ShaderResourceGroupLayout* ShaderResourceGroup::GetLayout() const
        {
            return m_layout;
        }

        AZ::RHI::ShaderResourceGroup* ShaderResourceGroup::GetRHIShaderResourceGroup()
        {
            return m_shaderResourceGroup.get();
        }

        bool ShaderResourceGroup::SetShaderVariantKeyFallbackValue(const ShaderVariantKey& shaderKey)
        {
            uint32_t keySize = GetLayout()->GetShaderVariantKeyFallbackSize();
            if (keySize == 0)
            {
                return false;
            }

            auto shaderFallbackIndex = GetLayout()->GetShaderVariantKeyFallbackConstantIndex();
            if (!shaderFallbackIndex.IsValid())
            {
                return false;
            }

            return SetConstantRaw(shaderFallbackIndex, shaderKey.data(), 0, AZStd::min(keySize, (uint32_t) ShaderVariantKeyBitCount) / 8);
        }

        bool ShaderResourceGroup::HasShaderVariantKeyFallbackEntry() const
        {
            return GetLayout()->HasShaderVariantKeyFallbackEntry();
        }

        bool ShaderResourceGroup::SetImage(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::Data::Instance<Image>& image, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImage(inputIndex.GetImageIndex(), image, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImage(AZ::RHI::ShaderInputImageIndex inputIndex, const AZ::Data::Instance<Image>& image, uint32_t arrayIndex)
        {
            const AZ::RHI::ImageView* imageView = image ? image->GetImageView() : nullptr;

            if (m_data.SetImageView(inputIndex, imageView, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Track the RPI image entry at the same slot.
                m_imageGroup[interval.m_min + arrayIndex] = image;

                return true;
            }

            return false;
        }

        bool ShaderResourceGroup::SetImageArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::Data::Instance<Image>> images, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageArray(inputIndex.GetImageIndex(), images, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageArray(AZ::RHI::ShaderInputImageIndex inputIndex, AZStd::span<const AZ::Data::Instance<Image>> images, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(images.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < images.size(); ++i)
                {
                    isValidAll &= SetImage(inputIndex, images[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        const AZ::Data::Instance<Image>& ShaderResourceGroup::GetImage(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return GetImage(inputIndex.GetImageIndex(), arrayIndex);
            }
            return s_nullImage;
        }

        const AZ::Data::Instance<Image>& ShaderResourceGroup::GetImage(AZ::RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return m_imageGroup[interval.m_min + arrayIndex];
            }
            return s_nullImage;
        }

        AZStd::span<const AZ::Data::Instance<Image>> ShaderResourceGroup::GetImageArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return GetImageArray(inputIndex.GetImageIndex());
            }
            return {};
        }

        AZStd::span<const AZ::Data::Instance<Image>> ShaderResourceGroup::GetImageArray(AZ::RHI::ShaderInputImageIndex inputIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, 0))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return AZStd::span<const AZ::Data::Instance<Image>>(&m_imageGroup[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        bool ShaderResourceGroup::SetImageView(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::ImageView* imageView, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageView(inputIndex.GetImageIndex(), imageView, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageView(AZ::RHI::ShaderInputImageIndex inputIndex, const AZ::RHI::ImageView* imageView, uint32_t arrayIndex)
        {
            if (m_data.SetImageView(inputIndex, imageView, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Reset the RPI image entry, since an RHI version now takes precedence.
                m_imageGroup[interval.m_min + arrayIndex] = nullptr;

                return true;
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::ImageView* const> imageViews, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageViewArray(inputIndex.GetImageIndex(), imageViews, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewArray(AZ::RHI::ShaderInputImageIndex inputIndex, AZStd::span<const AZ::RHI::ImageView * const> imageViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(imageViews.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < imageViews.size(); ++i)
                {
                    isValidAll &= SetImageView(inputIndex, imageViews[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewUnboundedArray(AZ::RHI::ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const AZ::RHI::ImageView* const> imageViews)
        {
            return m_data.SetImageViewUnboundedArray(inputIndex, imageViews);
        }

        bool ShaderResourceGroup::SetBufferView(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::BufferView *bufferView, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferView(inputIndex.GetBufferIndex(), bufferView, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferView(AZ::RHI::ShaderInputBufferIndex inputIndex, const AZ::RHI::BufferView *bufferView, uint32_t arrayIndex)
        {
            if (m_data.SetBufferView(inputIndex, bufferView, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Reset the RPI image entry, since an RHI version now takes precedence.
                m_bufferGroup[interval.m_min + arrayIndex] = nullptr;

                return true;
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::BufferView* const> bufferViews, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferViewArray(inputIndex.GetBufferIndex(), bufferViews, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewArray(AZ::RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const AZ::RHI::BufferView * const> bufferViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(bufferViews.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < bufferViews.size(); ++i)
                {
                    isValidAll &= SetBufferView(inputIndex, bufferViews[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewUnboundedArray(AZ::RHI::ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const AZ::RHI::BufferView * const> bufferViews)
        {
            return m_data.SetBufferViewUnboundedArray(inputIndex, bufferViews);
        }

        bool ShaderResourceGroup::SetSampler(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::RHI::SamplerState& sampler, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindSamplerIndex(GetLayout()))
            {
                return SetSampler(inputIndex.GetSamplerIndex(), sampler, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetSampler(AZ::RHI::ShaderInputSamplerIndex inputIndex, const AZ::RHI::SamplerState& sampler, uint32_t arrayIndex)
        {
            return m_data.SetSampler(inputIndex, sampler, arrayIndex);
        }

        bool ShaderResourceGroup::SetSamplerArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::RHI::SamplerState> samplers, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindSamplerIndex(GetLayout()))
            {
                return SetSamplerArray(inputIndex.GetSamplerIndex(), samplers, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetSamplerArray(AZ::RHI::ShaderInputSamplerIndex inputIndex, AZStd::span<const AZ::RHI::SamplerState> samplers, uint32_t arrayIndex)
        {
            return m_data.SetSamplerArray(inputIndex, samplers, arrayIndex);
        }

        bool ShaderResourceGroup::SetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteCount)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantRaw(inputIndex.GetConstantIndex(), bytes, byteCount);
            }
            return false;
        }

        bool ShaderResourceGroup::SetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
        {
            return m_data.SetConstantRaw(inputIndex, bytes, byteCount);
        }

        bool ShaderResourceGroup::SetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantRaw(inputIndex.GetConstantIndex(), bytes, byteOffset, byteCount);
            }
            return false;
        }

        bool ShaderResourceGroup::SetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            return m_data.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        bool ShaderResourceGroup::ApplyDataMappings(const AZ::RHI::ShaderDataMappings& mappings)
        {
            bool success = true;

            success = success && ApplyDataMappingArray(mappings.m_colorMappings);
            success = success && ApplyDataMappingArray(mappings.m_uintMappings);
            success = success && ApplyDataMappingArray(mappings.m_floatMappings);
            success = success && ApplyDataMappingArray(mappings.m_float2Mappings);
            success = success && ApplyDataMappingArray(mappings.m_float3Mappings);
            success = success && ApplyDataMappingArray(mappings.m_float4Mappings);
            success = success && ApplyDataMappingArray(mappings.m_matrix3x3Mappings);
            success = success && ApplyDataMappingArray(mappings.m_matrix4x4Mappings);

            return success;
        }

        const AZ::RHI::ConstPtr<AZ::RHI::ImageView>& ShaderResourceGroup::GetImageView(
            AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindImageIndex(GetLayout());
            return GetImageView(inputIndex.GetImageIndex(), arrayIndex);
        }

        const AZ::RHI::ConstPtr<AZ::RHI::ImageView>& ShaderResourceGroup::GetImageView(AZ::RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetImageView(inputIndex, arrayIndex);
        }

        AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::ImageView>> ShaderResourceGroup::GetImageViewArray(
            AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindImageIndex(GetLayout());
            return GetImageViewArray(inputIndex.GetImageIndex());
        }

        AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::ImageView>> ShaderResourceGroup::GetImageViewArray(AZ::RHI::ShaderInputImageIndex inputIndex) const
        {
            return m_data.GetImageViewArray(inputIndex);
        }

        const AZ::RHI::ConstPtr<AZ::RHI::BufferView>& ShaderResourceGroup::GetBufferView(
            AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindBufferIndex(GetLayout());
            return GetBufferView(inputIndex.GetBufferIndex(), arrayIndex);
        }

        const AZ::RHI::ConstPtr<AZ::RHI::BufferView>& ShaderResourceGroup::GetBufferView(AZ::RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetBufferView(inputIndex, arrayIndex);
        }

        AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::BufferView>> ShaderResourceGroup::GetBufferViewArray(
            AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindBufferIndex(GetLayout());
            return GetBufferViewArray(inputIndex.GetBufferIndex());
        }

        AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::BufferView>> ShaderResourceGroup::GetBufferViewArray(AZ::RHI::ShaderInputBufferIndex inputIndex) const
        {
            return m_data.GetBufferViewArray(inputIndex);
        }

        bool ShaderResourceGroup::SetBuffer(AZ::RHI::ShaderInputNameIndex& inputIndex, const AZ::Data::Instance<Buffer>& buffer, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBuffer(inputIndex.GetBufferIndex(), buffer, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBuffer(AZ::RHI::ShaderInputBufferIndex inputIndex, const AZ::Data::Instance<Buffer>& buffer, uint32_t arrayIndex)
        {
            const auto bufferView =
                buffer ? buffer->GetBufferView() : nullptr;

            if (m_data.SetBufferView(inputIndex, bufferView, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Track the RPI buffer entry at the same slot.
                m_bufferGroup[interval.m_min + arrayIndex] = buffer;

                return true;
            }

            return false;
        }

        bool ShaderResourceGroup::SetBufferArray(AZ::RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const AZ::Data::Instance<Buffer>> buffers, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferArray(inputIndex.GetBufferIndex(), buffers, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferArray(AZ::RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const AZ::Data::Instance<Buffer>> buffers, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(buffers.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < buffers.size(); ++i)
                {
                    isValidAll &= SetBuffer(inputIndex, buffers[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        const AZ::Data::Instance<Buffer>& ShaderResourceGroup::GetBuffer(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return GetBuffer(inputIndex.GetBufferIndex(), arrayIndex);
            }
            return s_nullBuffer;
        }

        const AZ::Data::Instance<Buffer>& ShaderResourceGroup::GetBuffer(AZ::RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, arrayIndex))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return m_bufferGroup[interval.m_min + arrayIndex];
            }
            return s_nullBuffer;
        }

        AZStd::span<const AZ::Data::Instance<Buffer>> ShaderResourceGroup::GetBufferArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return GetBufferArray(inputIndex.GetBufferIndex());
            }
            return {};
        }

        AZStd::span<const AZ::Data::Instance<Buffer>> ShaderResourceGroup::GetBufferArray(AZ::RHI::ShaderInputBufferIndex inputIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, 0))
            {
                const AZ::RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return AZStd::span<const AZ::Data::Instance<Buffer>>(&m_bufferGroup[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        void ShaderResourceGroup::ResetViews()
        {
            m_data.ResetViews();
        }

        const AZ::RHI::SamplerState& ShaderResourceGroup::GetSampler(AZ::RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindSamplerIndex(GetLayout());
            return GetSampler(inputIndex.GetSamplerIndex(), arrayIndex);
        }

        const AZ::RHI::SamplerState& ShaderResourceGroup::GetSampler(AZ::RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetSampler(inputIndex, arrayIndex);
        }

        AZStd::span<const AZ::RHI::SamplerState> ShaderResourceGroup::GetSamplerArray(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindSamplerIndex(GetLayout());
            return GetSamplerArray(inputIndex.GetSamplerIndex());
        }

        AZStd::span<const AZ::RHI::SamplerState> ShaderResourceGroup::GetSamplerArray(AZ::RHI::ShaderInputSamplerIndex inputIndex) const
        {
            return m_data.GetSamplerArray(inputIndex);
        }

        AZStd::span<const uint8_t> ShaderResourceGroup::GetConstantRaw(AZ::RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindConstantIndex(GetLayout());
            return GetConstantRaw(inputIndex.GetConstantIndex());
        }

        AZStd::span<const uint8_t> ShaderResourceGroup::GetConstantRaw(AZ::RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstantRaw(inputIndex);
        }

        bool ShaderResourceGroup::CopyShaderResourceGroupData(const ShaderResourceGroup& other)
        {
            bool isFullCopy = true;
            // Copy Buffer Shader Inputs
            for (const AZ::RHI::ShaderInputBufferDescriptor& desc : m_layout->GetShaderInputListForBuffers())
            {
                AZ::RHI::ShaderInputBufferIndex otherIndex = other.m_layout->FindShaderInputBufferIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputBufferDescriptor& otherDesc = other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_access == otherDesc.m_access && desc.m_count == otherDesc.m_count &&
                        desc.m_strideSize == otherDesc.m_strideSize && desc.m_type == otherDesc.m_type,
                    "ShaderInputBuffer %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZ::RHI::ShaderInputBufferIndex index = m_layout->FindShaderInputBufferIndex(desc.m_name);
                auto bufferViewArray = other.GetBufferViewArray(otherIndex);
                auto bufferArray = other.GetBufferArray(otherIndex);
                AZ_Assert(bufferViewArray.size() == bufferArray.size(), "Different size between buffers and buffer views");
                for (uint32_t i = 0; i < bufferViewArray.size(); ++i)
                {
                    if (bufferArray[i])
                    {
                        SetBuffer(index, bufferArray[i], i);
                    }
                    else
                    {
                        SetBufferView(index, bufferViewArray[i].get(), i);
                    }
                }
            }

            // Copy Image Shader Inputs
            for (const AZ::RHI::ShaderInputImageDescriptor& desc : m_layout->GetShaderInputListForImages())
            {
                AZ::RHI::ShaderInputImageIndex otherIndex = other.m_layout->FindShaderInputImageIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputImageDescriptor& otherDesc = other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_access == otherDesc.m_access && desc.m_count == otherDesc.m_count &&
                        desc.m_type == otherDesc.m_type,
                    "ShaderInputImage %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZ::RHI::ShaderInputImageIndex index = m_layout->FindShaderInputImageIndex(desc.m_name);
                auto imageViewArray = other.GetImageViewArray(otherIndex);
                auto imageArray = other.GetImageArray(otherIndex);
                AZ_Assert(imageViewArray.size() == imageArray.size(), "Different size between image and image views");
                for (uint32_t i = 0; i < imageViewArray.size(); ++i)
                {
                    if (imageArray[i])
                    {
                        SetImage(index, imageArray[i].get(), i);
                    }
                    else
                    {
                        SetImageView(index, imageViewArray[i].get(), i);
                    }
                }
            }

            // Copy Sample Shader Inputs
            for (const AZ::RHI::ShaderInputSamplerDescriptor& desc : m_layout->GetShaderInputListForSamplers())
            {
                AZ::RHI::ShaderInputSamplerIndex otherIndex = other.m_layout->FindShaderInputSamplerIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputSamplerDescriptor& otherDesc = other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_count == otherDesc.m_count,
                    "ShaderInputSampler %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZStd::span<const AZ::RHI::SamplerState> samplerViewArray = other.m_data.GetSamplerArray(otherIndex);
                SetSamplerArray(m_layout->FindShaderInputSamplerIndex(desc.m_name), samplerViewArray);
            }

            // Copy Constants Shader Inputs
            for (const AZ::RHI::ShaderInputConstantDescriptor& desc : m_layout->GetShaderInputListForConstants())
            {
                AZ::RHI::ShaderInputConstantIndex otherIndex = other.m_layout->FindShaderInputConstantIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputConstantDescriptor& otherDesc = other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_constantByteCount == otherDesc.m_constantByteCount,
                    "ShaderInputConstant %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZStd::span<const uint8_t> constantRaw = other.m_data.GetConstantRaw(otherIndex);
                SetConstantRaw(m_layout->FindShaderInputConstantIndex(desc.m_name), constantRaw.data(), static_cast<uint32_t>(constantRaw.size()));
            }

            // Copy Unbound Buffer Array Inputs
            AZStd::vector<const AZ::RHI::BufferView*> bufferViewPtrArray;
            for (const AZ::RHI::ShaderInputBufferUnboundedArrayDescriptor& desc : m_layout->GetShaderInputListForBufferUnboundedArrays())
            {
                AZ::RHI::ShaderInputBufferUnboundedArrayIndex otherIndex =
                    other.m_layout->FindShaderInputBufferUnboundedArrayIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputBufferUnboundedArrayDescriptor& otherDesc =
                    other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_type == otherDesc.m_type && desc.m_access == otherDesc.m_access,
                    "ShaderInputBufferUnboundedArray %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::BufferView>> bufferViewArray = other.m_data.GetBufferViewUnboundedArray(otherIndex);
                bufferViewPtrArray.resize(bufferViewArray.size());
                AZStd::transform(
                    bufferViewArray.begin(),
                    bufferViewArray.end(),
                    bufferViewPtrArray.begin(),
                    [](auto& item)
                    {
                        return item.get();
                    });
                SetBufferViewUnboundedArray(m_layout->FindShaderInputBufferUnboundedArrayIndex(desc.m_name), bufferViewPtrArray);
            }

            // Copy Unbound Image Array Inputs
            AZStd::vector<const AZ::RHI::ImageView*> imageViewPtrArray;
            for (const AZ::RHI::ShaderInputImageUnboundedArrayDescriptor& desc : m_layout->GetShaderInputListForImageUnboundedArrays())
            {
                AZ::RHI::ShaderInputImageUnboundedArrayIndex otherIndex = other.m_layout->FindShaderInputImageUnboundedArrayIndex(desc.m_name);
                if (!otherIndex.IsValid())
                {
                    isFullCopy = false;
                    continue;
                }

                [[maybe_unused]] const AZ::RHI::ShaderInputImageUnboundedArrayDescriptor& otherDesc =
                    other.m_layout->GetShaderInput(otherIndex);
                AZ_Error(
                    "ShaderResourceGroup",
                    desc.m_type == otherDesc.m_type && desc.m_access == otherDesc.m_access,
                    "ShaderInputImageUnboundedArray %s does not match when copying shader resource group data",
                    desc.m_name.GetCStr());

                AZStd::span<const AZ::RHI::ConstPtr<AZ::RHI::ImageView>> imageViewArray = other.m_data.GetImageViewUnboundedArray(otherIndex);
                imageViewPtrArray.resize(imageViewArray.size());
                AZStd::transform(
                    imageViewArray.begin(),
                    imageViewArray.end(),
                    imageViewPtrArray.begin(),
                    [](auto& item)
                    {
                        return item.get();
                    });
                SetImageViewUnboundedArray(m_layout->FindShaderInputImageUnboundedArrayIndex(desc.m_name), imageViewPtrArray);
            }

            // Copy Bindless Inputs
            AZStd::vector<bool> isReadOnlyBuffer;
            AZStd::vector<bool> isReadOnlyImage;
            for (const auto& entry : other.m_data.GetBindlessResourceViews())
            {
                AZ::RHI::ShaderInputBufferIndex otherIndirectResourceBufferIndex = entry.first.first;
                const AZ::RHI::ConstPtr<AZ::RHI::BufferView>& indirectResourceBuffer = other.GetBufferView(otherIndirectResourceBufferIndex);
                AZ::RHI::ShaderInputBufferDescriptor otherIndirectResourceBufferDesc =
                    other.m_layout->GetShaderInput(otherIndirectResourceBufferIndex);

                AZ::RHI::ShaderInputBufferIndex indirectResourceBufferIndex =
                    FindShaderInputBufferIndex(otherIndirectResourceBufferDesc.m_name);

                if (!indirectResourceBufferIndex.IsValid())
                {
                    continue;
                }

                size_t maxSize = entry.second.m_bindlessResources.size();
                bufferViewPtrArray.clear();
                bufferViewPtrArray.reserve(maxSize);
                imageViewPtrArray.clear();
                imageViewPtrArray.reserve(maxSize);
                isReadOnlyBuffer.clear();
                isReadOnlyBuffer.reserve(maxSize);
                isReadOnlyImage.clear();
                isReadOnlyImage.reserve(maxSize);
                for (ConstPtr<AZ::RHI::ResourceView> resourceView : entry.second.m_bindlessResources)
                {
                    AZ::RHI::BindlessResourceType type = entry.second.m_bindlessResourceType;
                    switch (type)
                    {
                    case AZ::RHI::BindlessResourceType::m_ByteAddressBuffer:
                    case AZ::RHI::BindlessResourceType::m_RWByteAddressBuffer:
                        bufferViewPtrArray.push_back(static_cast<const AZ::RHI::BufferView*>(resourceView.get()));
                        isReadOnlyBuffer.push_back(type != AZ::RHI::BindlessResourceType::m_RWByteAddressBuffer);
                        break;
                    case AZ::RHI::BindlessResourceType::m_Texture2D:
                    case AZ::RHI::BindlessResourceType::m_RWTexture2D:
                    case AZ::RHI::BindlessResourceType::m_TextureCube:
                        imageViewPtrArray.push_back(static_cast<const AZ::RHI::ImageView*>(resourceView.get()));
                        isReadOnlyBuffer.push_back(type != AZ::RHI::BindlessResourceType::m_RWTexture2D);
                        break;
                    default:
                        AZ_Assert(false, "Invalid RHI::BindlessResourceType %d", type);
                        continue;
                    }
                }

                if (!bufferViewPtrArray.empty())
                {
                    SetBindlessViews(
                        indirectResourceBufferIndex,
                        indirectResourceBuffer.get(),
                        bufferViewPtrArray,
                        {},
                        isReadOnlyBuffer,
                        entry.first.second);
                }

                if (!imageViewPtrArray.empty())
                {
                    SetBindlessViews(
                        indirectResourceBufferIndex,
                        indirectResourceBuffer.get(),
                        imageViewPtrArray,
                        {},
                        isReadOnlyImage,
                        entry.first.second);
                }
            }

            return isFullCopy;
        }

        const AZ::Data::Asset<ShaderAsset>& ShaderResourceGroup::GetShaderAsset() const
        {
            return m_asset;
        }

        SupervariantIndex ShaderResourceGroup::GetSupervariantIndex() const
        {
            return m_supervariantIndex;
        }

        void ShaderResourceGroup::SetBindlessViews(
            AZ::RHI::ShaderInputBufferIndex indirectResourceBufferIndex,
            const AZ::RHI::BufferView* indirectResourceBuffer,
            AZStd::span<const AZ::RHI::ImageView* const> imageViews,
            AZStd::unordered_map<int, uint32_t*> outIndices,
            AZStd::span<bool> isViewReadOnly,
            uint32_t arrayIndex)
        {
            m_data.SetBindlessViews(
                indirectResourceBufferIndex, indirectResourceBuffer, imageViews, outIndices, isViewReadOnly, arrayIndex);
        }

        void ShaderResourceGroup::SetBindlessViews(
            AZ::RHI::ShaderInputBufferIndex indirectResourceBufferIndex,
            const AZ::RHI::BufferView* indirectResourceBuffer,
            AZStd::span<const AZ::RHI::BufferView* const> bufferViews,
            AZStd::unordered_map<int, uint32_t*> outIndices,
            AZStd::span<bool> isViewReadOnly,
            uint32_t arrayIndex)
        {
            m_data.SetBindlessViews(indirectResourceBufferIndex, indirectResourceBuffer,
                                    bufferViews, outIndices, isViewReadOnly, arrayIndex);
        }

    } // namespace RPI
} // namespace AZ
