/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImageView.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Reflect/Pass/PassDescriptor.h>


namespace JE::Render::RPI
{
    //! This pass takes a texture without auxiliary mip slices as input
    //! a texture with mip slices as output.
    //! It then recursively downsamples that mip to lower mip levels using single dispatch of compute shader.
    class JE_RENDER_INTEGRATION_API DownsampleSinglePassLuminancePass final
        : public ComputePass
    {
        AZ_RPI_PASS(DownsampleSinglePassMipChainPass);

    public:
        AZ_RTTI(DownsampleSinglePassLuminancePass, "{6842F4D2-D884-4E2A-B48B-E9240BCB8F45}", ComputePass);
        AZ_CLASS_ALLOCATOR(DownsampleSinglePassLuminancePass, AZ::SystemAllocator);

        static Ptr<DownsampleSinglePassLuminancePass> Create(const PassDescriptor& descriptor);
        virtual ~DownsampleSinglePassLuminancePass() = default;

    private:
        struct SpdGlobalAtomicBuffer
        {
            uint32_t m_counter;
        };

        explicit DownsampleSinglePassLuminancePass(const PassDescriptor& descriptor, AZ::Name supervariant);

        // Pass Behaviour Overrides...
        void BuildInternal() override;
        void ResetInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

        // Scope producer functions...
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

        void BuildGlobalAtomicBuffer();
        void InitializeIndices();
        void GetDestinationInfo();
        void CalculateSpdThreadDimensionAndMips();
        void BuildPassAttachment();

        void SetConstants();

        static constexpr uint32_t SpdMipLevelCountMax = 13;
        static constexpr uint32_t GloballyCoherentMipIndex = 6;
        const AZ::Name Mip6Name{"m_mip6"};
        const AZ::Name GlobalAtomicName{"m_globalAtomic"};

        // Dimension of the destination mip chain image.
        AZStd::array<uint32_t, 2> m_destinationImageSize = {0, 0};
        uint32_t m_destinationMipLevelCount = 0;

        // Number of mip levels for SPD computation
        // which can be slightly greater than m_destinationMipLevelCount for
        // computation of non power of 2 width.
        uint32_t m_spdMipLevelCount = 0;

        bool m_indicesAreInitialized = false;
        uint32_t m_targetThreadCountWidth = 1;
        uint32_t m_targetThreadCountHeight = 1;
        AZ::RHI::ShaderInputConstantIndex m_spdMipLevelCountIndex;
        AZ::RHI::ShaderInputConstantIndex m_destinationMipLevelCountIndex;
        AZ::RHI::ShaderInputConstantIndex m_numWorkGroupsIndex;
        AZ::RHI::ShaderInputConstantIndex m_imageSizeIndex;
        AZ::RHI::ShaderInputImageIndex m_imageDestinationIndex;
        AZ::RHI::ShaderInputImageIndex m_mip6ImageIndex;
        AZ::RHI::ShaderInputBufferIndex m_globalAtomicIndex;

        // Attachment for transient image and its image descriptor.
        Ptr<PassAttachment> m_mip6PassAttachment;
        AZ::RHI::ImageDescriptor m_mip6ImageDescriptor;

        // Attachment for transient buffer.
        Ptr<PassAttachment> m_counterPassAttachment;

        // Retainer of image views for each mip level of in/out image.
        AZStd::array<Ptr<AZ::RHI::ImageView>, SpdMipLevelCountMax> m_imageViews;

        AZ::Data::Instance<Buffer> m_globalAtomicBuffer;
    };
}   // namespace AZ::RPI
