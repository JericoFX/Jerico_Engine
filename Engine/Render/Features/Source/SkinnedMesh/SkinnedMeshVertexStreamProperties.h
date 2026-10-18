/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/Name/Name.h>

namespace JE::Render
{

    namespace Features
    {
        class SkinnedMeshVertexStreamProperties
            : public SkinnedMeshVertexStreamPropertyInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::SkinnedMeshVertexStreamProperties, "AZ::Render::SkinnedMeshVertexStreamProperties", "{8912239E-8412-4B9E-BDE6-AE6BA67A207C}", JE::Render::Features::SkinnedMeshVertexStreamPropertyInterface);

            SkinnedMeshVertexStreamProperties();

            const SkinnedMeshVertexStreamInfo* GetInputStreamInfo(const AZ::RHI::ShaderSemantic& shaderSemantic ) const override;
            const SkinnedMeshVertexStreamInfo& GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const override;
            const SkinnedMeshVertexStreamInfo& GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const override;
            const SkinnedMeshOutputVertexStreamInfo* GetOutputStreamInfo(const AZ::RHI::ShaderSemantic& shaderSemantic) const override;
            const SkinnedMeshOutputVertexStreamInfo& GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const override;

            AZ::Data::Asset<JE::Render::RPI::ResourcePoolAsset> GetOutputStreamResourcePool() const override;

            uint32_t GetMaxSupportedVertexCount() const override;
            const JE::Render::RPI::ShaderInputContract& GetComputeShaderInputContract() const override;
        private:
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputStreamInfo;
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticStreamInfo;
            AZStd::array<SkinnedMeshOutputVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)> m_outputStreamInfo;
            
            AZ::Data::Asset<JE::Render::RPI::ResourcePoolAsset> m_outputStreamResourcePool;

            JE::Render::RPI::ShaderInputContract m_computeShaderInputContract;
        };
    } // namespace Features
} // namespace JE::Render
