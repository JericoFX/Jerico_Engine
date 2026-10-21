/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Graph Model
#include <JE/Editor/Tools/GraphModel/Integration/ReadOnlyDataInterface.h>
#include <JE/Editor/Tools/GraphModel/Integration/IntegrationBus.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    ReadOnlyDataInterface::ReadOnlyDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    AZStd::string ReadOnlyDataInterface::GetString() const
    {
        JE::Editor::Tools::GraphModel::SlotPtr slot = m_slot.lock();
        return slot ? slot->GetValue<AZStd::string>() : AZStd::string();
    }
} // namespace JE::Editor::Tools::GraphModel::Integration
