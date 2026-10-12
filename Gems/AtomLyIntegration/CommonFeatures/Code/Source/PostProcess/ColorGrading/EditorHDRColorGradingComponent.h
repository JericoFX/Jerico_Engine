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
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Feature/ColorGrading/LutGenerationCaptureBus.h>
#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ColorGrading/EditorHDRColorGradingBus.h>

namespace AZ
{
    namespace Render
    {
        static constexpr const char* const GeneratedLutRelativePath = { "LutGeneration/SavedLut_%s" };

        class EditorHDRColorGradingComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>
            , private TickBus::Handler
            , private FrameCaptureNotificationBus::Handler
            , private JE::ColorGrading::LutGenerationCaptureNotificationBus::Handler
            , private EditorHDRColorGradingRequestBus::Handler
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorHDRColorGradingComponent, "{C1FAB0B1-5847-4533-B08E-7314AC807B8E}", BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRColorGradingComponent() = default;
            EditorHDRColorGradingComponent(const HDRColorGradingComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

        private:
            // AZ::TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // FrameCaptureNotificationBus overrides ...
            void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

            void OnLutGenerationCapture(const JE::ColorGrading::LutGenerationCaptureMetadata& metadata) override;

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
            ShaperPresetType m_generatedLutShaper = ShaperPresetType::None;
            float m_generatedLutMinimumExposure = 0.0f;
            float m_generatedLutMaximumExposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
