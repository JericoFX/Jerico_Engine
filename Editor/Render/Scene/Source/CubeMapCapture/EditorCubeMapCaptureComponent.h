/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CubeMapCapture/CubeMapCaptureComponent.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Scene/CubeMapCapture/EditorCubeMapCaptureBus.h>

namespace JE::Editor::Render
{
    namespace Scene
    {        
        class EditorCubeMapCaptureComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::CubeMapCaptureComponentController, JE::Render::Scene::CubeMapCaptureComponent, JE::Render::Scene::CubeMapCaptureComponentConfig>
            , public JE::Render::Scene::EditorCubeMapCaptureBus::Handler
            , private EditorCubeMapRenderer
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::CubeMapCaptureComponentController, JE::Render::Scene::CubeMapCaptureComponent, JE::Render::Scene::CubeMapCaptureComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorCubeMapCaptureComponent, "AZ::Render::EditorCubeMapCaptureComponent", JE::Render::Scene::EditorCubeMapCaptureComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorCubeMapCaptureComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorCubeMapCaptureComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorCubeMapCaptureComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorCubeMapCaptureComponent() = default;
            EditorCubeMapCaptureComponent(const JE::Render::Scene::CubeMapCaptureComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

        private:
            // initiate the cubemap capture, returns the refresh value for the ChangeNotify UIElement attribute 
            AZ::u32 CaptureCubeMap() override;
        };

    } // namespace Render
} // namespace AZ
