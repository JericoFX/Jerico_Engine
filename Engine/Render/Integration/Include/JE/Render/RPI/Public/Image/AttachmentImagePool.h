/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Device.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <AzCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
        class ImagePool;
    }

    }

namespace JE::Render::RPI
{
        class ResourcePoolAsset;

        class JE_RENDER_INTEGRATION_API AttachmentImagePool final
            : public AZ::Data::InstanceData
        {
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(AttachmentImagePool, "{18295AA3-1098-4346-8B31-71E3A2BE0AC7}");
            AZ_CLASS_ALLOCATOR(AttachmentImagePool, AZ::SystemAllocator);

            //! Instantiates or returns an existing attachment image pool using a paired resource pool asset.
            static AZ::Data::Instance<AttachmentImagePool> FindOrCreate(const AZ::Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~AttachmentImagePool() override;

            AZ::RHI::ImagePool* GetRHIPool();

            const AZ::RHI::ImagePool* GetRHIPool() const;

        private:
            AttachmentImagePool();

            // Standard asset creation path.
            static AZ::Data::Instance<AttachmentImagePool> CreateInternal(ResourcePoolAsset& poolAsset);
            AZ::RHI::ResultCode Init(ResourcePoolAsset& poolAsset);

            /// The RHI image pool instance.
            AZ::RHI::Ptr<AZ::RHI::ImagePool> m_pool;
        };
    }

namespace AZ
{
}
