/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Components/NodePropertyDisplay/ReadOnlyDataInterface.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    //! Satisfies GraphCanvas API requirements for showing read only property widgets in nodes.
    class ReadOnlyDataInterface : public JE::Editor::Tools::GraphCanvas::ReadOnlyDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(ReadOnlyDataInterface, AZ::SystemAllocator);

        ReadOnlyDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot);
        ~ReadOnlyDataInterface() = default;

        // GraphCanvas::ReadOnlyDataInterface overrides ...
        AZStd::string GetString() const override;

    private:
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Slot> m_slot;
    };
} // namespace JE::Editor::Tools::GraphModel::Integration
