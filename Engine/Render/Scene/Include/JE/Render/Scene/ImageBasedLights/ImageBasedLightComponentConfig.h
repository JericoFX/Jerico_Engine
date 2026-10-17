/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace Scene
    {
        //! Common settings for ImageBasedLightComponents and EditorImageBasedLightComponent.
        class ImageBasedLightComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(ImageBasedLightComponentConfig, "{2BD353A5-562B-4D84-9508-B2EFAFF1415E}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(ImageBasedLightComponentConfig, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> m_specularImageAsset;
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> m_diffuseImageAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
