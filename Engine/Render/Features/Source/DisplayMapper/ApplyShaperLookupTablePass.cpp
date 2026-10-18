/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/ApplyShaperLookupTablePass.h>
#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ApplyShaperLookupTablePass> ApplyShaperLookupTablePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ApplyShaperLookupTablePass> pass = aznew ApplyShaperLookupTablePass(descriptor);
            return pass;
        }

        ApplyShaperLookupTablePass::ApplyShaperLookupTablePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : DisplayMapperFullScreenPass(descriptor)
        {
        }

        ApplyShaperLookupTablePass::~ApplyShaperLookupTablePass()
        {
            ReleaseLutImage();
        }

        void ApplyShaperLookupTablePass::InitializeInternal()
        {
            DisplayMapperFullScreenPass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "ApplyShaperLookupTablePass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputLutImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name{ "m_lut" });

                m_shaderShaperTypeIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name{ "m_shaperType" });
                m_shaderShaperBiasIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name{ "m_shaperBias" });
                m_shaderShaperScaleIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name{ "m_shaperScale" });
            }
            UpdateShaperSrg();
        }

        void ApplyShaperLookupTablePass::SetupFrameGraphDependenciesCommon([[maybe_unused]] AZ::RHI::FrameGraphInterface frameGraph)
        {
            if (m_needToReloadLut)
            {
                ReleaseLutImage();
                AcquireLutImage();
                m_needToReloadLut = false;
            }

            AZ_Assert(m_lutResource.m_lutStreamingImage != nullptr, "ApplyShaperLookupTablePass unable to acquire LUT image");
        }

        void ApplyShaperLookupTablePass::CompileResourcesCommon([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
        }

        void ApplyShaperLookupTablePass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);
            SetupFrameGraphDependenciesCommon(frameGraph);
            frameGraph.SetEstimatedItemCount(1);
        }

        void ApplyShaperLookupTablePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "ApplyShaperLookupTablePass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            CompileResourcesCommon(context);
            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void ApplyShaperLookupTablePass::AcquireLutImage()
        {
            auto displayMapper = m_pipeline->GetScene()->GetFeatureProcessor<JE::Render::Features::AcesDisplayMapperFeatureProcessor>();
            displayMapper->GetLutFromAssetId(m_lutResource, m_lutAssetId);
            UpdateShaperSrg();
        }

        void ApplyShaperLookupTablePass::ReleaseLutImage()
        {
            m_lutResource.m_lutStreamingImage.reset();
        }

        void ApplyShaperLookupTablePass::SetShaperParameters(const AZ::Render::ShaperParams& shaperParams)
        {
            m_shaperParams = shaperParams;
            UpdateShaperSrg();
        }

        void ApplyShaperLookupTablePass::SetLutAssetId(const AZ::Data::AssetId& assetId)
        {
            m_lutAssetId = assetId;
        }

        const AZ::Data::AssetId& ApplyShaperLookupTablePass::GetLutAssetId() const
        {
            return m_lutAssetId;
        }

        void ApplyShaperLookupTablePass::UpdateShaperSrg()
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "ApplyShaperLookupTablePass %s has a null shader resource group when calling UpdateShaperSrg.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                if (m_lutResource.m_lutStreamingImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputLutImageIndex, m_lutResource.m_lutStreamingImage->GetImageView());

                    m_shaderResourceGroup->SetConstant(m_shaderShaperTypeIndex, m_shaperParams.m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderShaperBiasIndex, m_shaperParams.m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderShaperScaleIndex, m_shaperParams.m_scale);
                }
            }
        }
    } // namespace Features
} // namespace JE::Render
