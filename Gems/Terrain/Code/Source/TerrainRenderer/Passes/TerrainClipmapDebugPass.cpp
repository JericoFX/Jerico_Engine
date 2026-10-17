/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Passes/TerrainClipmapDebugPass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <JE/Render/RPI/Public/ViewportContextBus.h>
#include <AzCore/Console/Console.h>

namespace Terrain
{
    AZ_CVAR(
        bool,
        r_terrainClipmapDebugEnable,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Turn on clipmap debug rendering on the screen.");

    JE::Render::RPI::Ptr<TerrainClipmapDebugPass> TerrainClipmapDebugPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<TerrainClipmapDebugPass> pass = aznew TerrainClipmapDebugPass(descriptor);
        return pass;
    }

    TerrainClipmapDebugPass::TerrainClipmapDebugPass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void TerrainClipmapDebugPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        JE::Render::RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainClipmapDebugPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }
        }

        JE::Render::RPI::FullscreenTrianglePass::CompileResources(context);
    }

    bool TerrainClipmapDebugPass::IsEnabled() const
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

        return r_terrainClipmapDebugEnable;
    }
}
