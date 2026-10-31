/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace LandscapeCanvasEditor
{
    class FindSelectedNodesAction
        : public JE::Editor::Tools::GraphCanvas::ContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(FindSelectedNodesAction, AZ::SystemAllocator);

        FindSelectedNodesAction(QObject* parent);
        virtual ~FindSelectedNodesAction() = default;

        JE::Editor::Tools::GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using JE::Editor::Tools::GraphCanvas::ContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::ContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        void UpdateActionState();
    };
}
