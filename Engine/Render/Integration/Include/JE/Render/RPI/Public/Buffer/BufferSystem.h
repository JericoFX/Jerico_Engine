/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>

namespace AZ
{
    class Job;
    class JobContext;
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        class Buffer;

        //! Manages system-wide initialization and support for Buffer classes
        class JE_RENDER_INTEGRATION_API BufferSystem final
            : public BufferSystemInterface
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            // BufferSystemInterface overrides...
            AZ::RHI::Ptr<AZ::RHI::BufferPool> GetCommonBufferPool(CommonBufferPoolType poolType) override;
            AZ::Data::Instance<Buffer> CreateBufferFromCommonPool(const CommonBufferDescriptor& descriptor) override;
            AZ::Data::Instance<Buffer> FindCommonBuffer(AZStd::string_view uniqueBufferName) override;

            void Init();
            void Shutdown();

        protected:
            bool CreateCommonBufferPool(CommonBufferPoolType poolType);

        private:
            AZ::RHI::Ptr<AZ::RHI::BufferPool> m_commonPools[static_cast<uint8_t>(CommonBufferPoolType::Count)];

            bool m_initialized = false;
        };
    }

namespace AZ
{ // namespace RPI
} // namespace AZ
