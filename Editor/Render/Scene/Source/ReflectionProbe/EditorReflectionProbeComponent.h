/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <ReflectionProbe/ReflectionProbeComponent.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>
#include <JE/Editor/Render/Scene/ReflectionProbe/EditorReflectionProbeBus.h>

namespace JE::Editor::Render
{
    namespace Scene
    {        
        class EditorReflectionProbeComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::ReflectionProbeComponentController, JE::Render::Scene::ReflectionProbeComponent, JE::Render::Scene::ReflectionProbeComponentConfig>
            , public EditorReflectionProbeBus::Handler
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private AZ::TickBus::Handler
            , private EditorCubeMapRenderer
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::ReflectionProbeComponentController, JE::Render::Scene::ReflectionProbeComponent, JE::Render::Scene::ReflectionProbeComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorReflectionProbeComponent, "AZ::Render::EditorReflectionProbeComponent", JE::Render::Scene::EditorReflectionProbeComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorReflectionProbeComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorReflectionProbeComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorReflectionProbeComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorReflectionProbeComponent() = default;
            EditorReflectionProbeComponent(const JE::Render::Scene::ReflectionProbeComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

            // AzFramework::EntityDebugDisplayEventBus::Handler overrides
            void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        private:

            // AZ::TickBus overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // validation
            AZ::Outcome<void, AZStd::string> OnUseBakedCubemapValidate(void* newValue, const AZ::Uuid& valueType);

            // change notifications
            AZ::u32 OnUseBakedCubemapChanged();
            AZ::u32 OnAuthoredCubemapChanged();
            AZ::u32 OnBakeExposureChanged();

            // retrieves visibility for baked or authored cubemap controls
            AZ::u32 GetBakedCubemapVisibilitySetting();
            AZ::u32 GetAuthoredCubemapVisibilitySetting();

            // initiate the reflection probe cubemap generation, returns the refresh value for the ChangeNotify UIElement attribute 
            AZ::u32 BakeReflectionProbe() override;

            // EditorComponentSelectionRequestsBus overrides
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool SupportsEditorRayIntersect() override;

            // UI settings
            // the user can select between a baked cubemap or an authored cubemap asset
            bool m_useBakedCubemap = true;
            JE::Render::Scene::CubeMapSpecularQualityLevel m_bakedCubeMapQualityLevel = JE::Render::Scene::CubeMapSpecularQualityLevel::Medium;
            AZStd::string m_bakedCubeMapRelativePath;
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> m_authoredCubeMapAsset;
            float m_bakeExposure = 0.0f;

            // handler for the inner extents being updated
            AZ::Event<bool>::Handler m_innerExtentsChangedHandler;
        };

    } // namespace Render
} // namespace AZ
