/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>
#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/Utils/MaterialUtils.h>

namespace JE::Editor::Render::Features
{
    class Transform2DFunctorSourceData;
}

namespace JE::Render
{
    namespace Features
    {
        //! Materials can use this functor to map 2D scale, translate, and rotate properties into a float3x3 transform matrix.
        class JE_RENDER_FEATURES_API Transform2DFunctor final
            : public JE::Render::RPI::MaterialFunctor
        {
            friend class JE::Editor::Render::Features::Transform2DFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(Transform2DFunctor, AZ::SystemAllocator)
            AZ_RTTI(Transform2DFunctor, "{3E9C4357-6B2D-4A22-89DB-462441C9D8CD}", JE::Render::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context);

            using JE::Render::RPI::MaterialFunctor::Process;
            void Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            bool UpdateShaderParameterConnections(const JE::Render::RPI::MaterialShaderParameterLayout* layout) override;

        private:
            AZStd::vector<AZ::Render::TransformType> m_transformOrder; //!< Controls the order in which Scale, Translate, Rotate are performed

            // Material property inputs...
            JE::Render::RPI::MaterialPropertyIndex m_center;        //!< index of material property for the center of scaling and rotation
            JE::Render::RPI::MaterialPropertyIndex m_scale;         //!< index of material property for scaling in both directions
            JE::Render::RPI::MaterialPropertyIndex m_scaleX;        //!< index of material property for X scale
            JE::Render::RPI::MaterialPropertyIndex m_scaleY;        //!< index of material property for Y scale
            JE::Render::RPI::MaterialPropertyIndex m_translateX;    //!< index of material property for X translation
            JE::Render::RPI::MaterialPropertyIndex m_translateY;    //!< index of material property for Y translation
            JE::Render::RPI::MaterialPropertyIndex m_rotateDegrees; //!< index of material property for rotating

            // Shader setting output...
            // Each matrix is stored as 3 separate row_major float4 rows to work around a Vulkan structured buffer bug with float3x3.
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixRow0; //!< Row 0 of the UV transform matrix (float4, w unused)
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixRow1; //!< Row 1 of the UV transform matrix (float4, w unused)
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixRow2; //!< Row 2 of the UV transform matrix (float4, w unused)
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow0; //!< Row 0 of the inverse UV transform matrix (float4, w unused)
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow1; //!< Row 1 of the inverse UV transform matrix (float4, w unused)
            JE::Render::RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow2; //!< Row 2 of the inverse UV transform matrix (float4, w unused)
        };

    } // namespace Features

} // namespace JE::Render
