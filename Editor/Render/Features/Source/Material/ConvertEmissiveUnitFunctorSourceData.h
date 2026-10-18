/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Material/ConvertEmissiveUnitFunctor.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceData.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Math/Vector2.h>

namespace JE::Editor::Render
{
    namespace Features
    {
        class ConvertEmissiveUnitFunctorSourceData;

        //! Builds a ConvertEmissiveUnitFunctor
        //! Please add it into the shaderInputFunctors list in material file
        class ConvertEmissiveUnitFunctorSourceData final
            : public JE::Render::RPI::MaterialFunctorSourceData
        {
        public:
            AZ_CLASS_ALLOCATOR(ConvertEmissiveUnitFunctorSourceData, AZ::SystemAllocator)
            AZ_RTTI(ConvertEmissiveUnitFunctorSourceData, "{B476A346-C5E0-4DB9-BCFD-B2AFA8587D24}", JE::Render::RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;
            FunctorResult CreateFunctor(const EditorContext& context) const override;

        private:
            // Name of the intensity property in material
            AZStd::string m_intensityPropertyName;

            // Name of the light unit property in material
            AZStd::string m_lightUnitPropertyName;

            // Name of the srg constant input in shader
            AZStd::string m_shaderInputName;

            // The indices of photometric units in the dropdown list
            uint32_t m_ev100Index = 0;
            uint32_t m_nitIndex = 1;

            // Minimum and Maximum value for different photometric units
            AZ::Vector2 m_ev100MinMax;
            AZ::Vector2 m_nitMinMax;
        };
    }
}
