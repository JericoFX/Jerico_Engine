/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Image/AttachmentImageAssetCreator.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>

#include <AzCore/Asset/AssetManager.h>

namespace JE::Render
{
    namespace RPI
    {
        void AttachmentImageAssetCreator::Begin(const AZ::Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void AttachmentImageAssetCreator::SetImageDescriptor(const AZ::RHI::ImageDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetImageViewDescriptor(const AZ::RHI::ImageViewDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageViewDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetPoolAsset(const AZ::Data::Asset<ResourcePoolAsset>& poolAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolAsset = poolAsset;
            }
        }

        void AttachmentImageAssetCreator::SetOptimizedClearValue(const AZ::RHI::ClearValue& clearValue)
        {
            if (ValidateIsReady())
            {
                m_asset->m_optimizedClearValue = AZStd::make_shared<AZ::RHI::ClearValue>(clearValue);
            }
        }

        bool AttachmentImageAssetCreator::End(AZ::Data::Asset<AttachmentImageAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            // If the pool wasn't provided, use the system default attachment pool
            if (!m_asset->m_poolAsset.GetId().IsValid())
            {
                AZ::Data::Instance<JE::Render::RPI::AttachmentImagePool> pool = JE::Render::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
                m_asset->m_poolAsset = {pool->GetAssetId(), azrtti_typeid<ResourcePoolAsset>()};
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
        
        void AttachmentImageAssetCreator::SetAssetHint(AZStd::string_view hint)
        {
            m_asset.SetHint(hint);
        }

        void AttachmentImageAssetCreator::SetName(const AZ::Name& uniqueName, bool isUniqueName)
        {
            if (uniqueName.IsEmpty())
            {
                m_asset->m_isUniqueName = false;
                AZ_Warning("RPI", false, "Can't set empty string as unique name");
            }
            else
            {
                m_asset->m_isUniqueName = isUniqueName;
            }
            m_asset->m_name = uniqueName;

        }
    }
}
