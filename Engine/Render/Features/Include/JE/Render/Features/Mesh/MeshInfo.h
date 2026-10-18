/*/
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/IndexFormat.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/VertexFormat.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Model/UvStreamTangentBitmask.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>

namespace JE::Render::Features
{
    using MeshInfoHandle = AZ::RHI::Handle<int32_t>;
    using MeshInfoHandleList = AZStd::vector<MeshInfoHandle>;

    // Utility to access the Geometry - Data from a mesh without the input-assembly
    struct BufferViewIndexAndOffset
    {
        // Streambuffer-view and format
        AZ::RHI::StreamBufferView m_streamBufferView{};
        AZ::RHI::VertexFormat m_vertexFormat = AZ::RHI::VertexFormat::Unknown;

        // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
        AZ::RHI::Ptr<AZ::RHI::BufferView> m_bufferView = nullptr;
        uint32_t m_byteOffset = 0;
        AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex = {};

        // utility function to create an entry from a generic RHI buffer
        static BufferViewIndexAndOffset Create(
            const AZ::RHI::Buffer* rhiBuffer, const uint32_t byteOffset, AZ::RHI::VertexFormat vertexFormat, bool createStreamBufferView)
        {
            auto result = CreateInternal(rhiBuffer, byteOffset, vertexFormat);
            if (createStreamBufferView)
            {
                auto desc = result.m_bufferView->GetDescriptor();
                uint32_t elementSize = desc.m_elementSize;
                if (desc.m_elementFormat != AZ::RHI::Format::Unknown)
                {
                    elementSize = GetFormatSize(desc.m_elementFormat);
                }
                result.m_streamBufferView = AZ::RHI::StreamBufferView(
                    *result.m_bufferView->GetBuffer(),
                    (desc.m_elementOffset * elementSize) + byteOffset,
                    (desc.m_elementCount * elementSize) - byteOffset,
                    AZ::RHI::GetVertexFormatSize(vertexFormat));
            }
            return result;
        }

        // utility function to create an entry from a Streambuffer
        static BufferViewIndexAndOffset Create(const AZ::RHI::StreamBufferView& streamBufferView, const AZ::RHI::VertexFormat vertexFormat)
        {
            auto rhiBuffer = streamBufferView.GetBuffer();
            auto result = BufferViewIndexAndOffset::CreateInternal(rhiBuffer, streamBufferView.GetByteOffset(), vertexFormat);
            result.m_streamBufferView = streamBufferView;
            return result;
        }

    private:
        static BufferViewIndexAndOffset CreateInternal(
            const AZ::RHI::Buffer* rhiBuffer, const uint32_t byteOffset, AZ::RHI::VertexFormat vertexFormat)
        {
            BufferViewIndexAndOffset result;
            uint32_t byteCount = static_cast<uint32_t>(rhiBuffer->GetDescriptor().m_byteCount);

            AZ::RHI::BufferViewDescriptor desc = AZ::RHI::BufferViewDescriptor::CreateRaw(0, byteCount);
            if (byteCount > 0)
            {
                result.m_bufferView = rhiBuffer->GetBufferView(desc);
                result.m_bindlessReadIndex = result.m_bufferView->GetBindlessReadIndex();
                result.m_byteOffset = byteOffset;
                result.m_vertexFormat = vertexFormat;
            }
            result.m_vertexFormat = vertexFormat;
            return result;
        };
    };

    // Utility to access the Indices for the Geometry - Data from a mesh without the input-assembly
    struct IndexBufferViewIndexAndOffset
    {
        // Indexbuffer-view and format
        AZ::RHI::IndexBufferView m_indexBufferView{};
        AZ::RHI::IndexFormat m_indexFormat = AZ::RHI::IndexFormat::Unknown;

        // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
        AZ::RHI::Ptr<AZ::RHI::BufferView> m_bufferView = nullptr;
        uint32_t m_byteOffset = 0;
        AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex = {};

        // utility function to create an entry from an IndexBufferView
        static IndexBufferViewIndexAndOffset Create(const AZ::RHI::IndexBufferView& indexBufferView)
        {
            uint32_t byteCount = static_cast<uint32_t>(indexBufferView.GetBuffer()->GetDescriptor().m_byteCount);
            // The 'raw' bufferview is for a ByteAddresBuffer, which has to be R32_UINT.
            AZ::RHI::BufferViewDescriptor desc = AZ::RHI::BufferViewDescriptor::CreateRaw(0, byteCount);

            auto* rhiBuffer = indexBufferView.GetBuffer();

            // multi-device buffer bindless read index and offset
            IndexBufferViewIndexAndOffset result{};
            result.m_bufferView = rhiBuffer->GetBufferView(desc);
            result.m_bindlessReadIndex = result.m_bufferView->GetBindlessReadIndex();
            result.m_byteOffset = indexBufferView.GetByteOffset();
            result.m_indexBufferView = indexBufferView;
            result.m_indexFormat = indexBufferView.GetIndexFormat();

            return result;
        }
    };

    // Data for for the MeshInfo - entries of one Mesh
    struct MeshInfoEntry : public AZStd::intrusive_base
    {
        // Info from the mesh about the geometry buffers
        JE::Render::RPI::UvStreamTangentBitmask m_streamTangentBitmask{};
        JE::Render::RPI::ShaderOptionGroup m_optionalInputStreamShaderOptions{};

        // Geometry buffers and index buffer
        AZStd::unordered_map<AZ::RHI::ShaderSemantic, BufferViewIndexAndOffset> m_meshBuffers{};
        IndexBufferViewIndexAndOffset m_indexBuffer{};
        AZ::RHI::Ptr<AZ::RHI::BufferView> m_clusterOffsetBuffer{};

        // additional data per mesh
        int32_t m_materialTypeId = -1;
        int32_t m_materialInstanceId = -1;
        uint32_t m_lightingChannels = 0;
        uint32_t m_objectIdForTransform = 0;
        bool m_isSkinnedMesh = false;
        bool m_hasClusterGeometryBuffers = false;
    };
} // namespace JE::Render::Features
