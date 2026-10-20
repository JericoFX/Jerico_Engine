/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/EditorStateMaskRenderer.h>
#include <Pass/EditorStatePassSystem.h>

#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace JE::Editor
{
    namespace Render::Features
    {
        //! Feature processor for Editor Mode Feedback visual effect system.
        class EditorModeFeatureProcessor
            : public JE::Render::RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorModeFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(EditorModeFeatureProcessor, "AZ::Render::EditorModeFeatureProcessor", "{78D40D57-F564-4ECD-B9F5-D8C9784B15D0}", JE::Render::RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline) override;
            void Render(const RenderPacket& packet) override;
            void Simulate(const SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* pipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            // AZ::TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            //! Enable or disable the rendering editor mode feedback.
            void SetEnableRender(bool enableRender);

        private:
            //! The pass system for the editor state feedback effects.
            AZStd::unique_ptr<EditorStatePassSystem> m_editorStatePassSystem;

            //! Map of all mask renderers for the draw tags used by the editor state feedback effects.
            AZStd::unordered_map<AZ::Name, EditorStateMaskRenderer> m_maskRenderers;

            //! Material for sending draw packets to the entity mask pass.
            AZ::Data::Instance<JE::Render::RPI::Material> m_maskMaterial = nullptr;
        };
    } // namespace Render::Features
} // namespace JE::Editor
