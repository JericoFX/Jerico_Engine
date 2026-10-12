/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeCommon.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <JE/Geometry/EditorWhiteBoxComponentBus.h>
#include <JE/Geometry/WhiteBoxToolApi.h>

namespace JE::Geometry
{
    using ComponentModeRequestBus = AzToolsFramework::ComponentModeFramework::ComponentModeRequestBus;

    bool AllowPlanarGeometryEdit(const AZ::EntityComponentIdPair entityComponentIdPair)
    {
        bool manual = false;
        EditorWhiteBoxComponentRequestBus::EventResult(manual, entityComponentIdPair,
            &EditorWhiteBoxComponentRequests::HasManualUVs);
        if (manual)
        {
            AZ_Warning("WhiteBox", false, "Geometry edit rejected to preserve authored UVs. Use the White Box Inspector's Reset All UVs to Planar action before geometry edits that reproject UVs.");
        }
        return !manual;
    }
    void RecordWhiteBoxAction(
        WhiteBoxMesh& whiteBox, const AZ::EntityComponentIdPair entityComponentIdPair, const char* undoRedoDesc, const UVCommitMode uvMode)
    {
        // update UVs
        if (uvMode == UVCommitMode::Reproject) { Api::CalculatePlanarUVs(whiteBox); }

        // record an undo step
        AzToolsFramework::ScopedUndoBatch undoBatch(undoRedoDesc);
        AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityComponentIdPair.GetEntityId());

        // notify the component things have changed
        EditorWhiteBoxComponentRequestBus::Event(
            entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

        // notify the white box mesh things have changed
        EditorWhiteBoxComponentNotificationBus::Event(
            entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);

        // notify the component mode things have changed
        ComponentModeRequestBus::Event(entityComponentIdPair, &ComponentModeRequestBus::Events::Refresh);
    }

    bool InputFlipEdge(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return mouseInteraction.m_mouseInteraction.m_mouseButtons.Right() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down;
    }

    bool InputRestore(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down;
    }
} // namespace JE::Geometry
