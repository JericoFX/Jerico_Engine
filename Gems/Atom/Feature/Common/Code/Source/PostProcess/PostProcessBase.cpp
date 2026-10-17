/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

#include <PostProcess/PostProcessBase.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        PostProcessBase::PostProcessBase(PostProcessFeatureProcessor* featureProcessor)
            : m_featureProcessor(featureProcessor)
        { }

        JE::Render::RPI::Scene* PostProcessBase::GetParentScene() const
        {
            if (m_featureProcessor)
            {
                return m_featureProcessor->GetParentScene();
            }
            return nullptr;
        }

        JE::Render::RPI::ShaderResourceGroup* PostProcessBase::GetSceneSrg() const
        {
            if (GetParentScene())
            {
                return GetParentScene()->GetShaderResourceGroup().get();
            }
            return nullptr;
        }

        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> PostProcessBase::GetDefaultViewSrg() const
        {
            if (GetParentScene() &&
                GetParentScene()->GetDefaultRenderPipeline() &&
                GetParentScene()->GetDefaultRenderPipeline()->GetDefaultView())
            {
                return GetParentScene()->GetDefaultRenderPipeline()->GetDefaultView()->GetShaderResourceGroup();
            }
            return nullptr;
        }

    }
}
