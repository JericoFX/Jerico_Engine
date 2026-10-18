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
#include <JE/Render/Features/RenderFeaturesExports.h>
#include <JE/Render/Features/CoreLights/PhotometricValue.h>

namespace JE::Editor::Render::Features
{
    class ConvertEmissiveUnitFunctorSourceData;
}

namespace JE::Render
{
    namespace Features
    {
        //! The functor can be used to convert between different emissive light unit
        //! Only support Ev100 and Lux
        class JE_RENDER_FEATURES_API ConvertEmissiveUnitFunctor final
            : public JE::Render::RPI::MaterialFunctor
        {
            friend class JE::Editor::Render::Features::ConvertEmissiveUnitFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(ConvertEmissiveUnitFunctor, AZ::SystemAllocator)
            AZ_RTTI(ConvertEmissiveUnitFunctor, "{F272CFAB-FD71-4E78-AA47-D0D2E88CE30C}", JE::Render::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context);

            using JE::Render::RPI::MaterialFunctor::Process;
            void Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            void Process(JE::Render::RPI::MaterialFunctorAPI::EditorContext& context) override;
            float GetProcessedValue(float originalEmissiveIntensity, uint32_t lightUnitIndex) const;

            bool UpdateShaderParameterConnections(const JE::Render::RPI::MaterialShaderParameterLayout* layout) override;

        private:

            JE::Render::RPI::MaterialPropertyIndex m_intensityPropertyIndex;
            JE::Render::RPI::MaterialPropertyIndex m_lightUnitPropertyIndex;
            JE::Render::RPI::MaterialShaderParameterNameIndex m_shaderParameter;

            uint32_t m_ev100Index;
            uint32_t m_nitIndex;

            float m_ev100Min = 0.0f;
            float m_ev100Max = 0.0f;
            float m_nitMin = 0.0f;
            float m_nitMax = 0.0f;
        };
    }
}
