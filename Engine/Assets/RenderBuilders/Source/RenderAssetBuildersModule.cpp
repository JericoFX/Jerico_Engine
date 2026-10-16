/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>

#include <Model/ModelExporterComponent.h>
#include <Model/ModelAssetBuilderComponent.h>
#include <Model/MaterialAssetBuilderComponent.h>
#include <BuilderComponent.h>

namespace JE::Assets::RenderBuilders
{
    RenderAssetBuildersModule::RenderAssetBuildersModule()
    {
        m_descriptors.push_back(ModelExporterComponent::CreateDescriptor());
        m_descriptors.push_back(ModelAssetBuilderComponent::CreateDescriptor());
        m_descriptors.push_back(ModelAssetDependenciesComponent::CreateDescriptor());
        m_descriptors.push_back(MaterialAssetBuilderComponent::CreateDescriptor());
        m_descriptors.push_back(MaterialAssetDependenciesComponent::CreateDescriptor());
        m_descriptors.push_back(BuilderComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList RenderAssetBuildersModule::GetRequiredSystemComponents() const
    {
        return {};
    }

    AZ::Module* CreateToolsModule()
    {
        return aznew RenderAssetBuildersModule();
    }
} // namespace JE::Assets::RenderBuilders
