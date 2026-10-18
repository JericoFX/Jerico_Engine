/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Common/JsonUtils.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettings.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor
{
    namespace
    {
        constexpr AZStd::string_view NativePreviewPrefix = "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/";

        AZStd::string CanonicalPreviewPath(const AZStd::string& path)
        {
            constexpr AZStd::string_view legacyPrefix = "@gemroot:MaterialEditor@/Assets/MaterialEditor/";
            const auto normalized = AZ::IO::FixedMaxPath(path).LexicallyNormal().StringAsPosix();
            if (normalized.starts_with(legacyPrefix))
            {
                return AZStd::string(NativePreviewPrefix) + normalized.substr(legacyPrefix.size());
            }

            // Only the former preview root moves; project/custom presets retain their paths.
            const auto absolute = AZ::IO::FixedMaxPath(AtomToolsFramework::GetPathWithoutAlias(path)).LexicallyNormal();
            for (const auto root : {
                "@engroot@/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor"})
            {
                const auto absoluteRoot = AZ::IO::FixedMaxPath(AtomToolsFramework::GetPathWithoutAlias(root)).LexicallyNormal();
                if (absolute.IsAbsolute() && absoluteRoot.IsAbsolute() && absolute.IsRelativeTo(absoluteRoot))
                {
                    return AZStd::string(NativePreviewPrefix) + absolute.LexicallyRelative(absoluteRoot).StringAsPosix();
                }
            }
            return path;
        }

        AZStd::string CanonicalPreviewAlias(const AZStd::string& path)
        {
            const auto canonical = CanonicalPreviewPath(path);
            return canonical.starts_with(NativePreviewPrefix) ? canonical : AtomToolsFramework::GetPathWithAlias(canonical);
        }

        AZStd::set<AZStd::string> CanonicalPreviewPaths(const AZStd::set<AZStd::string>& paths)
        {
            AZStd::set<AZStd::string> result;
            for (const auto& path : paths)
            {
                result.insert(CanonicalPreviewPath(path));
            }
            return result;
        }
    }
} // namespace JE::Editor

namespace AtomToolsFramework
{
    void EntityPreviewViewportSettingsSystem::Reflect(AZ::ReflectContext* context)
    {
        EntityPreviewViewportSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EntityPreviewViewportSettingsSystem>()
                ->Version(0);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EntityPreviewViewportSettingsSystem>(QT_TRANSLATE_NOOP("AtomToolsFramework", "EntityPreviewViewportSettingsSystem"), QT_TRANSLATE_NOOP("AtomToolsFramework", "Manages and serializes settings for the application viewport"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EntityPreviewViewportSettingsRequestBus>("EntityPreviewViewportSettingsRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("SetLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::SetLightingPreset)
                ->Event("GetLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::GetLightingPreset)
                ->Event("SaveLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::SaveLightingPreset)
                ->Event("LoadLightingPreset", &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset)
                ->Event("LoadLightingPresetByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPresetByAssetId)
                ->Event("GetLastLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPath)
                ->Event("GetLastLightingPresetPathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastLightingPresetPathWithoutAlias)
                ->Event("RegisterLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterLightingPresetPath)
                ->Event("UnregisterLightingPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterLightingPresetPath)
                ->Event("GetRegisteredLightingPresetPaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredLightingPresetPaths)
                ->Event("SetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset)
                ->Event("GetModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::GetModelPreset)
                ->Event("SaveModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset)
                ->Event("LoadModelPreset", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset)
                ->Event("LoadModelPresetByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPresetByAssetId)
                ->Event("GetLastModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPath)
                ->Event("GetLastModelPresetPathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastModelPresetPathWithoutAlias)
                ->Event("RegisterModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterModelPresetPath)
                ->Event("UnregisterModelPresetPath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterModelPresetPath)
                ->Event("GetRegisteredModelPresetPaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredModelPresetPaths)
                ->Event("LoadRenderPipeline", &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline)
                ->Event("LoadRenderPipelineByAssetId", &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipelineByAssetId)
                ->Event("GetLastRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::GetLastRenderPipelinePath)
                ->Event("GetLastRenderPipelinePathWithoutAlias", &EntityPreviewViewportSettingsRequestBus::Events::GetLastRenderPipelinePathWithoutAlias)
                ->Event("RegisterRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::RegisterRenderPipelinePath)
                ->Event("UnregisterRenderPipelinePath", &EntityPreviewViewportSettingsRequestBus::Events::UnregisterRenderPipelinePath)
                ->Event("GetRegisteredRenderPipelinePaths", &EntityPreviewViewportSettingsRequestBus::Events::GetRegisteredRenderPipelinePaths)
                ->Event("SetShadowCatcherEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetShadowCatcherEnabled)
                ->Event("GetShadowCatcherEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetShadowCatcherEnabled)
                ->Event("SetGridEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetGridEnabled)
                ->Event("GetGridEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetGridEnabled)
                ->Event("SetAlternateSkyboxEnabled", &EntityPreviewViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled)
                ->Event("GetAlternateSkyboxEnabled", &EntityPreviewViewportSettingsRequestBus::Events::GetAlternateSkyboxEnabled)
                ->Event("SetFieldOfView", &EntityPreviewViewportSettingsRequestBus::Events::SetFieldOfView)
                ->Event("GetFieldOfView", &EntityPreviewViewportSettingsRequestBus::Events::GetFieldOfView)
                ;

            behaviorContext->EBus<EntityPreviewViewportSettingsNotificationBus>("EntityPreviewViewportSettingsNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("OnViewportSettingsChanged", &EntityPreviewViewportSettingsNotificationBus::Events::OnViewportSettingsChanged)
                ;
        }
    }

    EntityPreviewViewportSettingsSystem::EntityPreviewViewportSettingsSystem(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        EntityPreviewViewportSettingsRequestBus::Handler::BusConnect(m_toolId);
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        PreloadPresets();
    }

    EntityPreviewViewportSettingsSystem::~EntityPreviewViewportSettingsSystem()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        EntityPreviewViewportSettingsRequestBus::Handler::BusDisconnect();
        ClearContent();
    }

    void EntityPreviewViewportSettingsSystem::ClearContent()
    {
        m_lightingPresetCache.clear();
        m_lightingPreset = {};

        m_modelPresetCache.clear();
        m_modelPreset = {};

        m_settingsNotificationPending = {};
    }

    void EntityPreviewViewportSettingsSystem::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_settingsNotificationPending)
        {
            m_settingsNotificationPending = false;
            EntityPreviewViewportSettingsNotificationBus::Event(
                m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnViewportSettingsChanged);
        }
    }

    void EntityPreviewViewportSettingsSystem::SetLightingPreset(const JE::Render::Features::LightingPreset& preset)
    {
        m_lightingPreset = preset;
        m_settingsNotificationPending = true;
    }

    const JE::Render::Features::LightingPreset& EntityPreviewViewportSettingsSystem::GetLightingPreset() const
    {
        return m_lightingPreset;
    }

    bool EntityPreviewViewportSettingsSystem::SaveLightingPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        if (!pathWithoutAlias.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_lightingPreset, pathWithoutAlias).IsSuccess())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
            RegisterLightingPreset(pathWithAlias, m_lightingPreset);
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadLightingPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        auto cacheItr = m_lightingPresetCache.find(pathWithAlias);
        if (cacheItr != m_lightingPresetCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
            m_lightingPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::Features::LightingPreset>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath", pathWithAlias);
                m_lightingPreset = AZStd::any_cast<JE::Render::Features::LightingPreset>(loadResult.GetValue());
                RegisterLightingPreset(pathWithAlias, m_lightingPreset);
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadLightingPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadLightingPreset(JE::Render::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastLightingPresetPath() const
    {
        return JE::Editor::CanonicalPreviewPath(GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/LightingPresetPath",
            "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset"));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastLightingPresetPathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastLightingPresetPath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastLightingPresetAssetId() const
    {
        const auto& result = JE::Render::RPI::AssetUtils::MakeAssetId(GetLastLightingPresetPath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterLightingPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::Features::LightingPreset::Extension))
        {
            auto paths = GetRegisteredLightingPresetPaths();
            paths.insert(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterLightingPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::Features::LightingPreset::Extension))
        {
            auto paths = GetRegisteredLightingPresetPaths();
            paths.erase(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredLightingPresetPaths() const
    {
        return JE::Editor::CanonicalPreviewPaths(GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredLightingPresetPaths",
            AZStd::set<AZStd::string>{
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/LightingPresets/lythwood_room.lightingpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset" }));
    }

    void EntityPreviewViewportSettingsSystem::SetModelPreset(const JE::Render::Features::ModelPreset& preset)
    {
        m_modelPreset = preset;
        m_settingsNotificationPending = true;
    }

    const JE::Render::Features::ModelPreset& EntityPreviewViewportSettingsSystem::GetModelPreset() const
    {
        return m_modelPreset;
    }

    bool EntityPreviewViewportSettingsSystem::SaveModelPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        if (!pathWithoutAlias.empty() && AZ::JsonSerializationUtils::SaveObjectToFile(&m_modelPreset, pathWithoutAlias).IsSuccess())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
            RegisterModelPreset(pathWithAlias, m_modelPreset);
            return true;
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadModelPreset(const AZStd::string& path)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        auto cacheItr = m_modelPresetCache.find(pathWithAlias);
        if (cacheItr != m_modelPresetCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
            m_modelPreset = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::Features::ModelPreset>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath", pathWithAlias);
                m_modelPreset = AZStd::any_cast<JE::Render::Features::ModelPreset>(loadResult.GetValue());
                RegisterModelPreset(pathWithAlias, m_modelPreset);
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadModelPresetByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadModelPreset(JE::Render::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastModelPresetPath() const
    {
        return JE::Editor::CanonicalPreviewPath(GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/ModelPresetPath",
            "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Shaderball.modelpreset.azasset"));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastModelPresetPathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastModelPresetPath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastModelPresetAssetId() const
    {
        const auto& result = JE::Render::RPI::AssetUtils::MakeAssetId(GetLastModelPresetPath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterModelPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::Features::ModelPreset::Extension))
        {
            auto paths = GetRegisteredModelPresetPaths();
            paths.insert(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterModelPresetPath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::Features::ModelPreset::Extension))
        {
            auto paths = GetRegisteredModelPresetPaths();
            paths.erase(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredModelPresetPaths() const
    {
        return JE::Editor::CanonicalPreviewPaths(GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredModelPresetPaths",
            AZStd::set<AZStd::string>{
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/BeveledCone.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/BeveledCube.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/BeveledCylinder.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Caduceus.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Cone.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Cube.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Cylinder.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Hermanubis.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Plane_1x1.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Plane_3x3.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/PlatonicSphere.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/PolarSphere.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/QuadSphere.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Shaderball.modelpreset.azasset",
                "@engroot@/Editor/Materials/Preview/Assets/MaterialEditor/ViewportModels/Torus.modelpreset.azasset" }));
    }

    bool EntityPreviewViewportSettingsSystem::LoadRenderPipeline(const AZStd::string& path)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        const auto& pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        auto cacheItr = m_renderPipelineDescriptorCache.find(pathWithAlias);
        if (cacheItr != m_renderPipelineDescriptorCache.end())
        {
            SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", pathWithAlias);
            m_renderPipelineDescriptor = cacheItr->second;
            m_settingsNotificationPending = true;
            return true;
        }

        if (!pathWithoutAlias.empty())
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::RPI::RenderPipelineDescriptor>())
            {
                SetSettingsValue("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath", pathWithAlias);
                m_renderPipelineDescriptor = AZStd::any_cast<JE::Render::RPI::RenderPipelineDescriptor>(loadResult.GetValue());
                RegisterRenderPipeline(pathWithAlias, m_renderPipelineDescriptor);
                return true;
            }
        }
        return false;
    }

    bool EntityPreviewViewportSettingsSystem::LoadRenderPipelineByAssetId(const AZ::Data::AssetId& assetId)
    {
        return LoadRenderPipeline(JE::Render::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastRenderPipelinePath() const
    {
        return GetPathWithoutAlias(GetSettingsValue<AZStd::string>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RenderPipelinePath",
            "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset"));
    }

    AZStd::string EntityPreviewViewportSettingsSystem::GetLastRenderPipelinePathWithoutAlias() const
    {
        return GetPathWithoutAlias(GetLastRenderPipelinePath());
    }

    AZ::Data::AssetId EntityPreviewViewportSettingsSystem::GetLastRenderPipelineAssetId() const
    {
        const auto& result = JE::Render::RPI::AssetUtils::MakeAssetId(GetLastRenderPipelinePath(), 0);
        return result.IsSuccess() ? result.GetValue() : AZ::Data::AssetId();
    }

    void EntityPreviewViewportSettingsSystem::RegisterRenderPipelinePath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::RPI::RenderPipelineDescriptor::Extension))
        {
            auto paths = GetRegisteredRenderPipelinePaths();
            paths.insert(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", paths);
        }
    }

    void EntityPreviewViewportSettingsSystem::UnregisterRenderPipelinePath(const AZStd::string& path)
    {
        if (path.ends_with(JE::Render::RPI::RenderPipelineDescriptor::Extension))
        {
            auto paths = GetRegisteredRenderPipelinePaths();
            paths.erase(JE::Editor::CanonicalPreviewAlias(path));
            SetSettingsObject<AZStd::set<AZStd::string>>(
                "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths", paths);
        }
    }

    AZStd::set<AZStd::string> EntityPreviewViewportSettingsSystem::GetRegisteredRenderPipelinePaths() const
    {
        return JE::Editor::CanonicalPreviewPaths(GetSettingsObject<AZStd::set<AZStd::string>>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/RegisteredRenderPipelinePaths",
            AZStd::set<AZStd::string>{
                "@gemroot:Atom_Feature_Common@/Assets/Passes/MainRenderPipeline.azasset" }));
    }

    void EntityPreviewViewportSettingsSystem::SetShadowCatcherEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableShadowCatcher", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetShadowCatcherEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableShadowCatcher", true);
    }

    void EntityPreviewViewportSettingsSystem::SetGridEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableGrid", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetGridEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableGrid", true);
    }

    void EntityPreviewViewportSettingsSystem::SetAlternateSkyboxEnabled(bool enable)
    {
        SetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableAlternateSkybox", enable);
        m_settingsNotificationPending = true;
    }

    bool EntityPreviewViewportSettingsSystem::GetAlternateSkyboxEnabled() const
    {
        return GetSettingsValue<bool>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/EnableAlternateSkybox", false);
    }

    void EntityPreviewViewportSettingsSystem::SetFieldOfView(float fieldOfView)
    {
        SetSettingsValue<double>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/FieldOfView", aznumeric_cast<double>(fieldOfView));
        m_settingsNotificationPending = true;
    }

    float EntityPreviewViewportSettingsSystem::GetFieldOfView() const
    {
        return aznumeric_cast<float>(GetSettingsValue<double>("/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/FieldOfView", 90.0));
    }

    void EntityPreviewViewportSettingsSystem::SetDisplayMapperOperationType(AZ::Render::DisplayMapperOperationType operationType)
    {
        SetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/DisplayMapperOperationType", aznumeric_cast<AZ::u64>(operationType));
        m_settingsNotificationPending = true;
    }

    AZ::Render::DisplayMapperOperationType EntityPreviewViewportSettingsSystem::GetDisplayMapperOperationType() const
    {
        return aznumeric_cast<AZ::Render::DisplayMapperOperationType>(GetSettingsValue<AZ::u64>(
            "/O3DE/AtomToolsFramework/EntityPreviewViewportSettings/DisplayMapperOperationType",
            aznumeric_cast<AZ::u64>(AZ::Render::DisplayMapperOperationType::Aces)));
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (assetInfo.m_assetType == JE::Render::RPI::AnyAsset::RTTI_Type())
        {
            PreloadPreset(JE::Render::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId));
        }
    }

    void EntityPreviewViewportSettingsSystem::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        OnCatalogAssetChanged(assetId);
    }

    void EntityPreviewViewportSettingsSystem::PreloadPreset(const AZStd::string& path)
    {
        const auto pathWithoutAlias = GetPathWithoutAlias(JE::Editor::CanonicalPreviewPath(path));
        if (path.ends_with(JE::Render::Features::LightingPreset::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::Features::LightingPreset>())
            {
                RegisterLightingPreset(path, AZStd::any_cast<JE::Render::Features::LightingPreset>(loadResult.GetValue()));
            }
            return;
        }

        if (path.ends_with(JE::Render::Features::ModelPreset::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::Features::ModelPreset>())
            {
                RegisterModelPreset(path, AZStd::any_cast<JE::Render::Features::ModelPreset>(loadResult.GetValue()));
            }
            return;
        }

        if (path.ends_with(JE::Render::RPI::RenderPipelineDescriptor::Extension))
        {
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(pathWithoutAlias);
            if (loadResult && loadResult.GetValue().is<JE::Render::RPI::RenderPipelineDescriptor>())
            {
                RegisterRenderPipeline(path, AZStd::any_cast<JE::Render::RPI::RenderPipelineDescriptor>(loadResult.GetValue()));
            }
            return;
        }
    }

    void EntityPreviewViewportSettingsSystem::RegisterLightingPreset(const AZStd::string& path, const JE::Render::Features::LightingPreset& preset)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        m_lightingPresetCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterLightingPresetPath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnLightingPresetAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::RegisterModelPreset(const AZStd::string& path, const JE::Render::Features::ModelPreset& preset)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        m_modelPresetCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterModelPresetPath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnModelPresetAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::RegisterRenderPipeline(const AZStd::string& path, const JE::Render::RPI::RenderPipelineDescriptor& preset)
    {
        const auto& pathWithAlias = JE::Editor::CanonicalPreviewAlias(path);
        m_renderPipelineDescriptorCache[pathWithAlias] = preset;
        m_settingsNotificationPending = true;
        RegisterRenderPipelinePath(pathWithAlias);
        EntityPreviewViewportSettingsNotificationBus::Event(
            m_toolId, &EntityPreviewViewportSettingsNotificationBus::Events::OnRenderPipelineAdded, pathWithAlias);
    }

    void EntityPreviewViewportSettingsSystem::PreloadPresets()
    {
        // Preload the last active lighting and model presets so they are available for the viewport and selection controls.
        LoadLightingPreset(GetLastLightingPresetPath());
        LoadModelPreset(GetLastModelPresetPath());

        // This job will do background enumeration of lighting and preset files available in the project. Once the files have been
        // enumerated, the work will be passed along to the tick bus on the main thread to register all of the presets that were discovered.
        // It should be safe to fire and forget because the bus for this class is being addressed by ID. If the class is destroyed and
        // disconnected then the notifications will be ignored.
        AZ::Job* job = AZ::CreateJobFunction(
            [toolId = m_toolId]()
            {
                AZ_TracePrintf("EntityPreviewViewportSettingsSystem", "Enumerating presets started.");
                auto filterFn = [](const AZStd::string& path)
                {
                    return
                        path.ends_with(JE::Render::Features::LightingPreset::Extension) ||
                        path.ends_with(JE::Render::Features::ModelPreset::Extension) ||
                        path.ends_with(JE::Render::RPI::RenderPipelineDescriptor::Extension);
                };

                const auto& paths = GetPathsInSourceFoldersMatchingFilter(filterFn);
                AZ_TracePrintf("EntityPreviewViewportSettingsSystem", "Enumerating presets finished.");

                AZ::TickBus::QueueFunction(
                    [toolId, paths]()
                    {
                        EntityPreviewViewportSettingsRequestBus::Event(
                            toolId,
                            [paths](EntityPreviewViewportSettingsRequests* viewportRequests)
                            {
                                for (const auto& path : paths)
                                {
                                    viewportRequests->PreloadPreset(path);
                                }
                            });
                    });
            },
            true);
        job->Start();
    }
} // namespace AtomToolsFramework
