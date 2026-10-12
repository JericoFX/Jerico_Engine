/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "JEEditorFixtureVerification.h"
#include "JEEditorFixtureAuthoring.h"
#include "JEWhiteBoxSurfaceMaterials.h"
#include "EditorWhiteBoxComponent.h"
#include "WhiteBoxComponent.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utility/pair.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <EditorToolsApplicationAPI.h>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#if defined(AZ_PLATFORM_WINDOWS)
#include <AzCore/PlatformIncl.h>
#endif

namespace JE::BSP::Editor
{
    namespace
    {
        constexpr const char* VerifyCommand = "je_verify_room_fixture";
        constexpr const char* CancelCommand = "je_cancel_room_fixture_verification";
        constexpr const char* PlayAction = "o3de.action.game.play";
        constexpr auto RunTimeout = std::chrono::seconds(120);
        constexpr unsigned RenderFramesBeforeCapture = 3;

        QString Text(AZStd::string_view value)
        {
            return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
        }

        bool LocalAbsolute(const QString& path)
        {
            const auto normalized = QDir::fromNativeSeparators(path);
            return !normalized.isEmpty() && !normalized.contains(QChar(0)) && !normalized.startsWith("//") &&
                !normalized.contains("://") && QDir::isAbsolutePath(normalized) &&
                !normalized.split('/').contains("..");
        }

        // Existing ancestors are inspected root-to-leaf before accessing a child through them.
        bool UnlinkedExistingPath(const QString& path)
        {
            if (!LocalAbsolute(path)) { return false; }
            AZStd::vector<QString> ancestors;
            for (QFileInfo current(path); ; current = QFileInfo(current.absolutePath()))
            {
                ancestors.push_back(current.absoluteFilePath());
                if (current.absoluteFilePath() == current.absolutePath()) { break; }
            }
            for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it)
            {
                const QFileInfo info(*it);
                if (!info.exists() || info.isSymLink()) { return false; }
#if defined(AZ_PLATFORM_WINDOWS)
                const auto attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(it->utf16()));
                if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) { return false; }
#endif
            }
            return true;
        }

        FixtureEntityObservation ObserveFixtureEntity(const AZ::Entity& entity, uint32_t index)
        {
            FixtureEntityObservation observation;
            observation.m_id = entity.GetId();
            observation.m_sourceIndex = index;
            observation.m_name = entity.GetName();
            if (entity.FindComponent<JE::Geometry::WhiteBoxComponent>())
            {
                observation.m_componentType = azrtti_typeid<JE::Geometry::WhiteBoxComponent>();
            }
            else if (entity.FindComponent(CameraComponentTypeId))
            {
                observation.m_componentType = CameraComponentTypeId;
            }
            observation.m_active = entity.GetState() == AZ::Entity::State::Active;
            observation.m_spawnTicket = entity.GetEntitySpawnTicketId();
            AzFramework::EntityIdContextQueryBus::EventResult(observation.m_context, entity.GetId(),
                &AzFramework::EntityIdContextQueries::GetOwningContextId);
            return observation;
        }

        bool RunningGame()
        {
            bool running = false, requesting = false;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                running, &AzToolsFramework::EditorEntityContextRequests::IsEditorRunningGame);
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                requesting, &AzToolsFramework::EditorEntityContextRequests::IsEditorRequestingGame);
            return !IsFixtureGameModeReady(false, running, requesting);
        }

        JE::Geometry::EditorWhiteBoxComponent* RoomComponent(AZ::EntityId& room)
        {
            AzToolsFramework::EntitySearchFilter filter;
            filter.m_names = {FixtureCompleteEntityName};
            filter.m_namesCaseSensitive = true;
            auto* prefab = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
            if (!prefab) { return nullptr; }
            filter.m_roots = {prefab->GetLevelInstanceContainerEntityId()};
            AzToolsFramework::EntityIdList found;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(
                found, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);
            if (found.size() != 1) { return nullptr; }
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, found.front());
            auto* component = entity ? entity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>() : nullptr;
            auto* mesh = component ? component->GetWhiteBoxMesh() : nullptr;
            if (!mesh || !JE::Geometry::Api::MeshFaceCount(*mesh) || !JE::Geometry::Api::MeshVertexCount(*mesh)) { return nullptr; }
            room = found.front();
            return component;
        }

        bool ReadFallback(const AZ::EntityComponentIdPair& component, AZ::Vector3& tint, bool& useTexture)
        {
            using Bus = AzToolsFramework::EditorComponentAPIBus;
            AZStd::vector<AZStd::string> paths;
            Bus::BroadcastResult(paths, &Bus::Events::BuildComponentPropertyList, component);
            unsigned tintCount = 0, textureCount = 0;
            for (const auto& path : paths)
            {
                const bool isTint = path == "White Box Material|Tint";
                const bool isTexture = path == "White Box Material|Use Texture";
                if (!isTint && !isTexture) { continue; }
                AzToolsFramework::EditorComponentAPIRequests::PropertyOutcome value;
                Bus::BroadcastResult(value, &Bus::Events::GetComponentProperty, component, path);
                if (!value.IsSuccess()) { return false; }
                if (isTint)
                {
                    const auto* typed = AZStd::any_cast<AZ::Vector3>(&value.GetValue());
                    if (!typed) { return false; }
                    tint = *typed;
                    ++tintCount;
                }
                else
                {
                    const auto* typed = AZStd::any_cast<bool>(&value.GetValue());
                    if (!typed) { return false; }
                    useTexture = *typed;
                    ++textureCount;
                }
            }
            return tintCount == 1 && textureCount == 1;
        }
    }

    AZ::Outcome<void, AZStd::string> ValidateFixtureVerificationOutputPath(
        AZStd::string_view engine, AZStd::string_view output)
    {
        const auto engineText = Text(engine);
        const auto outputText = Text(output);
        if (!LocalAbsolute(engineText) || !LocalAbsolute(outputText))
        {
            return AZ::Failure(AZStd::string("Evidence requires absolute local paths without URI, UNC or traversal."));
        }
        const auto allowed = QDir::cleanPath(QDir::fromNativeSeparators(engineText) + "/build/n0-validation");
        const auto candidate = QDir::cleanPath(QDir::fromNativeSeparators(outputText));
        if (!candidate.startsWith(allowed + '/'))
        {
            return AZ::Failure(AZStd::string("Evidence must be a new directory inside this checkout's build/n0-validation."));
        }
        const QFileInfo info(candidate);
        if (!UnlinkedExistingPath(allowed) || !UnlinkedExistingPath(info.absolutePath()) ||
            !QFileInfo(info.absolutePath()).isDir() || info.exists() || info.isSymLink())
        {
            return AZ::Failure(AZStd::string("Evidence directory exists, its parent is missing, or an ancestor is linked/reparse."));
        }
        return AZ::Success();
    }

    bool HasFixtureCaptureContent(const QImage& image)
    {
        if (image.isNull() || image.width() < 2 || image.height() < 2) { return false; }
        const auto first = image.pixel(0, 0) & 0x00ffffffu;
        for (int y = 0; y < image.height(); ++y)
        {
            for (int x = 0; x < image.width(); ++x)
            {
                if ((image.pixel(x, y) & 0x00ffffffu) != first) { return true; }
            }
        }
        return false;
    }

    AZ::Outcome<FixtureSourceIndices, AZStd::string> ResolveFixtureSourceIndices(
        AZStd::span<const FixtureEntityObservation> entities, AZStd::string_view roomName, AZStd::string_view cameraName)
    {
        FixtureSourceIndices result;
        size_t rooms = 0, cameras = 0;
        for (const auto& entity : entities)
        {
            if (entity.m_name == roomName && entity.m_componentType == azrtti_typeid<JE::Geometry::WhiteBoxComponent>())
            {
                result.m_room = entity.m_sourceIndex;
                ++rooms;
            }
            if (entity.m_name == cameraName && entity.m_componentType == CameraComponentTypeId)
            {
                result.m_camera = entity.m_sourceIndex;
                ++cameras;
            }
        }
        if (roomName.empty() || cameraName.empty() || rooms != 1 || cameras != 1 || result.m_room == result.m_camera)
        {
            return AZ::Failure(AZStd::string("Root.spawnable source room/camera identity is missing or ambiguous"));
        }
        return AZ::Success(result);
    }

    AZ::Outcome<FixtureRuntimeIdentity, AZStd::string> ResolveFixtureRuntimeIdentity(
        AZStd::span<const FixtureEntityObservation> entities, FixtureSourceIndices indices, AZStd::string_view roomName,
        AZStd::string_view cameraName, const AZ::Uuid& runtimeContext, uint32_t spawnTicket)
    {
        FixtureRuntimeIdentity result;
        size_t rooms = 0, cameras = 0;
        if (runtimeContext.IsNull() || spawnTicket == 0 || indices.m_room == indices.m_camera)
        {
            return AZ::Failure(AZStd::string("Runtime context, spawn ticket or source indices are invalid"));
        }
        for (const auto& entity : entities)
        {
            const bool room = entity.m_sourceIndex == indices.m_room;
            const bool camera = entity.m_sourceIndex == indices.m_camera;
            if (!room && !camera) { continue; }
            if (!entity.m_id.IsValid() || !entity.m_active || entity.m_context != runtimeContext || entity.m_spawnTicket != spawnTicket ||
                entity.m_name != (room ? roomName : cameraName) ||
                entity.m_componentType != (room ? azrtti_typeid<JE::Geometry::WhiteBoxComponent>() : CameraComponentTypeId))
            {
                return AZ::Failure(AZStd::string("Runtime source-index entity has wrong state/context/ticket/type/name"));
            }
            if (room) { result.m_room = entity.m_id; ++rooms; }
            else { result.m_camera = entity.m_id; ++cameras; }
        }
        if (rooms != 1 || cameras != 1 || result.m_room == result.m_camera)
        {
            return AZ::Failure(AZStd::string("Runtime room/camera source index is missing or duplicated on the original ticket"));
        }
        return AZ::Success(result);
    }

    bool IsFixtureGameModeReady(bool runtime, bool running, bool requesting)
    {
        return runtime ? running : !running && !requesting;
    }

    class FixtureVerification::Implementation final
        : public AZ::TickBus::Handler
        , public AZ::RPI::ViewportContextNotificationBus::Handler
        , public AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        ~Implementation() override { Deactivate(); }

        void RegisterCommands()
        {
            if (m_registered || !AzFramework::CommandRegistrationBus::HasHandlers()) { return; }
            bool registered = false;
            AzFramework::CommandRegistrationBus::BroadcastResult(registered,
                &AzFramework::CommandRegistration::RegisterCommand, VerifyCommand,
                "Verify the exact JE fixture through native Editor/Atom; argument is a new local build/n0-validation directory.",
                AzFramework::CommandFlags::NoValue,
                [this](const AZStd::vector<AZStd::string_view>& args)
                {
                    // Native CommandRegistration callbacks include the command identifier in args[0].
                    if (args.size() != 2) { return AzFramework::CommandResult::ErrorWrongNumberOfArguments; }
                    return Start(args[1]) ? AzFramework::CommandResult::Success : AzFramework::CommandResult::Error;
                });
            if (!registered) { AZ_Error("JEEditorFixture", false, "Could not register native verification command."); return; }
            AzFramework::CommandRegistrationBus::BroadcastResult(m_registered,
                &AzFramework::CommandRegistration::RegisterCommand, CancelCommand, "Cancel the active JE fixture verification.",
                AzFramework::CommandFlags::NoValue,
                [this](const AZStd::vector<AZStd::string_view>& args)
                {
                    if (args.size() != 1) { return AzFramework::CommandResult::ErrorWrongNumberOfArguments; }
                    if (m_stage != Stage::Idle) { Finish(false, "Explicit cancellation."); }
                    return AzFramework::CommandResult::Success;
                });
            if (!m_registered)
            {
                AzFramework::CommandRegistrationBus::Broadcast(&AzFramework::CommandRegistration::UnregisterCommand, VerifyCommand);
                AZ_Error("JEEditorFixture", false, "Could not register native cancellation command.");
            }
        }

        void Deactivate()
        {
            if (m_stage != Stage::Idle) { Finish(false, "Editor WhiteBox deactivated during verification."); }
            if (m_registered)
            {
                AzFramework::CommandRegistrationBus::Broadcast(&AzFramework::CommandRegistration::UnregisterCommand, VerifyCommand);
                AzFramework::CommandRegistrationBus::Broadcast(&AzFramework::CommandRegistration::UnregisterCommand, CancelCommand);
                m_registered = false;
            }
        }

    private:
        enum class Stage { Idle, Services, Loaded, Reopened, EditorView, EditorCapture, RuntimeView, RuntimeCapture, Stopped };

        bool Start(AZStd::string_view output)
        {
            if (m_stage != Stage::Idle || !IsFixtureProjectOpen())
            {
                AZ_Error("JEEditorFixture", false, "Verification requires the exact fixture project and no active verification.");
                return false;
            }
            const auto engine = AZ::Utils::GetEnginePath();
            const auto valid = ValidateFixtureVerificationOutputPath(engine, output);
            if (!valid.IsSuccess()) { AZ_Error("JEEditorFixture", false, "%s", valid.GetError().c_str()); return false; }
            const QFileInfo destination(Text(output));
            if (!QDir(destination.absolutePath()).mkdir(destination.fileName()))
            {
                AZ_Error("JEEditorFixture", false, "Could not exclusively create the new evidence directory.");
                return false;
            }
            m_output = QDir::fromNativeSeparators(destination.canonicalFilePath());
            m_level = (AZ::IO::Path(AZ::Utils::GetProjectPath()) / "Levels/JEEditorFixture/JEEditorFixture.prefab").Native();
            m_deadline = std::chrono::steady_clock::now() + RunTimeout;
            m_startedUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            m_stage = Stage::Services;
            m_enteredGame = false;
            m_selectedCamera = false;
            m_captureId = AZ::Render::InvalidFrameCaptureId;
            m_editorCaptureId = m_runtimeCaptureId = AZ::Render::InvalidFrameCaptureId;
            m_editorCaptured = m_runtimeCaptured = false;
            m_captureFinished = false;
            m_captureResult = AZ::Render::FrameCaptureResult::None;
            m_captureInfo.clear();
            m_capturePath.clear();
            m_backend.clear();
            m_width = m_height = 0;
            m_room = m_camera = m_previousView = AZ::EntityId();
            m_runtimeIdentity.reset();
            m_runtimeIdentityFailed = false;
            m_sourceCameraName.clear();
            m_lastReadinessGate = "waiting for native Editor services";
            m_hasMaterialOwner = m_manualUVs = m_useTexture = false;
            m_tint = AZ::Vector3::CreateZero();
            m_frames = 0;
            m_snapshot.clear();
            m_bindings.clear();
            m_materials.clear();
            m_expectedMaterials.clear();
            m_editorGridId = m_editorMetalId = m_runtimeGridId = m_runtimeMetalId = AZ::Data::AssetId();
            AZ::TickBus::Handler::BusConnect();
            AZ_Printf("JEEditorFixture", "Native verification scheduled, timeout 120 seconds, evidence %s. This is not visual acceptance.\n",
                m_output.toUtf8().constData());
            return true;
        }

        bool CleanScene()
        {
            auto* prefab = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
            auto* system = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
            if (!prefab || !system) { return false; }
            const auto root = prefab->GetLevelInstanceContainerEntityId();
            if (root.IsValid())
            {
                const auto source = prefab->GetOwningInstancePrefabPath(root);
                const auto id = system->GetTemplateIdFromFilePath(source);
                if (id == AzToolsFramework::Prefab::InvalidTemplateId || system->AreDirtyTemplatesPresent(id)) { return false; }
            }
            AzToolsFramework::UndoSystem::URSequencePoint* batch = nullptr;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                batch, &AzToolsFramework::ToolsApplicationRequests::GetCurrentUndoBatch);
            return batch == nullptr;
        }

        bool SnapshotRoom(bool compare)
        {
            auto* component = RoomComponent(m_room);
            if (!component) { return false; }
            JE::Geometry::Api::WhiteBoxMeshStream stream;
            if (!JE::Geometry::Api::WriteMesh(*component->GetWhiteBoxMesh(), stream)) { return false; }
            JE::Render::MaterialAssignmentMap materials;
            JE::Render::MaterialComponentRequestBus::EventResult(materials, m_room,
                &JE::Render::MaterialComponentRequests::GetMaterialMapCopy);
            const bool hasOwner = JE::Render::MaterialComponentRequestBus::HasHandlers(m_room);
            AZStd::string materialError;
            if (!hasOwner || !ValidateFixtureSurfaceMaterials(*component->GetWhiteBoxMesh(), component->GetSurfaceMaterialBindings(),
                materials, m_expectedMaterials, materialError))
            {
                AZ_Warning("JEEditorFixture", false, "Fixture material verification rejected: %s",
                    hasOwner ? materialError.c_str() : "Atom Material owner is missing. Existing/manual fixture was not retrofitted.");
                return false;
            }
            AZ::Vector3 tint;
            bool useTexture = false;
            if (!ReadFallback({m_room, component->GetId()}, tint, useTexture)) { return false; }
            if (compare)
            {
                return stream == m_snapshot && component->GetSurfaceMaterialBindings() == m_bindings &&
                    component->HasManualUVs() == m_manualUVs && hasOwner == m_hasMaterialOwner &&
                    MaterialAssignmentsEquivalent(materials, m_materials) && tint == m_tint && useTexture == m_useTexture;
            }
            m_snapshot = AZStd::move(stream);
            m_bindings = component->GetSurfaceMaterialBindings();
            m_materials = AZStd::move(materials);
            m_manualUVs = component->HasManualUVs();
            m_hasMaterialOwner = hasOwner;
            m_tint = tint;
            m_useTexture = useTexture;
            return true;
        }

        bool SelectExistingCamera()
        {
            AzToolsFramework::EntitySearchFilter filter;
            filter.m_names = {"Camera"};
            filter.m_namesCaseSensitive = true;
            filter.m_roots = {AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get()->GetLevelInstanceContainerEntityId()};
            AzToolsFramework::EntityIdList found;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(found,
                &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);
            if (found.size() != 1 || !Camera::CameraRequestBus::HasHandlers(found.front())) { return false; }
            m_camera = found.front();
            AZ::Entity* sourceCamera = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(sourceCamera, &AZ::ComponentApplicationRequests::FindEntity, m_camera);
            if (!sourceCamera) { return false; }
            m_sourceCameraName = sourceCamera->GetName();
            Camera::EditorCameraRequestBus::BroadcastResult(m_previousView, &Camera::EditorCameraRequests::GetCurrentViewEntityId);
            // Select the existing camera's view; never set its transform, FoV or component properties.
            Camera::EditorCameraRequestBus::Broadcast(&Camera::EditorCameraRequests::SetViewFromEntityPerspective, m_camera);
            m_selectedCamera = true;
            return true;
        }

        struct RuntimeIdentityRequest
        {
            AZStd::mutex m_mutex;
            bool m_cancelled = false;
            bool m_completed = false;
            AZStd::string m_error;
            FixtureSourceIndices m_indices;
            FixtureRuntimeIdentity m_identity;
            AZ::Uuid m_context;
            uint32_t m_ticket = 0;
        };

        bool NotReady(AZStd::string_view gate)
        {
            m_lastReadinessGate = gate;
            return false;
        }

        bool ResolveRuntimeIdentity(AZ::EntityId& room, AZ::EntityId& camera)
        {
            if (!m_runtimeIdentity)
            {
                AZ::Uuid context;
                AzFramework::GameEntityContextRequestBus::BroadcastResult(context,
                    &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
                auto* ownership = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                auto* spawnables = AzFramework::SpawnableEntitiesInterface::Get();
                if (context.IsNull() || !ownership || !spawnables) { return NotReady("runtime context/ownership/spawnable interface unavailable"); }
                const auto& assets = ownership->GetPlayInEditorAssetData();
                const auto root = assets.find(AzFramework::Spawnable::DefaultMainSpawnableName);
                if (root == assets.end()) { return NotReady("current play-in-editor Root.spawnable asset unavailable"); }
                AZ::Entity* runtimeRoom = nullptr;
                size_t rooms = 0;
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities,
                    [&runtimeRoom, &rooms, &context](AZ::Entity* entity)
                    {
                        if (!entity || entity->GetName() != FixtureCompleteEntityName || !entity->FindComponent<JE::Geometry::WhiteBoxComponent>()) { return; }
                        const auto observed = ObserveFixtureEntity(*entity, 0);
                        if (observed.m_active && observed.m_context == context)
                        {
                            runtimeRoom = entity;
                            ++rooms;
                        }
                    });
                if (rooms != 1 || !runtimeRoom->GetEntitySpawnTicketId())
                {
                    m_runtimeIdentityFailed = rooms > 1;
                    return NotReady(rooms > 1 ? "duplicate active fixture rooms in game context" : "active fixture room/spawn ticket unavailable in game context");
                }
                auto state = AZStd::make_shared<RuntimeIdentityRequest>();
                state->m_context = context;
                state->m_ticket = runtimeRoom->GetEntitySpawnTicketId();
                m_runtimeIdentity = state;
                const auto rootAssetId = root->second.m_spawnableAssetId;
                const auto cameraName = m_sourceCameraName;
                // One queued lookup for this run. No callback owns this/controller or edits scene/camera state.
                auto publish = [state](AZ::Outcome<FixtureRuntimeIdentity, AZStd::string> result)
                {
                    AZStd::lock_guard lock(state->m_mutex);
                    if (state->m_cancelled) { return; }
                    state->m_completed = true;
                    if (result.IsSuccess()) { state->m_identity = result.GetValue(); }
                    else { state->m_error = result.GetError(); }
                };
                spawnables->RetrieveTicket(state->m_ticket,
                    [state, rootAssetId, cameraName, publish](AzFramework::EntitySpawnTicket&& ticket)
                    {
                        {
                            AZStd::lock_guard lock(state->m_mutex);
                            if (state->m_cancelled) { return; }
                        }
                        const auto* asset = ticket.GetSpawnable();
                        if (!ticket.IsValid() || !asset || !asset->IsReady() || asset->GetId() != rootAssetId)
                        {
                            publish(AZ::Failure(AZStd::string("fixture ticket does not belong to the current Root.spawnable")));
                            return;
                        }
                        AZStd::vector<FixtureEntityObservation> prototypes;
                        const auto& entities = (*asset)->GetEntities();
                        for (size_t index = 0; index < entities.size(); ++index)
                        {
                            if (entities[index]) { prototypes.push_back(ObserveFixtureEntity(*entities[index], aznumeric_cast<uint32_t>(index))); }
                        }
                        const auto indices = ResolveFixtureSourceIndices(prototypes, FixtureCompleteEntityName, cameraName);
                        if (!indices.IsSuccess()) { publish(AZ::Failure(indices.GetError())); return; }
                        {
                            AZStd::lock_guard lock(state->m_mutex);
                            if (state->m_cancelled) { return; }
                            state->m_indices = indices.GetValue();
                        }
                        auto* currentSpawnables = AzFramework::SpawnableEntitiesInterface::Get();
                        if (!currentSpawnables) { publish(AZ::Failure(AZStd::string("spawnable interface disappeared during ticket lookup"))); return; }
                        currentSpawnables->ListIndicesAndEntities(ticket,
                            [state, cameraName, publish](AzFramework::EntitySpawnTicket::Id ticketId, AzFramework::SpawnableConstIndexEntityContainerView view)
                            {
                                {
                                    AZStd::lock_guard lock(state->m_mutex);
                                    if (state->m_cancelled) { return; }
                                }
                                if (ticketId != state->m_ticket) { publish(AZ::Failure(AZStd::string("runtime entity listing returned another ticket"))); return; }
                                AZStd::vector<FixtureEntityObservation> observations;
                                for (auto iterator = view.begin(); iterator != view.end(); ++iterator)
                                {
                                    if (const auto* entity = iterator->GetEntity())
                                    {
                                        observations.push_back(ObserveFixtureEntity(*entity, iterator->GetIndex()));
                                    }
                                }
                                publish(ResolveFixtureRuntimeIdentity(observations, state->m_indices, FixtureCompleteEntityName,
                                    cameraName, state->m_context, state->m_ticket));
                            });
                    });
                return NotReady("waiting for original Root.spawnable ticket/source-index identity");
            }
            AZStd::lock_guard lock(m_runtimeIdentity->m_mutex);
            if (!m_runtimeIdentity->m_completed) { return NotReady("waiting for original Root.spawnable ticket/source-index identity"); }
            if (!m_runtimeIdentity->m_error.empty())
            {
                m_runtimeIdentityFailed = true;
                return NotReady(m_runtimeIdentity->m_error);
            }
            room = m_runtimeIdentity->m_identity.m_room;
            camera = m_runtimeIdentity->m_identity.m_camera;
            AZStd::vector<FixtureEntityObservation> observations;
            for (const auto& entry : {AZStd::make_pair(room, m_runtimeIdentity->m_indices.m_room), AZStd::make_pair(camera, m_runtimeIdentity->m_indices.m_camera)})
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry.first);
                if (entity) { observations.push_back(ObserveFixtureEntity(*entity, entry.second)); }
            }
            const auto live = ResolveFixtureRuntimeIdentity(observations, m_runtimeIdentity->m_indices, FixtureCompleteEntityName,
                m_sourceCameraName, m_runtimeIdentity->m_context, m_runtimeIdentity->m_ticket);
            return live.IsSuccess() || NotReady(live.GetError());
        }

        bool MaterialsReady(AZ::EntityId entity, bool runtime)
        {
            if (!JE::Render::MaterialComponentRequestBus::HasHandlers(entity) ||
                !JE::Render::MaterialConsumerRequestBus::HasHandlers(entity)) { return NotReady("Atom Material owner/consumer bus unavailable"); }
            JE::Render::MaterialAssignmentMap materialSlots;
            JE::Render::MaterialConsumerRequestBus::EventResult(materialSlots, entity,
                &JE::Render::MaterialConsumerRequests::GetDefaultMaterialMap);
            // Check actual generated model slots, not unused entries on a Material owner.
            if (materialSlots.find(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureGridSlot)) == materialSlots.end() ||
                materialSlots.find(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, FixtureMetalSlot)) == materialSlots.end() ||
                materialSlots.find(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, 0)) != materialSlots.end()) { return NotReady("generated model does not have exactly the fixture grid/metal slots"); }
            JE::Render::MaterialAssignmentMap materials;
            JE::Render::MaterialComponentRequestBus::EventResult(materials, entity,
                &JE::Render::MaterialComponentRequests::GetMaterialMapCopy);
            if (!MaterialAssignmentsEquivalent(materials, m_materials)) { return NotReady("material assignments differ from saved Editor snapshot"); }
            AZ::Data::AssetId grid, metal;
            for (const auto slot : {FixtureGridSlot, FixtureMetalSlot})
            {
                const auto id = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, slot);
                const auto assignment = materials.find(id), expected = m_expectedMaterials.find(id);
                if (assignment == materials.end() || expected == m_expectedMaterials.end() ||
                    assignment->second.m_materialAsset.GetId() != expected->second.m_materialAsset.GetId()) { return NotReady("grid/metal slot has wrong or missing material asset ID"); }
                bool ready = false;
                JE::Render::MaterialComponentRequestBus::EventResult(ready, entity,
                    &JE::Render::MaterialComponentRequests::IsMaterialAssetReady, id);
                const auto& instance = assignment->second.m_materialInstance;
                if (!ready || !assignment->second.m_materialAsset.IsReady() || !instance || instance->NeedsCompile() ||
                    instance->GetAsset().GetId() != expected->second.m_materialAsset.GetId()) { return NotReady("grid/metal material asset or matching compiled instance not ready"); }
                (slot == FixtureGridSlot ? grid : metal) = assignment->second.m_materialAsset.GetId();
            }
            if (!grid.IsValid() || !metal.IsValid() || grid == metal) { return NotReady("grid/metal material IDs are invalid or not distinct"); }
            (runtime ? m_runtimeGridId : m_editorGridId) = grid;
            (runtime ? m_runtimeMetalId : m_editorMetalId) = metal;
            return true;
        }

        bool RenderReady(bool runtime)
        {
            auto* requests = AZ::RPI::ViewportContextRequests::Get();
            const auto viewport = requests ? requests->GetDefaultViewportContext() : nullptr;
            if (!viewport || !viewport->GetWindowHandle() || !viewport->GetRenderScene() || !viewport->GetCurrentPipeline() ||
                !viewport->GetDefaultView() || !AZ::RHI::Factory::IsReady()) { return NotReady("viewport window/scene/pipeline/view/RHI unavailable"); }
            const auto size = viewport->GetViewportSize();
            if (!size.m_width || !size.m_height) { return NotReady("viewport dimensions are zero"); }
            bool canCapture = false;
            AZ::Render::FrameCaptureRequestBus::BroadcastResult(canCapture, &AZ::Render::FrameCaptureRequests::CanCapture);
            if (!canCapture) { return NotReady("Atom frame capture is unavailable"); }
            AZ::EntityId activeCamera;
            Camera::CameraSystemRequestBus::BroadcastResult(activeCamera, &Camera::CameraSystemRequests::GetActiveCamera);
            if (runtime)
            {
                bool running = false, requesting = true;
                AZ::EntityId camera, room;
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(running,
                    &AzToolsFramework::EditorEntityContextRequests::IsEditorRunningGame);
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(requesting,
                    &AzToolsFramework::EditorEntityContextRequests::IsEditorRequestingGame);
                if (!IsFixtureGameModeReady(true, running, requesting)) { return NotReady("Editor has not entered running game mode"); }
                if (!ResolveRuntimeIdentity(room, camera)) { return false; }
                AZ::Entity* runtimeEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(runtimeEntity, &AZ::ComponentApplicationRequests::FindEntity, room);
                auto* whiteBox = runtimeEntity ? runtimeEntity->FindComponent<JE::Geometry::WhiteBoxComponent>() : nullptr;
                if (!whiteBox || !whiteBox->WhiteBoxIsVisible()) { return NotReady("runtime WhiteBox mesh missing or invisible"); }
                if (activeCamera != camera) { return NotReady("active camera differs from the original Root.spawnable camera instance"); }
                if (!MaterialsReady(room, true)) { return false; }
            }
            else
            {
                AZ::EntityId viewed;
                Camera::EditorCameraRequestBus::BroadcastResult(viewed, &Camera::EditorCameraRequests::GetCurrentViewEntityId);
                AZ::EntityId room;
                auto* component = RoomComponent(room);
                if (RunningGame() || viewed != m_camera || activeCamera != m_camera || !component || !component->HasRenderMesh() ||
                    room != m_room) { return NotReady("Editor fixture mesh or original selected/active camera unavailable"); }
                if (!MaterialsReady(room, false)) { return false; }
            }
            if (!AZ::RPI::ViewportContextNotificationBus::Handler::BusIsConnected())
            {
                AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(viewport->GetName());
            }
            m_backend = AZ::RHI::Factory::Get().GetName().GetCStr();
            m_width = size.m_width;
            m_height = size.m_height;
            m_lastReadinessGate = "waiting for three rendered frames of the validated view";
            return true;
        }

        bool Capture(bool runtime)
        {
            m_capturePath = m_output + (runtime ? "/runtime.png" : "/editor.png");
            const QFileInfo destination(m_capturePath);
            if (!UnlinkedExistingPath(m_output) || destination.exists() || destination.isSymLink()) { return false; }
            const auto viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
            AZ::Render::FrameCaptureOutcome result;
            AZ::Render::FrameCaptureRequestBus::BroadcastResult(result,
                &AZ::Render::FrameCaptureRequests::CaptureScreenshotForWindow,
                AZStd::string(m_capturePath.toUtf8().constData()), viewport->GetWindowHandle());
            if (!result.IsSuccess() || result.GetValue() == AZ::Render::InvalidFrameCaptureId)
            {
                AZ_Error("JEEditorFixture", false, "Atom capture rejected: %s", result.IsSuccess() ? "invalid ID" : result.GetError().m_errorMessage.c_str());
                return false;
            }
            m_captureId = result.GetValue();
            if (runtime) { m_runtimeCaptureId = m_captureId; }
            else { m_editorCaptureId = m_captureId; }
            m_captureFinished = false;
            m_stage = runtime ? Stage::RuntimeCapture : Stage::EditorCapture;
            m_lastReadinessGate = "waiting for matching Atom capture notification";
            AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(m_captureId);
            AZ_Printf("JEEditorFixture", "Atom %s capture ID %u, backend %s, %ux%u, path %s.\n",
                runtime ? "runtime" : "Editor", m_captureId, m_backend.c_str(), m_width, m_height, m_capturePath.toUtf8().constData());
            return true;
        }

        void OnRenderTick() override { if (m_stage == Stage::EditorView || m_stage == Stage::RuntimeView) { ++m_frames; } }

        void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override
        {
            const auto* id = AZ::Render::FrameCaptureNotificationBus::GetCurrentBusId();
            if (!id || *id != m_captureId || (m_stage != Stage::EditorCapture && m_stage != Stage::RuntimeCapture)) { return; }
            m_captureFinished = true;
            m_captureResult = result;
            m_captureInfo = info;
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
        }

        void OnTick(float, AZ::ScriptTimePoint) override
        {
            if (std::chrono::steady_clock::now() >= m_deadline) { Finish(false, (AZStd::string("Native verification timed out (120 seconds); pending gate: ") + m_lastReadinessGate).c_str()); return; }
            if (!IsFixtureProjectOpen()) { Finish(false, "Fixture project changed during verification."); return; }
            if (!UnlinkedExistingPath(m_output)) { Finish(false, "Evidence directory disappeared or became linked/reparse."); return; }
            if (m_stage == Stage::Services)
            {
                if (!EditorInternal::EditorToolsApplicationRequestBus::HasHandlers() ||
                    !AzToolsFramework::EditorEntityContextRequestBus::HasHandlers() ||
                    !AzToolsFramework::EditorEntitySearchBus::HasHandlers() ||
                    !AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get() ||
                    !AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()) { return; }
                if (RunningGame() || !CleanScene()) { Finish(false, "Exit play mode and save/discard pending edits before verification; no edits were discarded."); return; }
                AZStd::string level, name;
                EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(level, &EditorInternal::EditorToolsApplicationRequests::GetCurrentLevelPath);
                EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(name, &EditorInternal::EditorToolsApplicationRequests::GetCurrentLevelName);
                if (!level.empty() && !IsFixtureLevelPath(AZ::Utils::GetProjectPath(), level, name))
                { Finish(false, "A different level is open; verification will not replace it."); return; }
                if (!UnlinkedExistingPath(Text(m_level))) { Finish(false, "Fixture level is missing or linked/reparse."); return; }
                bool opened = false;
                EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(opened, &EditorInternal::EditorToolsApplicationRequests::OpenLevel, m_level);
                if (!opened) { Finish(false, "Native OpenLevel rejected the existing fixture."); return; }
                m_stage = Stage::Loaded;
            }
            else if (m_stage == Stage::Loaded)
            {
                auto* ownership = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                auto* manager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
                if (!ownership || !ownership->IsRootPrefabAssigned() || !manager || !manager->IsActionRegistered(FixtureAuthorActionId)) { return; }
                const auto materials = PreflightFixtureMaterials();
                if (!materials.IsSuccess()) { Finish(false, materials.GetError().c_str()); return; }
                m_expectedMaterials = materials.GetValue();
                const auto authored = InvokeFixtureActionForVerification();
                if (!authored.IsSuccess()) { Finish(false, authored.GetError().c_str()); return; }
                if (!SnapshotRoom(false) || !CleanScene()) { Finish(false, "Authoring did not yield one completed, saved fixture with an Atom Material owner and both exact face-material assignments. Existing/manual fixtures are not retrofitted."); return; }
                bool reopened = false;
                EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(reopened, &EditorInternal::EditorToolsApplicationRequests::OpenLevel, m_level);
                if (!reopened) { Finish(false, "Native fixture reopening failed."); return; }
                m_stage = Stage::Reopened;
            }
            else if (m_stage == Stage::Reopened)
            {
                auto* ownership = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                if (!ownership || !ownership->IsRootPrefabAssigned()) { return; }
                if (!SnapshotRoom(true) || !CleanScene()) { Finish(false, "Reopened fixture differs in mesh/UV/identity, material bindings/overrides, fallback or manual-UV state."); return; }
                if (!SelectExistingCamera()) { Finish(false, "The fixture's unique existing Camera is unavailable."); return; }
                m_stage = Stage::EditorView;
            }
            else if (m_stage == Stage::EditorView || m_stage == Stage::RuntimeView)
            {
                const bool runtime = m_stage == Stage::RuntimeView;
                if (!RenderReady(runtime))
                {
                    m_frames = 0;
                    if (runtime && m_runtimeIdentityFailed) { Finish(false, m_lastReadinessGate.c_str()); }
                    return;
                }
                if (m_frames >= RenderFramesBeforeCapture && !Capture(runtime)) { Finish(false, "Could not schedule the native viewport capture."); }
            }
            else if (m_stage == Stage::EditorCapture || m_stage == Stage::RuntimeCapture)
            {
                if (!m_captureFinished) { return; }
                if (m_captureResult != AZ::Render::FrameCaptureResult::Success || !UnlinkedExistingPath(m_capturePath))
                { Finish(false, (AZStd::string("Atom capture failed: ") + m_captureInfo).c_str()); return; }
                const QImage image(m_capturePath);
                if (image.width() != static_cast<int>(m_width) || image.height() != static_cast<int>(m_height) || !HasFixtureCaptureContent(image))
                { Finish(false, "Readback is unreadable, wrong-sized or uniform/blank; no visual success is claimed."); return; }
                if (m_stage == Stage::EditorCapture)
                {
                    m_editorCaptured = true;
                    auto* manager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
                    if (!manager) { Finish(false, "No native ActionManager."); return; }
                    const auto enabled = manager->IsActionEnabled(PlayAction);
                    if (!enabled.IsSuccess() || !enabled.GetValue() || !manager->TriggerAction(PlayAction).IsSuccess())
                    { Finish(false, "The native Play Game action is unavailable or disabled."); return; }
                    m_enteredGame = true;
                    m_frames = 0;
                    m_stage = Stage::RuntimeView;
                }
                else
                {
                    m_runtimeCaptured = true;
                    EditorInternal::EditorToolsApplicationRequestBus::Broadcast(&EditorInternal::EditorToolsApplicationRequests::RequestExitGameMode);
                    m_stage = Stage::Stopped;
                    m_lastReadinessGate = "waiting for complete native game-mode exit";
                }
            }
            else if (m_stage == Stage::Stopped)
            {
                if (!AzToolsFramework::EditorEntityContextRequestBus::HasHandlers() || RunningGame()) { return; }
                m_enteredGame = false;
                if (!SnapshotRoom(true)) { Finish(false, "Editor authoring snapshot changed after runtime."); return; }
                Finish(true, "Native route and nonuniform Atom captures completed; visual/product acceptance remains pending.");
            }
        }

        void Finish(bool completed, const char* reason)
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            if (m_runtimeIdentity)
            {
                AZStd::lock_guard lock(m_runtimeIdentity->m_mutex);
                m_runtimeIdentity->m_cancelled = true;
            }
            // Atom exposes no CancelCapture: a submitted readback may write its file after FAIL/cancel/deactivation.
            // Its ID is disconnected, never credited as successful, and its evidence is never deleted.
            if (m_enteredGame && EditorInternal::EditorToolsApplicationRequestBus::HasHandlers())
            {
                EditorInternal::EditorToolsApplicationRequestBus::Broadcast(&EditorInternal::EditorToolsApplicationRequests::RequestExitGameMode);
            }
            if (m_selectedCamera && !RunningGame())
            {
                AZ::EntityId viewed;
                Camera::EditorCameraRequestBus::BroadcastResult(viewed, &Camera::EditorCameraRequests::GetCurrentViewEntityId);
                if (viewed == m_camera)
                {
                    Camera::EditorCameraRequestBus::Broadcast(&Camera::EditorCameraRequests::SetViewFromEntityPerspective, m_previousView);
                }
            }
            QJsonObject report;
            report.insert("status", completed ? "CAPTURES_COMPLETED_REVIEW_REQUIRED" : "FAIL");
            report.insert("reason", QString::fromUtf8(reason));
            report.insert("pendingGate", Text(m_lastReadinessGate));
            report.insert("startedUtc", m_startedUtc);
            report.insert("finishedUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            report.insert("backend", Text(m_backend));
            report.insert("level", Text(m_level));
            report.insert("editorCapture", m_output + "/editor.png");
            report.insert("runtimeCapture", m_output + "/runtime.png");
            report.insert("editorCaptureVerified", m_editorCaptured);
            report.insert("runtimeCaptureVerified", m_runtimeCaptured);
            report.insert("lastCaptureId", static_cast<double>(m_captureId));
            report.insert("editorCaptureId", static_cast<double>(m_editorCaptureId));
            report.insert("runtimeCaptureId", static_cast<double>(m_runtimeCaptureId));
            report.insert("width", static_cast<double>(m_width));
            report.insert("height", static_cast<double>(m_height));
            report.insert("materialOwner", m_hasMaterialOwner);
            report.insert("surfaceBindingCount", static_cast<double>(m_bindings.size()));
            report.insert("gridSlot", static_cast<double>(FixtureGridSlot));
            report.insert("metalSlot", static_cast<double>(FixtureMetalSlot));
            report.insert("editorGridMaterialId", Text(m_editorGridId.ToString<AZStd::string>()));
            report.insert("editorMetalMaterialId", Text(m_editorMetalId.ToString<AZStd::string>()));
            report.insert("runtimeGridMaterialId", Text(m_runtimeGridId.ToString<AZStd::string>()));
            report.insert("runtimeMetalMaterialId", Text(m_runtimeMetalId.ToString<AZStd::string>()));
            report.insert("useTexture", m_useTexture);
            report.insert("limits", "Viewport pixels only; no Qt shell/Inspector/Undo interaction, full material/collider acceptance, or standalone launcher proof. An issued readback is not cancelable.");
            QFile file(m_output + "/verification.json");
            const auto bytes = QJsonDocument(report).toJson();
            if (!UnlinkedExistingPath(m_output) || !file.open(QIODevice::WriteOnly | QIODevice::NewOnly) || file.write(bytes) != bytes.size())
            {
                AZ_Error("JEEditorFixture", false, "Could not create verification.json without replacing existing evidence.");
                completed = false;
            }
            file.close();
            AZ_Printf("JEEditorFixture", "Native fixture verification %s: %s\n", completed ? "captures ready for review" : "FAILED", reason);
            m_stage = Stage::Idle;
        }

        Stage m_stage = Stage::Idle;
        bool m_registered = false;
        bool m_enteredGame = false;
        bool m_selectedCamera = false;
        bool m_captureFinished = false;
        bool m_editorCaptured = false;
        bool m_runtimeCaptured = false;
        bool m_manualUVs = false;
        bool m_hasMaterialOwner = false;
        bool m_useTexture = false;
        unsigned m_frames = 0;
        uint32_t m_width = 0, m_height = 0;
        AZ::EntityId m_room, m_camera, m_previousView;
        AZ::Vector3 m_tint = AZ::Vector3::CreateZero();
        AZ::Render::FrameCaptureId m_captureId = AZ::Render::InvalidFrameCaptureId;
        AZ::Render::FrameCaptureId m_editorCaptureId = AZ::Render::InvalidFrameCaptureId;
        AZ::Render::FrameCaptureId m_runtimeCaptureId = AZ::Render::InvalidFrameCaptureId;
        AZ::Render::FrameCaptureResult m_captureResult = AZ::Render::FrameCaptureResult::None;
        AZStd::string m_captureInfo, m_backend, m_level;
        AZStd::string m_sourceCameraName, m_lastReadinessGate;
        AZStd::shared_ptr<RuntimeIdentityRequest> m_runtimeIdentity;
        bool m_runtimeIdentityFailed = false;
        QString m_output, m_capturePath, m_startedUtc;
        std::chrono::steady_clock::time_point m_deadline;
        JE::Geometry::Api::WhiteBoxMeshStream m_snapshot;
        SurfaceMaterialBindings m_bindings;
        JE::Render::MaterialAssignmentMap m_materials;
        JE::Render::MaterialAssignmentMap m_expectedMaterials;
        AZ::Data::AssetId m_editorGridId, m_editorMetalId, m_runtimeGridId, m_runtimeMetalId;
    };

    FixtureVerification::FixtureVerification() : m_impl(AZStd::make_unique<Implementation>()) {}
    FixtureVerification::~FixtureVerification() = default;
    void FixtureVerification::RegisterCommands() { m_impl->RegisterCommands(); }
    void FixtureVerification::Deactivate() { m_impl->Deactivate(); }
}
