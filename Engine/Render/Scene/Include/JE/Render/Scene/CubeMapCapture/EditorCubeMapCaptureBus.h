/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        // Provides an interface to the Editor CubeMapCapture component
        class EditorCubeMapCaptureInterface
            : public AZ::ComponentBus
        {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
                
                virtual ~EditorCubeMapCaptureInterface() = default;
                
                // Signals the component to capture the cubemap
                virtual AZ::u32 CaptureCubeMap() = 0;
        };
        
        using EditorCubeMapCaptureBus = AZ::EBus<EditorCubeMapCaptureInterface>;
    }
}
