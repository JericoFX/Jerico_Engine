/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <Atom/RHI.Edit/ShaderPlatformInterfaceBus.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>

#include "ShaderAssetBuilder.h"
#include "ShaderVariantAssetBuilder.h"
#include "PrecompiledShaderBuilder.h"
#include "ShaderPlatformInterfaceRequest.h"
#include "ShaderVariantListBuilder.h"

namespace JE::Assets::RenderBuilders
{
    namespace Shader
    {
        
        class AzslShaderBuilderSystemComponent
            : public AZ::Component
            , public AZ::RHI::ShaderPlatformInterfaceRegisterBus::Handler
            , public ShaderPlatformInterfaceRequestBus::Handler
        {
        public:
            AZ_COMPONENT(AzslShaderBuilderSystemComponent, "{56B5B944-8AF4-4478-A047-8DFDE38DA681}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AzslShaderBuilderRequestBus interface implementation

            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            // ShaderPlatformInterfaceRegisterBus interface overrides ...
            void RegisterShaderPlatformHandler(AZ::RHI::ShaderPlatformInterface* shaderPlatformInterface) override;
            void UnregisterShaderPlatformHandler(AZ::RHI::ShaderPlatformInterface* shaderPlatformInterface) override;

            // ShaderPlatformInterfaceRequestBus interface overrides ...
            AZStd::vector<AZ::RHI::ShaderPlatformInterface*> GetShaderPlatformInterface(const AssetBuilderSDK::PlatformInfo& platformInfo) override;

        private:
            ShaderAssetBuilder m_shaderAssetBuilder;

            // The ShaderVariantAssetBuilder can be disabled with this registry key.
            // By default it is enabled. A user might want to disable it when doing look development
            // work with shaders or doing lots of iterative changes to shaders. In these cases
            // GPU performance doesn't matter at all so it is important to not waste time
            // building ShaderVariantAssets (Other than the Root ShaderVariantAsset, of course.).
            static constexpr char EnableShaderVariantAssetBuilderRegistryKey[] = "/O3DE/Atom/Shaders/BuildVariants";
            bool m_enableShaderVariantAssetBuilder = true;

            ShaderVariantAssetBuilder m_shaderVariantAssetBuilder;

            PrecompiledShaderBuilder m_precompiledShaderBuilder;

            ShaderVariantListBuilder m_shaderVariantListBuilder;

            /// Contains the ShaderPlatformInterface for all registered RHIs
            AZStd::unordered_map<AZ::RHI::APIType, AZ::RHI::ShaderPlatformInterface*> m_shaderPlatformInterfaces;
        };
        
    } // Shader
} //JE::Assets::RenderBuilders
