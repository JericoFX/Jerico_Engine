/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ResourcePool/ResourcePoolBuilder.h>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>

#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAssetCreator.h>

namespace JE::Assets::RenderBuilders
{
    // The file extension for the source file of resource pool Asset
    const char* s_sourcePoolAssetExt = "resourcepool";

    void ResourcePoolBuilder::RegisterBuilder()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;

        builderDescriptor.m_name = "Atom Resource Pool Asset Builder";
        builderDescriptor.m_version = 2; //ATOM-15196
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern(AZStd::string("*.") + s_sourcePoolAssetExt,
            AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<ResourcePoolBuilder>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&ResourcePoolBuilder::CreateJobs, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&ResourcePoolBuilder::ProcessJob, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    ResourcePoolBuilder::~ResourcePoolBuilder()
    {
        BusDisconnect();
    }

    void ResourcePoolBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Atom Resource Pool";
            descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            descriptor.m_critical = false;

            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void ResourcePoolBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        if (jobCancelListener.IsCancelled())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::RPI::ResourcePoolSourceData poolSourceData;
        AZ::Outcome<void, AZStd::string> loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<AZ::RPI::ResourcePoolSourceData>(poolSourceData,
            request.m_fullPath);

        if (!loadResult.IsSuccess())
        {
            AZ_Error("PoolAssetProducer", false, "Failed to load source asset file %s", request.m_fullPath.c_str());
            AZ_Error("PoolAssetProducer", false, "Loading issues: %s", loadResult.GetError().c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ_TracePrintf("AssetBuilder", "Load source data success\n");

        // Convert source format to asset used for runtime
        AZ::Data::AssetData* assetData = nullptr;
        const char* extension;
        AZ::Data::Asset<AZ::Data::AssetData> poolAsset;
        if (poolSourceData.m_poolType == AZ::RPI::ResourcePoolAssetType::StreamingImagePool)
        {
            extension = AZ::RPI::StreamingImagePoolAsset::Extension;
            poolAsset = CreateStreamingPoolAssetFromSource(poolSourceData);
        }
        else
        {
            extension = AZ::RPI::ResourcePoolAsset::Extension;
            poolAsset = CreatePoolAssetFromSource(poolSourceData);
        }

        assetData = poolAsset.GetData();

        if (!assetData)
        {
            AZ_Error("PoolAssetProducer", false, "Failed to create asset data");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ_TracePrintf("AssetBuilder", "Convert data success\n");

        // Get file name from source file path, then replace the extension to generate product file name
        AZStd::string destFileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
        AzFramework::StringFunc::Path::ReplaceExtension(destFileName, extension);

        // Construct product full path
        AZStd::string destPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), destPath, true);

        // Save the asset to binary format for production
        bool result = AZ::Utils::SaveObjectToFile(destPath, AZ::DataStream::ST_BINARY, assetData, assetData->GetType(), nullptr);

        if (result == false)
        {
            AZ_Error("PoolAssetProducer", false, "Failed to save asset to cache")
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ_TracePrintf("AssetBuilder", "Saved data to file %s \n", destPath.c_str());

        // Success. Save output product(s) to response
        AssetBuilderSDK::JobProduct jobProduct(destPath, assetData->GetType(), 0);
        jobProduct.m_dependenciesHandled = true; // This builder has no dependencies to output.
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void ResourcePoolBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    AZ::Data::Asset<AZ::Data::AssetData> ResourcePoolBuilder::CreatePoolAssetFromSource(const AZ::RPI::ResourcePoolSourceData& sourceData)
    {
        AZ::RPI::ResourcePoolAssetCreator assetCreator;
        assetCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        assetCreator.SetPoolName(sourceData.m_poolName);
        switch (sourceData.m_poolType)
        {
        case AZ::RPI::ResourcePoolAssetType::BufferPool:
        {
            // [GFX TODO][ATOM-112] - Need to create pool descriptor based on device .
            AZStd::unique_ptr<AZ::RHI::BufferPoolDescriptor> bufferPoolDescriptor = AZStd::make_unique<AZ::RHI::BufferPoolDescriptor>();
            bufferPoolDescriptor->m_budgetInBytes = sourceData.m_budgetInBytes;
            bufferPoolDescriptor->m_heapMemoryLevel = sourceData.m_heapMemoryLevel;
            bufferPoolDescriptor->m_hostMemoryAccess = sourceData.m_hostMemoryAccess;
            bufferPoolDescriptor->m_bindFlags = sourceData.m_bufferPoolBindFlags;
            assetCreator.SetPoolDescriptor(AZStd::move(bufferPoolDescriptor));
            break;
        }
        case AZ::RPI::ResourcePoolAssetType::ImagePool:
        {
            AZStd::unique_ptr<AZ::RHI::ImagePoolDescriptor> imagePoolDescriptor = AZStd::make_unique<AZ::RHI::ImagePoolDescriptor>();
            imagePoolDescriptor->m_budgetInBytes = sourceData.m_budgetInBytes;
            imagePoolDescriptor->m_bindFlags = sourceData.m_imagePoolBindFlags;
            assetCreator.SetPoolDescriptor(AZStd::move(imagePoolDescriptor));
            break;
        }
        default:
            break;
        }

        AZ::Data::Asset<AZ::RPI::ResourcePoolAsset> poolAsset;

        if (assetCreator.End(poolAsset))
        {
            return poolAsset;
        }
        return {};
    }

    AZ::Data::Asset<AZ::Data::AssetData> ResourcePoolBuilder::CreateStreamingPoolAssetFromSource(const AZ::RPI::ResourcePoolSourceData& sourceData)
    {
        AZ_Assert(sourceData.m_poolType == AZ::RPI::ResourcePoolAssetType::StreamingImagePool, "Please use CreatePoolAssetFromSource for other type of pools");

        AZ::RPI::StreamingImagePoolAssetCreator assetCreator;
        assetCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        AZStd::unique_ptr<AZ::RHI::StreamingImagePoolDescriptor> poolDescriptor = AZStd::make_unique<AZ::RHI::StreamingImagePoolDescriptor>();
        poolDescriptor->m_budgetInBytes = sourceData.m_budgetInBytes;
        assetCreator.SetPoolDescriptor(AZStd::move(poolDescriptor));
        assetCreator.SetPoolName(sourceData.m_poolName);

        AZ::Data::Asset<AZ::RPI::StreamingImagePoolAsset> poolAsset;
        assetCreator.End(poolAsset);
        return poolAsset;
    }
} // namespace JE::Assets::RenderBuilders
