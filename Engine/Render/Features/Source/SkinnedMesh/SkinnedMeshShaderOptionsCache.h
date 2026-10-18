/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/SkinnedMesh/SkinnedMeshShaderOptions.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Instance/Instance.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class Shader;
    }

namespace JE::Render
{

    namespace Features
    {
        class CachedSkinnedMeshShaderOptions;

        //! Notifies listeners that the skinned mesh shader has reloaded and the shader options need to be updated
        class SkinnedMeshShaderOptionNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = CachedSkinnedMeshShaderOptions*;
            using MutexType = AZStd::mutex;

            virtual void OnShaderReinitialized(const CachedSkinnedMeshShaderOptions* cachedShaderOptions) = 0;
        };
        using SkinnedMeshShaderOptionNotificationBus = AZ::EBus<SkinnedMeshShaderOptionNotifications>;

        //! This class caches the indices of the skinned mesh shader options and uses them to more optimally create a ShaderOptionGroup
        class CachedSkinnedMeshShaderOptions
        {
        public:
            void SetShader(AZ::Data::Instance<JE::Render::RPI::Shader> shader);
            void ConnectToShaderReinitializedEvent(SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedEventHandler);
            JE::Render::RPI::ShaderOptionGroup CreateShaderOptionGroup(const SkinnedMeshShaderOptions& shaderOptions) const;
        private:
            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            JE::Render::RPI::ShaderOptionIndex m_skinningMethodOptionIndex;
            JE::Render::RPI::ShaderOptionValue m_skinningMethodLinearSkinningValue;
            JE::Render::RPI::ShaderOptionValue m_skinningMethodDualQuaternionValue;
            JE::Render::RPI::ShaderOptionValue m_skinningMethodNoSkinningValue;

            JE::Render::RPI::ShaderOptionIndex m_applyMorphTargetOptionIndex;
            JE::Render::RPI::ShaderOptionValue m_applyMorphTargetFalseValue;
            JE::Render::RPI::ShaderOptionValue m_applyMorphTargetTrueValue;
        };
    } // namespace Features
} // namespace JE::Render
