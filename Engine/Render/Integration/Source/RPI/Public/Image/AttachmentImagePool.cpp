/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        AttachmentImagePool::AttachmentImagePool() = default;

        AttachmentImagePool::~AttachmentImagePool() = default;

        AZ::Data::Instance<AttachmentImagePool> AttachmentImagePool::FindOrCreate(const AZ::Data::Asset<ResourcePoolAsset>& resourcePoolAsset)
        {
            return AZ::Data::InstanceDatabase<AttachmentImagePool>::Instance().FindOrCreate(
                AZ::Data::InstanceId::CreateFromAssetId(resourcePoolAsset.GetId()),
                resourcePoolAsset);
        }

        AZ::Data::Instance<AttachmentImagePool> AttachmentImagePool::CreateInternal(ResourcePoolAsset& poolAsset)
        {
            AZ::Data::Instance<AttachmentImagePool> imagePool = aznew AttachmentImagePool();
            AZ::RHI::ResultCode resultCode = imagePool->Init(poolAsset);

            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                return imagePool;
            }

            return nullptr;
        }

        AZ::RHI::ResultCode AttachmentImagePool::Init(ResourcePoolAsset& poolAsset)
        {
            AZ::RHI::Ptr<AZ::RHI::ImagePool> imagePool = aznew AZ::RHI::ImagePool;
            if (!imagePool)
            {
                AZ_Error("RPI::ImagePool", false, "Failed to create RHI::ImagePool");
                return AZ::RHI::ResultCode::Fail;
            }

            AZ::RHI::ImagePoolDescriptor *desc = azrtti_cast<AZ::RHI::ImagePoolDescriptor*>(poolAsset.GetPoolDescriptor().get());
            if (!desc)
            {
                AZ_Error("RPI::ImagePool", false, "The resource pool asset does not contain an image pool descriptor.");
                return AZ::RHI::ResultCode::Fail;
            }

            imagePool->SetName(AZ::Name(poolAsset.GetPoolName()));
            AZ::RHI::ResultCode resultCode = imagePool->Init(*desc);
            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                m_pool = imagePool;
            }
            return resultCode;
        }

        const AZ::RHI::ImagePool* AttachmentImagePool::GetRHIPool() const
        {
            return m_pool.get();
        }

        AZ::RHI::ImagePool* AttachmentImagePool::GetRHIPool()
        {
            return m_pool.get();
        }
    }
}
