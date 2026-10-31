/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/BookmarkContextMenu.h>

namespace JE::Editor::Tools::GraphCanvas
{
    ////////////////////////
    // BookmarkContextMenu
    ////////////////////////

    BookmarkContextMenu::BookmarkContextMenu(EditorId editorId, QWidget* parent)
        : JE::Editor::Tools::GraphCanvas::EditorContextMenu(editorId, parent)
    {
        m_editActionMenuGroup.PopulateMenu(this);
    }

    void BookmarkContextMenu::OnRefreshActions(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);

        m_editActionMenuGroup.SetPasteEnabled(false);
    }
}
