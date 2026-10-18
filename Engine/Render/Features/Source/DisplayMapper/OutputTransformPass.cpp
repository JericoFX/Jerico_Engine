/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/OutputTransformPass.h>
#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<OutputTransformPass> OutputTransformPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<OutputTransformPass> pass = aznew OutputTransformPass(descriptor);
            return pass;
        }

        OutputTransformPass::OutputTransformPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : ApplyShaperLookupTablePass(descriptor)
            , m_toneMapperShaderVariantOptionName(ToneMapperShaderVariantOptionName)
            , m_transferFunctionShaderVariantOptionName(TransferFunctionShaderVariantOptionName)
            , m_ldrGradingLutShaderVariantOptionName(LdrGradingLutShaderVariantOptionName)
        {
            m_flags.m_bindViewSrg = true;
        }

        OutputTransformPass::~OutputTransformPass()
        {
        }

        void OutputTransformPass::InitializeInternal()
        {
            ApplyShaperLookupTablePass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputCinemaLimitsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name{ "m_cinemaLimits" });
            }

            InitializeShaderVariant();
        }

        void OutputTransformPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            if (GetLutAssetId().IsValid())
            {
                ApplyShaperLookupTablePass::SetupFrameGraphDependenciesCommon(frameGraph);
            }
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);

            frameGraph.SetEstimatedItemCount(1);
        }

        void OutputTransformPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            if (GetLutAssetId().IsValid())
            {
                ApplyShaperLookupTablePass::CompileResourcesCommon(context);
            }

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);

            m_shaderResourceGroup->SetConstant(m_shaderInputCinemaLimitsIndex, m_displayMapperParameters.m_cinemaLimits);
            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void OutputTransformPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "LookModificationPass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            AZ::RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            SetSrgsForDraw(context);

            m_item.SetPipelineState(GetPipelineStateFromShaderVariant());

            commandList->Submit(m_item.GetDeviceDrawItem(context.GetDeviceIndex()));
        }

        void OutputTransformPass::FrameBeginInternal(FramePrepareParams params)
        {
            ApplyShaperLookupTablePass::FrameBeginInternal(params);
            JE::Render::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                if (fp)
                {
                    JE::Render::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            settings->UpdateBuffer();
                            view->GetShaderResourceGroup()->SetBufferView(m_exposureControlBufferInputIndex, settings->GetBufferView());
                        }
                    }
                }
            }
        }

        void OutputTransformPass::SetToneMapperType(const AZ::Render::ToneMapperType& toneMapperType)
        {
            if (m_toneMapperType != toneMapperType)
            {
                m_toneMapperType = toneMapperType;
                m_needToUpdateShaderVariant = true;
            }
        }

        void OutputTransformPass::SetTransferFunctionType(const AZ::Render::TransferFunctionType& transferFunctionType)
        {
            if (m_transferFunctionType != transferFunctionType)
            {
                m_transferFunctionType = transferFunctionType;
                m_needToUpdateShaderVariant = true;
            }
        }

        void OutputTransformPass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "OutputTransformPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            AZStd::vector<AZ::Name> toneMapperVariationTypes = { AZ::Name("ToneMapperType::None"),
                                                                 AZ::Name("ToneMapperType::Reinhard"),
                                                                 AZ::Name("ToneMapperType::ReinhardExtended"),
                                                                 AZ::Name("ToneMapperType::AcesFitted"),
                                                                 AZ::Name("ToneMapperType::AcesFilmic"),
                                                                 AZ::Name("ToneMapperType::Filmic"),
                                                                 AZ::Name("ToneMapperType::Agx"),
                                                                 AZ::Name("ToneMapperType::AgxGolden"),
                                                                 AZ::Name("ToneMapperType::AgxPunchy"),
                                                                 AZ::Name("ToneMapperType::AgxWarm"),
                                                                 AZ::Name("ToneMapperType::PbrNeutral") };
            AZStd::vector<AZ::Name> transferFunctionVariationTypes = {
                AZ::Name("TransferFunctionType::None"),
                AZ::Name("TransferFunctionType::Gamma22"),
                AZ::Name("TransferFunctionType::PerceptualQuantizer") };

            AZStd::vector<AZ::Name> doLdrGradingTypes = {AZ::Name("true"), AZ::Name("false")};

            // Caching all pipeline state for each shader variation for performance reason.
            auto shaderOption = m_shader->CreateShaderOptionGroup();
            for (uint32_t tonemapperIndex = 0; tonemapperIndex < toneMapperVariationTypes.size(); ++tonemapperIndex)
            {
                for (uint32_t transferFunctionIndex = 0; transferFunctionIndex < transferFunctionVariationTypes.size(); ++transferFunctionIndex)
                {
                    for (uint32_t colorGradingIndex = 0; colorGradingIndex < doLdrGradingTypes.size(); ++colorGradingIndex)
                    {
                        shaderOption.SetValue(
                            m_toneMapperShaderVariantOptionName, toneMapperVariationTypes[tonemapperIndex]);
                        shaderOption.SetValue(
                            m_transferFunctionShaderVariantOptionName, transferFunctionVariationTypes[transferFunctionIndex]);
                        shaderOption.SetValue(
                            m_ldrGradingLutShaderVariantOptionName, doLdrGradingTypes[colorGradingIndex]);

                        PreloadShaderVariantForDraw(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
                    }
                }
            }

            m_needToUpdateShaderVariant = true;
        }

        void OutputTransformPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "OutputTransformPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            auto shaderOption = m_shader->CreateShaderOptionGroup();
            // Decide which shader to use.
            switch (m_toneMapperType)
            {
            case AZ::Render::ToneMapperType::Reinhard:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::Reinhard"));
                break;
            case AZ::Render::ToneMapperType::ReinhardExtended:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::ReinhardExtended"));
                break;
            case AZ::Render::ToneMapperType::AcesFitted:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AcesFitted"));
                break;
            case AZ::Render::ToneMapperType::Filmic:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::Filmic"));
                break;
            case AZ::Render::ToneMapperType::AcesFilmic:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AcesFilmic"));
                break;
            case AZ::Render::ToneMapperType::Agx:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::Agx"));
                break;
            case AZ::Render::ToneMapperType::AgxGolden:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AgxGolden"));
                break;
            case AZ::Render::ToneMapperType::AgxPunchy:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AgxPunchy"));
                break;
            case AZ::Render::ToneMapperType::AgxWarm:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AgxWarm"));
                break;
            case AZ::Render::ToneMapperType::PbrNeutral:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::PbrNeutral"));
                break;
            default:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::None"));
                break;
            }
            switch (m_transferFunctionType)
            {
            case AZ::Render::TransferFunctionType::Gamma22:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::Gamma22"));
                break;
            case AZ::Render::TransferFunctionType::PerceptualQuantizer:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::PerceptualQuantizer"));
                break;
            default:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::None"));
                break;
            }

            shaderOption.SetValue(m_ldrGradingLutShaderVariantOptionName, GetLutAssetId().IsValid() ? AZ::Name("true") : AZ::Name("false"));

            UpdateShaderVariant(shaderOption);
            m_needToUpdateShaderVariant = false;
        }

        void OutputTransformPass::SetDisplayBufferFormat(AZ::RHI::Format format)
        {
            if (m_displayBufferFormat != format)
            {
                m_displayBufferFormat = format;
                if (m_displayBufferFormat == AZ::RHI::Format::R8G8B8A8_UNORM ||
                    m_displayBufferFormat == AZ::RHI::Format::B8G8R8A8_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, AZ::Render::OutputDeviceTransformType_48Nits);
                }
                else if (m_displayBufferFormat == AZ::RHI::Format::R10G10B10A2_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, AZ::Render::OutputDeviceTransformType_1000Nits);
                }
                else
                {
                    AZ_Assert(false, "Not yet supported.");
                    // To work normally on unsupported environment, initialize the display parameters by OutputDeviceTransformType_48Nits.
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, AZ::Render::OutputDeviceTransformType_48Nits);
                }
            }
        }
    } // namespace Features
} // namespace JE::Render
