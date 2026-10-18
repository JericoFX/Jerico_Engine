/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshVertexStreamProperties.h>

#include <JE/Render/RPI/Reflect/ResourcePoolAssetCreator.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/Math/PackedVector3.h>

namespace JE::Render
{
    namespace Features
    {
        SkinnedMeshVertexStreamProperties::SkinnedMeshVertexStreamProperties()
        {
            // Attributes of the input buffers used for skinning
            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Position)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshInputPositions"},
                AZ::Name{"m_sourcePositions"},
                AZ::RHI::ShaderSemantic{AZ::Name{"POSITION"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Position
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Normal)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshInputNormals"},
                AZ::Name{"m_sourceNormals"},
                AZ::RHI::ShaderSemantic{AZ::Name{"NORMAL"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Normal
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Tangent)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                AZ::Name{"SkinnedMeshInputTangents"},
                AZ::Name{"m_sourceTangents"},
                AZ::RHI::ShaderSemantic{AZ::Name{"TANGENT"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Tangent
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BiTangent)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshInputBiTangents"},
                AZ::Name{"m_sourceBiTangents"},
                AZ::RHI::ShaderSemantic{AZ::Name{"BITANGENT"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::BiTangent
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendIndices)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32_UINT,
                sizeof(AZ::Vector4),
                AZ::Name{"SkinnedMeshInputBlendIndices"},
                AZ::Name{"m_sourceBlendIndices"},
                AZ::RHI::ShaderSemantic{AZ::Name{"SKIN_JOINTINDICES"}},
                true, // isOptional
                SkinnedMeshInputVertexStreams::BlendIndices
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendWeights)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32_FLOAT,
                sizeof(AZ::Vector4),
                AZ::Name{"SkinnedMeshInputBlendWeights"},
                AZ::Name{"m_sourceBlendWeights"},
                AZ::RHI::ShaderSemantic{AZ::Name{"SKIN_WEIGHTS"}},
                true, // isOptional
                SkinnedMeshInputVertexStreams::BlendWeights
            };

            // Attributes of the vertex buffers that are not used or modified during skinning, but are shared between all target models that share the same source
            m_staticStreamInfo[static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::UV_0)] = SkinnedMeshVertexStreamInfo{
                AZ::RHI::Format::R32G32_FLOAT,
                sizeof(float[2]),
                AZ::Name{"SkinnedMeshStaticUVs"},
                AZ::Name{"unused"},
                AZ::RHI::ShaderSemantic{AZ::Name{"UV"}}
            };

            // Attributes of the vertex streams of the target model that is written to during skinning
            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Position)] = SkinnedMeshOutputVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshOutputPositions"},
                AZ::Name{"m_targetPositions"},
                AZ::RHI::ShaderSemantic{AZ::Name{"POSITION"}},
                SkinnedMeshInputVertexStreams::Position
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Normal)] = SkinnedMeshOutputVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshOutputNormals"},
                AZ::Name{"m_targetNormals"},
                AZ::RHI::ShaderSemantic{AZ::Name{"NORMAL"}},
                SkinnedMeshInputVertexStreams::Normal
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Tangent)] = SkinnedMeshOutputVertexStreamInfo{
                AZ::RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                AZ::Name{"SkinnedMeshOutputTangents"},
                AZ::Name{"m_targetTangents"},
                AZ::RHI::ShaderSemantic{AZ::Name{"TANGENT"}},
                SkinnedMeshInputVertexStreams::Tangent
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::BiTangent)] = SkinnedMeshOutputVertexStreamInfo{
                AZ::RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                AZ::Name{"SkinnedMeshOutputBiTangents"},
                AZ::Name{"m_targetBiTangents"},
                AZ::RHI::ShaderSemantic{AZ::Name{"BITANGENT"}},
                SkinnedMeshInputVertexStreams::BiTangent
            };

            {
                auto bufferPoolDesc = AZStd::make_unique<AZ::RHI::BufferPoolDescriptor>();
                // Output buffers are both written to during skinning and used as input assembly buffers
                bufferPoolDesc->m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderReadWrite |
                    AZ::RHI::BufferBindFlags::CopyRead | AZ::RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc->m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;

                JE::Render::RPI::ResourcePoolAssetCreator creator;
                creator.Begin(AZ::Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SkinnedMeshOutputStreamPool");
                creator.End(m_outputStreamResourcePool);
            }

            // Set the required and optional input streams for the skinned mesh compute shader
            // in a ShaderInputContract for retrieving the streams from the model
            for (const SkinnedMeshVertexStreamInfo& inputStreamInfo : m_inputStreamInfo)
            {
                JE::Render::RPI::ShaderInputContract::StreamChannelInfo channelInfo;
                channelInfo.m_semantic = inputStreamInfo.m_semantic;
                channelInfo.m_isOptional = inputStreamInfo.m_isOptional;
                channelInfo.m_componentCount = AZ::RHI::GetFormatComponentCount(inputStreamInfo.m_elementFormat);

                m_computeShaderInputContract.m_streamChannels.push_back(channelInfo);
            }
        };

        const SkinnedMeshVertexStreamInfo* SkinnedMeshVertexStreamProperties::GetInputStreamInfo(const AZ::RHI::ShaderSemantic& shaderSemantic) const
        {
            auto FindVertexStreamInfo = [&shaderSemantic](const SkinnedMeshVertexStreamInfo& vertexStreamInfo)
            {
                return shaderSemantic == vertexStreamInfo.m_semantic;
            };

            if (auto foundIt = AZStd::find_if(m_inputStreamInfo.begin(), m_inputStreamInfo.end(), FindVertexStreamInfo);
                foundIt != m_inputStreamInfo.end())
            {
                return &*foundIt;
            }
            else
            {
                return nullptr;
            }
        }

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const
        {
            return m_inputStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const
        {
            return m_staticStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshOutputVertexStreamInfo* SkinnedMeshVertexStreamProperties::GetOutputStreamInfo(const AZ::RHI::ShaderSemantic& shaderSemantic) const
        {
            auto FindVertexStreamInfo = [&shaderSemantic](const SkinnedMeshOutputVertexStreamInfo& vertexStreamInfo)
            {
                return shaderSemantic == vertexStreamInfo.m_semantic;
            };

            if (auto foundIt = AZStd::find_if(m_outputStreamInfo.begin(), m_outputStreamInfo.end(), FindVertexStreamInfo);
                foundIt != m_outputStreamInfo.end())
            {
                return &*foundIt;
            }
            else
            {
                return nullptr;
            }
        }

        const SkinnedMeshOutputVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const
        {
            return m_outputStreamInfo[static_cast<uint8_t>(stream)];
        }

        AZ::Data::Asset<JE::Render::RPI::ResourcePoolAsset> SkinnedMeshVertexStreamProperties::GetOutputStreamResourcePool() const
        {
            return m_outputStreamResourcePool;
        }

        uint32_t SkinnedMeshVertexStreamProperties::GetMaxSupportedVertexCount() const
        {
            return aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max()) * aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        }

        const JE::Render::RPI::ShaderInputContract& SkinnedMeshVertexStreamProperties::GetComputeShaderInputContract() const
        {
            return m_computeShaderInputContract;
        }

    } // namespace Features
} // namespace JE::Render
