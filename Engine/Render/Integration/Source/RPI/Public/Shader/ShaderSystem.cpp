/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Shader/ShaderSystem.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroupPool.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadDebugTracker.h>

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantAsset.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantTreeAsset.h>
#include <JE/Render/RPI/Reflect/Shader/PrecompiledShaderAssetSourceData.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        void ShaderSystem::Reflect(AZ::ReflectContext* context)
        {
            ShaderOptionDescriptor::Reflect(context);
            ShaderOptionGroupLayout::Reflect(context);
            ShaderOptionGroupHints::Reflect(context);
            ShaderOptionGroup::Reflect(context);
            ShaderVariantId::Reflect(context);
            ShaderVariantStableId::Reflect(context);
            ShaderAsset::Reflect(context);
            ShaderInputContract::Reflect(context);
            ShaderOutputContract::Reflect(context);
            ShaderVariantAsset::Reflect(context);
            ShaderVariantTreeAsset::Reflect(context);
            ReflectShaderStageType(context);
            PrecompiledShaderAssetSourceData::Reflect(context);
        }

        ShaderSystemInterface* ShaderSystemInterface::Get()
        {
            return AZ::Interface<ShaderSystemInterface>::Get();
        }

        void ShaderSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ShaderAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantTreeAssetHandler>());
        }

        void ShaderSystem::Init()
        {
            m_shaderVariantAsyncLoader.Init();

            AZ::Interface<ShaderSystemInterface>::Register(this);

            {
                AZ::Data::InstanceHandler<Shader> handler;
                handler.m_createFunctionWithParam = [](AZ::Data::AssetData* shaderAsset, const AZStd::any* supervariantName)
                {
                    return Shader::CreateInternal(*(azrtti_cast<ShaderAsset*>(shaderAsset)), supervariantName);
                };
                AZ::Data::InstanceDatabase<Shader>::Create(azrtti_typeid<ShaderAsset>(), handler);
            }
            
            {
                AZ::Data::InstanceHandler<ShaderResourceGroup> handler;
                handler.m_createFunctionWithParam = [](AZ::Data::AssetData* shaderAsset, const AZStd::any* srgInitBlob)
                {
                    return ShaderResourceGroup::CreateInternal(*(azrtti_cast<ShaderAsset*>(shaderAsset)), srgInitBlob);
                };
                AZ::Data::InstanceDatabase<ShaderResourceGroup>::Create(azrtti_typeid<ShaderResourceGroup>(), handler, false);
            }

            {
                AZ::Data::InstanceHandler<ShaderResourceGroupPool> handler;
                handler.m_createFunctionWithParam = [](AZ::Data::AssetData* shaderAsset, const AZStd::any* srgInitBlob)
                {
                    return ShaderResourceGroupPool::CreateInternal(*(azrtti_cast<ShaderAsset*>(shaderAsset)), srgInitBlob);
                };
                AZ::Data::InstanceDatabase<ShaderResourceGroupPool>::Create(azrtti_typeid<ShaderResourceGroupPool>(), handler, false);
            }

            ShaderReloadDebugTracker::Init();
        }

        void ShaderSystem::Shutdown()
        {
            ShaderReloadDebugTracker::Shutdown();
            AZ::Data::InstanceDatabase<Shader>::Destroy();
            AZ::Data::InstanceDatabase<ShaderResourceGroup>::Destroy();
            AZ::Data::InstanceDatabase<ShaderResourceGroupPool>::Destroy();
            AZ::Interface<ShaderSystemInterface>::Unregister(this);
            m_shaderVariantAsyncLoader.Shutdown();
        }


        ///////////////////////////////////////////////////////////////////
        // ShaderSystemInterface overrides
        void ShaderSystem::SetGlobalShaderOption(const AZ::Name& shaderOptionName, ShaderOptionValue value)
        {
            bool changed = false;

            auto iter = m_globalShaderOptionValues.find(shaderOptionName);
            if (iter == m_globalShaderOptionValues.end())
            {
                changed = true;
                m_globalShaderOptionValues[shaderOptionName] = value;
            }
            else if (iter->second != value)
            {
                iter->second = value;
                changed = true;
            }

            if (changed)
            {
                m_globalShaderOptionUpdatedEvent.Signal(shaderOptionName, AZStd::move(value));
            }
        }

        ShaderOptionValue ShaderSystem::GetGlobalShaderOption(const AZ::Name& shaderOptionName)
        {
            ShaderOptionValue value;
            auto iter = m_globalShaderOptionValues.find(shaderOptionName);
            if (iter != m_globalShaderOptionValues.end())
            {
                value = iter->second;
            }
            return value;
        }

        const ShaderSystem::GlobalShaderOptionMap& ShaderSystem::GetGlobalShaderOptions() const
        {
            return m_globalShaderOptionValues;
        }

        void ShaderSystem::Connect(GlobalShaderOptionUpdatedEvent::Handler& handler)
        {
            handler.Connect(m_globalShaderOptionUpdatedEvent);
        }

        void ShaderSystem::SetSupervariantName(const AZ::Name& supervariantName)
        {
            m_supervariantName = supervariantName;
        }

        const AZ::Name& ShaderSystem::GetSupervariantName() const
        {
            return m_supervariantName;
        }
        ///////////////////////////////////////////////////////////////////

    } // namespace RPI
} // namespace AZ
