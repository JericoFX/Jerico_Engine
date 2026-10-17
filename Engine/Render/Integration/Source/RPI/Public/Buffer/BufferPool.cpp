/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Buffer/BufferPool.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferPool.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        AZ::Data::Instance<BufferPool> BufferPool::FindOrCreate(const AZ::Data::Asset<ResourcePoolAsset>& resourcePoolAsset)
        {
            return AZ::Data::InstanceDatabase<BufferPool>::Instance().FindOrCreate(
                AZ::Data::InstanceId::CreateFromAsset(resourcePoolAsset),
                resourcePoolAsset);
        }

        AZ::Data::Instance<BufferPool> BufferPool::CreateInternal(ResourcePoolAsset& poolAsset)
        {
            AZ::Data::Instance<BufferPool> bufferPool = aznew BufferPool();
            AZ::RHI::ResultCode resultCode = bufferPool->Init(poolAsset);
            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                return bufferPool;
            }

            return nullptr;
        }

        AZ::RHI::ResultCode BufferPool::Init(ResourcePoolAsset& poolAsset)
        {
            AZ::RHI::Ptr<AZ::RHI::BufferPool> bufferPool = aznew AZ::RHI::BufferPool;
            if (!bufferPool)
            {
                AZ_Error("RPI::BufferPool", false, "Failed to create RHI::BufferPool");
                return AZ::RHI::ResultCode::Fail;
            }

            AZ::RHI::BufferPoolDescriptor *desc = azrtti_cast<AZ::RHI::BufferPoolDescriptor*>(poolAsset.GetPoolDescriptor().get());
            if (!desc)
            {
                AZ_Error("RPI::BufferPool", false, "The resource pool asset does not contain a buffer pool descriptor.");
                return AZ::RHI::ResultCode::Fail;
            }

            bufferPool->SetName(AZ::Name{ poolAsset.GetPoolName() });
            AZ::RHI::ResultCode resultCode = bufferPool->Init(*desc);
            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                m_pool = bufferPool;
            }

            return resultCode;
        }

        const AZ::RHI::BufferPool* BufferPool::GetRHIPool() const
        {
            return m_pool.get();
        }

        AZ::RHI::BufferPool* BufferPool::GetRHIPool()
        {
            return m_pool.get();
        }
    }
}
