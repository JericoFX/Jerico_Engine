/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>

#include <JE/Render/RPI/Public/Shader/Shader.h>

#include <Atom/RHI/CommandList.h>

namespace JE::Render
{
    namespace Features
    {
        SkinnedMeshComputePass::SkinnedMeshComputePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ComputePass(descriptor)
        {
        }

        JE::Render::RPI::Ptr<SkinnedMeshComputePass> SkinnedMeshComputePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<SkinnedMeshComputePass> pass = aznew SkinnedMeshComputePass(descriptor);
            return pass;
        }

        AZ::Data::Instance<JE::Render::RPI::Shader> SkinnedMeshComputePass::GetShader() const
        {
            return m_shader;
        }

        void SkinnedMeshComputePass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            if (auto featureProcessor = GetSkinnedMeshFeatureProcessor())
            {
                featureProcessor->SetupSkinningScope(frameGraph, Pass::GetDeviceIndex());
            }

            ComputePass::SetupFrameGraphDependencies(frameGraph);
        }

        void SkinnedMeshComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            if (auto featureProcessor = GetSkinnedMeshFeatureProcessor())
            {
                SetSrgsForDispatch(context);

                featureProcessor->SubmitSkinningDispatchItems(
                    context, context.GetSubmitRange().m_startIndex, context.GetSubmitRange().m_endIndex);
            }
        }

        void SkinnedMeshComputePass::OnShaderReinitialized(const JE::Render::RPI::Shader& shader)
        {
            ComputePass::OnShaderReinitialized(shader);
            if (auto featureProcessor = GetSkinnedMeshFeatureProcessor())
            {
                featureProcessor->OnSkinningShaderReinitialized(m_shader);
            }
        }

        void SkinnedMeshComputePass::OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant)
        {
            ComputePass::OnShaderVariantReinitialized(shaderVariant);
            if (auto featureProcessor = GetSkinnedMeshFeatureProcessor())
            {
                featureProcessor->OnSkinningShaderReinitialized(m_shader);
            }
        }

        SkinnedMeshFeatureProcessor* SkinnedMeshComputePass::GetSkinnedMeshFeatureProcessor()
        {
            JE::Render::RPI::Scene* scene{ GetScene() };
            if (!scene)
            {
                return nullptr;
            }
            auto result = scene->GetFeatureProcessor<SkinnedMeshFeatureProcessor>();
            return result;
        }
    } // namespace Features
} // namespace JE::Render
