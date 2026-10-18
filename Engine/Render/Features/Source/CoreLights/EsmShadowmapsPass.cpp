/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/DepthExponentiationPass.h>

#include <JE/Render/Features/CoreLights/EsmShadowmapsPassData.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <Math/MathFilterDescriptor.h>
#include <Math/MathFilter.h>

namespace JE::Render
{
    namespace Features
    {
        // --- Pass Creation ---

        JE::Render::RPI::Ptr<EsmShadowmapsPass> EsmShadowmapsPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew EsmShadowmapsPass(descriptor);
        }

        EsmShadowmapsPass::EsmShadowmapsPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            const EsmShadowmapsPassData* esmData = JE::Render::RPI::PassUtils::GetPassData<EsmShadowmapsPassData>(descriptor);

            if (!esmData)
            {
                return;
            }

            m_lightTypeName = esmData->m_lightType;

            if (m_lightTypeName == AZ::Name("projected"))
            {
                m_lightType = EsmLightType::Projected;
            }
            else if (m_lightTypeName == AZ::Name("directional"))
            {
                m_lightType = EsmLightType::Directional;
            }
        }

        // --- Setters/Getters ---

        const AZ::Name& EsmShadowmapsPass::GetLightTypeName() const
        {
            return m_lightTypeName;
        }

        bool EsmShadowmapsPass::GetIsProjected() const
        {
            return m_lightType == EsmLightType::Projected;
        }

        void EsmShadowmapsPass::SetShadowmapIndexTableBuffer(const AZ::Data::Instance<JE::Render::RPI::Buffer>& tableBuffer)
        {
            m_shadowmapIndexTableBuffer = tableBuffer;
        }

        void EsmShadowmapsPass::SetFilterParameterBuffer(const AZ::Data::Instance<JE::Render::RPI::Buffer>& dataBuffer)
        {
            m_filterParameterBuffer = dataBuffer;
        }

        void EsmShadowmapsPass::SetEsmExponent(float exponent)
        {
            m_esmExponent = exponent;
        }

        void EsmShadowmapsPass::SetBlurParameters(AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg, const uint32_t childPassIndex)
        {
            if (m_shadowmapIndexTableBufferIndices[childPassIndex].IsNull())
            {
                m_shadowmapIndexTableBufferIndices[childPassIndex] = srg->FindShaderInputBufferIndex(AZ::Name("m_shadowmapIndexTable"));
            }
            srg->SetBuffer(m_shadowmapIndexTableBufferIndices[childPassIndex], m_shadowmapIndexTableBuffer);

            if (m_filterParameterBufferIndices[childPassIndex].IsNull())
            {
                m_filterParameterBufferIndices[childPassIndex] = srg->FindShaderInputBufferIndex(AZ::Name("m_filterParameters"));
            }
            srg->SetBuffer(m_filterParameterBufferIndices[childPassIndex], m_filterParameterBuffer);

            srg->SetConstant(srg->FindShaderInputConstantIndex(AZ::Name("m_esmExponent")), m_esmExponent);
        }

        void EsmShadowmapsPass::SetKawaseBlurSpecificParameters(AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg, uint32_t kawaseBlurIndex)
        {
            if (m_kawaseBlurConstantIndices[kawaseBlurIndex].IsNull())
            {
                m_kawaseBlurConstantIndices[kawaseBlurIndex] = srg->FindShaderInputConstantIndex(AZ::Name("m_rcpResolutionAndIteration"));
            }
            const AZ::Vector4 data(
                1.0f / m_shadowmapImageSize.m_width, 1.0f / m_shadowmapImageSize.m_height, aznumeric_cast<float>(kawaseBlurIndex), 0.0f);

            srg->SetConstant(m_kawaseBlurConstantIndices[kawaseBlurIndex], data);
        }

        // --- Frame Render ---

        void EsmShadowmapsPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetEnabledComputation(m_computationEnabled);
            if (m_computationEnabled)
            {
                UpdateChildren();
            }
            Base::FrameBeginInternal(params);
        }

        void EsmShadowmapsPass::BuildInternal()
        {
            if (GetInputOutputCount() > 0 && GetIsProjected())
            {
                if (!m_atlasAttachmentImage)
                {
                    auto binding = GetInputOutputBinding(0);
                    if (binding.GetAttachment() == nullptr)
                    {
                        // Make sure at least a dummy image is attached if lacking any other attachments.
                        // This may be necessary for a few frames during initialization.
                        auto tempAttachmentImage = JE::Render::RPI::ImageSystemInterface::Get()->GetSystemAttachmentImage(AZ::RHI::Format::R16_FLOAT);
                        AttachImageToSlot(AZ::Name("EsmShadowmaps"), tempAttachmentImage);
                    }
                }
                else
                {
                    AttachImageToSlot(AZ::Name("EsmShadowmaps"), m_atlasAttachmentImage);
                }
            }

            Base::BuildInternal();
        }

        void EsmShadowmapsPass::SetEnabledComputation(bool enabled)
        {
            if (m_lightTypeName.IsEmpty())
            {
                return;
            }

            for (const JE::Render::RPI::Ptr<JE::Render::RPI::Pass>& child : GetChildren())
            {
                child->SetEnabled(enabled);
            }

            if (enabled)
            {
                auto* exponentiationPass = azrtti_cast<DepthExponentiationPass*>(GetChildren()[static_cast<uint32_t>(EsmChildPassKind::Exponentiation)].get());
                AZ_Assert(exponentiationPass, "Child not found or not of type DepthExponentiationPass.");
                if (m_lightType == EsmLightType::Directional)
                {
                    exponentiationPass->SetShadowmapType(Shadow::ShadowmapType::Directional);
                }
                else if (m_lightType == EsmLightType::Projected)
                {
                    exponentiationPass->SetShadowmapType(Shadow::ShadowmapType::Projected);
                }
                else
                {
                    AZ_Assert(false, "LightType is illegal");
                }
            }

            m_computationEnabled = enabled;
        }

        void EsmShadowmapsPass::SetAtlasAttachmentImage(AZ::Data::Instance<JE::Render::RPI::AttachmentImage> atlasAttachmentIamge)
        {
            if (m_atlasAttachmentImage != atlasAttachmentIamge)
            {
                m_atlasAttachmentImage = atlasAttachmentIamge;
                QueueForBuildAndInitialization();
            }
        }

        void EsmShadowmapsPass::UpdateChildren()
        {
            const JE::Render::RPI::PassAttachmentBinding& inputBinding = GetInputBinding(0);

            if (!inputBinding.GetAttachment())
            {
                AZ_Assert(false, "[EsmShadowmapsPass %s] requires an input attachment", GetPathName().GetCStr());
                return;
            }

            AZ_Assert(inputBinding.GetAttachment()->m_descriptor.m_type == AZ::RHI::AttachmentType::Image, "[EsmShadowmapsPass %s] input attachment requires an image attachment", GetPathName().GetCStr());
            m_shadowmapImageSize = inputBinding.GetAttachment()->m_descriptor.m_image.m_size;
            m_shadowmapArraySize = inputBinding.GetAttachment()->m_descriptor.m_image.m_arraySize;

            const AZStd::span<const JE::Render::RPI::Ptr<JE::Render::RPI::Pass>>& children = GetChildren();
            AZ_Assert(children.size() == EsmChildPassKindCount, "[EsmShadowmapsPass '%s'] The count of children is wrong.", GetPathName().GetCStr());

            for (uint32_t childPassIndex = 0; childPassIndex < EsmChildPassKindCount; ++childPassIndex)
            {
                JE::Render::RPI::ComputePass* child = azrtti_cast<JE::Render::RPI::ComputePass*>(children[childPassIndex].get());
                AZ_Assert(child, "[EsmShadowmapsPass '%s'] A child does not compute.", GetPathName().GetCStr());

                AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg = child->GetShaderResourceGroup();

                SetBlurParameters(srg, childPassIndex);
                if (childPassIndex >= aznumeric_cast<uint32_t>(EsmChildPassKind::KawaseBlur0))
                {
                    SetKawaseBlurSpecificParameters(srg, childPassIndex - aznumeric_cast<uint32_t>(EsmChildPassKind::KawaseBlur0));
                }

                child->SetTargetThreadCounts(
                    m_shadowmapImageSize.m_width,
                    m_shadowmapImageSize.m_height,
                    m_shadowmapArraySize);
            }
        }
    } // namespace Features
} // namespace JE::Render
