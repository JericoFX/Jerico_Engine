/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <TerrainRenderer/Passes/TerrainClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <TerrainRenderer/TerrainClipmapManager.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/CommandList.h>

namespace Terrain
{
    JE::Render::RPI::Ptr<TerrainMacroClipmapGenerationPass> TerrainMacroClipmapGenerationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<TerrainMacroClipmapGenerationPass> pass = aznew TerrainMacroClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainMacroClipmapGenerationPass::TerrainMacroClipmapGenerationPass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::ComputePass(descriptor)
    {
    }

    void TerrainMacroClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroColor, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, attachmentDatabase);

            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroColor, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
        }
    }

    void TerrainMacroClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

            auto arguments{m_dispatchItem.GetArguments()};
            clipmapManager.GetMacroDispatchThreadNum(
                arguments.m_direct.m_totalNumberOfThreadsX,
                arguments.m_direct.m_totalNumberOfThreadsY,
                arguments.m_direct.m_totalNumberOfThreadsZ);
            m_dispatchItem.SetArguments(arguments);

            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }

            if (m_needsUpdate)
            {
                m_shaderResourceGroup->SetImage(
                    m_macroColorClipmapsIndex,
                    clipmapManager.GetClipmapImage(TerrainClipmapManager::ClipmapName::MacroColor)
                );

                m_shaderResourceGroup->SetImage(
                    m_macroNormalClipmapsIndex,
                    clipmapManager.GetClipmapImage(TerrainClipmapManager::ClipmapName::MacroNormal)
                );

                m_needsUpdate = false;
            }
        }

        JE::Render::RPI::ComputePass::CompileResources(context);
    }

    bool TerrainMacroClipmapGenerationPass::IsEnabled() const
    {
        if (!JE::Render::RPI::Pass::IsEnabled())
        {
            return false;
        }

        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (!terrainFeatureProcessor)
        {
            return false;
        }

        const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

        return clipmapManager.HasMacroClipmapUpdate();
    }

    JE::Render::RPI::Ptr<TerrainDetailClipmapGenerationPass> TerrainDetailClipmapGenerationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<TerrainDetailClipmapGenerationPass> pass = aznew TerrainDetailClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainDetailClipmapGenerationPass::TerrainDetailClipmapGenerationPass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::ComputePass(descriptor)
        , m_clipmapImageIndex{
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroColor]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroNormal]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailColor]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailNormal]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailHeight]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailRoughness]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailSpecularF0]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailMetalness]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailOcclusion])
        }
    {
    }

    void TerrainDetailClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            //! If this frame, macro clipmap update is skipped but detail is not,
            //! then the detail pass will be responsible for importing the clipmaps.
            if (!clipmapManager.HasMacroClipmapUpdate())
            {
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroColor, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, attachmentDatabase);
            }

            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailColor, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, attachmentDatabase);

            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroColor, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailColor, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
        }

        JE::Render::RPI::ComputePass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainDetailClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

            auto arguments{m_dispatchItem.GetArguments()};
            clipmapManager.GetMacroDispatchThreadNum(
                arguments.m_direct.m_totalNumberOfThreadsX,
                arguments.m_direct.m_totalNumberOfThreadsY,
                arguments.m_direct.m_totalNumberOfThreadsZ);
            m_dispatchItem.SetArguments(arguments);

            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }

            if (m_needsUpdate)
            {
                for (uint32_t i = 0; i < TerrainClipmapManager::ClipmapName::Count; ++i)
                {
                    m_shaderResourceGroup->SetImage(
                        m_clipmapImageIndex[i],
                        clipmapManager.GetClipmapImage(static_cast<TerrainClipmapManager::ClipmapName>(i))
                    );
                }

                m_needsUpdate = false;
            }
        }

        JE::Render::RPI::ComputePass::CompileResources(context);
    }

    bool TerrainDetailClipmapGenerationPass::IsEnabled() const
    {
        if (!JE::Render::RPI::Pass::IsEnabled())
        {
            return false;
        }

        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (!terrainFeatureProcessor)
        {
            return false;
        }

        const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

        if (!clipmapManager.IsClipmapEnabled())
        {
            return false;
        }

        return clipmapManager.HasDetailClipmapUpdate();
    }

    bool TerrainDetailClipmapGenerationPass::ClipmapFeatureIsEnabled() const
    {
        return JE::Render::RPI::Pass::IsEnabled();
    }
} // namespace Terrain
