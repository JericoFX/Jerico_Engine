/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <./LyShineFeatureProcessor.h>

#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>

namespace LyShine
{
    void LyShineFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineFeatureProcessor, JE::Render::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void LyShineFeatureProcessor::AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        // Only add LyShineParentPass if UIPass exists
        if (!renderPipeline->FindFirstPass(AZ::Name("UIPass")))
        {
            return;
        }

        // Get the pass request if it's not loaded
        if (!m_passRequestAsset)
        {
            const char* passRequestAssetFilePath = "Passes/LyShinePassRequest.azasset";
            m_passRequestAsset = JE::Render::RPI::AssetUtils::LoadAssetByProductPath<JE::Render::RPI::AnyAsset>(
                passRequestAssetFilePath, JE::Render::RPI::AssetUtils::TraceLevel::Warning);
            
        }

        const JE::Render::RPI::PassRequest *passRequest = nullptr;
        if (m_passRequestAsset->IsReady())
        {
            passRequest = m_passRequestAsset->GetDataAs<JE::Render::RPI::PassRequest>();
        }

        if (!passRequest)
        {
            AZ_Error("LyShine", false, "Failed to add LyShine parent pass. Can't load PassRequest from %s", m_passRequestAsset.GetHint().c_str());
            return;
        }

        // Return if the pass to be created already exists
        JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        JE::Render::RPI::Pass* pass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (pass)
        {
            return;
        }

        // Create the pass
        JE::Render::RPI::Ptr<JE::Render::RPI::Pass> lyShineParentPass  = JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!lyShineParentPass)
        {
            AZ_Error("LyShine", false, "Create LyShine parent pass from pass request failed");
            return;
        }

        // Insert the LyShineParentPass before UIPass
        bool success = renderPipeline->AddPassBefore(lyShineParentPass, AZ::Name("UIPass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("LyShine", false, "Add the LyShine parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
    }

}
