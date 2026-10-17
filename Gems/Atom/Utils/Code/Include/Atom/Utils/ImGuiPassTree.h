/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/AttachmentReadback.h>
#include <JE/Render/RPI/Public/Pass/Specific/ImageAttachmentPreviewPass.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        class ImGuiPassTree
        {
        public:
            ImGuiPassTree() = default;
            ~ImGuiPassTree() = default;

            void Draw(bool& draw, JE::Render::RPI::Pass* rootPass);

            void Reset();

        private:
            void DrawTreeView(JE::Render::RPI::Pass* rootPass, const AZStd::unordered_set<Name>& filteredPassNames);

            void ReadbackCallback(const JE::Render::RPI::AttachmentReadback::ReadbackResult& readbackResult);

            void DrawPassAttachments(JE::Render::RPI::Pass* pass);

            bool GetFilteredPassNames(JE::Render::RPI::Pass* pass, AZStd::unordered_set<Name>& filteredPassNames) const;

            AZStd::string GetPassDisplayName(JE::Render::RPI::Pass* pass) const;

            bool m_shouldPreviewAttachment = false;
            bool m_showAttachments = false;
            bool m_expandAllPasses = true;
            bool m_showDisabledPasses = true;

            JE::Render::RPI::Pass* m_selectedPass = nullptr;
            JE::Render::RPI::Pass* m_lastSelectedPass = nullptr;
            AZ::Name m_selectedPassPath;
            AZ::RHI::AttachmentId m_attachmentId;
            float m_attachmentColorTranformRange[2] = {0.0f, 1.0f};
            AZ::Name m_slotName;
            bool m_selectedChanged = false;

            AZStd::shared_ptr<JE::Render::RPI::AttachmentReadback> m_readback;

            JE::Render::RPI::Ptr<JE::Render::RPI::ImageAttachmentPreviewPass> m_previewPass;

            AZStd::string m_engineRoot;
            
            AZStd::string m_attachmentReadbackInfo;

            ImGuiTextFilter m_passFilter;
        };
    }
}

#include "ImGuiPassTree.inl"
