/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>

#include <JE/Render/RPI/Reflect/Base.h>
#include <JE/Render/RPI/Reflect/Image/AttachmentImageAsset.h>
#include <JE/Render/RPI/Reflect/Image/AttachmentImageAssetCreator.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImagePool.h>

#include <AzCore/Instance/InstanceDatabase.h>

namespace JE::Render
{
    namespace RPI
    {
        AZ::Data::Instance<AttachmentImage> AttachmentImage::FindOrCreate(const AZ::Data::Asset<AttachmentImageAsset>& imageAsset)
        {
            return AZ::Data::InstanceDatabase<AttachmentImage>::Instance().FindOrCreate(
                AZ::Data::InstanceId::CreateFromAssetId(imageAsset.GetId()), imageAsset);
        }

        AZ::Data::Instance<AttachmentImage> AttachmentImage::Create(
            const AttachmentImagePool& imagePool,
            const AZ::RHI::ImageDescriptor& imageDescriptor,
            const AZ::Name& imageName,
            const AZ::RHI::ClearValue* optimizedClearValue,
            const AZ::RHI::ImageViewDescriptor* imageViewDescriptor)
        {
            CreateAttachmentImageRequest createImageRequest;
            createImageRequest.m_imagePool = &imagePool;
            createImageRequest.m_imageDescriptor = imageDescriptor;
            createImageRequest.m_imageName = imageName;
            createImageRequest.m_isUniqueName = false;
            createImageRequest.m_optimizedClearValue = optimizedClearValue;
            createImageRequest.m_imageViewDescriptor = imageViewDescriptor;
            return Create(createImageRequest);
        }
                    
        AZ::Data::Instance<AttachmentImage> AttachmentImage::Create(const CreateAttachmentImageRequest& createImageRequest)
        {
            AZ::Data::Asset<AttachmentImageAsset> imageAsset;

            AZ::Data::AssetId assetId;
            if (createImageRequest.m_isUniqueName)
            {
                assetId = AZ::Uuid::CreateName(createImageRequest.m_imageName.GetCStr());
            }
            else
            {
                assetId = AZ::Uuid::CreateRandom();
            }

            AZ::Data::InstanceId instanceId = AZ::Data::InstanceId::CreateFromAssetId(assetId);

            AttachmentImageAssetCreator imageAssetCreator;
            imageAssetCreator.Begin(assetId);
            imageAssetCreator.SetImageDescriptor(createImageRequest.m_imageDescriptor);
            imageAssetCreator.SetPoolAsset({createImageRequest.m_imagePool->GetAssetId(), azrtti_typeid<ResourcePoolAsset>()});
            imageAssetCreator.SetName(createImageRequest.m_imageName, createImageRequest.m_isUniqueName);

            if (createImageRequest.m_imageViewDescriptor)
            {
                imageAssetCreator.SetImageViewDescriptor(*createImageRequest.m_imageViewDescriptor);
            }

            if (createImageRequest.m_optimizedClearValue)
            {
                imageAssetCreator.SetOptimizedClearValue(*createImageRequest.m_optimizedClearValue);
            }

            if (imageAssetCreator.End(imageAsset))
            {
                return AZ::Data::InstanceDatabase<AttachmentImage>::Instance().FindOrCreate(instanceId, imageAsset);
            }

            return nullptr;
        }

        AZ::Data::Instance<AttachmentImage> AttachmentImage::FindByUniqueName(const AZ::Name& uniqueAttachmentName)
        {            
            return ImageSystemInterface::Get()->FindRegisteredAttachmentImage(uniqueAttachmentName);
        }

        AZ::Data::Instance<AttachmentImage> AttachmentImage::CreateInternal(AttachmentImageAsset& imageAsset)
        {
            AZ::Data::Instance<AttachmentImage> image = aznew AttachmentImage();
            auto result = image->Init(imageAsset);

            if (result == AZ::RHI::ResultCode::Success)
            {
                image->m_imageAsset = { &imageAsset, AZ::Data::AssetLoadBehavior::PreLoad };
                return image;
            }

            return nullptr;
        }

        AttachmentImage::AttachmentImage() = default;

        AttachmentImage::~AttachmentImage()
        {
            Shutdown(); 
        }

        AZ::RHI::ResultCode AttachmentImage::Init(const AttachmentImageAsset& imageAsset)
        {
            AZ::Data::Instance<AttachmentImagePool> pool;
            if (imageAsset.GetPoolAsset().GetId().IsValid())
            {
                pool = AttachmentImagePool::FindOrCreate(imageAsset.GetPoolAsset());
            }
            else
            {
                pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
            }

            if (!pool)
            {
                AZ_Error("AttachmentImage", false, "Failed to acquire the attachment image pool instance.");
                return AZ::RHI::ResultCode::Fail;
            }

            AZ::RHI::ImagePool* rhiPool = pool->GetRHIPool();

            AZ::RHI::ImageInitRequest initRequest;
            initRequest.m_image = GetRHIImage();
            initRequest.m_descriptor = imageAsset.GetImageDescriptor();
            initRequest.m_optimizedClearValue = imageAsset.GetOptimizedClearValue();

            AZ::RHI::ResultCode resultCode = rhiPool->InitImage(initRequest);
            if (resultCode == AZ::RHI::ResultCode::Success)
            {
                m_imagePool = pool;
                m_imageView = m_image->GetImageView(imageAsset.GetImageViewDescriptor());
                if(!m_imageView.get())
                {
                    AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image view.");
                    return AZ::RHI::ResultCode::Fail;
                }
                
                m_image->SetName(imageAsset.GetName());
                m_attachmentId = imageAsset.GetAttachmentId();

                if (imageAsset.HasUniqueName())
                {
                    ImageSystemInterface::Get()->RegisterAttachmentImage(this);
                }

                return AZ::RHI::ResultCode::Success;
            }

            AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image [%d].", resultCode);
            return resultCode;
        }
        
        const AZ::RHI::AttachmentId& AttachmentImage::GetAttachmentId() const
        {
            return m_attachmentId;
        }
        
        void AttachmentImage::Shutdown()
        {
            if (m_imageAsset && m_imageAsset->HasUniqueName())
            {
                ImageSystemInterface::Get()->UnregisterAttachmentImage(this);
            }
        }
    }
}
