/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <JE/Render/RPI/Public/Shader/Shader.h>

namespace JE::Render
{
    namespace Features
    {
        void CachedSkinnedMeshShaderOptions::SetShader(AZ::Data::Instance<JE::Render::RPI::Shader> shader)
        {
            m_shader = shader;

            // Cache the different possible indices and values so they are efficent to set later
            const JE::Render::RPI::ShaderOptionGroupLayout* layout = m_shader->GetAsset()->GetShaderOptionGroupLayout();
            m_skinningMethodOptionIndex = layout->FindShaderOptionIndex(AZ::Name("o_skinningMethod"));
            m_skinningMethodLinearSkinningValue = layout->FindValue(m_skinningMethodOptionIndex, AZ::Name("SkinningMethod::LinearSkinning"));
            m_skinningMethodDualQuaternionValue = layout->FindValue(m_skinningMethodOptionIndex, AZ::Name("SkinningMethod::DualQuaternion"));
            m_skinningMethodNoSkinningValue = layout->FindValue(m_skinningMethodOptionIndex, AZ::Name("SkinningMethod::NoSkinning"));

            m_applyMorphTargetOptionIndex = layout->FindShaderOptionIndex(AZ::Name("o_applyMorphTargets"));
            m_applyMorphTargetFalseValue = layout->FindValue(m_applyMorphTargetOptionIndex, AZ::Name("false"));
            m_applyMorphTargetTrueValue = layout->FindValue(m_applyMorphTargetOptionIndex, AZ::Name("true"));

            SkinnedMeshShaderOptionNotificationBus::Event(this, &SkinnedMeshShaderOptionNotificationBus::Events::OnShaderReinitialized, this);
        }

        void CachedSkinnedMeshShaderOptions::ConnectToShaderReinitializedEvent(SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedEventHandler)
        {
            shaderReinitializedEventHandler.BusConnect(this);
        }

        JE::Render::RPI::ShaderOptionGroup CachedSkinnedMeshShaderOptions::CreateShaderOptionGroup(const SkinnedMeshShaderOptions& shaderOptions) const
        {
            JE::Render::RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            switch (shaderOptions.m_skinningMethod)
            {
            case SkinningMethod::DualQuaternion:
                shaderOptionGroup.SetValue(m_skinningMethodOptionIndex, m_skinningMethodDualQuaternionValue);
                break;
            case SkinningMethod::NoSkinning:
                shaderOptionGroup.SetValue(m_skinningMethodOptionIndex, m_skinningMethodNoSkinningValue);
                break;
            case SkinningMethod::LinearSkinning:
            default:
                shaderOptionGroup.SetValue(m_skinningMethodOptionIndex, m_skinningMethodLinearSkinningValue);
            }

            if (shaderOptions.m_applyMorphTargets)
            {
                shaderOptionGroup.SetValue(m_applyMorphTargetOptionIndex, m_applyMorphTargetTrueValue);
            }
            else
            {
                shaderOptionGroup.SetValue(m_applyMorphTargetOptionIndex, m_applyMorphTargetFalseValue);
            }

            shaderOptionGroup.SetUnspecifiedToDefaultValues();

            return shaderOptionGroup;
        }
    } // namespace Features
} // namespace JE::Render
