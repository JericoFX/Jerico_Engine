/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Mesh/ModelReloaderSystemInterface.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>

namespace JE::Render
{
    namespace Features
    {
        class ModelReloader;

        using RemoveModelFromReloaderSystemEvent = AZ::Event<const AZ::Data::AssetId&>;
        
        class ModelReloaderSystem
            : public ModelReloaderSystemInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ModelReloaderSystem, "Render::ModelReloaderSystem", "{8C85ECCD-B6C8-4949-B26C-9C4F1020F2B8}", JE::Render::Features::ModelReloaderSystemInterface);
            
            void ReloadModel(AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset, ModelReloadedEvent::Handler& onReloadedEventHandler) override;
            void RemoveReloader(const AZ::Data::AssetId& assetId) override;

        private:
            // Keep track of all the pending reloads so there are no duplicates
            AZStd::unordered_map<AZ::Data::AssetId, ModelReloader*> m_pendingReloads;
            AZStd::mutex m_pendingReloadMutex;

            friend class ModelReloader;
        };

    } // namespace Features
} // namespace JE::Render
