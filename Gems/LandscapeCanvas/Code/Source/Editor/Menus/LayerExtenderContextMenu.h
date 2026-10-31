/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace LandscapeCanvasEditor
{
    class LayerExtenderContextMenu
        : public JE::Editor::Tools::GraphCanvas::EditorContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LayerExtenderContextMenu, AZ::SystemAllocator);

        LayerExtenderContextMenu(const JE::Editor::Tools::GraphCanvas::NodePaletteConfig& nodePaletteConfig, QWidget* parent = nullptr);

    protected slots:
        void SetupDisplay() override;
    };
}
