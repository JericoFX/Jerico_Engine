/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <Components/DiffuseProbeGridComponent.h>
#include <Components/DiffuseProbeGridComponentConstants.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>

namespace JE::Editor::Render
{
    namespace Features
    {        
        class EditorDiffuseProbeGridComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Features::DiffuseProbeGridComponentController, JE::Render::Features::DiffuseProbeGridComponent, JE::Render::Features::DiffuseProbeGridComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private AZ::TickBus::Handler
            , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Features::DiffuseProbeGridComponentController, JE::Render::Features::DiffuseProbeGridComponent, JE::Render::Features::DiffuseProbeGridComponentConfig>;
            AZ_RTTI_WITH_NAME(EditorDiffuseProbeGridComponent, "AZ::Render::EditorDiffuseProbeGridComponent", JE::Render::Features::EditorDiffuseProbeGridComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase);
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorDiffuseProbeGridComponent)
            AZ_COMPONENT_BASE(EditorDiffuseProbeGridComponent)
            AZ_CLASS_ALLOCATOR(EditorDiffuseProbeGridComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDiffuseProbeGridComponent();
            EditorDiffuseProbeGridComponent(const JE::Render::Features::DiffuseProbeGridComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

        private:

            // AZ::TickBus overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // EditorComponentSelectionRequestsBus overrides
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool SupportsEditorRayIntersect() override;

            // EditorEntityInfoNotifications overrides
            void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;

            // helper functions
            AZStd::string ValidateOrCreateNewTexturePath(const AZStd::string& relativePath, const char* fileSuffix);
            void CheckoutSourceTextureFile(const AZStd::string& fullPath);
            void CheckTextureAssetNotification(const AZStd::string& relativePath, AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& configurationAsset);
            AZStd::vector<AZ::Edit::EnumConstant<JE::Render::Features::DiffuseProbeGridNumRaysPerProbe>> GetNumRaysPerProbeEnumList() const;

            // property change notifications
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateX(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateY(void* newValue, const AZ::Uuid& valueType);
            AZ::Outcome<void, AZStd::string> OnProbeSpacingValidateZ(void* newValue, const AZ::Uuid& valueType);
            AZ::u32 OnProbeSpacingChanged();
            AZ::u32 OnAmbientMultiplierChanged();
            AZ::u32 OnViewBiasChanged();
            AZ::u32 OnNormalBiasChanged();
            AZ::u32 OnNumRaysPerProbeChanged();
            AZ::Outcome<void, AZStd::string> OnScrollingChangeValidate(void* newValue, const AZ::Uuid& valueType);
            AZ::u32 OnScrollingChanged();
            AZ::u32 OnEdgeBlendIblChanged();
            AZ::u32 OnFrameUpdateCountChanged();
            AZ::u32 OnTransparencyModeChanged();
            AZ::u32 OnEmissiveMultiplierChanged();
            AZ::u32 OnEditorModeChanged();
            AZ::u32 OnRuntimeModeChanged();
            AZ::u32 OnShowVisualizationChanged();
            AZ::u32 OnShowInactiveProbesChanged();
            AZ::u32 OnVisualizationSphereRadiusChanged();
            AZ::Outcome<void, AZStd::string> OnModeChangeValidate(void* newValue, const AZ::Uuid& valueType);

            // Button handler
            AZ::u32 BakeDiffuseProbeGrid();
            AZ::u32 GetBakeDiffuseProbeGridVisibilitySetting();

            // properties
            float m_probeSpacingX = JE::Render::Features::DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingY = JE::Render::Features::DefaultDiffuseProbeGridSpacing;
            float m_probeSpacingZ = JE::Render::Features::DefaultDiffuseProbeGridSpacing;
            float m_ambientMultiplier = JE::Render::Features::DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = JE::Render::Features::DefaultDiffuseProbeGridViewBias;
            float m_normalBias = JE::Render::Features::DefaultDiffuseProbeGridNormalBias;
            JE::Render::Features::DiffuseProbeGridNumRaysPerProbe m_numRaysPerProbe = JE::Render::Features::DefaultDiffuseProbeGridNumRaysPerProbe;
            bool m_scrolling = false;
            bool m_edgeBlendIbl = true;
            uint32_t m_frameUpdateCount = 1;
            JE::Render::Features::DiffuseProbeGridTransparencyMode m_transparencyMode = JE::Render::Features::DefaultDiffuseProbeGridTransparencyMode;
            float m_emissiveMultiplier = JE::Render::Features::DefaultDiffuseProbeGridEmissiveMultiplier;
            JE::Render::Features::DiffuseProbeGridMode m_editorMode = JE::Render::Features::DiffuseProbeGridMode::RealTime;
            JE::Render::Features::DiffuseProbeGridMode m_runtimeMode = JE::Render::Features::DiffuseProbeGridMode::RealTime;
            bool m_showVisualization = false;
            bool m_showInactiveProbes = false;
            float m_visualizationSphereRadius = JE::Render::Features::DefaultVisualizationSphereRadius;

            // flags
            bool m_editorModeSet = false;
            AZStd::atomic_bool m_bakeInProgress = false;

            // handler for the diffuse probe grid changing the underlying box dimensions
            AZ::Event<bool>::Handler m_boxChangedByGridHandler;
        };
    } // namespace Features
} // namespace JE::Editor::Render
