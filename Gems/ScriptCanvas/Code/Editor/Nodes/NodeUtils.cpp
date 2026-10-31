/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeUtils.h"

#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>

#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Core/NodeBus.h>

namespace ScriptCanvasEditor::Nodes
{
    void UpdateSlotDatumLabel(const AZ::EntityId& graphCanvasNodeId, ScriptCanvas::SlotId scSlotId, const AZStd::string& name)
    {
        AZStd::any* userData = nullptr;
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scNodeEntityId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (scNodeEntityId.IsValid())
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ScriptCanvas::NodeRequestBus::Event(scNodeEntityId, &ScriptCanvas::NodeRequests::FindModifiableDatumView, scSlotId, datumView);

            datumView.RelabelDatum(name);
        }
    }

    void UpdateSlotDatumLabels(AZ::EntityId graphCanvasNodeId)
    {
        AZStd::vector<AZ::EntityId> graphCanvasSlotIds;
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, graphCanvasNodeId, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);
        for (AZ::EntityId graphCanvasSlotId : graphCanvasSlotIds)
        {
            AZStd::any* slotUserData{};
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotUserData, graphCanvasSlotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetUserData);

            if (auto scriptCanvasSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData))
            {
                AZStd::string slotName;
                JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotName, graphCanvasSlotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetName);
                UpdateSlotDatumLabel(graphCanvasNodeId, *scriptCanvasSlotId, slotName);
            }
        }
    }
}
