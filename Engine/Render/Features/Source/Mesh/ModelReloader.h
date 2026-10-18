/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <JE/Render/Features/Mesh/ModelReloaderSystemInterface.h>
#include <Mesh/ModelReloaderSystem.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class ModelAsset;
    }

namespace JE::Render
{

    namespace Features
    {
        //! ModelReloader takes care of reloading Buffer, ModelLod, and Model assets in the correct order
        //! The ModelReloaderSystem should be used to reload a model, rather than using a ModelReloader directly
        class ModelReloader
            : private AZ::Data::AssetBus::MultiHandler
        {
            using DependencyList = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ModelReloader, "AZ::Render::ModelReloader", "{99B75A6A-62B6-490A-9953-029BE7D69452}");

            ModelReloader() = default;

            //! Reload a model asset
            //! @param modelAsset - the asset to be reloaded
            ModelReloader(AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset);

            //! Connects a handler that will handle an event when the model is finished reloading
            void ConnectOnReloadedEventHandler(ModelReloadedEvent::Handler& onReloadedEventHandler);

        private:
            enum class State
            {
                WaitingForMeshDependencies,
                WaitingForModelDependencies,
                WaitingForModel
            };

            // Data::AssetBus::MultiHandler overrides...
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;            
            void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            void InsertMeshDependencyIfUnique(AZ::Data::Asset<AZ::Data::AssetData> asset);
            void ReloadDependenciesAndWait();
            void AdvanceToNextLevelOfHierarchy();
            DependencyList& GetPendingDependencyList();

            ModelReloadedEvent m_onModelReloaded;

            // Keep track of all the asset references for each level of the hierarchy
            DependencyList m_modelAsset;
            DependencyList m_meshDependencies;
            DependencyList m_modelDependencies;
            
            AZStd::bitset<1024> m_pendingDependencyListStatus;
            State m_state;
        };

    } // namespace Features
} // namespace JE::Render
