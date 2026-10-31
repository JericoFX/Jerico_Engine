/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Integration/IntegrationBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    template<class Type, int ElementCount>
    class VectorDataInterface : public JE::Editor::Tools::GraphCanvas::VectorDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(VectorDataInterface, AZ::SystemAllocator);

        VectorDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot)
            : m_slot(slot)
        {
        }

        ~VectorDataInterface() = default;

        const char* GetLabel(int index) const override
        {
            switch (index)
            {
            case 0:
                return "X";
            case 1:
                return "Y";
            case 2:
                return "Z";
            case 3:
                return "W";
            }
            return "???";
        }

        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }

        AZStd::string GetElementStyle(int index) const override
        {
            return AZStd::string::format("vector_%i", index);
        }

        int GetElementCount() const override
        {
            return ElementCount;
        }

        double GetValue(int index) const override
        {
            JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
            return slot && index < ElementCount ? slot->GetValue<Type>().GetElement(index) : 0.0;
        }

        void SetValue(int index, double value) override
        {
            JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
            if (slot && index < ElementCount)
            {
                Type vector = slot->GetValue<Type>();
                if (vector.GetElement(index) != value)
                {
                    const JE::Editor::Tools::GraphCanvas::GraphId graphCanvasSceneId = GetDisplay()->GetSceneId();
                    JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                    vector.SetElement(index, aznumeric_cast<float>(value));
                    slot->SetValue(vector);

                    GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelSlotModified, slot);
                    GraphControllerNotificationBus::Event(graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, slot->GetParentNode());
                }
            }
        }

    private:
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Slot> m_slot;
    };
}
