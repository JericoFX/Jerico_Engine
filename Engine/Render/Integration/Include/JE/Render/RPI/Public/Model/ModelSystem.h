/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        //! Manages system-wide initialization and support for Model classes
        class JE_RENDER_INTEGRATION_API ModelSystem
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init();
            void Shutdown();
        };
    }

namespace AZ
{ // namespace RPI
} // namespace AZ
