/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Integration/IntegrationBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    //! Satisfies GraphCanvas API requirements for showing int property widgets in nodes.
    template<typename T>
    class IntegerDataInterface : public JE::Editor::Tools::GraphCanvas::NumericDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(IntegerDataInterface, AZ::SystemAllocator);

        IntegerDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot)
            : m_slot(slot)
        {
        }

        ~IntegerDataInterface() = default;

        double GetNumber() const
        {
            JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
            return static_cast<double>(slot ? slot->GetValue<T>() : 0.0);
        }

        void SetNumber(double value)
        {
            JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
            if (slot && slot->GetValue<T>() != static_cast<T>(value))
            {
                const JE::Editor::Tools::GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
                JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                slot->SetValue(static_cast<T>(value));
                GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
                GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
            }
        }

        int GetDecimalPlaces() const
        {
            return 0;
        }

        double GetMin() const
        {
            return static_cast<double>(AZStd::numeric_limits<T>::min());
        }

        double GetMax() const
        {
            return static_cast<double>(AZStd::numeric_limits<T>::max());
        }

    private:
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Slot> m_slot;
    };
} // namespace JE::Editor::Tools::GraphModel::Integration
