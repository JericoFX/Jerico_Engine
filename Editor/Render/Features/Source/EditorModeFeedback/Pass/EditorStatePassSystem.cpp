/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorStatePassSystem.h>
#include <Pass/EditorStatePassSystemUtils.h>
#include <Pass/EditorModeFeedbackParentPass.h>
#include <Pass/State/EditorStateBufferCopyPass.h>
#include <Pass/Child/EditorModeDesaturationPass.h>
#include <Pass/Child/EditorModeTintPass.h>
#include <Pass/Child/EditorModeBlurPass.h>
#include <Pass/Child/EditorModeOutlinePass.h>

#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>

namespace JE::Editor::Render::Features
{
    static constexpr const char* const MainPassParentTemplateName = "EditorModeFeedbackParentTemplate";
    static constexpr const char* const MainPassParentTemplatePassClassName = "EditorModeFeedbackParentPass";
    static constexpr const char* const MainPassParentPassName = "EditorModeFeedback";
    
    static constexpr const char* const EditorModeDesaturationPassName = "EditorModeDesaturationPass";
    static constexpr const char* const EditorModeTintPassPassName = "EditorModeTintPass";
    static constexpr const char* const EditorModeBlurPassName = "EditorModeBlurPass";
    static constexpr const char* const EditorModeOutlinePassName = "EditorModeOutlinePass";

    EditorStatePassSystem::EditorStatePassSystem(EditorStateList&& editorStates)
        : m_editorStates(AZStd::move(editorStates))
    {
        auto* passSystem = JE::Render::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        if (!passSystem->HasCreatorForClass(AZ::Name(MainPassParentTemplatePassClassName)))
        {
            passSystem->AddPassCreator(AZ::Name(MainPassParentTemplatePassClassName), &EditorModeFeedbackParentPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(BufferCopyStatePassTemplatePassClassName)))
        {
            passSystem->AddPassCreator(AZ::Name(BufferCopyStatePassTemplatePassClassName), &EditorStateBufferCopyPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(StatePassTemplatePassClassName)))
        {
            passSystem->AddPassCreator(AZ::Name(StatePassTemplatePassClassName), &EditorStateParentPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(EditorModeDesaturationPassName)))
        {
            passSystem->AddPassCreator(AZ::Name(EditorModeDesaturationPassName), &EditorModeDesaturationPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(EditorModeTintPassPassName)))
        {
            passSystem->AddPassCreator(AZ::Name(EditorModeTintPassPassName), &EditorModeTintPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(EditorModeBlurPassName)))
        {
            passSystem->AddPassCreator(AZ::Name(EditorModeBlurPassName), &EditorModeBlurPass::Create);
        }
        if (!passSystem->HasCreatorForClass(AZ::Name(EditorModeOutlinePassName)))
        {
            passSystem->AddPassCreator(AZ::Name(EditorModeOutlinePassName), &EditorModeOutlinePass::Create);
        }

        // Editor state child effect passes
        // The mapping is global; each scene needs all child templates, including the nested blur pass.
        if (!passSystem->HasTemplate(AZ::Name("EditorModeDesaturationTemplate")) ||
            !passSystem->HasTemplate(AZ::Name("EditorModeTintTemplate")) ||
            !passSystem->HasTemplate(AZ::Name("EditorModeBlurParentTemplate")) ||
            !passSystem->HasTemplate(AZ::Name("EditorModeBlurTemplate")) ||
            !passSystem->HasTemplate(AZ::Name("EditorModeOutlineTemplate")))
        {
            passSystem->LoadPassTemplateMappings("Passes/Child/EditorModeFeedback_ChildPassTemplates.azasset");
        }
    }

    void EditorStatePassSystem::AddPassesToRenderPipeline(JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        const auto templateName = AZ::Name(MainPassParentTemplateName);

        // Early return if pass is already found in render pipeline or if the pipeline is not the default one (i.e it is an XR pipeline).
        auto passFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(templateName, renderPipeline);
        auto foundPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (foundPass || renderPipeline->GetViewType() != JE::Render::RPI::ViewType::Default)
        {
            return;
        }

        // check if the reference pass of insert position exist
        AZ::Name postProcessPassName = AZ::Name("PostProcessPass");
        if (renderPipeline->FindFirstPass(postProcessPassName) == nullptr)
        {
            AZ_Warning("EditorModeFeedback", false, "Can't find %s in the render pipeline. Editor mode feedback is disabled", postProcessPassName.GetCStr());
            return;
        }

        auto mainParentPassTemplate = JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(templateName);
        if (!mainParentPassTemplate)
        {
            // Create the pass template and add it to the pass system.
            auto newPassTemplate = AZStd::make_shared<JE::Render::RPI::PassTemplate>();
            newPassTemplate->m_name = templateName;
            newPassTemplate->m_passClass = AZ::Name(MainPassParentTemplatePassClassName);

            // Input depth slot
            {
                JE::Render::RPI::PassSlot slot;
                slot.m_name = AZ::Name("InputDepth");
                slot.m_slotType = JE::Render::RPI::PassSlotType::Input;
                newPassTemplate->AddSlot(slot);
            }

            // Input/output color slot
            {
                JE::Render::RPI::PassSlot slot;
                slot.m_name = AZ::Name("ColorInputOutput");
                slot.m_slotType = JE::Render::RPI::PassSlotType::InputOutput;
                newPassTemplate->AddSlot(slot);
            }

            // Entity mask passes
            m_masks = CreateMaskPassTemplatesFromEditorStates(m_editorStates);
            for (const auto& drawList : m_masks)
            {
                JE::Render::RPI::PassRequest pass;
                pass.m_passName = GetMaskPassNameForDrawList(drawList);
                pass.m_templateName = GetMaskPassTemplateNameForDrawList(drawList);

                // Input depth
                {
                    JE::Render::RPI::PassConnection connection;
                    connection.m_localSlot = AZ::Name("InputDepth");
                    connection.m_attachmentRef = { AZ::Name("Parent"), AZ::Name("InputDepth") };
                    pass.AddInputConnection(connection);
                }

                newPassTemplate->AddPassRequest(pass);
            }

            // Editor state passes
            auto previousOutput = AZStd::make_pair(AZ::Name("Parent"), AZ::Name("ColorInputOutput"));
            for (const auto& state : m_editorStates)
            {
                CreateAndAddStateParentPassTemplate(*state);
                JE::Render::RPI::PassRequest pass;
                pass.m_passName = state->GetPassName();
                pass.m_templateName = state->GetPassTemplateName();

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
                    connection.m_attachmentRef = { GetMaskPassNameForDrawList(state->GetEntityMaskDrawList()), AZ::Name("OutputEntityMask") };
                    pass.AddInputConnection(connection);
                }

                // Input color
                {
                    JE::Render::RPI::PassConnection connection;
                    connection.m_localSlot = AZ::Name("InputColor");
                    connection.m_attachmentRef = { previousOutput.first, previousOutput.second };
                    pass.AddInputConnection(connection);
                }

                newPassTemplate->AddPassRequest(pass);

                // Buffer copy pass
                {
                    CreateAndAddBufferCopyPassTemplate(*state);
                    JE::Render::RPI::PassRequest buffercopy;
                    buffercopy.m_passName = GetBufferCopyPassNameForState(*state);
                    buffercopy.m_templateName = GetBufferCopyPassTemplateName(*state);

                    // Input color
                    {
                        JE::Render::RPI::PassConnection connection;
                        connection.m_localSlot = AZ::Name("InputColor");
                        connection.m_attachmentRef = { pass.m_passName, AZ::Name("OutputColor") };
                        buffercopy.AddInputConnection(connection);
                    }

                    newPassTemplate->AddPassRequest(buffercopy);
                    previousOutput = { buffercopy.m_passName, AZ::Name("OutputColor") };
                }
            }

            JE::Render::RPI::PassSystemInterface::Get()->AddPassTemplate(newPassTemplate->m_name, newPassTemplate);

            mainParentPassTemplate = newPassTemplate;
        }

        JE::Render::RPI::PassRequest passRequest;
        passRequest.m_passName = AZ::Name(MainPassParentPassName);
        passRequest.m_templateName = mainParentPassTemplate->m_name;
        passRequest.AddInputConnection(
            JE::Render::RPI::PassConnection{ AZ::Name("ColorInputOutput"), JE::Render::RPI::PassAttachmentRef{ postProcessPassName, AZ::Name("Output") } });
        passRequest.AddInputConnection(
            JE::Render::RPI::PassConnection{ AZ::Name("InputDepth"), JE::Render::RPI::PassAttachmentRef{ AZ::Name("DepthPrePass"), AZ::Name("Depth") } });

        JE::Render::RPI::Ptr<JE::Render::RPI::Pass> parentPass = JE::Render::RPI::PassSystemInterface::Get()->CreatePassFromRequest(&passRequest);
        if (!parentPass)
        {
            AZ_Error("EditorStatePassSystem", false, "Create editor mode feedback parent pass from pass request failed", renderPipeline->GetId().GetCStr());
            return;
        }

        // Inject the parent pass after the PostProcess pass
        if (!renderPipeline->AddPassAfter(parentPass, postProcessPassName))
        {
            AZ_Error(
                "EditorStatePassSystem", false, "Add editor mode feedback parent pass to render pipeline [%s] failed", renderPipeline->GetId().GetCStr());
            return;
        }
    }

    EntityMaskMap EditorStatePassSystem::GetEntitiesForEditorStates() const
    {
        EntityMaskMap entityMaskMap;

        for (const auto& state : m_editorStates)
        {
            if (state->IsEnabled())
            {
                const auto entityIds = state->GetMaskedEntities();
                auto& mask = entityMaskMap[state->GetEntityMaskDrawList()];
                for (const auto& entityId : entityIds)
                {
                    mask.insert(entityId);
                }
            }
        }

        return entityMaskMap;
    }

    void EditorStatePassSystem::Update()
    {
        for (auto& state : m_editorStates)
        {
            state->UpdatePassDataForPipelines();
        }
    }
    
    const char* EditorStatePassSystem::GetParentPassTemplateName() const
    {
        return MainPassParentTemplateName;
    }

    void EditorStatePassSystem::ConfigureStatePassesForPipeline([[maybe_unused]]JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        JE::Render::RPI::PassFilter mainPassParentPassFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name(MainPassParentPassName), renderPipeline);
        JE::Render::RPI::Ptr<JE::Render::RPI::Pass> mainPass = JE::Render::RPI::PassSystemInterface::Get()->FindFirstPass(mainPassParentPassFilter);

        if (!mainPass)
        {
            return;
        }

        auto mainPassParent = azdynamic_cast<EditorModeFeedbackParentPass*>(mainPass.get());
        for (auto& state : m_editorStates)
        {
            auto statePass = mainPassParent->FindChildPass(AZ::Name(state->GetPassName()));
            state->AddParentPassForPipeline(renderPipeline->GetId(), statePass);
        }
    }

    void EditorStatePassSystem::RemoveStatePassesForPipeline([[maybe_unused]]JE::Render::RPI::RenderPipeline* renderPipeline)
    {
        for (auto& state : m_editorStates)
        {
            state->RemoveParentPassForPipeline(renderPipeline->GetId());
        }
    }
} // namespace JE::Editor::Render::Features
