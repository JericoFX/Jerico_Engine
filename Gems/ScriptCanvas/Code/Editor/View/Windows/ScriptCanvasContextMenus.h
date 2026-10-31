/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector2.h>

#include <QMenu>
#include <QWidgetAction>

#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuAction.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuAction.h>
#include <ScriptCanvas/Core/Slot.h>

namespace AzToolsFramework::AssetBrowser
{
    class AssetBrowserFilterModel;
}

namespace ScriptCanvasEditor
{
    class NodePaletteModel;

    //////////////////
    // CustomActions
    //////////////////

    class EndpointSelectionAction
        : public QAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EndpointSelectionAction, AZ::SystemAllocator);

        EndpointSelectionAction(const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint);
        ~EndpointSelectionAction() = default;

        const JE::Editor::Tools::GraphCanvas::Endpoint& GetEndpoint() const;

    private:
        JE::Editor::Tools::GraphCanvas::Endpoint m_endpoint;
    };

    class RemoveUnusedVariablesMenuAction
        : public JE::Editor::Tools::GraphCanvas::SceneContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoveUnusedVariablesMenuAction, AZ::SystemAllocator);

        RemoveUnusedVariablesMenuAction(QObject* parent);
        virtual ~RemoveUnusedVariablesMenuAction() = default;

        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using JE::Editor::Tools::GraphCanvas::SceneContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::SceneContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;
    };

    class ConvertVariableNodeToReferenceAction
        : public JE::Editor::Tools::GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConvertVariableNodeToReferenceAction, AZ::SystemAllocator);

        ConvertVariableNodeToReferenceAction(QObject* parent);
        virtual ~ConvertVariableNodeToReferenceAction() = default;

        JE::Editor::Tools::GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using JE::Editor::Tools::GraphCanvas::ContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::ContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        AZ::EntityId m_targetId;
    };

    class SlotManipulationMenuAction
        : public JE::Editor::Tools::GraphCanvas::ContextMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SlotManipulationMenuAction, AZ::SystemAllocator);

        SlotManipulationMenuAction(AZStd::string_view actionName, QObject* parent);

        static ScriptCanvas::Slot* GetScriptCanvasSlot(const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint) ;
    
    };

    class ConvertReferenceToVariableNodeAction
        : public SlotManipulationMenuAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConvertReferenceToVariableNodeAction, AZ::SystemAllocator);

        ConvertReferenceToVariableNodeAction(QObject* parent);
        virtual ~ConvertReferenceToVariableNodeAction() = default;

        JE::Editor::Tools::GraphCanvas::ActionGroupId GetActionGroupId() const override;

        using SlotManipulationMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using SlotManipulationMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        AZ::EntityId m_targetId;
    };

    class ExposeSlotMenuAction
        : public JE::Editor::Tools::GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ExposeSlotMenuAction, AZ::SystemAllocator);

        ExposeSlotMenuAction(QObject* parent);
        virtual ~ExposeSlotMenuAction() = default;

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    protected:

        void CreateNodeling(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, JE::Editor::Tools::GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, JE::Editor::Tools::GraphCanvas::ConnectionType connectionType);
    };

    class SetDataSlotTypeMenuAction
        : public JE::Editor::Tools::GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SetDataSlotTypeMenuAction, AZ::SystemAllocator);

        explicit SetDataSlotTypeMenuAction(QObject* parent);
        virtual ~SetDataSlotTypeMenuAction() = default;
        static bool IsSupportedSlotType(const AZ::EntityId& slotId);

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        ScriptCanvas::Slot* GetSlot(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId);

    };

    //! Context Menu Action for Creating an AzEventHandler node from a data slot of a Behavior Method node
    //! which returns an AZ::Event<Params...> type
    class CreateAzEventHandlerSlotMenuAction
        : public JE::Editor::Tools::GraphCanvas::SlotContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateAzEventHandlerSlotMenuAction, AZ::SystemAllocator);

        CreateAzEventHandlerSlotMenuAction(QObject* parent);

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::SlotContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

        static const AZ::BehaviorMethod* FindBehaviorMethodWithAzEventReturn(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, AZ::EntityId targetId);

    protected:

        const AZ::BehaviorMethod* m_methodWithAzEventReturn{};
        JE::Editor::Tools::GraphCanvas::Endpoint m_methodNodeAzEventEndpoint;

    };

    /////////////////
    // ContextMenus
    /////////////////

    class SceneContextMenu
        : public JE::Editor::Tools::GraphCanvas::SceneContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SceneContextMenu, AZ::SystemAllocator);

        SceneContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        ~SceneContextMenu() = default;

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId);

        // EditConstructContextMenu
        void OnRefreshActions(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId) override;
        ////

        void SetupDisplayForProposal();

    protected:

        AZ::EntityId                      m_sourceSlotId;
    };

    class ConnectionContextMenu
        : public JE::Editor::Tools::GraphCanvas::ConnectionContextMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConnectionContextMenu, AZ::SystemAllocator);

        ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel);
        ~ConnectionContextMenu() = default;

    protected:

        void OnRefreshActions(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId);

    private:

        AZ::EntityId                      m_connectionId;
    };

    //////////////////////////
    // RenameFunctionDefinitionNode
    //////////////////////////

    class NodeDescriptorComponent;

    //! Context menu to rename a node
    class RenameFunctionDefinitionNodeAction
        : public JE::Editor::Tools::GraphCanvas::NodeContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(RenameFunctionDefinitionNodeAction, AZ::SystemAllocator);

        RenameFunctionDefinitionNodeAction(NodeDescriptorComponent* descriptor, QObject* parent);
        virtual ~RenameFunctionDefinitionNodeAction() = default;

        using JE::Editor::Tools::GraphCanvas::NodeContextMenuAction::RefreshAction;
        void RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId) override;

        using JE::Editor::Tools::GraphCanvas::NodeContextMenuAction::TriggerAction;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos) override;

        NodeDescriptorComponent* m_descriptor;
    };

}
