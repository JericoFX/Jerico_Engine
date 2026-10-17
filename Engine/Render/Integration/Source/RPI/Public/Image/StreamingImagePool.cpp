/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Base.h>

#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <JE/Render/RPI/Public/Image/StreamingImagePool.h>
#include <JE/Render/RPI/Public/Image/StreamingImageController.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        AZ::Data::Instance<StreamingImagePool> StreamingImagePool::FindOrCreate(const AZ::Data::Asset<StreamingImagePoolAsset>& streamingImagePoolAsset)
        {
            return AZ::Data::InstanceDatabase<StreamingImagePool>::Instance().FindOrCreate(
                AZ::Data::InstanceId::CreateFromAssetId(streamingImagePoolAsset.GetId()),
                streamingImagePoolAsset);
        }

        AZ::Data::Instance<StreamingImagePool> StreamingImagePool::CreateInternal(StreamingImagePoolAsset& streamingImagePoolAsset)
        {
            AZ::Data::Instance<StreamingImagePool> streamingImagePool = aznew StreamingImagePool();
            const AZ::RHI::ResultCode resultCode = streamingImagePool->Init(streamingImagePoolAsset);

            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                return streamingImagePool;
            }

            return nullptr;
        }

        AZ::RHI::ResultCode StreamingImagePool::Init(StreamingImagePoolAsset& poolAsset)
        {
            AZ_PROFILE_FUNCTION(RPI);

            if (Validation::IsEnabled())
            {
                if (m_pool)
                {
                    AZ_Error("StreamingImagePool", false, "Invalid Operation: Attempted to initialize an already initialized pool.");
                    return AZ::RHI::ResultCode::InvalidOperation;
                }
            }

            AZ::RHI::Ptr<AZ::RHI::StreamingImagePool> pool = aznew AZ::RHI::StreamingImagePool;
            const AZ::RHI::ResultCode resultCode = pool->Init(poolAsset.GetPoolDescriptor());

            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                m_controller = StreamingImageController::Create(*pool);
                m_pool = AZStd::move(pool);
                m_pool->SetName(AZ::Name{ poolAsset.GetPoolName() });
                return AZ::RHI::ResultCode::Success;
            }

            AZ_Warning("StreamingImagePoolAsset", false, "Failed to initialize RHI::StreamingImagePool.");
            return resultCode;
        }

        void StreamingImagePool::AttachImage(StreamingImage* image)
        {
            // Add image to controller only if the image is streamable 
            if (image->IsStreamable())
            {
                m_controller->AttachImage(image);
            }
        }

        void StreamingImagePool::DetachImage(StreamingImage* image)
        {
            if (image->IsStreamable())
            {
                m_controller->DetachImage(image);
            }
        }

        void StreamingImagePool::Update()
        {
            m_controller->Update();
        }

        AZ::RHI::StreamingImagePool* StreamingImagePool::GetRHIPool()
        {
            return m_pool.get();
        }

        const AZ::RHI::StreamingImagePool* StreamingImagePool::GetRHIPool() const
        {
            return m_pool.get();
        }

        uint32_t StreamingImagePool::GetImageCount() const
        {
            return m_pool->GetResourceCount();
        }

        uint32_t StreamingImagePool::GetStreamableImageCount() const
        {
            return m_controller->GetStreamableImageCount();
        }

        bool StreamingImagePool::SetMemoryBudget(size_t newBudgetInBytes)
        {
            size_t currentBudget = GetMemoryBudget();
            bool isSet = m_pool->SetMemoryBudget(newBudgetInBytes);
            if (currentBudget > 0)
            {
                if (newBudgetInBytes == 0 || newBudgetInBytes > currentBudget)
                {
                    if (isSet)
                    {
                        m_controller->ResetLowMemoryState();
                    }
                }
            }
            return isSet;
        }

        size_t StreamingImagePool::GetMemoryBudget() const
        {
            return m_pool->GetHeapMemoryUsage(AZ::RHI::HeapMemoryLevel::Device).m_budgetInBytes;
        }

        bool StreamingImagePool::IsMemoryLow() const
        {
            return m_controller->IsMemoryLow();
        }

        void StreamingImagePool::SetMipBias(int16_t mipBias)
        {
            m_controller->SetMipBias(mipBias);
        }
                        
        int16_t StreamingImagePool::GetMipBias() const
        {
            return m_controller->GetMipBias();
        }
    }
}
