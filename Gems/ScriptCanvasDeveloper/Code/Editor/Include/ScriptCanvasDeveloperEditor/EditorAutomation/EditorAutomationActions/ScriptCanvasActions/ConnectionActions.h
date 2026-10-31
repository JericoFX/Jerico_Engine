/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Editor/Tools/GraphCanvas/Types/Endpoint.h>
#include <JE/Editor/Tools/GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will couple the two specified nodes together. The ordering of the coupling is decided by the specified connection type from the perspective of the 'nodeToPickUp'
    */
    class CoupleNodesAction
        : public CompoundAction
        , public JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CoupleNodesAction, AZ::SystemAllocator);
        AZ_RTTI(CoupleNodesAction, "{DC6B3198-E127-4833-BC44-B27083A36045}", CompoundAction);

        CoupleNodesAction(JE::Editor::Tools::GraphCanvas::NodeId nodeToPickUp, JE::Editor::Tools::GraphCanvas::ConnectionType connectionType, JE::Editor::Tools::GraphCanvas::NodeId coupleTarget);

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        AZStd::vector< JE::Editor::Tools::GraphCanvas::ConnectionId > GetConnectionIds() const;

        // GraphCanvas::SceneNotificaionBus
        void OnConnectionAdded(const AZ::EntityId& connectionId) override;
        ////

    protected:

        void SetupAction() override;

        void OnActionsComplete() override;

    private:

        QRectF m_sceneRect;

        QRectF m_pickUpRect;
        QRectF m_targetRect;

        JE::Editor::Tools::GraphCanvas::NodeId m_nodeToPickUp;

        JE::Editor::Tools::GraphCanvas::ConnectionType m_connectionType;

        JE::Editor::Tools::GraphCanvas::NodeId m_targetNode;

        AZStd::vector< JE::Editor::Tools::GraphCanvas::ConnectionId > m_connections;
    };

    /**
        EditorAutomationAction that will attempt to create a connection between the two specified endpoints
    */
    class ConnectEndpointsAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ConnectEndpointsAction, AZ::SystemAllocator);
        AZ_RTTI(ConnectEndpointsAction, "{94941CCD-D8FA-4B01-8876-5D32723BD0C2}", CompoundAction);

        ConnectEndpointsAction(JE::Editor::Tools::GraphCanvas::Endpoint startEndpoint, JE::Editor::Tools::GraphCanvas::Endpoint targetEndpoint);

        JE::Editor::Tools::GraphCanvas::ConnectionId GetConnectionId() const;

    protected:

        void OnActionsComplete() override;

    private:

        QRectF m_sceneRect;

        JE::Editor::Tools::GraphCanvas::ConnectionId m_connectionId;

        JE::Editor::Tools::GraphCanvas::Endpoint m_startEndpoint;
        JE::Editor::Tools::GraphCanvas::Endpoint m_targetEndpoint;
    };
}
