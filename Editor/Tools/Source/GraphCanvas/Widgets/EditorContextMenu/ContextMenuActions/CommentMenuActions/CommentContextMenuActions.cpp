/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentContextMenuActions.h>

#include <JE/Editor/Tools/GraphCanvas/Components/GridBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>

#include <JE/Editor/Tools/GraphCanvas/Utils/GraphUtils.h>

namespace JE::Editor::Tools::GraphCanvas
{
    //////////////////////////
    // EditCommentMenuAction
    //////////////////////////

    EditCommentMenuAction::EditCommentMenuAction(QObject* parent)
        : CommentContextMenuAction("Edit comment", parent)
    {
        setToolTip("Edits the selected comment");
    }

    void EditCommentMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        setEnabled(GraphUtils::IsComment(targetId));
    }

    ContextMenuAction::SceneReaction EditCommentMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();

        CommentUIRequestBus::Event(targetId, &CommentUIRequests::SetEditable, true);

        return SceneReaction::Nothing;
    }
}
