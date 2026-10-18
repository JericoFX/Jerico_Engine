/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>

namespace JE::Render
{
    namespace Features
    {
        /**
         * Fullscreen pass as a base for children passes to the DisplayMapperPass.
         * Children fullscreen passes created by the DisplayMapper subclass this and it provides
         * a way to tidy up the code for connecting to the input attachment of this pass with an
         * attachment from another pass. This is provided as the reference pass and reference
         * attachment. These are determined when the children passes are created by the DisplayMapper.
         */
        class DisplayMapperFullScreenPass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DisplayMapperFullScreenPass);

        public:
            AZ_RTTI(DisplayMapperFullScreenPass, "{11E9EBE0-DF6D-4260-9281-C99E386231BF}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DisplayMapperFullScreenPass, AZ::SystemAllocator);
            virtual ~DisplayMapperFullScreenPass();

            /// Creates a DisplayMapperFullScreenPass
            static JE::Render::RPI::Ptr<DisplayMapperFullScreenPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetInputReferencePassName(const AZ::Name& passName);
            void SetInputReferenceAttachmentName(const AZ::Name& attachmentName);

        protected:
            explicit DisplayMapperFullScreenPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            virtual void BuildInternal() override;

        private:
            AZ::Name m_inputReferencePassName = AZ::Name{ "Parent" };
            AZ::Name m_inputReferenceAttachmentName = AZ::Name{ "Input" };
            const AZ::Name InputAttachmentName = AZ::Name{ "Input" };
        };
    } // namespace Features
} // namespace JE::Render
