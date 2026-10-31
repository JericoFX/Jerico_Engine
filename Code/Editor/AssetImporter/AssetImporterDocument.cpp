/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporter/AssetImporterDocument.h>
#include <Util/PathUtil.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <IEditor.h>
#include <QFile>
#include <QWidget>
#include <QPointer>
#include <QThread>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <QMessageBox>
#include <QPushButton>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AssetImporter/ActionOutput.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

AssetImporterDocument::AssetImporterDocument()
{
}

bool AssetImporterDocument::LoadScene(const AZStd::string& sceneFullPath)
{
    AZ_PROFILE_FUNCTION(Editor);
    ++m_epoch;
    namespace SceneEvents = AZ::SceneAPI::Events;
    SceneEvents::SceneSerializationBus::BroadcastResult(m_scene, &SceneEvents::SceneSerializationBus::Events::LoadScene, sceneFullPath, AZ::Uuid::CreateNull(), "");
    return !!m_scene;
}

void AssetImporterDocument::SaveScene(AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete)
{
    if (!m_scene)
    {
        if (output)
        {
            output->AddError("No scene file was loaded.");
        }

        if (onSaveComplete)
        {
            onSaveComplete(false);
        }

        return;
    }

    // If a save is requested, the user is going to want to see the asset re-processed, even if they didn't actually make a change.
    bool fingerprintCleared = false;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
        fingerprintCleared, &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, m_scene->GetManifestFilename());
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
        fingerprintCleared, &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, m_scene->GetSourceFilename());

    m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();

    // Add a no-op saver to put the FBX into source control. The benefit of doing it this way
    //  rather than invoking the SourceControlCommands bus directly is that we enable ourselves
    //  to have a single callback point for fbx & the manifest.
    auto fbxNoOpSaver = m_saveRunner->GenerateController();
    fbxNoOpSaver->AddSaveOperation(m_scene->GetSourceFilename(), nullptr);

    // Save the manifest
    SaveManifest();

    m_saveRunner->Run(output,
        [this, onSaveComplete](bool success)
        {
            if (onSaveComplete)
            {
                onSaveComplete(success);
            }

            m_saveRunner = nullptr;
    }, AZ::AsyncSaveRunner::ControllerOrder::Sequential);
}

void AssetImporterDocument::ClearScene()
{
    ++m_epoch;
    m_scene.reset();
}

void AssetImporterDocument::SaveManifest()
{
    // Create the save controller and add the save operation for the manifest job to it
    AZStd::shared_ptr<AZ::SaveOperationController> saveController = m_saveRunner->GenerateController();

    saveController->AddSaveOperation(m_scene->GetManifestFilename(),
        [this](const AZStd::string& fullPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
        {
            AZ_UNUSED(actionOutput);
            return m_scene->GetManifest().SaveToFile(fullPath.c_str());
        });
}

AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& AssetImporterDocument::GetScene()
{
    return m_scene;
}


namespace
{
    using SurfaceRule = AssetImporterDocument::SurfaceRule;
    using RuleSnapshot = AZStd::optional<SurfaceRule>;
    using MeshGroup = AZ::SceneAPI::DataTypes::IMeshGroup;
    using Scene = AZ::SceneAPI::Containers::Scene;
    MeshGroup* FindIdentityGroup(Scene& scene, const AZ::Uuid& id)
    {
        if (id.IsNull()) { return nullptr; }
        MeshGroup* found = nullptr;
        for (const auto& object : scene.GetManifest().GetValueStorage())
        {
            if (auto* group = azrtti_cast<MeshGroup*>(object.get()); group && group->GetId() == id)
            {
                if (found) { return nullptr; } found = group;
            }
        }
        return found;
    }
    bool SameIdentity(const RuleSnapshot& lhs, const RuleSnapshot& rhs)
    {
        if (lhs.has_value() != rhs.has_value()) { return false; }
        if (!lhs) { return true; }
        if (lhs->m_schema != rhs->m_schema || lhs->m_regions != rhs->m_regions || lhs->m_nodes.size() != rhs->m_nodes.size()) { return false; }
        for (size_t i = 0; i != lhs->m_nodes.size(); ++i)
        {
            const auto& a = lhs->m_nodes[i]; const auto& b = rhs->m_nodes[i];
            if (a.m_nodePath != b.m_nodePath || a.m_topology != b.m_topology || a.m_faces != b.m_faces) { return false; }
        }
        return true;
    }
    bool ReadIdentity(const MeshGroup& group, RuleSnapshot& snapshot, size_t& position)
    {
        snapshot.reset(); const auto& rules = group.GetRuleContainerConst(); position = rules.GetRuleCount();
        for (size_t i = 0; i != rules.GetRuleCount(); ++i)
        {
            if (const auto* rule = azrtti_cast<const SurfaceRule*>(rules.GetRule(i).get()))
            {
                if (snapshot) { return false; } snapshot = *rule; position = i;
            }
        }
        return true;
    }
    class SurfaceIdentityCommand final : public AzToolsFramework::UndoSystem::URSequencePoint
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceIdentityCommand, AZ::SystemAllocator)

        SurfaceIdentityCommand(const AZStd::shared_ptr<Scene>& scene, AZ::Uuid group, QObject* owner,
            AZStd::function<bool()> isCurrent, AssetImporterDocument::SurfaceIdentityChanged changed,
            RuleSnapshot before, RuleSnapshot after, size_t position)
            : URSequencePoint("Edit Surface Identity"), m_scene(scene), m_group(group), m_owner(owner),
              m_isCurrent(AZStd::move(isCurrent)), m_changed(AZStd::move(changed)),
              m_before(AZStd::move(before)), m_after(AZStd::move(after)), m_position(position) {}
        bool Changed() const override { return !SameIdentity(m_before, m_after); }
        void Undo() override { if (!Apply(m_after, m_before)) { AZ_Warning("SceneSettings", false, "Surface Identity Undo no longer belongs to the current document."); } }
        void Redo() override { if (!Apply(m_before, m_after)) { AZ_Warning("SceneSettings", false, "Surface Identity Redo no longer belongs to the current document."); } }
        bool Apply(const RuleSnapshot& expected, const RuleSnapshot& desired)
        {
            if (!m_owner || QThread::currentThread() != m_owner->thread() || !m_isCurrent || !m_isCurrent()) { return false; }
            const auto scene = m_scene.lock(); if (!scene) { return false; }
            auto* group = FindIdentityGroup(*scene, m_group); if (!group) { return false; }
            RuleSnapshot current; size_t position;
            if (!ReadIdentity(*group, current, position) || !SameIdentity(current, expected) ||
                (desired && !SurfaceRule::MatchesMeshGroup(*scene, *group, *desired))) { return false; }
            auto& rules = group->GetRuleContainer();
            if (!current && desired && m_position > rules.GetRuleCount()) { return false; }
            AZStd::shared_ptr<SurfaceRule> replacement;
            if (desired) { replacement = AZStd::make_shared<SurfaceRule>(*desired); }
            if (current) { rules.GetRule(position)->OnUserRemoved(); rules.RemoveRule(position); }
            if (replacement) { replacement->OnUserAdded(); rules.InsertRule(replacement, current ? position : m_position); }
            if (m_changed) { m_changed(*scene, *group); }
            return true;
        }
    private:
        AZStd::weak_ptr<Scene> m_scene;
        AZ::Uuid m_group;
        QPointer<QObject> m_owner;
        AZStd::function<bool()> m_isCurrent;
        AssetImporterDocument::SurfaceIdentityChanged m_changed;
        RuleSnapshot m_before, m_after;
        size_t m_position;
    };
}

AZ::Outcome<bool, AZStd::string> AssetImporterDocument::ApplySurfaceIdentityChange(const AZ::Uuid& groupId,
    const AZStd::optional<SurfaceRule>& proposed, QObject* owner, AZStd::function<bool()> ownerIsCurrent,
    SurfaceIdentityChanged changed)
{
    if (!owner || QThread::currentThread() != owner->thread() || !ownerIsCurrent || !ownerIsCurrent() || m_saveRunner || !m_scene)
    { return AZ::Failure(AZStd::string("Scene Settings is not ready to edit this document.")); }
    auto* group = FindIdentityGroup(*m_scene, groupId);
    if (!group) { return AZ::Failure(AZStd::string("The selected mesh group is missing or has an ambiguous UUID.")); }
    RuleSnapshot before; size_t position;
    if (!ReadIdentity(*group, before, position) || (before && !SurfaceRule::MatchesMeshGroup(*m_scene, *group, *before)) ||
        (proposed && !SurfaceRule::MatchesMeshGroup(*m_scene, *group, *proposed)))
    { return AZ::Failure(AZStd::string("Surface Identity is stale or invalid; no document changes were made.")); }
    if (SameIdentity(before, proposed)) { return AZ::Success(false); }
    AzToolsFramework::ScopedUndoBatch batch("Edit Surface Identity");
    if (!batch.GetUndoBatch()) { return AZ::Failure(AZStd::string("The Editor Undo owner is unavailable.")); }
    auto command = AZStd::make_unique<SurfaceIdentityCommand>(m_scene, groupId, owner, AZStd::move(ownerIsCurrent),
        AZStd::move(changed), before, proposed, position);
    if (!command->Apply(before, proposed)) { return AZ::Failure(AZStd::string("The document changed before the edit could be committed.")); }
    command->SetParent(batch.GetUndoBatch()); command.release();
    return AZ::Success(true);
}
