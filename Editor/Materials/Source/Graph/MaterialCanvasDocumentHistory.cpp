/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace JE::Editor
{
    namespace
    {
        using JE::Editor::Tools::AtomToolsDocumentRequestBus;
        using JE::Editor::Tools::DocumentHistoryState;

        struct ReplayResult
        {
            bool m_success = true;
        };

        AzToolsFramework::UndoSystem::UndoStack* GetUndoStack()
        {
            AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                stack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
            return stack;
        }

        class CanvasDocumentUndo final : public AzToolsFramework::UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(CanvasDocumentUndo, AZ::SystemAllocator);
            CanvasDocumentUndo(const AZ::Uuid& id, const AZStd::string& path,
                DocumentHistoryState before, DocumentHistoryState after, AZStd::shared_ptr<ReplayResult> result)
                : URSequencePoint("Edit Material Graph Document")
                , m_id(id), m_path(path), m_before(AZStd::move(before)), m_after(AZStd::move(after)), m_result(AZStd::move(result)) {}

            bool Changed() const override { return m_before != m_after; }
            void Undo() override { Restore(m_after, m_before); }
            void Redo() override { Restore(m_before, m_after); }

        private:
            void Restore(const DocumentHistoryState& expected, const DocumentHistoryState& target)
            {
                auto* document = AtomToolsDocumentRequestBus::FindFirstHandler(m_id);
                if (!document || m_path.empty() || document->GetAbsolutePath() != m_path)
                {
                    m_result->m_success = false;
                    AZ_Error("MaterialCanvasDocumentHistory", false,
                        "Cannot replay graph document edit: document was destroyed, closed, or changed source path.");
                    return;
                }
                // Both Graph and Any documents validate and deserialize a candidate before replacing live data.
                m_result->m_success &= document->RestoreHistoryState(expected, target);
            }

            const AZ::Uuid m_id;
            const AZStd::string m_path;
            const DocumentHistoryState m_before;
            const DocumentHistoryState m_after;
            const AZStd::shared_ptr<ReplayResult> m_result;
        };
    }

    JE::Editor::Tools::DocumentHistoryPolicy CreateMaterialCanvasDocumentHistoryPolicy()
    {
        auto replayResult = AZStd::make_shared<ReplayResult>();
        JE::Editor::Tools::DocumentHistoryPolicy policy;
        policy.m_commit = [replayResult](const AZ::Uuid& id, const AZStd::string& path,
            const DocumentHistoryState& before, const DocumentHistoryState& after)
        {
            if (!GetUndoStack() || path.empty() || before.empty() || after.empty())
            {
                AZ_Error("MaterialCanvasDocumentHistory", false,
                    "Cannot record graph document edit: Editor Undo stack, source path, or snapshot is unavailable.");
                return false;
            }
            AzToolsFramework::ScopedUndoBatch batch("Edit Material Graph Document");
            auto* command = aznew CanvasDocumentUndo(id, path, before, after, replayResult);
            command->SetParent(batch.GetUndoBatch());
            return true;
        };
        policy.m_canUndo = []() { auto* stack = GetUndoStack(); return stack && stack->CanUndo(); };
        policy.m_canRedo = []() { auto* stack = GetUndoStack(); return stack && stack->CanRedo(); };
        policy.m_undo = [replayResult]()
        {
            auto* stack = GetUndoStack();
            if (!stack || !stack->CanUndo()) { return false; }
            replayResult->m_success = true;
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::UndoPressed);
            return replayResult->m_success;
        };
        policy.m_redo = [replayResult]()
        {
            auto* stack = GetUndoStack();
            if (!stack || !stack->CanRedo()) { return false; }
            replayResult->m_success = true;
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::RedoPressed);
            return replayResult->m_success;
        };
        return policy;
    }
}
