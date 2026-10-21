/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Graph Canvas
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    //! Satisfies GraphCanvas API requirements for showing float property widgets in nodes.
    class FloatDataInterface : public GraphCanvas::NumericDataInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(FloatDataInterface, AZ::SystemAllocator);

        FloatDataInterface(JE::Editor::Tools::GraphModel::SlotPtr slot);
        ~FloatDataInterface() = default;

        double GetNumber() const override;
        void SetNumber(double value) override;

        double GetMin() const override;
        double GetMax() const override;

    private:
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Slot> m_slot;
    };
} // namespace JE::Editor::Tools::GraphModel::Integration
