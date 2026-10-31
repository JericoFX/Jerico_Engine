/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Buffer/BufferAsset.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/Json/ByteStreamSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <meshoptimizer.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(JE::Render::RPI::CommonBufferPoolType, "{E3FD19DF-4395-46FD-8092-D27BC73A3688}", "RPI::CommonBufferPoolType");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(JE::Render::RPI::BufferAsset::CompressionFormat, "{D6624140-8972-4356-9A88-56BA8DD24D0A}", "RPI::BufferAsset::CompressionFormat");

    }

namespace JE::Render::RPI
{
        AZ_CLASS_ALLOCATOR_IMPL(BufferAsset, BufferAssetAllocator)

        void BufferAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BufferAsset>()
                    ->Version(4)
                    ->Field("Name", &BufferAsset::m_name)
                    ->Field("Buffer", &BufferAsset::m_buffer)
                    ->Field("BufferDescriptor", &BufferAsset::m_bufferDescriptor)
                    ->Field("BufferViewDescriptor", &BufferAsset::m_bufferViewDescriptor)
                    ->Field("BufferPoolAsset", &BufferAsset::m_poolAsset)
                    ->Field("CommonBufferPoolType", &BufferAsset::m_poolType)
                    ->Field("CompressionFormat", &BufferAsset::m_compressionFormat)
                    ;

                // register enum strings
                serializeContext->Enum<CommonBufferPoolType>()
                    ->Value("Constant", CommonBufferPoolType::Constant)
                    ->Value("StaticInputAssembly", CommonBufferPoolType::StaticInputAssembly)
                    ->Value("DynamicInputAssembly", CommonBufferPoolType::DynamicInputAssembly)
                    ->Value("ReadBack", CommonBufferPoolType::ReadBack)
                    ->Value("ReadWrite", CommonBufferPoolType::ReadWrite)
                    ->Value("ReadOnly", CommonBufferPoolType::ReadOnly)
                    ->Value("Indirect", CommonBufferPoolType::Indirect)
                    ->Value("Invalid", CommonBufferPoolType::Invalid)
                    ;
                serializeContext->Enum<BufferAsset::CompressionFormat>()
                    ->Value("Uncompressed", BufferAsset::CompressionFormat::Uncompressed)
                    ->Value("Index", BufferAsset::CompressionFormat::Index)
                    ->Value("Vertex", BufferAsset::CompressionFormat::Vertex)
                    ;
            }
            if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<AZ::JsonByteStreamSerializer<Allocator>>()->HandlesType<AZStd::vector<uint8_t, Allocator>>();
            }
        }

        AZStd::span<const uint8_t> BufferAsset::GetBuffer() const
        {
            return AZStd::span<const uint8_t>(m_buffer);
        }

        AZStd::optional<AZStd::span<const uint8_t>> BufferAsset::GetDecodedCpuBuffer(
            BufferData& scratch, size_t maxDecodedBytes) const
        {
            const uint64_t decodedBytes = m_bufferDescriptor.m_byteCount;
            if (decodedBytes > maxDecodedBytes || decodedBytes > AZStd::numeric_limits<size_t>::max())
            {
                return AZStd::nullopt;
            }
            if (m_compressionFormat == CompressionFormat::Uncompressed)
            {
                return m_buffer.size() <= maxDecodedBytes ? AZStd::optional<AZStd::span<const uint8_t>>(GetBuffer()) : AZStd::nullopt;
            }
            const size_t elementSize = m_bufferViewDescriptor.m_elementSize;
            if (decodedBytes == 0 || elementSize == 0 || decodedBytes % elementSize != 0 || m_buffer.empty() ||
                decodedBytes > scratch.max_size())
            {
                return AZStd::nullopt;
            }
            const size_t elementCount = static_cast<size_t>(decodedBytes) / elementSize;
            if (m_compressionFormat == CompressionFormat::Index)
            {
                if ((elementSize != 2 && elementSize != 4) || elementCount % 3 != 0) { return AZStd::nullopt; }
            }
            else if (m_compressionFormat == CompressionFormat::Vertex)
            {
                if (elementSize > 256 || elementSize % 4 != 0) { return AZStd::nullopt; }
            }
            else
            {
                return AZStd::nullopt;
            }
            // The encoded byte length does not identify its representation: it can be smaller, equal or larger.
            scratch.resize_no_construct(static_cast<size_t>(decodedBytes));
            const int result = m_compressionFormat == CompressionFormat::Index ?
                meshopt_decodeIndexBuffer(scratch.data(), elementCount, elementSize, m_buffer.data(), m_buffer.size()) :
                meshopt_decodeVertexBuffer(scratch.data(), elementCount, elementSize, m_buffer.data(), m_buffer.size());
            if (result != 0)
            {
                scratch.clear();
                return AZStd::nullopt;
            }
            return AZStd::span<const uint8_t>(scratch);
        }

        const AZ::RHI::BufferDescriptor& BufferAsset::GetBufferDescriptor() const
        {
            return m_bufferDescriptor;
        }

        const AZ::RHI::BufferViewDescriptor& BufferAsset::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }

        void BufferAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        const AZ::Data::Asset<ResourcePoolAsset>& BufferAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        CommonBufferPoolType BufferAsset::GetCommonPoolType() const
        {
            return m_poolType;
        }

        const AZStd::string& BufferAsset::GetName() const
        {
            return m_name;
        }

        const BufferAsset::CompressionFormat BufferAsset::GetCompressionFormat() const
        {
            return m_compressionFormat;
        }

        AZ::Data::AssetHandler::LoadResult BufferAssetHandler::LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (AssetHandler<BufferAsset>::LoadAssetData(asset, stream, assetLoadFilterCB) ==
                AZ::Data::AssetHandler::LoadResult::LoadComplete)
            {
                BufferAsset* bufferAsset = asset.GetAs<BufferAsset>();
                BufferAsset::BufferData scratch;
                const auto decoded = bufferAsset->GetDecodedCpuBuffer(scratch, scratch.max_size());
                if (!decoded)
                {
                    AZ_Error("BufferAssetHandler", false, "Failure decompressing buffer payload");
                    return AZ::Data::AssetHandler::LoadResult::Error;
                }
                if (bufferAsset->m_compressionFormat != BufferAsset::CompressionFormat::Uncompressed)
                {
                    bufferAsset->m_buffer = AZStd::move(scratch);
                    // Keep resident bytes and their serialized representation coherent for all save/clone paths.
                    bufferAsset->m_compressionFormat = BufferAsset::CompressionFormat::Uncompressed;
                }
                return AZ::Data::AssetHandler::LoadResult::LoadComplete;
            }
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
    }

namespace AZ
{ //namespace RPI
} // namespace AZ
