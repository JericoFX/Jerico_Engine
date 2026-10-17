/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Image/AttachmentImageAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        void AttachmentImageAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AttachmentImageAsset, ImageAsset>()
                    ->Version(2)
                    ->Field("Name", &AttachmentImageAsset::m_name)
                    ->Field("IsUniqueName", &AttachmentImageAsset::m_isUniqueName)
                    ->Field("OptimizedClearValue", &AttachmentImageAsset::m_optimizedClearValue)
                    ;
            }
        }

        const AZ::Data::Asset<ResourcePoolAsset>& AttachmentImageAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        const AZ::RHI::ClearValue* AttachmentImageAsset::GetOptimizedClearValue() const
        {
            return m_optimizedClearValue.get();
        }

        const AZ::Name& AttachmentImageAsset::GetName() const
        {
            return m_name;
        }

        AZ::RHI::AttachmentId AttachmentImageAsset::GetAttachmentId() const
        {
            if (HasUniqueName())
            {
                return m_name;
            }
            return AZ::Name(m_assetId.ToString<AZStd::string>());
        }

         bool AttachmentImageAsset::HasUniqueName() const
        {
             // The name can still be empty if the asset was loaded for data file but not from AttachmentImageAssetCreator
            return m_isUniqueName && !m_name.IsEmpty();
        }
    }
}
