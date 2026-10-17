/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <GradientSignal/GradientSampler.h>

namespace JE::Render
{
    namespace Scene
    {
        class GradientWeightModifierComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(GradientWeightModifierComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::GradientWeightModifierComponentConfig, "AZ::Render::GradientWeightModifierComponentConfig", "{03CF0D7B-6763-4196-B329-7E247EBEDEF8}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            GradientSignal::GradientSampler m_gradientSampler;
        };
    }
}
