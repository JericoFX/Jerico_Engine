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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Integration/Helpers.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

// Landscape Canvas
#include "BaseAreaNode.h"
#include <Editor/Core/GraphContext.h>

#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>

namespace LandscapeCanvas
{
    void BaseAreaNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BaseAreaNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BaseAreaNode>(
                    QT_TRANSLATE_NOOP("LandscapeCanvas", "BaseAreaNode"), "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(JE::Editor::Tools::GraphModel::Integration::Attributes::TitlePaletteOverride, "VegetationAreaNodeTitlePalette")
                    ;
            }
        }
    }

    BaseAreaNode::BaseAreaNode(JE::Editor::Tools::GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
    }

    AZ::Component* BaseAreaNode::GetReferenceShapeComponent() const
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, GetVegetationEntityId());
        if (!entity)
        {
            return nullptr;
        }

        AZ::Component* component = entity->FindComponent(LmbrCentral::EditorReferenceShapeComponentTypeId);
        if (component)
        {
            return component;
        }

        return nullptr;
    }

    void BaseAreaNode::RegisterSlots()
    {
        CreateEntityNameSlot();

        JE::Editor::Tools::GraphModel::DataTypePtr boundsDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Bounds);
        JE::Editor::Tools::GraphModel::DataTypePtr areaDataType = GetGraphContext()->GetDataType(LandscapeCanvasDataTypeEnum::Area);

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            PLACEMENT_BOUNDS_SLOT_ID,
            PLACEMENT_BOUNDS_SLOT_LABEL.toUtf8().constData(),
            PLACEMENT_BOUNDS_INPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            JE::Editor::Tools::GraphModel::DataTypeList{ boundsDataType },
            AZStd::any(AZ::EntityId())));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            OUTBOUND_AREA_SLOT_ID,
            OUTBOUND_AREA_SLOT_LABEL.toUtf8().constData(),
            OUTBOUND_AREA_OUTPUT_SLOT_DESCRIPTION.toUtf8().constData(),
            JE::Editor::Tools::GraphModel::DataTypeList{ areaDataType }));
    }
} // namespace LandscapeCanvas
