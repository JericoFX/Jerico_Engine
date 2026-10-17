/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <JE/Editor/Render/RPI/Edit/ResourcePool/ResourcePoolSourceData.h>

namespace JE::Assets::RenderBuilders
{
    class ResourcePoolBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_TYPE_INFO(ResourcePoolBuilder, "{5F8B71F1-9D4C-49DD-9F3C-8C92CBF0600C}");

        ResourcePoolBuilder() = default;
        ~ResourcePoolBuilder();

        // Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        // AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        /// Register builder which using this worker to process AP jobs
        void RegisterBuilder();

        /// Convert pool source data to runtime image pool asset or buffer pool asset
        static AZ::Data::Asset<AZ::Data::AssetData> CreatePoolAssetFromSource(const JE::Render::RPI::ResourcePoolSourceData& sourceData);

        /// Convert resource pool source data to runtime streaming image pool asset
        static AZ::Data::Asset<AZ::Data::AssetData> CreateStreamingPoolAssetFromSource(const JE::Render::RPI::ResourcePoolSourceData& sourceData);

        bool m_isShuttingDown = false;
    };
} // namespace JE::Assets::RenderBuilders
