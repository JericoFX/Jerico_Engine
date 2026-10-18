/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace JE::Render
{
    namespace Features
    {
        /**
         *  The look modification transform pass.
         */
        class LookModificationPass
            : public JE::Render::RPI::ParentPass
        {
        public:
            AZ_RTTI(LookModificationPass, "{68C3A664-FB97-40ED-9638-21938D6692B3}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(LookModificationPass, AZ::SystemAllocator);
            virtual ~LookModificationPass() = default;

            //! Creates a LookModificationPass
            static JE::Render::RPI::Ptr<LookModificationPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        protected:
            LookModificationPass(const JE::Render::RPI::PassDescriptor& descriptor);

            //! Pass overrides ...
            void FrameBeginInternal(FramePrepareParams params) override;

            void BuildInternal() override;

        private:
            const JE::Render::RPI::PassAttachmentBinding* m_pipelineOutput = nullptr;
            AZ::RHI::Format m_displayBufferFormat = AZ::RHI::Format::Unknown;
            AZ::Render::OutputDeviceTransformType m_outputDeviceTransformType = AZ::Render::OutputDeviceTransformType_48Nits;
            AZ::Render::ShaperParams m_shaperParams;
        };
    } // namespace Features
} // namespace JE::Render
