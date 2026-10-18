/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/DisplayMapperFullScreenPass.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>

#include <Atom/RHI/Factory.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DisplayMapperFullScreenPass> DisplayMapperFullScreenPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DisplayMapperFullScreenPass> pass = aznew DisplayMapperFullScreenPass(descriptor);
            return pass;
        }

        DisplayMapperFullScreenPass::DisplayMapperFullScreenPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        DisplayMapperFullScreenPass::~DisplayMapperFullScreenPass()
        {
        }

        void DisplayMapperFullScreenPass::BuildInternal()
        {
            JE::Render::RPI::PassConnection inConnection;
            inConnection.m_localSlot = InputAttachmentName;
            inConnection.m_attachmentRef.m_pass = m_inputReferencePassName;
            inConnection.m_attachmentRef.m_attachment = m_inputReferenceAttachmentName;
            ProcessConnection(inConnection);
        }

        void DisplayMapperFullScreenPass::SetInputReferencePassName(const AZ::Name& passName)
        {
            m_inputReferencePassName = passName;
        }

        void DisplayMapperFullScreenPass::SetInputReferenceAttachmentName(const AZ::Name& attachmentName)
        {
            m_inputReferenceAttachmentName = attachmentName;
        }

    } // namespace Features
} // namespace JE::Render
