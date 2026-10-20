/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/Child/EditorModeTintPass.h>

#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

// Temporary measure for setting the color tint pass shader parameters at runtime until GHI 3455 is implemented
JE_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeTintPass, 0.0f, 0.0f, 0.0f, 1.0f);
JE_EDITOR_MODE_PASS_CVAR(float, cl_editorModeTintPass, TintAmount, 0.5f);
JE_EDITOR_MODE_PASS_CVAR(AZ::Color, cl_editorModeTintPass, TintColor, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f));

namespace JE::Editor
{
    namespace Render::Features
    {
        JE::Render::RPI::Ptr<EditorModeTintPass> EditorModeTintPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<EditorModeTintPass> pass = aznew EditorModeTintPass(descriptor);
            return pass;
        }

        EditorModeTintPass::EditorModeTintPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : EditorModeFeedbackChildPassBase(descriptor)
        {
        }

        void EditorModeTintPass::InitializeInternal()
        {
            EditorModeFeedbackChildPassBase::InitializeInternal();
            m_tintAmountIndex.Reset();
        }

        void EditorModeTintPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackChildPassBase::FrameBeginInternal(params);
        }

        void EditorModeTintPass::SetTintAmount(const float amount)
        {
            m_tintAmount = amount;
        }

        void EditorModeTintPass::SetTintColor(AZ::Color color)
        {
            m_tintColor = color;
        }

        void EditorModeTintPass::SetSrgConstants()
        {
            // Temporary measure for setting the pass shader parameters at runtime until GHI 3455 is implemented
            SetMinDepthTransitionValue(cl_editorModeTintPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeTintPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeTintPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeTintPass_FinalBlendAmount);
            SetTintAmount(cl_editorModeTintPass_TintAmount);
            SetTintColor(cl_editorModeTintPass_TintColor);

            m_shaderResourceGroup->SetConstant(m_tintAmountIndex, m_tintAmount);
            m_shaderResourceGroup->SetConstant(m_tintColorIndex, m_tintColor);
        }
    } // namespace Render::Features
} // namespace JE::Editor
