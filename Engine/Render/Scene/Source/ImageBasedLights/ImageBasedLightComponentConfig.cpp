/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void ImageBasedLightComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImageBasedLightComponentConfig, AZ::ComponentConfig>()
                    ->Version(1)
                    ->Field("diffuseImageAsset", &ImageBasedLightComponentConfig::m_diffuseImageAsset)
                    ->Field("specularImageAsset", &ImageBasedLightComponentConfig::m_specularImageAsset)
                    ->Field("exposure", &ImageBasedLightComponentConfig::m_exposure)
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
