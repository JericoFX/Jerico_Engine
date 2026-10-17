/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Configuration.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        class StreamingImageController;

        //! The streaming image controller descriptor is a configuration data block used to instantiate a
        //! concrete streaming image controller instance. Users should derive from this class, store
        //! configuration data on it for your specific back-end, and implement the factory method to create
        //! an instance at runtime.
        class JE_RENDER_INTEGRATION_API StreamingImageControllerAsset
            : public AZ::Data::AssetData
        {
        public:
            AZ_RTTI(StreamingImageControllerAsset, "{0797F48F-B097-4209-8D6F-DA40DC0FAB42}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(StreamingImageControllerAsset, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            virtual ~StreamingImageControllerAsset() = default;
        };
    }

namespace AZ
{
}
