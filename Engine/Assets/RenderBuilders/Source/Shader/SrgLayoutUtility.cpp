/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SrgLayoutUtility.h"

#include <Atom/RHI.Edit/Utils.h>

namespace JE::Assets::RenderBuilders
{
    namespace Shader
    {
        namespace SrgLayoutUtility
        {
            AZ::RHI::ShaderInputImageType ToShaderInputImageType(TextureType textureType)
            {
                switch (textureType)
                {
                case TextureType::Texture1D:
                    return AZ::RHI::ShaderInputImageType::Image1D;
                case TextureType::Texture1DArray:
                    return AZ::RHI::ShaderInputImageType::Image1DArray;
                case TextureType::Texture2D:
                    return AZ::RHI::ShaderInputImageType::Image2D;
                case TextureType::Texture2DArray:
                    return AZ::RHI::ShaderInputImageType::Image2DArray;
                case TextureType::Texture2DMS:
                    return AZ::RHI::ShaderInputImageType::Image2DMultisample;
                case TextureType::Texture2DMSArray:
                    return AZ::RHI::ShaderInputImageType::Image2DMultisampleArray;
                case TextureType::Texture3D:
                    return AZ::RHI::ShaderInputImageType::Image3D;
                case TextureType::TextureCube:
                    return AZ::RHI::ShaderInputImageType::ImageCube;
                case TextureType::RwTexture1D:
                    return AZ::RHI::ShaderInputImageType::Image1D;
                case TextureType::RwTexture1DArray:
                    return AZ::RHI::ShaderInputImageType::Image1DArray;
                case TextureType::RwTexture2D:
                    return AZ::RHI::ShaderInputImageType::Image2D;
                case TextureType::RwTexture2DArray:
                    return AZ::RHI::ShaderInputImageType::Image2DArray;
                case TextureType::RwTexture3D:
                    return AZ::RHI::ShaderInputImageType::Image3D;
                case TextureType::RasterizerOrderedTexture1D:
                    return AZ::RHI::ShaderInputImageType::Image1D;
                case TextureType::RasterizerOrderedTexture1DArray:
                    return AZ::RHI::ShaderInputImageType::Image1DArray;
                case TextureType::RasterizerOrderedTexture2D:
                    return AZ::RHI::ShaderInputImageType::Image2D;
                case TextureType::RasterizerOrderedTexture2DArray:
                    return AZ::RHI::ShaderInputImageType::Image2DArray;
                case TextureType::RasterizerOrderedTexture3D:
                    return AZ::RHI::ShaderInputImageType::Image3D;
                case TextureType::SubpassInput:
                    return AZ::RHI::ShaderInputImageType::SubpassInput;
                default:
                    AZ_Assert(false, "Unhandled TextureType");
                    return AZ::RHI::ShaderInputImageType::Unknown;
                }
            }

            AZ::RHI::ShaderInputBufferType ToShaderInputBufferType(BufferType bufferType)
            {
                switch (bufferType)
                {
                case BufferType::Buffer:
                case BufferType::RwBuffer:
                case BufferType::RasterizerOrderedBuffer:
                    return AZ::RHI::ShaderInputBufferType::Typed;
                case BufferType::AppendStructuredBuffer:
                case BufferType::ConsumeStructuredBuffer:
                case BufferType::RasterizerOrderedStructuredBuffer:
                case BufferType::RwStructuredBuffer:
                case BufferType::StructuredBuffer:
                    return AZ::RHI::ShaderInputBufferType::Structured;
                case BufferType::RasterizerOrderedByteAddressBuffer:
                case BufferType::ByteAddressBuffer:
                case BufferType::RwByteAddressBuffer:
                    return AZ::RHI::ShaderInputBufferType::Raw;
                case BufferType::RaytracingAccelerationStructure:
                    return AZ::RHI::ShaderInputBufferType::AccelerationStructure;
                default:
                    AZ_Assert(false, "Unhandled BufferType");
                    return AZ::RHI::ShaderInputBufferType::Unknown;
                }
            }

            bool LoadShaderResourceGroupLayouts(
                [[maybe_unused]] const char* builderName,
                const SrgDataContainer& resourceGroups,
                JE::Render::RPI::ShaderResourceGroupLayoutList& srgLayoutList)
            {
                // Load all SRGs included in source file
                for (const SrgData& srgData : resourceGroups)
                {
                    AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> newSrgLayout = AZ::RHI::ShaderResourceGroupLayout::Create();
                    newSrgLayout->SetName(AZ::Name{srgData.m_name.c_str()});
                    newSrgLayout->SetUniqueId(srgData.m_containingFileName);
                    newSrgLayout->SetBindingSlot(srgData.m_bindingSlot.m_index);

                    // Samplers
                    for (const SamplerSrgData& samplerData : srgData.m_samplers)
                    {
                        if (samplerData.m_isDynamic)
                        {
                            newSrgLayout->AddShaderInput({ samplerData.m_nameId,
                                                           samplerData.m_count,
                                                           samplerData.m_registerId,
                                                           samplerData.m_spaceId });
                        }
                        else
                        {
                            newSrgLayout->AddStaticSampler({ samplerData.m_nameId,
                                                             samplerData.m_descriptor,
                                                             samplerData.m_registerId,
                                                             samplerData.m_spaceId });
                        }
                    }

                    // Images
                    for (const TextureSrgData& textureData : srgData.m_textures)
                    {
                        const AZ::RHI::ShaderInputImageAccess imageAccess =
                            textureData.m_isReadOnlyType ? AZ::RHI::ShaderInputImageAccess::Read : AZ::RHI::ShaderInputImageAccess::ReadWrite;

                        const AZ::RHI::ShaderInputImageType imageType = SrgLayoutUtility::ToShaderInputImageType(textureData.m_type);

                        if (imageType != AZ::RHI::ShaderInputImageType::Unknown)
                        {
                            if (textureData.m_count != aznumeric_cast<uint32_t>(-1))
                            {
                                newSrgLayout->AddShaderInput(AZ::RHI::ShaderInputImageDescriptor{
                                    textureData.m_nameId, imageAccess, imageType, textureData.m_count,
                                    textureData.m_registerId, textureData.m_spaceId });
                            }
                            else
                            {
                                // unbounded array
                                newSrgLayout->AddShaderInput(AZ::RHI::ShaderInputImageUnboundedArrayDescriptor{
                                    textureData.m_nameId, imageAccess, imageType,
                                    textureData.m_registerId, textureData.m_spaceId });
                            }
                        }
                        else
                        {
                            AZ_Error(
                                builderName, false, "Failed to build Shader Resource Group Asset: Image %s has an unknown type.",
                                textureData.m_nameId.GetCStr());
                            return false;
                        }
                    }

                    // Buffers
                    {
                        for (const ConstantBufferData& cbData : srgData.m_constantBuffers)
                        {
                            newSrgLayout->AddShaderInput(AZ::RHI::ShaderInputBufferDescriptor{
                                cbData.m_nameId, AZ::RHI::ShaderInputBufferAccess::Constant, AZ::RHI::ShaderInputBufferType::Constant,
                                cbData.m_count, cbData.m_strideSize, cbData.m_registerId, cbData.m_spaceId });
                        }

                        for (const BufferSrgData& bufferData : srgData.m_buffers)
                        {
                            const AZ::RHI::ShaderInputBufferAccess bufferAccess =
                                bufferData.m_isReadOnlyType ? AZ::RHI::ShaderInputBufferAccess::Read : AZ::RHI::ShaderInputBufferAccess::ReadWrite;

                            const AZ::RHI::ShaderInputBufferType bufferType = SrgLayoutUtility::ToShaderInputBufferType(bufferData.m_type);

                            if (bufferType != AZ::RHI::ShaderInputBufferType::Unknown)
                            {
                                if (bufferData.m_count != aznumeric_cast<uint32_t>(-1))
                                {
                                    newSrgLayout->AddShaderInput(AZ::RHI::ShaderInputBufferDescriptor{
                                        bufferData.m_nameId, bufferAccess, bufferType, bufferData.m_count, bufferData.m_strideSize,
                                        bufferData.m_registerId, bufferData.m_spaceId });
                                }
                                else
                                {
                                    // unbounded array
                                    newSrgLayout->AddShaderInput(AZ::RHI::ShaderInputBufferUnboundedArrayDescriptor{
                                        bufferData.m_nameId, bufferAccess, bufferType, bufferData.m_strideSize,
                                        bufferData.m_registerId, bufferData.m_spaceId });
                                }
                            }
                            else
                            {
                                AZ_Error(
                                    builderName, false,
                                    "Failed to build Shader Resource Group Asset: Buffer %s has un unknown type.",
                                    bufferData.m_nameId.GetCStr());
                                return false;
                            }
                        }
                    }

                    // SRG Constants
                    for (const SrgConstantData& srgConstants : srgData.m_srgConstantData)
                    {
                        newSrgLayout->AddShaderInput({ srgConstants.m_nameId, srgConstants.m_constantByteOffset,
                                                       srgConstants.m_constantByteSize, srgData.m_srgConstantDataRegisterId,
                                                       srgData.m_srgConstantDataSpaceId });
                    }

                    // Shader Variant Key fallback
                    if (srgData.m_fallbackSize > 0)
                    {
                        // Designates this SRG as a ShaderVariantKey fallback
                        newSrgLayout->SetShaderVariantKeyFallback(srgData.m_fallbackName, srgData.m_fallbackSize);
                    }

                    srgLayoutList.push_back(newSrgLayout);
                }

                return true;
            }

        }  // namespace SrgLayoutUtility
    } // namespace Shader
} // JE::Assets::RenderBuilders
