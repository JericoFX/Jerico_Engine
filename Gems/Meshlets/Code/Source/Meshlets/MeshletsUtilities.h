/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Instance/Instance.h>
#include <AzCore/Instance/InstanceData.h>

#include <Atom/RHI/ImagePool.h>

#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

#include <SharedBuffer.h>

namespace AZ
{
    namespace RHI
    {
        class BufferAssetView;
    }

    }

namespace JE::Render::RPI
{
        class Buffer;
    }

namespace AZ
{

    namespace Meshlets
    {
        class UtilityClass
        {
        public:
            UtilityClass() = default;

            static Data::Instance<JE::Render::RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<JE::Render::RPI::Shader> shader,
                const char* shaderResourceGroupId,
                const char* moduleName
            );

            static Data::Instance<JE::Render::RPI::Buffer> CreateBuffer(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg = nullptr
            );

            static Data::Instance<JE::Render::RPI::Buffer> CreateBufferAndBindToSrg(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg
            );

            static bool BindBufferToSrg(
                const char* warningHeader,
                Data::Instance<JE::Render::RPI::Buffer> buffer,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg = nullptr
            );

            static Data::Instance<RHI::BufferView> CreateSharedBufferView(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<Meshlets::SharedBufferAllocation>& bufferAllocator
            );

            static bool BindBufferViewToSrg(
                [[maybe_unused]] const char* warningHeader,
                Data::Instance<RHI::BufferView> bufferView,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg
            );

            static Data::Instance<RHI::BufferView> CreateSharedBufferViewAndBindToSrg(
                const char* warningHeader,
                SrgBufferDescriptor& bufferDesc,
                Data::Instance<Meshlets::SharedBufferAllocation>& outputBufferAllocator,
                Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg
            );

            //! Utility function to create a resource view into the shared buffer memory area. This
            //! resource view can have a different type than the shared buffer data.
            //! Since the shared buffer class is used as a buffer allocation for many sub-buffers, this
            //! method should be used after creating a new allocation within the shared buffer.
            static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(
                uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
                RHI::Format format, RHI::BufferBindFlags overrideBindFlags
            );
        };

    } // namespace Meshlets
} // namespace AZ
