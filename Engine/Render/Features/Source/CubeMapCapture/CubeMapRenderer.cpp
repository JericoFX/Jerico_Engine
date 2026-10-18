/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/CubeMapRenderer.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Reflect/Pass/EnvironmentCubeMapPassData.h>

namespace JE::Render
{
    namespace Features
    {
        void CubeMapRenderer::StartRender(RenderCubeMapCallback callback, const AZ::Transform& transform, float exposure)
        {
            AZ_Assert(m_scene, "CubeMapRenderer::StartRender called without a valid scene");
            AZ_Assert(m_renderingCubeMap == false, "CubeMapRenderer::StartRender called while a cubemap render was already in progress");
            if (m_renderingCubeMap)
            {
                return;
            }

            m_renderingCubeMap = true;
            m_callback = callback;
            m_exposure = exposure;

            JE::Render::RPI::RenderPipelineDescriptor environmentCubeMapPipelineDesc;
            environmentCubeMapPipelineDesc.m_mainViewTagName = "MainCamera";
            environmentCubeMapPipelineDesc.m_renderSettings.m_multisampleState = JE::Render::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
            environmentCubeMapPipelineDesc.m_renderSettings.m_size.m_width = JE::Render::RPI::EnvironmentCubeMapPass::CubeMapFaceSize;
            environmentCubeMapPipelineDesc.m_renderSettings.m_size.m_height = JE::Render::RPI::EnvironmentCubeMapPass::CubeMapFaceSize;
            environmentCubeMapPipelineDesc.m_allowModification = true; // Enable pipeline modification since we need to have GI lighting for the baking

            // create a unique name for the pipeline
            AZ::Uuid uuid = AZ::Uuid::CreateRandom();
            AZStd::string uuidString;
            uuid.ToString(uuidString);
            environmentCubeMapPipelineDesc.m_name = AZStd::string::format("EnvironmentCubeMapPipeline_%s", uuidString.c_str());
            JE::Render::RPI::RenderPipelinePtr environmentCubeMapPipeline = JE::Render::RPI::RenderPipeline::CreateRenderPipeline(environmentCubeMapPipelineDesc);
            m_environmentCubeMapPipelineId = environmentCubeMapPipeline->GetId();

            AZStd::shared_ptr<JE::Render::RPI::EnvironmentCubeMapPassData> passData = AZStd::make_shared<JE::Render::RPI::EnvironmentCubeMapPassData>();
            passData->m_position = transform.GetTranslation();

            JE::Render::RPI::PassDescriptor environmentCubeMapPassDescriptor(AZ::Name("EnvironmentCubeMapPass"));
            environmentCubeMapPassDescriptor.m_passData = passData;

            m_environmentCubeMapPass = JE::Render::RPI::EnvironmentCubeMapPass::Create(environmentCubeMapPassDescriptor);
            m_environmentCubeMapPass->SetRenderPipeline(environmentCubeMapPipeline.get());

            const JE::Render::RPI::Ptr<JE::Render::RPI::ParentPass>& rootPass = environmentCubeMapPipeline->GetRootPass();
            rootPass->AddChild(m_environmentCubeMapPass);

            // store the current exposure values
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> sceneSrg = m_scene->GetShaderResourceGroup();
            m_previousGlobalIblExposure = sceneSrg->GetConstant<float>(m_globalIblExposureConstantIndex);
            m_previousSkyBoxExposure = sceneSrg->GetConstant<float>(m_skyBoxExposureConstantIndex);

            // add the pipeline to the scene
            m_scene->AddRenderPipeline(environmentCubeMapPipeline);
        }

        void CubeMapRenderer::Update()
        {
            if (m_renderingCubeMap)
            {
                AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> sceneSrg = m_scene->GetShaderResourceGroup();

                // set exposures to the user specified value while baking the cubemap
                sceneSrg->SetConstant(m_globalIblExposureConstantIndex, m_exposure);
                sceneSrg->SetConstant(m_skyBoxExposureConstantIndex, m_exposure);
            }
        }

        void CubeMapRenderer::CheckAndRemovePipeline()
        {
            if (m_environmentCubeMapPass && m_environmentCubeMapPass->IsFinished())
            {
                AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> sceneSrg = m_scene->GetShaderResourceGroup();

                // all faces of the cubemap have been rendered, invoke the callback
                m_callback(m_environmentCubeMapPass->GetTextureData(), m_environmentCubeMapPass->GetTextureFormat());

                // restore exposures
                sceneSrg->SetConstant(m_globalIblExposureConstantIndex, m_previousGlobalIblExposure);
                sceneSrg->SetConstant(m_skyBoxExposureConstantIndex, m_previousSkyBoxExposure);

                m_renderingCubeMap = false;

                // remove the cubemap pipeline
                // Note: this must not be called in the scope of a feature processor Simulate or Render to avoid a race condition with other feature processors
                m_scene->RemoveRenderPipeline(m_environmentCubeMapPipelineId);
                m_environmentCubeMapPass = nullptr;
            }
        }

        void CubeMapRenderer::SetDefaultView(JE::Render::RPI::RenderPipeline* renderPipeline)
        {
            // check for an active cubemap build and matching pipeline
            if (m_environmentCubeMapPass
                && m_renderingCubeMap
                && m_environmentCubeMapPipelineId == renderPipeline->GetId())
            {
                m_environmentCubeMapPass->SetDefaultView();
            }
        }
    } // namespace Features
} // namespace JE::Render
