/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void GradientWeightModifierComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GradientWeightModifierComponentConfig, AZ::ComponentConfig>()
                    ->Version(0)
                    ->Field("GradientSampler", &GradientWeightModifierComponentConfig::m_gradientSampler)
                    ;
            }
        }
    }
}


