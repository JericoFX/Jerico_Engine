/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/GraphCanvasConstructActionsMenuGroup.h>

#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/BookmarkConstructMenuActions.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructContextMenuAction.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

#include <JE/Editor/Tools/GraphCanvas/Types/ConstructPresets.h>

namespace JE::Editor::Tools::GraphCanvas
{
    /////////////////////////////////////////
    // GraphCanvasConstructActionsMenuGroup
    /////////////////////////////////////////

    GraphCanvasConstructActionsMenuGroup::GraphCanvasConstructActionsMenuGroup()
        : m_createBookmark(nullptr)
    {
    }

    GraphCanvasConstructActionsMenuGroup::~GraphCanvasConstructActionsMenuGroup()
    {
    }

    void GraphCanvasConstructActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(ConstructContextMenuAction::GetConstructContextMenuActionGroupId());

        m_createBookmark = aznew AddBookmarkMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_createBookmark);

        m_commentPresets.PopulateMenu(contextMenu);
    }

    void GraphCanvasConstructActionsMenuGroup::RefreshGroup()
    {
        m_commentPresets.RefreshPresets();
    }

    void GraphCanvasConstructActionsMenuGroup::SetAddBookmarkEnabled(bool enabled)
    {
        m_createBookmark->setEnabled(enabled);
    }

    void GraphCanvasConstructActionsMenuGroup::SetCommentsEnabled(bool enabled)
    {
        m_commentPresets.SetEnabled(enabled);
    }
}
