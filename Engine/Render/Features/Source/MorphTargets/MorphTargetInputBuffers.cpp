/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>

#include <JE/Render/RPI/Reflect/ResourcePoolAssetCreator.h>
#include <JE/Render/RPI/Reflect/Buffer/BufferAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAssetCreator.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/Math/PackedVector3.h>

namespace JE::Render
{
    namespace Features
    {
        MorphTargetInputBuffers::MorphTargetInputBuffers(const JE::Render::RPI::BufferAssetView& bufferAssetView, const AZStd::string& bufferNamePrefix)
        {            
            m_vertexDeltaBuffer = JE::Render::RPI::Buffer::FindOrCreate(bufferAssetView.GetBufferAsset());
            if (m_vertexDeltaBuffer)
            {
                m_vertexDeltaBufferView = m_vertexDeltaBuffer->GetRHIBuffer()->GetBufferView(bufferAssetView.GetBufferViewDescriptor());
                m_vertexDeltaBufferView->SetName(AZ::Name(bufferNamePrefix + "MorphTargetVertexDeltaView"));
            }
        }

        void MorphTargetInputBuffers::SetBufferViewsOnShaderResourceGroup(const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Set the delta buffer
            AZ::RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(AZ::Name{ "m_vertexDeltas" });
            AZ_Error("MorphTargetInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for 'm_positionDeltas' in the skinning compute shader per-instance SRG.");

            [[maybe_unused]] bool success = perInstanceSRG->SetBufferView(
                srgIndex, m_vertexDeltaBufferView.get());
            AZ_Error("MorphTargetInputBuffers", success, "Failed to bind buffer view for vertex deltas");
        }
    } // namespace Features
} // namespace JE::Render
