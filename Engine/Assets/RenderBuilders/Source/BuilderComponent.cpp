/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>

#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPipelineSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyValueSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <JE/Editor/Render/RPI/Edit/Material/LuaMaterialFunctorSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderVariantListSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetAliasesSourceData.h>

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImagePoolAsset.h>
#include <JE/Render/RPI/Reflect/Buffer/BufferAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/Model/MorphTargetMetaAsset.h>
#include <JE/Render/RPI/Reflect/Model/SkinMetaAsset.h>
#include <JE/Render/RPI/Reflect/Pass/PassAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantTreeAsset.h>

#include <BuilderComponent.h>
#include <Common/AnyAssetBuilder.h>
#include <Material/MaterialBuilder.h>
#include <Material/MaterialPipelineScriptRunner.h>
#include <Material/MaterialTypeBuilder.h>
#include <ResourcePool/ResourcePoolBuilder.h>
#include <Pass/PassBuilder.h>

namespace JE::Assets::RenderBuilders
{
    void BuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }

        JE::Render::RPI::MaterialPropertySourceData::Reflect(context);
        JE::Render::RPI::MaterialTypeSourceData::Reflect(context);
        JE::Render::RPI::MaterialSourceData::Reflect(context);
        JE::Render::RPI::MaterialPipelineSourceData::Reflect(context);
        JE::Render::RPI::MaterialPropertyValueSourceData::Reflect(context);
        JE::Render::RPI::MaterialFunctorSourceData::Reflect(context);
        JE::Render::RPI::MaterialFunctorSourceDataHolder::Reflect(context);
        JE::Render::RPI::LuaMaterialFunctorSourceData::Reflect(context);
        JE::Render::RPI::ResourcePoolSourceData::Reflect(context);
        JE::Render::RPI::ConvertibleSource::Reflect(context);
        JE::Render::RPI::AssetAliasesSourceData::Reflect(context);
        JE::Render::RPI::ShaderSourceData::Reflect(context);
        JE::Render::RPI::ShaderVariantListSourceData::Reflect(context);
        MaterialPipelineScriptRunner::Reflect(context);
    }

    BuilderComponent::BuilderComponent()
    {
        m_materialFunctorRegistration.Init();
    }

    BuilderComponent::~BuilderComponent()
    {
        m_materialFunctorRegistration.Shutdown();
    }

    void BuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RPIBuilderService"));
    }

    void BuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("ScriptService"));
    }

    void BuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RPIBuilderService"));
    }

    void BuilderComponent::Activate()
    {
        // Register asset workers
        m_assetWorkers.emplace_back(MakeAssetBuilder<MaterialBuilder>());
        m_assetWorkers.emplace_back(MakeAssetBuilder<MaterialTypeBuilder>());
        m_assetWorkers.emplace_back(MakeAssetBuilder<ResourcePoolBuilder>());
        m_assetWorkers.emplace_back(MakeAssetBuilder<AnyAssetBuilder>());
        m_assetWorkers.emplace_back(MakeAssetBuilder<PassBuilder>());

        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ShaderAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialTypeAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ResourcePoolAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::StreamingImagePoolAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::BufferAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ModelLodAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ModelAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::PassAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ShaderVariantAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::ShaderVariantTreeAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::SkinMetaAssetHandler>());
        m_assetHandlers.emplace_back(JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MorphTargetMetaAssetHandler>());
    }

    void BuilderComponent::Deactivate()
    {
        m_assetHandlers.clear();
        m_assetWorkers.clear();
    }
} // namespace JE::Assets::RenderBuilders
