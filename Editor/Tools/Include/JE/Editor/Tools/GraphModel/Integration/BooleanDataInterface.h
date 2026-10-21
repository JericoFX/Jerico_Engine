/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/BooleanDataInterface.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    //! Satisfies GraphCanvas API requirements for showing bool property widgets in nodes.
    class BooleanDataInterface : public GraphCanvas::BooleanDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(BooleanDataInterface, AZ::SystemAllocator);

        BooleanDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot);
        ~BooleanDataInterface() = default;

        bool GetBool() const override;
        void SetBool(bool enabled) override;

    private:
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Slot> m_slot;
    };
} // namespace JE::Editor::Tools::GraphModel::Integration
