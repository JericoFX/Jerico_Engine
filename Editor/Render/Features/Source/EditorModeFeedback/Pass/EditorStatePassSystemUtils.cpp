/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBufferCopyPassData.h>
#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/State/EditorStateParentPassData.h>
#include <Pass/EditorStatePassSystem.h>
#include <Pass/EditorStatePassSystemUtils.h>

#include <JE/Render/RPI/Reflect/Pass/RasterPassData.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>

namespace JE::Editor::Render::Features
{
    AZ::Name GetMaskPassTemplateNameForDrawList(const AZ::Name& drawList)
    {
        return AZ::Name(AZStd::string(drawList.GetStringView()) + "_EditorModeMaskTemplate");
    }

    AZ::Name GetMaskPassNameForDrawList(const AZ::Name& drawList)
    {
        return AZ::Name(AZStd::string(drawList.GetStringView()) + "_EntityMaskPass");
    }

    AZ::Name GetBufferCopyPassTemplateName(const EditorStateBase& state)
    {
        return AZ::Name(state.GetStateName() + "BufferCopyTemplate");
    }
    
    AZ::Name GetBufferCopyPassNameForState(const EditorStateBase& state)
    {
        return AZ::Name(state.GetStateName() + "BufferCopyPass");
    }

    void CreateAndAddStateParentPassTemplate(const EditorStateBase& state)
    {
        const auto templateName = state.GetPassTemplateName();
        if (JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto stateParentPassTemplate = AZStd::make_shared<JE::Render::RPI::PassTemplate>();
        stateParentPassTemplate->m_name = templateName;
        stateParentPassTemplate->m_passClass = StatePassTemplatePassClassName;

         // Input depth slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("InputDepth");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Input entity mask slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("InputEntityMask");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Input color slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("InputColor");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Output color slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("OutputColor");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Output;
            stateParentPassTemplate->AddSlot(slot);
        }

        // Fallback connections
        {
            JE::Render::RPI::PassFallbackConnection fallbackConnection;
            fallbackConnection.m_inputSlotName = AZ::Name("InputColor");
            fallbackConnection.m_outputSlotName = AZ::Name("OutputColor");
            stateParentPassTemplate->m_fallbackConnections.push_back(fallbackConnection);
        }

        // Pass data
        {
            auto passData = AZStd::make_shared<JE::Editor::Render::Features::EditorStateParentPassData>();
            passData->editorStatePass = &state;
            stateParentPassTemplate->m_passData = passData;
        }

        // Child passes
        auto previousOutput = AZStd::make_pair(AZ::Name("Parent"), AZ::Name("InputColor"));
        AZ::u32 passCount = 0;
        for (const auto& childPassTemplate : state.GetChildPassNameList())
        {
            auto childPassName = state.GetGeneratedChildPassName(passCount);
            JE::Render::RPI::PassRequest pass;
            pass.m_passName = childPassName;
            pass.m_templateName = childPassTemplate;
            
            // Input depth
            {
                JE::Render::RPI::PassConnection connection;
                connection.m_localSlot = AZ::Name("InputDepth");
                connection.m_attachmentRef = { AZ::Name("Parent"), AZ::Name("InputDepth") };
                pass.AddInputConnection(connection);
            }
            
            // Input entity mask
            {
                JE::Render::RPI::PassConnection connection;
                connection.m_localSlot = AZ::Name("InputEntityMask");
                connection.m_attachmentRef = { AZ::Name("Parent"), AZ::Name("InputEntityMask") };
                pass.AddInputConnection(connection);
            }
            
            // Input color
            {
                JE::Render::RPI::PassConnection connection;
                connection.m_localSlot = AZ::Name("InputColor");
                connection.m_attachmentRef = { previousOutput.first, previousOutput.second };
                pass.AddInputConnection(connection);
            }
            
            stateParentPassTemplate->AddPassRequest(pass);
            previousOutput = { pass.m_passName, AZ::Name("OutputColor") };
            passCount++;
        }

        // Connections
        {
            JE::Render::RPI::PassConnection connection;
            connection.m_localSlot = AZ::Name("OutputColor");
            connection.m_attachmentRef.m_pass = previousOutput.first;
            connection.m_attachmentRef.m_attachment = previousOutput.second;
            stateParentPassTemplate->AddOutputConnection(connection);
        }

        JE::Render::RPI::PassSystemInterface::Get()->AddPassTemplate(stateParentPassTemplate->m_name, stateParentPassTemplate);
    }

    void CreateAndAddBufferCopyPassTemplate(const EditorStateBase& state)
    {
        const auto templateName = GetBufferCopyPassTemplateName(state);
        if (JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto passTemplate = AZStd::make_shared<JE::Render::RPI::PassTemplate>();
        passTemplate->m_name = templateName;
        passTemplate->m_passClass = BufferCopyStatePassTemplatePassClassName;
    
        // Input color slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("InputColor");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            slot.m_shaderInputName = AZ::Name("m_framebuffer");
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::Shader;
            passTemplate->AddSlot(slot);
        }

        // Output color slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("OutputColor");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;
            slot.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;
            passTemplate->AddSlot(slot);
        }

        // Connections
        {
            JE::Render::RPI::PassConnection connection;
            connection.m_localSlot = AZ::Name("OutputColor");
            connection.m_attachmentRef.m_pass = AZ::Name("Parent");
            connection.m_attachmentRef.m_attachment = AZ::Name("ColorInputOutput");
            passTemplate->AddOutputConnection(connection);
        }

        // Fallback connections
        {
            JE::Render::RPI::PassFallbackConnection fallbackConnection;
            passTemplate->m_fallbackConnections.push_back({ AZ::Name("InputColor"), AZ::Name("OutputColor") });
        }

        // Pass data
        {
            const auto shaderFilePath = "shaders/editormodebuffercopy.azshader";
            AZ::Data::AssetId shaderAssetId = JE::Render::RPI::AssetUtils::GetAssetIdForProductPath(shaderFilePath, JE::Render::RPI::AssetUtils::TraceLevel::Assert, azrtti_typeid<JE::Render::RPI::ShaderAsset>());
            if (!shaderAssetId.IsValid())
            {
                return;
            }

            auto passData = AZStd::make_shared<JE::Editor::Render::Features::EditorStateBufferCopyPassData>();
            passData->m_pipelineViewTag = "MainCamera";
            passData->m_shaderAsset.m_filePath = shaderFilePath;
            passData->m_shaderAsset.m_assetId = shaderAssetId;
            passData->editorStatePass = &state;
            passTemplate->m_passData = passData;
        }

        JE::Render::RPI::PassSystemInterface::Get()->AddPassTemplate(passTemplate->m_name, passTemplate);
    }

    void CreateAndAddMaskPassTemplate(const AZ::Name& drawList)
    {
        const auto templateName = GetMaskPassTemplateNameForDrawList(drawList);
        if (JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(templateName))
        {
            // Template was created by another pipeline, do not to create again
            return;
        }

        auto maskPassTemplate = AZStd::make_shared<JE::Render::RPI::PassTemplate>();
        maskPassTemplate->m_name = templateName;
        maskPassTemplate->m_passClass = AZ::Name("RasterPass");

        // Input depth slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("InputDepth");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            slot.m_shaderInputName = AZ::Name("m_existingDepth");
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::Shader;
            slot.m_shaderImageDimensionsName = AZ::Name("m_existingDepthDimensions");
            slot.m_imageViewDesc = AZStd::make_shared<AZ::RHI::ImageViewDescriptor>();
            slot.m_imageViewDesc->m_aspectFlags = AZ::RHI::ImageAspectFlags::Depth;
            maskPassTemplate->AddSlot(slot);
        }

        // Output entity mask slot
        {
            JE::Render::RPI::PassSlot slot;
            slot.m_name = AZ::Name("OutputEntityMask");
            slot.m_slotType = JE::Render::RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;
            slot.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Clear;
            slot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateVector4Float(0.0, 0.0, 0.0, 0.0);
            maskPassTemplate->AddSlot(slot);
        }

        // Output entity mask attachment
        JE::Render::RPI::PassImageAttachmentDesc imageAttachment;
        imageAttachment.m_name = AZ::Name("OutputEntityMaskAttachment");
        imageAttachment.m_sizeSource.m_source.m_pass = AZ::Name("This");
        imageAttachment.m_sizeSource.m_source.m_attachment = AZ::Name("InputDepth");
        imageAttachment.m_imageDescriptor.m_format = AZ::RHI::Format::R8G8_UNORM;
        imageAttachment.m_imageDescriptor.m_sharedQueueMask = AZ::RHI::HardwareQueueClassMask::Graphics;
        maskPassTemplate->AddImageAttachment(imageAttachment);

        // Output entity mask
        JE::Render::RPI::PassConnection connection;
        connection.m_localSlot = AZ::Name("OutputEntityMask");
        connection.m_attachmentRef = { AZ::Name("This"), AZ::Name("OutputEntityMaskAttachment") };
        maskPassTemplate->AddOutputConnection(connection);

        // Pass data
        {
            auto passData = AZStd::make_shared<JE::Render::RPI::RasterPassData>();
            passData->m_drawListTag = AZ::Name(drawList);
            passData->m_passSrgShaderReference.m_filePath = "shaders/editormodemask.azshader";
            passData->m_pipelineViewTag = "MainCamera";
            maskPassTemplate->m_passData = passData;
        }

        JE::Render::RPI::PassSystemInterface::Get()->AddPassTemplate(maskPassTemplate->m_name, maskPassTemplate);
    }

    AZStd::unordered_set<AZ::Name> CreateMaskPassTemplatesFromEditorStates(
        const EditorStateList& editorStates)
    {
        AZStd::unordered_set<AZ::Name> drawLists;
        for (const auto& state : editorStates)
        {
            if (const auto drawList = state->GetEntityMaskDrawList();
                !drawLists.contains(drawList))
            {
                CreateAndAddMaskPassTemplate(drawList);
                drawLists.insert(drawList);
            }
        }

        return drawLists;
    }
} // namespace JE::Editor::Render::Features
