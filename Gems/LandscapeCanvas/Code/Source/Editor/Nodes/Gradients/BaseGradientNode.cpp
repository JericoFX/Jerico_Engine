/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Integration/Helpers.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

// Landscape Canvas
#include "BaseGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void BaseGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseGradientNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BaseGradientNode>(
                    QT_TRANSLATE_NOOP("LandscapeCanvas", "BaseGradientNode"), "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(JE::Editor::Tools::GraphModel::Integration::Attributes::TitlePaletteOverride, "GradientNodeTitlePalette")
                    ;
            }
        }
    }

    BaseGradientNode::BaseGradientNode(JE::Editor::Tools::GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
    }

    void BaseGradientNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        JE::Editor::Tools::GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        JE::Editor::Tools::GraphModel::DataTypePtr gradientDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Gradient);

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            PREVIEW_BOUNDS_SLOT_ID,
            PREVIEW_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            PREVIEW_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            JE::Editor::Tools::GraphModel::DataTypeList{ boundsDataType },
            AZStd::any(AZ::EntityId())));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            OUTBOUND_GRADIENT_SLOT_ID,
            OUTBOUND_GRADIENT_SLOT_LABEL.toUtf8().constData(),
            OUTBOUND_GRADIENT_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            JE::Editor::Tools::GraphModel::DataTypeList{ gradientDataType }));
    }
} // namespace LandscapeCanvas
