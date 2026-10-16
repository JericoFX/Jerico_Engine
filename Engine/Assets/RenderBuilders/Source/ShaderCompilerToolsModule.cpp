/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Shader/AzslShaderBuilderSystemComponent.h>

namespace JE::Assets::RenderBuilders
{
    class ShaderCompilerToolsModule final : public AZ::Module
    {
    public:
        // Retain the original shader module RTTI name and UUID for existing module identity.
        AZ_RTTI_WITH_NAME(ShaderCompilerToolsModule, "AzslShaderBuilderModule", "{43370465-DBF1-44BB-968D-97C0B42F5EA0}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ShaderCompilerToolsModule, AZ::SystemAllocator);

        ShaderCompilerToolsModule()
        {
            m_descriptors.push_back(Shader::AzslShaderBuilderSystemComponent::CreateDescriptor());
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return { azrtti_typeid<Shader::AzslShaderBuilderSystemComponent>() };
        }
    };

    AZ::Module* CreateShaderCompilerToolsModule()
    {
        return aznew ShaderCompilerToolsModule();
    }
} // namespace JE::Assets::RenderBuilders
