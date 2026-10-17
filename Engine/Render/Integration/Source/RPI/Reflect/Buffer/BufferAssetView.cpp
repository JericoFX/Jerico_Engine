/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Buffer/BufferAssetView.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace JE::Render
{
    namespace RPI
    {
        void BufferAssetView::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BufferAssetView>()
                    ->Version(0)
                    ->Field("BufferAsset", &BufferAssetView::m_bufferAsset)
                    ->Field("BufferViewDescriptor", &BufferAssetView::m_bufferViewDescriptor)
                    ;
            }
        }

        BufferAssetView::BufferAssetView(
            const AZ::Data::Asset<BufferAsset>& bufferAsset,
            const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor)
            : m_bufferAsset(bufferAsset)
            , m_bufferViewDescriptor(bufferViewDescriptor)
        {}

        const AZ::Data::Asset<BufferAsset>& BufferAssetView::GetBufferAsset() const
        {
            return m_bufferAsset;
        }

        const AZ::RHI::BufferViewDescriptor& BufferAssetView::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }

        void BufferAssetView::LoadBufferAsset()
        {
            if (m_bufferAsset.QueueLoad())
            {
                m_bufferAsset.BlockUntilLoadComplete();
            }
        }

        void BufferAssetView::ReleaseBufferAsset()
        {
            m_bufferAsset.Release();
        }

    } // namespace RPI
} // namespace AZ
