/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/EBus/EBus.h>

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    class GraphController;

    //! Bus functions that allow the GraphModel Integration system to callback to the client system.
    class IntegrationBusInterface : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Return the Graph Canvas EntityId for whichever Graph Canvas scene is active in the Editor
        virtual JE::Editor::Tools::GraphCanvas::GraphId GetActiveGraphCanvasSceneId() const = 0;
        
        //! Notifies the client node graph system that the graph data has changed
        virtual void SignalSceneDirty(JE::Editor::Tools::GraphCanvas::GraphId graphCanvasSceneId) = 0;
    };

    using IntegrationBus = AZ::EBus<IntegrationBusInterface>;
}
