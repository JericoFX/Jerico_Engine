/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Native adaptation of the JEEditorFixture author_room.py guards and the
 * existing RoomCorridorFixture. Uses WhiteBox's mesh, CSG, Undo and prefab APIs.
 */

#include "JEEditorFixtureAuthoring.h"
#include "EditorWhiteBoxComponent.h"
#include "JEWhiteBoxSurfaceMaterials.h"
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Materials/MaterialComponentConstants.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <EditorToolsApplicationAPI.h>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <cmath>

namespace JE::BSP::Editor
{
    namespace Api = JE::Geometry::Api;
    namespace Prefab = AzToolsFramework::Prefab;

    namespace
    {
        constexpr const char* ActionId = FixtureAuthorActionId;
        constexpr const char* Complete = FixtureCompleteEntityName;
        bool VerificationDispatch = false;
        AZStd::optional<AZ::Outcome<void, AZStd::string>> VerificationOutcome;
        constexpr const char* Partial = "JEFixture_F63AAB68_RoomCorridor_partial";

        AZStd::string Normalized(AZStd::string_view path)
        {
            AZStd::string result(path);
            AZStd::replace(result.begin(), result.end(), '\\', '/');
            return result;
        }

        bool LocalAbsolute(AZStd::string_view path)
        {
            const auto value = Normalized(path);
            return !value.empty() && value.substr(0, 2) != "//" && value.find("://") == AZStd::string::npos &&
                AZ::IO::PathView(value).IsAbsolute();
        }

        // Called only after lexical local-path checks, never on a network/relative path.
        QString CanonicalLocal(AZStd::string_view path)
        {
            if (!LocalAbsolute(path))
            {
                return {};
            }
            QFileInfo info(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
            AZStd::vector<QFileInfo> ancestors;
            for (QFileInfo ancestor = info; ; ancestor = QFileInfo(ancestor.absolutePath()))
            {
                ancestors.push_back(ancestor);
                if (ancestor.filePath() == ancestor.absolutePath())
                {
                    break;
                }
            }
            // Check root-to-leaf before querying the child through a linked ancestor.
            for (auto ancestor = ancestors.rbegin(); ancestor != ancestors.rend(); ++ancestor)
            {
                if (ancestor->isSymLink())
                {
                    return {};
                }
            }
            return QDir::fromNativeSeparators(info.canonicalFilePath());
        }

        bool IsFixtureProject()
        {
            auto registry = AZ::SettingsRegistry::Get();
            AZStd::string id;
            const AZStd::string key = AZStd::string(AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey) + "/project_id";
            return AZ::Utils::GetProjectName() == "JEEditorFixture" && registry && registry->Get(id, key) &&
                AZ::Uuid::CreateString(id.c_str()) == AZ::Uuid::CreateString("{F63AAB68-D01D-46FC-892A-BD9B5828A7CE}");
        }

        Api::WhiteBoxMeshPtr Box(const AZ::Vector3& minimum, const AZ::Vector3& maximum)
        {
            auto mesh = Api::CreateWhiteBoxMesh();
            Api::InitializeAsUnitCube(*mesh);
            for (const auto vertex : Api::MeshVertexHandles(*mesh))
            {
                const auto point = Api::VertexPosition(*mesh, vertex) + AZ::Vector3(0.5f);
                Api::SetVertexPosition(*mesh, vertex, minimum + point * (maximum - minimum));
            }
            Api::CalculateNormals(*mesh);
            Api::CalculatePlanarUVs(*mesh);
            return mesh;
        }

        bool Apply(JE::Geometry::WhiteBoxMesh& target, const JE::Geometry::WhiteBoxMesh& operand, Api::BooleanOperation operation, double volume)
        {
            if (!Api::ApplyMeshBoolean(target, operand, AZ::Transform::CreateIdentity(), operation) ||
                Api::MeshVertexCount(target) == 0 || Api::MeshFaceCount(target) == 0)
            {
                return false;
            }
            double actual = 0.0;
            for (const auto& face : Api::MeshFaces(target))
            {
                actual += static_cast<double>(face[0].Dot(face[1].Cross(face[2]))) / 6.0;
            }
            return std::isfinite(actual) && std::abs(actual - volume) <= 1e-4;
        }
    }

    AZ::Outcome<JE::Render::MaterialAssignmentMap, AZStd::string> PreflightFixtureMaterials()
    {
        if (!AZ::Data::AssetCatalogRequestBus::HasHandlers() || !AzFramework::AssetSystemRequestBus::HasHandlers())
        {
            return AZ::Failure(AZStd::string("Fixture materials require the native asset catalog and Asset Processor connection."));
        }
        bool connected = false, ready = false;
        AzFramework::AssetSystemRequestBus::BroadcastResult(connected,
            &AzFramework::AssetSystem::AssetSystemRequests::ConnectedWithAssetProcessor);
        AzFramework::AssetSystemRequestBus::BroadcastResult(ready,
            &AzFramework::AssetSystem::AssetSystemRequests::AssetProcessorIsReady);
        if (!connected || !ready)
        {
            return AZ::Failure(AZStd::string("Asset Processor is not connected/ready; compile the fixture's grid and metal materials before authoring."));
        }
        JE::Render::MaterialAssignmentMap materials;
        for (const auto slot : {FixtureGridSlot, FixtureMetalSlot})
        {
            const auto* path = slot == FixtureGridSlot ? FixtureGridMaterial : FixtureMetalMaterial;
            AZ::Data::AssetId id;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(id, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                path, azrtti_typeid<AZ::RPI::MaterialAsset>(), false);
            AZ::Data::AssetInfo info;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, id);
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(status,
                &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatus, AZStd::string(path));
            if (!id.IsValid() || info.m_assetType != azrtti_typeid<AZ::RPI::MaterialAsset>() ||
                status != AzFramework::AssetSystem::AssetStatus_Compiled)
            {
                return AZ::Failure(AZStd::string::format("Asset Processor has not produced the required material %s; no fixture entity was created.", path));
            }
            // The native MaterialComponent queues loading and builds instances asynchronously.
            materials.emplace(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, slot), JE::Render::MaterialAssignment(id));
        }
        if (materials.at(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureGridSlot)).m_materialAsset.GetId() ==
            materials.at(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureMetalSlot)).m_materialAsset.GetId())
        {
            return AZ::Failure(AZStd::string("Grid and metal must resolve to distinct material product IDs."));
        }
        return AZ::Success(AZStd::move(materials));
    }

    AZ::Outcome<SurfaceMaterialBindings, AZStd::string> BuildFixtureSurfaceBindings(const JE::Geometry::WhiteBoxMesh& mesh)
    {
        SurfaceMaterialBindings bindings;
        bool upper = false, other = false;
        for (const auto face : Api::MeshFaceHandles(mesh))
        {
            const auto id = Api::FaceSurfaceId(mesh, face);
            const auto normal = Api::FaceNormal(mesh, face);
            if (id.IsNull() || !normal.IsFinite())
            {
                return AZ::Failure(AZStd::string("Fixture face lacks a valid surface identity or normal."));
            }
            const auto slot = normal.Dot(AZ::Vector3::CreateAxisZ()) > 0.999f ? FixtureGridSlot : FixtureMetalSlot;
            upper |= slot == FixtureGridSlot;
            other |= slot == FixtureMetalSlot;
            const auto found = AZStd::find_if(bindings.begin(), bindings.end(), [&](const auto& value) { return value.m_surfaceId == id; });
            if (found == bindings.end()) { bindings.push_back({id, slot}); }
            else if (found->m_materialSlot != slot)
            {
                return AZ::Failure(AZStd::string("One inherited fixture surface spans incompatible material groups; no geometry or identity was changed."));
            }
        }
        if (!upper || !other)
        {
            return AZ::Failure(AZStd::string("Fixture requires nonempty upper (+Z) and remaining-face material groups."));
        }
        return AZ::Success(AZStd::move(bindings));
    }

    bool ValidateFixtureSurfaceMaterials(const JE::Geometry::WhiteBoxMesh& mesh, const SurfaceMaterialBindings& bindings,
        const JE::Render::MaterialAssignmentMap& materials, const JE::Render::MaterialAssignmentMap& expected, AZStd::string& error)
    {
        const auto plan = BuildFixtureSurfaceBindings(mesh);
        if (!plan.IsSuccess()) { error = plan.GetError(); return false; }
        if (bindings.size() != plan.GetValue().size())
        {
            error = "Fixture requires explicit material bindings for every UUID128 surface; legacy gray/fallback is not verification.";
            return false;
        }
        for (const auto& binding : plan.GetValue())
        {
            const auto count = AZStd::count_if(bindings.begin(), bindings.end(), [&](const auto& value) { return value == binding; });
            if (count != 1) { error = "Fixture surface material binding is missing, duplicated or in the wrong slot."; return false; }
        }
        if (materials.size() != 2 || expected.size() != 2)
        {
            error = "Fixture requires an Atom Material owner with exactly two explicit grid/metal slot assignments.";
            return false;
        }
        AZ::Data::AssetId grid, metal;
        for (const auto slot : {FixtureGridSlot, FixtureMetalSlot})
        {
            const auto id = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, slot);
            const auto actual = materials.find(id), required = expected.find(id);
            if (actual == materials.end() || required == expected.end() || !required->second.m_materialAsset.GetId().IsValid() ||
                actual->second.m_materialAsset.GetId() != required->second.m_materialAsset.GetId())
            {
                error = "Fixture slot does not reference its required real material product ID.";
                return false;
            }
            (slot == FixtureGridSlot ? grid : metal) = actual->second.m_materialAsset.GetId();
        }
        if (grid == metal) { error = "Fixture grid/metal product IDs must be distinct."; return false; }
        error.clear();
        return true;
    }

    AZ::Outcome<void, AZStd::string> ConfigureFixtureMaterial(const AZ::EntityComponentIdPair& component)
    {
        using Bus = AzToolsFramework::EditorComponentAPIBus;
        AZStd::vector<AZStd::string> paths;
        Bus::BroadcastResult(paths, &Bus::Events::BuildComponentPropertyList, component);
        const auto set = [&](AZStd::string_view label, const AZStd::any& value)
        {
            AZStd::string matched;
            size_t matches = 0;
            const AZStd::string propertyPath = AZStd::string("White Box Material|") + AZStd::string(label);
            for (const auto& path : paths)
            {
                if (path == propertyPath)
                {
                    matched = path;
                    ++matches;
                }
            }
            if (matches != 1)
            {
                return false;
            }
            AzToolsFramework::EditorComponentAPIRequests::PropertyOutcome outcome;
            Bus::BroadcastResult(outcome, &Bus::Events::SetComponentProperty, component, matched, value);
            bool equal = false;
            Bus::BroadcastResult(equal, &Bus::Events::CompareComponentProperty, component, matched, value);
            return outcome.IsSuccess() && equal;
        };
        if (!set("Use Texture", AZStd::any(false)) || !set("Tint", AZStd::any(AZ::Vector3(0.72f))))
        {
            return AZ::Failure(AZStd::string("WhiteBox tint/texture properties could not be authored and read back."));
        }
        return AZ::Success();
    }

    bool IsFixtureLevelPath(AZStd::string_view project, AZStd::string_view level, AZStd::string_view name)
    {
        if (!LocalAbsolute(project) || !LocalAbsolute(level) || (name != "JEEditorFixture" && name != "JEEditorFixture.prefab"))
        {
            return false;
        }
        const auto directory = (AZ::IO::Path(Normalized(project)) / "Levels/JEEditorFixture").LexicallyNormal();
        const auto current = AZ::IO::Path(Normalized(level)).LexicallyNormal();
        return current == directory || current == directory / "JEEditorFixture.prefab";
    }

    AZ::Outcome<Api::WhiteBoxMeshPtr, AZStd::string> BuildRoomFixture()
    {
        auto room = Box({-5, -4, 0}, {5, 4, 4});
        auto roomInner = Box({-4.75f, -3.75f, 0.25f}, {4.75f, 3.75f, 3.75f});
        if (!Apply(*room, *roomInner, Api::BooleanOperation::Subtraction, 70.625))
        {
            return AZ::Failure(AZStd::string("Native CSG rejected the room shell."));
        }
        auto corridor = Box({-1.5f, -10, 0}, {1.5f, -3.75f, 4});
        auto corridorInner = Box({-1.25f, -9.75f, 0.25f}, {1.25f, -4, 3.75f});
        if (!Apply(*corridor, *corridorInner, Api::BooleanOperation::Subtraction, 24.6875) ||
            !Apply(*room, *corridor, Api::BooleanOperation::Union, 92.3125))
        {
            return AZ::Failure(AZStd::string("Native CSG rejected the corridor shell/union."));
        }
        auto door = Box({-0.75f, -4.25f, 0.25f}, {0.75f, -3.5f, 2.75f});
        auto window = Box({4.5f, -0.75f, 1.25f}, {5.25f, 0.75f, 2.5f});
        if (!Apply(*room, *door, Api::BooleanOperation::Subtraction, 91.375) ||
            !Apply(*room, *window, Api::BooleanOperation::Subtraction, 90.90625))
        {
            return AZ::Failure(AZStd::string("Native CSG rejected the doorway/window."));
        }
        return AZ::Success(AZStd::move(room));
    }

    AZ::Outcome<void, AZStd::string> AuthorRoomFixture()
    {
        if (!IsFixtureProject())
        {
            return AZ::Failure(AZStd::string("Open the JEEditorFixture project before authoring."));
        }
        const auto project = AZ::Utils::GetProjectPath();
        AZStd::string level, name;
        EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(
            level, &EditorInternal::EditorToolsApplicationRequests::GetCurrentLevelPath);
        EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(
            name, &EditorInternal::EditorToolsApplicationRequests::GetCurrentLevelName);
        if (!IsFixtureLevelPath(project, level, name))
        {
            return AZ::Failure(AZStd::string("Open this project's local JEEditorFixture level; other scenes are unchanged."));
        }
        const auto expected = AZ::IO::Path(project) / "Levels/JEEditorFixture/JEEditorFixture.prefab";
        const auto canonicalPrefab = CanonicalLocal(expected.Native());
        const auto canonicalLevel = CanonicalLocal(level);
        if (canonicalPrefab.isEmpty() || canonicalLevel.isEmpty() ||
            (canonicalLevel != canonicalPrefab && canonicalLevel != QFileInfo(canonicalPrefab).absolutePath()))
        {
            return AZ::Failure(AZStd::string("Fixture paths are missing or contain a filesystem link."));
        }
        auto prefab = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        auto loader = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        auto system = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        auto focus = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
        auto ownership = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        auto readOnly = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        if (!prefab || !loader || !system || !focus || !ownership || !readOnly || !ownership->IsRootPrefabAssigned() ||
            !AzToolsFramework::EditorEntitySearchBus::HasHandlers() || !AzToolsFramework::EditorComponentAPIBus::HasHandlers())
        {
            return AZ::Failure(AZStd::string("Native Editor prefab/entity services are not ready."));
        }
        const auto root = prefab->GetLevelInstanceContainerEntityId();
        AzFramework::EntityContextId context;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            context, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
        const auto source = prefab->GetOwningInstancePrefabPath(root);
        const auto sourceText = Normalized(source.Native());
        if (sourceText.empty() || sourceText.substr(0, 2) == "//" || sourceText.find("://") != AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Level prefab source must be a local path."));
        }
        const auto templateId = system->GetTemplateIdFromFilePath(source);
        if (!root.IsValid() || templateId == Prefab::InvalidTemplateId ||
            CanonicalLocal(loader->GetFullPath(source).Native()) != canonicalPrefab ||
            focus->GetFocusedPrefabContainerEntityId(context) != root)
        {
            return AZ::Failure(AZStd::string("Authoring requires the exact level prefab focused at its root."));
        }
        AzToolsFramework::EntitySearchFilter filter;
        filter.m_names = {"JEFixture_F63AAB68_*"};
        filter.m_namesCaseSensitive = true;
        AzToolsFramework::EntityIdList owned;
        AzToolsFramework::EditorEntitySearchBus::BroadcastResult(owned, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);
        if (!owned.empty())
        {
            AZStd::string existingName;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                existingName, owned.front(), &AzToolsFramework::EditorEntityInfoRequests::GetName);
            AzToolsFramework::EditorComponentAPIRequests::GetComponentOutcome existing;
            AzToolsFramework::EditorComponentAPIBus::BroadcastResult(existing,
                &AzToolsFramework::EditorComponentAPIRequests::GetComponentOfType, owned.front(), azrtti_typeid<JE::Geometry::EditorWhiteBoxComponent>());
            JE::Geometry::WhiteBoxMesh* mesh = nullptr;
            if (existing.IsSuccess())
            {
                JE::Geometry::EditorWhiteBoxComponentRequestBus::EventResult(
                    mesh, existing.GetValue(), &JE::Geometry::EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            }
            if (owned.size() == 1 && existingName == Complete && mesh && Api::MeshFaceCount(*mesh) && Api::MeshVertexCount(*mesh))
            {
                AZ_Printf("JEEditorFixture", "Existing fixture preserved, including manual edits and locks.\n");
                return AZ::Success();
            }
            return AZ::Failure(AZStd::string("Partial, empty or ambiguous fixture exists; use Editor Undo/manual inspection."));
        }
        bool locked = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(locked, root, &AzToolsFramework::EditorEntityInfoRequests::IsLocked);
        AzToolsFramework::UndoSystem::URSequencePoint* currentBatch = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            currentBatch, &AzToolsFramework::ToolsApplicationRequests::GetCurrentUndoBatch);
        if (locked || readOnly->IsReadOnly(root) || currentBatch || system->AreDirtyTemplatesPresent(templateId))
        {
            return AZ::Failure(AZStd::string("Unlock the level and save/discard pending edits before authoring; no unrelated changes are saved."));
        }
        const auto fixtureMaterials = PreflightFixtureMaterials();
        if (!fixtureMaterials.IsSuccess()) { return AZ::Failure(fixtureMaterials.GetError()); }
        auto built = BuildRoomFixture();
        if (!built.IsSuccess())
        {
            return AZ::Failure(built.GetError());
        }
        const auto fixtureBindings = BuildFixtureSurfaceBindings(*built.GetValue());
        if (!fixtureBindings.IsSuccess()) { return AZ::Failure(fixtureBindings.GetError()); }
        Api::WhiteBoxMeshStream stream;
        if (!Api::WriteMesh(*built.GetValue(), stream))
        {
            return AZ::Failure(AZStd::string("Native fixture mesh could not be serialized before entity creation."));
        }

        AzToolsFramework::ScopedUndoBatch undo("Author JE room/corridor fixture");
        const auto created = prefab->CreateEntity(root, AZ::Vector3::CreateZero());
        if (!created.IsSuccess())
        {
            return AZ::Failure(created.GetError());
        }
        const auto entity = created.GetValue();
        AzToolsFramework::EditorEntityAPIBus::Event(entity, &AzToolsFramework::EditorEntityAPIRequests::SetName, AZStd::string(Partial));
        const auto commit = [&]() -> AZ::Outcome<void, AZStd::string>
        {
            using Bus = AzToolsFramework::EditorComponentAPIBus;
            AzToolsFramework::EditorComponentAPIRequests::AddComponentsOutcome components;
            Bus::BroadcastResult(components, &Bus::Events::AddComponentsOfType, entity,
                AZ::ComponentTypeList{azrtti_typeid<JE::Geometry::EditorWhiteBoxComponent>(), JE::Render::EditorMaterialComponentTypeId});
            if (!components.IsSuccess() || components.GetValue().size() != 2)
            {
                return AZ::Failure(AZStd::string("Could not add native WhiteBox and Atom Material components."));
            }
            bool enabled = false;
            Bus::BroadcastResult(enabled, &Bus::Events::EnableComponents, components.GetValue());
            AzToolsFramework::EditorComponentAPIRequests::GetComponentOutcome whiteBoxComponent;
            Bus::BroadcastResult(whiteBoxComponent, &Bus::Events::GetComponentOfType, entity, azrtti_typeid<JE::Geometry::EditorWhiteBoxComponent>());
            if (!whiteBoxComponent.IsSuccess() || !JE::Render::MaterialComponentRequestBus::HasHandlers(entity))
            {
                return AZ::Failure(AZStd::string("Native WhiteBox or Atom Material owner is unavailable on the new entity."));
            }
            const auto component = whiteBoxComponent.GetValue();
            JE::Geometry::WhiteBoxMesh* mesh = nullptr;
            JE::Geometry::EditorWhiteBoxComponentRequestBus::EventResult(
                mesh, component, &JE::Geometry::EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            if (!enabled || !mesh || Api::ReadMesh(*mesh, stream) != Api::ReadResult::Full)
            {
                return AZ::Failure(AZStd::string("Could not import the fixture's native mesh."));
            }
            auto material = ConfigureFixtureMaterial(component);
            if (!material.IsSuccess())
            {
                return material;
            }
            AZ::Entity* authoredEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(authoredEntity, &AZ::ComponentApplicationRequests::FindEntity, entity);
            auto* whiteBox = authoredEntity ? authoredEntity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>() : nullptr;
            if (!whiteBox) { return AZ::Failure(AZStd::string("New fixture WhiteBox component could not be accessed.")); }
            Api::PolygonHandles upper, other;
            for (const auto& polygon : Api::MeshPolygonHandles(*mesh))
            {
                if (polygon.m_faceHandles.empty()) { return AZ::Failure(AZStd::string("Fixture contains an empty polygon.")); }
                const auto slot = FindSurfaceMaterialSlot(fixtureBindings.GetValue(), Api::FaceSurfaceId(*mesh, polygon.m_faceHandles.front()));
                for (const auto face : polygon.m_faceHandles)
                {
                    if (FindSurfaceMaterialSlot(fixtureBindings.GetValue(), Api::FaceSurfaceId(*mesh, face)) != slot)
                    { return AZ::Failure(AZStd::string("Fixture polygon spans incompatible material slots.")); }
                }
                (slot == FixtureGridSlot ? upper : other).push_back(polygon);
            }
            if (!whiteBox->AssignPolygonMaterialSlots(upper, FixtureGridSlot) || !whiteBox->AssignPolygonMaterialSlots(other, FixtureMetalSlot))
            { return AZ::Failure(AZStd::string("Could not assign both fixture surface groups using native WhiteBox authoring.")); }
            JE::Render::MaterialComponentRequestBus::Event(entity, &JE::Render::MaterialComponentRequests::SetMaterialMap, fixtureMaterials.GetValue());
            JE::Render::MaterialAssignmentMap readback;
            JE::Render::MaterialComponentRequestBus::EventResult(readback, entity, &JE::Render::MaterialComponentRequests::GetMaterialMapCopy);
            AZStd::string materialError;
            if (!ValidateFixtureSurfaceMaterials(*mesh, whiteBox->GetSurfaceMaterialBindings(), readback, fixtureMaterials.GetValue(), materialError))
            { return AZ::Failure(materialError); }
            JE::Geometry::EditorWhiteBoxComponentNotificationBus::Event(
                component, &JE::Geometry::EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            JE::Geometry::EditorWhiteBoxComponentRequestBus::Event(component, &JE::Geometry::EditorWhiteBoxComponentRequests::SerializeWhiteBox);
            AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entity);
            AzToolsFramework::EditorEntityAPIBus::Event(
                entity, &AzToolsFramework::EditorEntityAPIRequests::SetName, AZStd::string(Complete));
            AZStd::string authoredName;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                authoredName, entity, &AzToolsFramework::EditorEntityInfoRequests::GetName);
            if (authoredName != Complete)
            {
                return AZ::Failure(AZStd::string("Could not assign the fixture's ownership marker."));
            }
            // Capture the serialized mesh/material/name in the native template before saving.
            // This updates the template synchronously; propagation to other instances uses the next tick.
            const auto patch = prefab->GenerateUndoNodesForEntityChangeAndUpdateCache(entity, undo.GetUndoBatch());
            if (!patch.IsSuccess())
            {
                return patch;
            }
            return prefab->SavePrefab(source);
        };
        auto committed = commit();
        if (!committed.IsSuccess())
        {
            // No pre-existing ID is ever deleted. Do not unlock an object to force cleanup.
            bool ownLocked = true;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                ownLocked, entity, &AzToolsFramework::EditorEntityInfoRequests::IsLocked);
            if (!ownLocked && !readOnly->IsReadOnly(entity))
            {
                AzToolsFramework::EditorEntityAPIBus::Event(entity, &AzToolsFramework::EditorEntityAPIRequests::SetName, AZStd::string(Partial));
                const auto cleanup = prefab->DeleteEntitiesAndAllDescendantsInInstance({entity});
                if (!cleanup.IsSuccess())
                {
                    return AZ::Failure(committed.GetError() + " Cleanup failed: " + cleanup.GetError() + "; use Editor Undo.");
                }
            }
            else
            {
                return AZ::Failure(committed.GetError() + "; newly created entity is locked/read-only; use Editor Undo.");
            }
            return committed;
        }
        AZ_Printf("JEEditorFixture", "Native room/corridor authored and prefab saved. Editor runtime verification remains required.\n");
        return AZ::Success();
    }

    bool IsFixtureProjectOpen()
    {
        return IsFixtureProject();
    }

    AZ::Outcome<void, AZStd::string> InvokeFixtureActionForVerification()
    {
        auto* manager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!manager || !manager->IsActionRegistered(ActionId) || VerificationDispatch)
        {
            return AZ::Failure(AZStd::string("Native fixture action is unavailable or already dispatching."));
        }
        const auto enabled = manager->IsActionEnabled(ActionId);
        if (!enabled.IsSuccess() || !enabled.GetValue())
        {
            return AZ::Failure(AZStd::string("Native fixture action is disabled."));
        }
        VerificationOutcome.reset();
        VerificationDispatch = true;
        const auto dispatched = manager->TriggerAction(ActionId);
        VerificationDispatch = false;
        if (!dispatched.IsSuccess() || !VerificationOutcome)
        {
            return AZ::Failure(AZStd::string("Native fixture action did not report an authoring outcome."));
        }
        auto result = AZStd::move(*VerificationOutcome);
        VerificationOutcome.reset();
        return result;
    }

    void RegisterFixtureAction()
    {
        if (!IsFixtureProject())
        {
            return;
        }
        auto manager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!manager)
        {
            return;
        }
        AzToolsFramework::ActionProperties properties;
        properties.m_name = "Author JE Room/Corridor Fixture";
        properties.m_description = "Build the native WhiteBox room in the open JEEditorFixture level and save its prefab.";
        properties.m_category = "JE BSP Authoring";
        const auto registered = manager->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, ActionId, properties, []
        {
            const auto result = AuthorRoomFixture();
            if (VerificationDispatch)
            {
                VerificationOutcome = result;
            }
            else if (!result.IsSuccess())
            {
                QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "JE room fixture", result.GetError().c_str());
            }
        });
        AZ_Error("JEEditorFixture", registered.IsSuccess(), "Action registration failed.");
    }

    void BindFixtureActionToMenu()
    {
        auto manager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        auto menu = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (manager && menu && manager->IsActionRegistered(ActionId))
        {
            const auto added = menu->AddActionToMenu(EditorIdentifiers::ToolsMenuIdentifier, ActionId, 9000);
            AZ_Error("JEEditorFixture", added.IsSuccess(), "Could not add native fixture action to Tools.");
        }
    }
}
