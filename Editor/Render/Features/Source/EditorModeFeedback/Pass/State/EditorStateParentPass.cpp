/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateParentPass.h>
#include <Pass/State/EditorStateParentPassData.h>

#include <JE/Render/RPI/Public/Pass/PassUtils.h>

namespace JE::Editor::Render::Features
{
    JE::Render::RPI::Ptr<EditorStateParentPass> EditorStateParentPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<EditorStateParentPass> pass = aznew EditorStateParentPass(descriptor);
        return pass;
    }

    EditorStateParentPass::EditorStateParentPass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::ParentPass(descriptor)
        , m_passDescriptor(descriptor)
    {
    }

    bool EditorStateParentPass::IsEnabled() const
    {
        const JE::Editor::Render::Features::EditorStateParentPassData* passData =
            JE::Render::RPI::PassUtils::GetPassData<JE::Editor::Render::Features::EditorStateParentPassData>(m_passDescriptor);
        if (passData == nullptr)
        {
                AZ_Error(
                    "EditorStateParentPass", false, "[EditorStateParentPassData '%s']: Trying to construct without valid EditorStateParentPassData!", GetPathName().GetCStr());
                return false;
        }
        return passData->editorStatePass->IsEnabled();
    }
} // namespace JE::Editor::Render::Features
