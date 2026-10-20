/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorModeFeedbackParentPass.h>

#include <JE/Editor/Render/Features/EditorModeFeedback/EditorModeFeedbackInterface.h>

namespace JE::Editor
{
    namespace Render::Features
    {
        JE::Render::RPI::Ptr<EditorModeFeedbackParentPass> EditorModeFeedbackParentPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<EditorModeFeedbackParentPass> pass = aznew EditorModeFeedbackParentPass(descriptor);
            return pass;
        }

        EditorModeFeedbackParentPass::EditorModeFeedbackParentPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        {
        }

        bool EditorModeFeedbackParentPass::IsEnabled() const
        {
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return ParentPass::IsEnabled() && editorModeFeedback->IsEnabled();
            }

            return false;
        }
    } // namespace Render::Features
} // namespace JE::Editor
