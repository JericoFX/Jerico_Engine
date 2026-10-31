/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasTreeItem.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class CategorizerInterface
    {
    public:
        virtual JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem* CreateCategoryNode(AZStd::string_view categoryPath, AZStd::string_view categoryName, GraphCanvasTreeItem* parent) const = 0;
    };

    // Gives some utils for creating pathed categories inside of the tree.
    // i.e. "Order/Family/Genus/Species"
    // Order, Family, Genus are all categories as they all might have more then one element. Where as Species would be a leaf
    // node as there is a single species.
    //
    // This class just provides an easy interface to pass in the full category path and have it manage all of the elements.
    // Only caveat is that this must be used when removing elements so it can maintain the internal state.
    class GraphCanvasTreeCategorizer
    {
    private:
        typedef AZStd::pair<JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem*, AZStd::string> CategoryKey;
        typedef AZStd::unordered_map<CategoryKey, JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem*> CategoryRootsMap;
    public:
        GraphCanvasTreeCategorizer(const CategorizerInterface& nodePaletteModel);

        void RegisterCategoryNode(JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem* treeItem, const char* subCategory, JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem* parentRoot);
        JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem* GetCategoryNode(const char* categoryPath, JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem* parentRoot);

        void PruneEmptyNodes();
        void PruneNode(GraphCanvasTreeItem* treeItem);

        void EnableDebug(bool enabled)
        {
            m_debugEnabled = enabled;
        }

    private:

        void PruneNodes(AZStd::unordered_set< JE::Editor::Tools::GraphCanvas::GraphCanvasTreeItem*> recursiveSet);

        const CategorizerInterface& m_categorizerInterface;
        CategoryRootsMap m_rootMaps;

        bool m_debugEnabled;
    };    
}
