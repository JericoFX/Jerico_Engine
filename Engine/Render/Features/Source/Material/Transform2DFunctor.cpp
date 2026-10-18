/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./Transform2DFunctor.h"
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace Features
    {
        void Transform2DFunctor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Enum<AZ::Render::TransformType>()
                    ->Value("Scale", AZ::Render::TransformType::Scale)
                    ->Value("Rotate", AZ::Render::TransformType::Rotate)
                    ->Value("Translate", AZ::Render::TransformType::Translate)
                    ;

                serializeContext->Class<Transform2DFunctor, JE::Render::RPI::MaterialFunctor>()
                    ->Version(3)
                    ->Field("transformOrder", &Transform2DFunctor::m_transformOrder)
                    ->Field("center", &Transform2DFunctor::m_center)
                    ->Field("scale", &Transform2DFunctor::m_scale)
                    ->Field("scaleX", &Transform2DFunctor::m_scaleX)
                    ->Field("scaleY", &Transform2DFunctor::m_scaleY)
                    ->Field("translateX", &Transform2DFunctor::m_translateX)
                    ->Field("translateY", &Transform2DFunctor::m_translateY)
                    ->Field("rotateDegrees", &Transform2DFunctor::m_rotateDegrees)
                    ->Field("transformMatrixRow0", &Transform2DFunctor::m_transformMatrixRow0)
                    ->Field("transformMatrixRow1", &Transform2DFunctor::m_transformMatrixRow1)
                    ->Field("transformMatrixRow2", &Transform2DFunctor::m_transformMatrixRow2)
                    ->Field("transformMatrixInverseRow0", &Transform2DFunctor::m_transformMatrixInverseRow0)
                    ->Field("transformMatrixInverseRow1", &Transform2DFunctor::m_transformMatrixInverseRow1)
                    ->Field("transformMatrixInverseRow2", &Transform2DFunctor::m_transformMatrixInverseRow2)
                    ;
            }
        }
        
        void Transform2DFunctor::Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            using namespace JE::Render::RPI;

            AZ::Render::UvTransformDescriptor desc;
            desc.m_center = context.GetMaterialPropertyValue<AZ::Vector2>(m_center);
            desc.m_scale = context.GetMaterialPropertyValue<float>(m_scale);
            desc.m_scaleX = context.GetMaterialPropertyValue<float>(m_scaleX);
            desc.m_scaleY = context.GetMaterialPropertyValue<float>(m_scaleY);
            desc.m_translateX = context.GetMaterialPropertyValue<float>(m_translateX);
            desc.m_translateY = context.GetMaterialPropertyValue<float>(m_translateY);
            desc.m_rotateDegrees = context.GetMaterialPropertyValue<float>(m_rotateDegrees);

            AZ::Matrix3x3 transform = CreateUvTransformMatrix(desc, m_transformOrder);

            // Store each matrix as 3 separate row_major float4s (w=0) to avoid the Vulkan structured buffer bug with float3x3.
            auto SetMatrixRows = [&context](
                MaterialShaderParameterNameIndex& row0,
                MaterialShaderParameterNameIndex& row1,
                MaterialShaderParameterNameIndex& row2,
                const AZ::Matrix3x3& mat)
            {
                context.GetMaterialShaderParameter()->SetParameter(row0, AZ::Vector4(mat.GetRow(0), 0.0f));
                context.GetMaterialShaderParameter()->SetParameter(row1, AZ::Vector4(mat.GetRow(1), 0.0f));
                context.GetMaterialShaderParameter()->SetParameter(row2, AZ::Vector4(mat.GetRow(2), 0.0f));
            };

            SetMatrixRows(m_transformMatrixRow0, m_transformMatrixRow1, m_transformMatrixRow2, transform);
            if (m_transformMatrixInverseRow0.GetIndex().IsValid())
            {
                SetMatrixRows(m_transformMatrixInverseRow0, m_transformMatrixInverseRow1, m_transformMatrixInverseRow2, transform.GetInverseFull());
            }
        }

        bool Transform2DFunctor::UpdateShaderParameterConnections(const JE::Render::RPI::MaterialShaderParameterLayout* layout)
        {
            bool valid = true;
            auto ValidateRow = [&](JE::Render::RPI::MaterialShaderParameterNameIndex& idx) -> bool
            {
                if (idx.ValidateOrFindIndex(layout) == false)
                {
                    AZ_Error("Transform2DFunctorSourceData", false, "Could not find shader parameter '%s'", idx.GetName().GetCStr());
                    valid = false;
                    return false;
                }
                return true;
            };

            ValidateRow(m_transformMatrixRow0);
            ValidateRow(m_transformMatrixRow1);
            ValidateRow(m_transformMatrixRow2);

            // There are some cases where the matrix is required but the inverse is not.
            if (!m_transformMatrixInverseRow0.GetName().IsEmpty())
            {
                ValidateRow(m_transformMatrixInverseRow0);
                ValidateRow(m_transformMatrixInverseRow1);
                ValidateRow(m_transformMatrixInverseRow2);
            }
            return valid;
        }
    } // namespace Features
} // namespace JE::Render
