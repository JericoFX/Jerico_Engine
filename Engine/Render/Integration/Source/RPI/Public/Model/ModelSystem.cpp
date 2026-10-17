/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Model/ModelSystem.h>

#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Model/ModelLod.h>

#include <JE/Render/RPI/Reflect/Model/ModelLodAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/Model/MorphTargetMetaAsset.h>
#include <JE/Render/RPI/Reflect/Model/SkinMetaAsset.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        void ModelSystem::Reflect(AZ::ReflectContext* context)
        {
            ModelLodAsset::Reflect(context);
            ModelAsset::Reflect(context);
            ModelMaterialSlot::Reflect(context);
            MorphTargetMetaAsset::Reflect(context);
            SkinMetaAsset::Reflect(context);
        }

        void ModelSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ModelLodAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ModelAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<MorphTargetMetaAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<SkinMetaAssetHandler>());
        }

        void ModelSystem::Init()
        {
            //Create Lod Database
            AZ::Data::InstanceHandler<ModelLod> lodInstanceHandler;
            lodInstanceHandler.m_createFunctionWithParam = [](AZ::Data::AssetData* modelLodAsset, const AZStd::any* modelAsset)
            {
                return ModelLod::CreateInternal(AZ::Data::Asset<ModelLodAsset>{modelLodAsset, AZ::Data::AssetLoadBehavior::PreLoad}, modelAsset);
            };
            AZ::Data::InstanceDatabase<ModelLod>::Create(azrtti_typeid<ModelLodAsset>(), lodInstanceHandler);

            //Create Model Database
            AZ::Data::InstanceHandler<Model> modelInstanceHandler;
            modelInstanceHandler.m_createFunction = [](AZ::Data::AssetData* modelAsset)
            {
                return Model::CreateInternal(AZ::Data::Asset<ModelAsset>{modelAsset, AZ::Data::AssetLoadBehavior::PreLoad});
            };
            AZ::Data::InstanceDatabase<Model>::Create(azrtti_typeid<ModelAsset>(), modelInstanceHandler);
        }

        void ModelSystem::Shutdown()
        {
            AZ::Data::InstanceDatabase<Model>::Destroy();
            AZ::Data::InstanceDatabase<ModelLod>::Destroy();
        }
    } // namespace RPI
} // namespace AZ
