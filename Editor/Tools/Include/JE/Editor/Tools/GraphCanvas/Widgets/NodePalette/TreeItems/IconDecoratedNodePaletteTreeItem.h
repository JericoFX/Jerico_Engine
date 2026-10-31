/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <JE/Editor/Tools/GraphCanvas/Components/StyleBus.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class IconDecoratedNodePaletteTreeItem
        : public NodePaletteTreeItem
        , public StyleManagerNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(IconDecoratedNodePaletteTreeItem, AZ::SystemAllocator);
        AZ_RTTI(IconDecoratedNodePaletteTreeItem, "{674FE7BB-C15C-4532-B580-336C7C6173A3}", NodePaletteTreeItem);

        IconDecoratedNodePaletteTreeItem(AZStd::string_view name, EditorId editorId);
        ~IconDecoratedNodePaletteTreeItem() override = default;

        void AddIconColorPalette(const AZStd::string& colorPalette);
        
        void OnStylesUnloaded() override;
        void OnStylesLoaded() override;
            
    protected:

        QVariant OnData(const QModelIndex& index, int role) const override;

        void OnTitlePaletteChanged() override;

    private:
        PaletteIconConfiguration m_paletteConfiguration;
        const QPixmap* m_iconPixmap = nullptr;
    };
}
