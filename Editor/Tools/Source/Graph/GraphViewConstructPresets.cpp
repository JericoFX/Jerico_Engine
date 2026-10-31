/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Graph/GraphViewConstructPresets.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/Comment/CommentBus.h>

#include <AzCore/Serialization/EditContext.h>

namespace JE::Editor::Tools
{
    void GraphViewConstructPresets::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphViewConstructPresets, JE::Editor::Tools::GraphCanvas::EditorConstructPresets>()
                ->Version(0);

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<GraphViewConstructPresets>("GraphViewConstructPresets", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void GraphViewConstructPresets::InitializeConstructType(JE::Editor::Tools::GraphCanvas::ConstructType constructType)
    {
        if (constructType == JE::Editor::Tools::GraphCanvas::ConstructType::NodeGroup)
        {
            auto presetBucket = static_cast<JE::Editor::Tools::GraphCanvas::NodeGroupPresetBucket*>(ModPresetBucket(JE::Editor::Tools::GraphCanvas::ConstructType::NodeGroup));
            if (presetBucket)
            {
                presetBucket->ClearPresets();

                // Create all of the initial node group presets using the default names and colors
                for (const auto& defaultGroupPresetPair : m_defaultGroupPresets)
                {
                    if (auto preset = presetBucket->CreateNewPreset(defaultGroupPresetPair.first))
                    {
                        const auto& presetSaveData = preset->GetPresetData();
                        if (auto saveData = presetSaveData.FindSaveDataAs<JE::Editor::Tools::GraphCanvas::CommentNodeTextSaveData>())
                        {
                            saveData->m_backgroundColor = defaultGroupPresetPair.second;
                        }
                    }
                }
            }
        }
        else if (constructType == JE::Editor::Tools::GraphCanvas::ConstructType::CommentNode)
        {
            auto presetBucket = static_cast<JE::Editor::Tools::GraphCanvas::CommentPresetBucket*>(ModPresetBucket(JE::Editor::Tools::GraphCanvas::ConstructType::CommentNode));
            if (presetBucket)
            {
                presetBucket->ClearPresets();
            }
        }
    }

    void GraphViewConstructPresets::SetDefaultGroupPresets(const AZStd::map<AZStd::string, AZ::Color>& defaultGroupPresets)
    {
        m_defaultGroupPresets = defaultGroupPresets;
    }
} // namespace JE::Editor::Tools
