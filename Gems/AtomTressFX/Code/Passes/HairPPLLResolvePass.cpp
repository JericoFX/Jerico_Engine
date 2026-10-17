/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/Shader/ShaderSystem.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>

#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>

#include <Passes/HairPPLLResolvePass.h>
#include <Rendering/HairFeatureProcessor.h>
#include <Rendering/HairLightingModels.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairPPLLResolvePass::HairPPLLResolvePass(const JE::Render::RPI::PassDescriptor& descriptor)
                : JE::Render::RPI::FullscreenTrianglePass(descriptor)
            {
                o_enableShadows = AZ::Name("o_enableShadows");
                o_enableDirectionalLights = AZ::Name("o_enableDirectionalLights");
                o_enablePunctualLights = AZ::Name("o_enablePunctualLights");
                o_enableAreaLights = AZ::Name("o_enableAreaLights");
                o_enableIBL = AZ::Name("o_enableIBL");
                o_hairLightingModel = AZ::Name("o_hairLightingModel");
                o_enableMarschner_R = AZ::Name("o_enableMarschner_R");
                o_enableMarschner_TRT = AZ::Name("o_enableMarschner_TRT");
                o_enableMarschner_TT = AZ::Name("o_enableMarschner_TT");
                o_enableLongtitudeCoeff = AZ::Name("o_enableLongtitudeCoeff");
                o_enableAzimuthCoeff = AZ::Name("o_enableAzimuthCoeff");
            }

            void HairPPLLResolvePass::UpdateGlobalShaderOptions()
            {
                JE::Render::RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

                m_featureProcessor->GetHairGlobalSettings(m_hairGlobalSettings);

                shaderOption.SetValue(o_enableShadows, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableShadows });
                shaderOption.SetValue(o_enableDirectionalLights, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableDirectionalLights });
                shaderOption.SetValue(o_enablePunctualLights, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enablePunctualLights });
                shaderOption.SetValue(o_enableAreaLights, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAreaLights });
                shaderOption.SetValue(o_enableIBL, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableIBL });
                shaderOption.SetValue(o_hairLightingModel, AZ::Name{ "HairLightingModel::" + AZStd::string(HairLightingModelNamespace::ToString(m_hairGlobalSettings.m_hairLightingModel)) });
                shaderOption.SetValue(o_enableMarschner_R, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_R });
                shaderOption.SetValue(o_enableMarschner_TRT, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TRT });
                shaderOption.SetValue(o_enableMarschner_TT, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableMarschner_TT });
                shaderOption.SetValue(o_enableLongtitudeCoeff, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableLongtitudeCoeff });
                shaderOption.SetValue(o_enableAzimuthCoeff, JE::Render::RPI::ShaderOptionValue{ m_hairGlobalSettings.m_enableAzimuthCoeff });
                shaderOption.SetUnspecifiedToDefaultValues();

                if (m_pipelineStateForDraw.GetShaderVariantId() != shaderOption.GetShaderVariantId())
                {
                    UpdateShaderOptions(shaderOption.GetShaderVariantId());
                }
            }

            JE::Render::RPI::Ptr<HairPPLLResolvePass> HairPPLLResolvePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
            {
                JE::Render::RPI::Ptr<HairPPLLResolvePass> pass = aznew HairPPLLResolvePass(descriptor);

                return pass;
            }

            void HairPPLLResolvePass::InitializeInternal()
            {
                if (GetScene())
                {
                    FullscreenTrianglePass::InitializeInternal();
                }
            }

            bool HairPPLLResolvePass::AcquireFeatureProcessor()
            {
                if (m_featureProcessor)
                {
                    return true;
                }

                JE::Render::RPI::Scene* scene = GetScene();
                if (scene)
                {
                    m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                }
                else
                {
                    return false;
                }

                if (!m_featureProcessor || !m_featureProcessor->IsInitialized())
                {
                    AZ_Warning("Hair Gem", m_featureProcessor,
                        "HairPPLLResolvePass [%s] - Failed to retrieve Hair feature processor from the scene",
                        GetName().GetCStr());
                    m_featureProcessor = nullptr;   // set it as null if not initialized to repeat this check.
                    return false;
                }
                return true;
            }

            void HairPPLLResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !AcquireFeatureProcessor())
                {
                    AZ_Error("Hair Gem", m_shaderResourceGroup, "HairPPLLResolvePass: PPLL list data was not bound - missing Srg");
                    return;     // no error message due to FP - initialization not complete yet, wait for the next frame
                }

                UpdateGlobalShaderOptions();

                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    JE::Render::RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                    PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                    Name{ "LinkedListNodesPPLL" }, Name{ "m_linkedListNodes" }, 0, 0
                );
                if (!UtilityClass::BindBufferToSrg("Hair Gem", m_featureProcessor->GetPerPixelListBuffer(), descriptor, m_shaderResourceGroup))
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePass: PPLL list data could not be bound.");
                }

                // Update the material array constant buffer within the per pass srg
                descriptor = SrgBufferDescriptor(
                    JE::Render::RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );

                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // All remaining srgs should compile here
                FullscreenTrianglePass::CompileResources(context);
            }

        } // namespace Hair
     }   // namespace Render
}   // namespace AZ

