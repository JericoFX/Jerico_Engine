/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        // Provides an interface to the Editor Reflection Probe component
        class EditorReflectionProbeInterface
            : public AZ::ComponentBus
        {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
                
                virtual ~EditorReflectionProbeInterface() = default;
                
                // Signals the component to bake the reflection probe
                virtual AZ::u32 BakeReflectionProbe() = 0;
        };
        
        using EditorReflectionProbeBus = AZ::EBus<EditorReflectionProbeInterface>;
    }
}
