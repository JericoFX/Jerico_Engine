/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>

#include "EBusSenderNodeDescriptorComponent.h"

#include "Editor/Translation/TranslationHelper.h"
#include "ScriptCanvas/Libraries/Core/Method.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // EBusSenderNodeDescriptorComponent
    //////////////////////////////////////
    
    void EBusSenderNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<EBusSenderNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(2)
            ;
        }
    }
    
    EBusSenderNodeDescriptorComponent::EBusSenderNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusSender)
    {
    }
    
    void EBusSenderNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& sceneId, const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasNodeId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::Method* method = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::Method>(entity);

            if (method)
            {
                if (method->HasBusID())
                {
                    ScriptCanvas::SlotId slotId = method->GetBusSlotId();

                    AZStd::vector< AZ::EntityId > graphCanvasSlots;
                    JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlots, GetEntityId(), &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);

                    for (const AZ::EntityId& graphCanvasId : graphCanvasSlots)
                    {
                        AZStd::any* slotData = nullptr;
                        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotData, graphCanvasId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetUserData);

                        if (slotData
                            && slotData->is< ScriptCanvas::SlotId >())
                        {
                            ScriptCanvas::SlotId currentSlotId = (*AZStd::any_cast<ScriptCanvas::SlotId>(slotData));

                            if (currentSlotId == slotId)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }        
    }
}
