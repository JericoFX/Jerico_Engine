/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Instance/Instance.h>
#include <AzCore/Instance/InstanceId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Atom/RHI.Reflect/Base.h>

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Asset/BuiltInAssetHandler.h>
#include <JE/Render/RPI/Reflect/Image/ImageSystemDescriptor.h>

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <JE/Render/RPI/Public/Image/StreamingImagePool.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }

    }

namespace JE::Render::RPI
{
        class JE_RENDER_INTEGRATION_API ImageSystem final
            : public ImageSystemInterface
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init(const ImageSystemDescriptor& desc);
            void Shutdown();

            //! Performs a streaming controller update tick, which will fetch / evict mips
            //! based on priority. This should be called once per frame. GPU uploads are
            //! performed during this call.
            void Update() override;

            //////////////////////////////////////////////////////////////////////////
            // ImageSystemInterface
            const AZ::Data::Instance<Image>& GetSystemImage(SystemImage systemImage) const override;
            const AZ::Data::Instance<AttachmentImage>& GetSystemAttachmentImage(AZ::RHI::Format format) override;
            const AZ::Data::Instance<StreamingImagePool>& GetSystemStreamingPool() const override;
            const AZ::Data::Instance<AttachmentImagePool>& GetSystemAttachmentPool() const override;
            const AZ::Data::Instance<StreamingImagePool>& GetStreamingPool() const override;
            bool RegisterAttachmentImage(AttachmentImage* attachmentImage) override;
            void UnregisterAttachmentImage(AttachmentImage* attachmentImage) override;
            AZ::Data::Instance<AttachmentImage> FindRegisteredAttachmentImage(const AZ::Name& uniqueName) const override;
            //////////////////////////////////////////////////////////////////////////

        private:
            void CreateDefaultResources(const ImageSystemDescriptor& desc);

            // The set of active (externally created) streaming image pools tracked by the system.
            AZStd::mutex m_activeStreamingPoolMutex;
            AZStd::vector<StreamingImagePool*> m_activeStreamingPools;

            AZ::Data::Instance<StreamingImagePool> m_systemStreamingPool;
            AZ::Data::Instance<AttachmentImagePool> m_systemAttachmentPool;

            // Streaming image pool for streaming image created from asset 
            AZ::Data::Instance<StreamingImagePool> m_assetStreamingPool;

            AZStd::fixed_vector<AZ::Data::Instance<Image>, static_cast<uint32_t>(SystemImage::Count)> m_systemImages;

            AZStd::shared_mutex m_systemAttachmentImagesUpdateMutex;
            AZStd::unordered_map<AZ::RHI::Format, AZ::Data::Instance<AttachmentImage>> m_systemAttachmentImages;

            bool m_initialized = false;

            // a collections of registered attachment images
            // Note: use AttachmentImage* instead of Data::Instance<AttachmentImage> so it can be released properly
            AZStd::unordered_map<AZ::RHI::AttachmentId, AttachmentImage*> m_registeredAttachmentImages;
        };
    }

namespace AZ
{
}
