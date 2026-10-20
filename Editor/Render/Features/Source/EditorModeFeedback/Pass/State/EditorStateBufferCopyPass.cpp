/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/State/EditorStateBufferCopyPassData.h>

#include <JE/Render/RPI/Public/Pass/PassUtils.h>

namespace JE::Editor::Render::Features
{
    JE::Render::RPI::Ptr<EditorStateBufferCopyPass> EditorStateBufferCopyPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<EditorStateBufferCopyPass> pass = aznew EditorStateBufferCopyPass(descriptor);
        return pass;
    }

    EditorStateBufferCopyPass::EditorStateBufferCopyPass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        , m_passDescriptor(descriptor)
    {
    }

    bool EditorStateBufferCopyPass::IsEnabled() const
    {
        const JE::Editor::Render::Features::EditorStateBufferCopyPassData* passData =
            JE::Render::RPI::PassUtils::GetPassData<JE::Editor::Render::Features::EditorStateBufferCopyPassData>(m_passDescriptor);

        if (passData == nullptr)
        {
            AZ_Error(
                "EditorStateBufferCopyPass", false, "[EditorStateBufferCopyPassData '%s']: Trying to construct without valid EditorStateBufferCopyPassData!", GetPathName().GetCStr());
            return false;
        }
        return passData->editorStatePass->IsEnabled();
    }
} // namespace JE::Editor::Render::Features
