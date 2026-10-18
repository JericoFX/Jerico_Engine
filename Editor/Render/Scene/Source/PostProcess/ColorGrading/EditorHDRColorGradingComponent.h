/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <JE/Render/Features/ColorGrading/LutGenerationCaptureBus.h>
#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>
#include <JE/Editor/Render/Scene/PostProcess/ColorGrading/EditorHDRColorGradingBus.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        static constexpr const char* const GeneratedLutRelativePath = { "LutGeneration/SavedLut_%s" };

        class EditorHDRColorGradingComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::HDRColorGradingComponentController, JE::Render::Scene::HDRColorGradingComponent, JE::Render::Scene::HDRColorGradingComponentConfig>
            , private AZ::TickBus::Handler
            , private JE::Render::Features::FrameCaptureNotificationBus::Handler
            , private JE::Render::Features::LutGenerationCaptureNotificationBus::Handler
            , private EditorHDRColorGradingRequestBus::Handler
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::HDRColorGradingComponentController, JE::Render::Scene::HDRColorGradingComponent, JE::Render::Scene::HDRColorGradingComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorHDRColorGradingComponent, "AZ::Render::EditorHDRColorGradingComponent", "{C1FAB0B1-5847-4533-B08E-7314AC807B8E}", BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorHDRColorGradingComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorHDRColorGradingComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorHDRColorGradingComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRColorGradingComponent() = default;
            EditorHDRColorGradingComponent(const JE::Render::Scene::HDRColorGradingComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

        private:
            // AZ::TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // FrameCaptureNotificationBus overrides ...
            void OnFrameCaptureFinished(JE::Render::Features::FrameCaptureResult result, const AZStd::string& info) override;

            void OnLutGenerationCapture(const JE::Render::Features::LutGenerationCaptureMetadata& metadata) override;

            //! EditorHDRColorGradingRequestBus overrides...
            void GenerateLutAsync() override;
            void ActivateLutAsync() override;

            void GenerateLut();
            AZ::u32 ActivateLut();

            bool GetGeneratedLutVisibilitySettings();

            void CancelOperation();
            void FailOperation(const AZStd::string& error);
            bool BeginAssetWait(const AZStd::string& absolutePath, bool activate);

            struct LutOperation;
            AZStd::shared_ptr<LutOperation> m_operation;

            AZStd::string m_generatedLutAbsolutePath;
            // Encoding belongs to the generated source, independent of later HDR configuration edits.
            bool m_generatedLutMetadataValid = false;
            AZ::Render::ShaperPresetType m_generatedLutShaper = AZ::Render::ShaperPresetType::None;
            float m_generatedLutMinimumExposure = 0.0f;
            float m_generatedLutMaximumExposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
