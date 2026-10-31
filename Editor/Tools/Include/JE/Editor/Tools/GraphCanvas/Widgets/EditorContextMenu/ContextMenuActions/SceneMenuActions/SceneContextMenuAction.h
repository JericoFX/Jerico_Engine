/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class SceneContextMenuAction
        : public ContextMenuAction
    {
    protected:
        SceneContextMenuAction(AZStd::string_view actionName, QObject* parent)
            : ContextMenuAction(actionName, parent)
        {
        }
        
    public:
    
        static ActionGroupId GetSceneContextMenuActionGroupId()
        {
            return AZ_CRC_CE("SceneActionGroup");
        }
    
        ActionGroupId GetActionGroupId() const override
        {
            return GetSceneContextMenuActionGroupId();
        }
    };
}
