/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace Render
    {
        //! This functor is used to pack and precalculate parameters required by subsurface scattering and transmission calculation
        class SubsurfaceTransmissionParameterFunctor final
            : public JE::Render::RPI::MaterialFunctor
        {
            friend class SubsurfaceTransmissionParameterFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(SubsurfaceTransmissionParameterFunctor, SystemAllocator)
            AZ_RTTI(SubsurfaceTransmissionParameterFunctor, "{1F95BF80-354E-4A65-9A9E-4C7276F8558F}", JE::Render::RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            using JE::Render::RPI::MaterialFunctor::Process;
            void Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            bool UpdateShaderParameterConnections(const JE::Render::RPI::MaterialShaderParameterLayout* layout) override;

        private:

            // Material property inputs...
            JE::Render::RPI::MaterialPropertyIndex m_mode;        
            JE::Render::RPI::MaterialPropertyIndex m_scale;
            JE::Render::RPI::MaterialPropertyIndex m_power;
            JE::Render::RPI::MaterialPropertyIndex m_distortion;
            JE::Render::RPI::MaterialPropertyIndex m_attenuation;
            JE::Render::RPI::MaterialPropertyIndex m_shrinkFactor;
            JE::Render::RPI::MaterialPropertyIndex m_transmissionNdLBias;
            JE::Render::RPI::MaterialPropertyIndex m_distanceAttenuation;
            JE::Render::RPI::MaterialPropertyIndex m_tintColor;
            JE::Render::RPI::MaterialPropertyIndex m_thickness;
            JE::Render::RPI::MaterialPropertyIndex m_enabled;
            JE::Render::RPI::MaterialPropertyIndex m_scatterDistanceColor;
            JE::Render::RPI::MaterialPropertyIndex m_scatterDistanceIntensity;

            // Shader setting output...
            JE::Render::RPI::MaterialShaderParameterNameIndex m_scatterDistance;
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transmissionParams;
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transmissionTintThickness;
        };
    } // namespace Render
} // namespace AZ
