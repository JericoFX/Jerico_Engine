/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzFramework/StringFunc/StringFunc.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Integration/IntegrationBus.h>
#include <JE/Editor/Tools/GraphModel/Integration/StringDataInterface.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    StringDataInterface::StringDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    AZStd::string StringDataInterface::GetString() const
    {
        JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
        return slot ? slot->GetValue<AZStd::string>() : AZStd::string();
    }

    void StringDataInterface::SetString(const AZStd::string& value)
    {
        if (JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock())
        {
            AZStd::string trimValue = value;
            AzFramework::StringFunc::TrimWhiteSpace(trimValue, true, true);

            if (trimValue != slot->GetValue<AZStd::string>())
            {
                const GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
                GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                slot->SetValue(trimValue);
                GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
                GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
            }
        }
    }
} // namespace JE::Editor::Tools::GraphModel::Integration
