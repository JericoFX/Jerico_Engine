/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ColorGrading/EditorHDRColorGradingComponent.h>
#include <PostProcess/ColorGrading/JEEditorLutWriter.h>
#include <PostProcess/LookModification/EditorLookModificationComponent.h>
#include <Atom/Feature/LookupTable/LookupTableAsset.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <JE/Render/Scene/PostProcess/LookModification/LookModificationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <JE/Render/RPI/Public/ViewportContextManager.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Base.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <cmath>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace Internal
        {
            struct EditorHDRColorGradingNotificationBusHandler final
                : public EditorHDRColorGradingNotificationBus::Handler
                , public AZ::BehaviorEBusHandler
            {
                AZ_EBUS_BEHAVIOR_BINDER(
                    EditorHDRColorGradingNotificationBusHandler, "{61FFB210-C2F9-4A82-9088-4C974C3E0EE7}", AZ::SystemAllocator
                        , OnGenerateLutCompleted, OnActivateLutCompleted);

                void OnGenerateLutCompleted(const AZStd::string& lutAssetAbsolutePath) override
                {
                    Call(FN_OnGenerateLutCompleted, lutAssetAbsolutePath);
                }

                void OnActivateLutCompleted() override
                {
                    Call(FN_OnActivateLutCompleted);
                }
            };
        }

        namespace
        {
            constexpr auto LutTimeout = AZStd::chrono::seconds(120);

            struct LutEntityEditState
            {
                bool m_editorEntity = false;
                bool m_editable = false;
                bool m_running = false;
                bool m_requesting = false;
                bool m_entityExists = false;
                int m_state = -1;
            };

            bool CanEditLutEntity(AZ::EntityId entityId, LutEntityEditState* detail = nullptr)
            {
                bool editorEntity = false;
                bool editable = false;
                bool running = false;
                bool requesting = false;
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    editorEntity, &AzToolsFramework::EditorEntityContextRequests::IsEditorEntity, entityId);
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    running, &AzToolsFramework::EditorEntityContextRequests::IsEditorRunningGame);
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    requesting, &AzToolsFramework::EditorEntityContextRequests::IsEditorRequestingGame);
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                    editable, &AzToolsFramework::ToolsApplicationRequests::IsEntityEditable, entityId);
                const auto* entity = AzToolsFramework::GetEntityById(entityId);
                const int state = entity ? static_cast<int>(entity->GetState()) : -1;
                if (detail)
                {
                    *detail = {editorEntity, editable, running, requesting, entity != nullptr, state};
                }
                return editorEntity && editable && !running && !requesting && entity && state == static_cast<int>(AZ::Entity::State::Active);
            }

            AZ::Outcome<AZStd::string, AZStd::string> LutProductPath(const AZStd::string& absolutePath)
            {
                auto* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
                char projectRoot[AZ_MAX_PATH_LEN] = {};
                if (!fileIO || !fileIO->ResolvePath("@projectroot@", projectRoot, AZ_MAX_PATH_LEN))
                {
                    return AZ::Failure(AZStd::string("The project root is unavailable."));
                }
                const AZ::IO::Path root = AZ::IO::Path(projectRoot).LexicallyNormal();
                const AZ::IO::Path source = AZ::IO::Path(absolutePath).LexicallyNormal();
                if (root.StringAsPosix().starts_with("//") || source.StringAsPosix().starts_with("//") ||
                    !root.IsAbsolute() || !source.IsAbsolute() || !source.IsRelativeTo(root) || source.Extension() != ".azasset")
                {
                    return AZ::Failure(AZStd::string("The LUT source must be an absolute .azasset path inside this project."));
                }
                return AZ::Success(source.LexicallyRelative(root).StringAsPosix());
            }

            bool ValidLutShaper(AZ::Render::ShaperPresetType shaper, float minimum, float maximum)
            {
                return static_cast<AZ::u32>(shaper) < static_cast<AZ::u32>(AZ::Render::ShaperPresetType::NumShaperTypes) &&
                    std::isfinite(minimum) && std::isfinite(maximum) &&
                    ((shaper != AZ::Render::ShaperPresetType::LinearCustomRange && shaper != AZ::Render::ShaperPresetType::Log2CustomRange) || minimum < maximum);
            }

            bool ConvertGeneratedLutMetadata(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& node)
            {
                if (node.GetVersion() < 3)
                {
                    // Historical LUT paths have no encoding metadata: activation must require regeneration.
                    const int index = node.FindElement(AZ_CRC_CE("generatedLutMetadataValid"));
                    return index >= 0 ? node.GetSubElement(index).SetData(context, false) :
                        node.AddElementWithData(context, "generatedLutMetadataValid", false) >= 0;
                }
                return true;
            }

            // Composition can deactivate this HDR component. The transaction owns local snapshots,
            // never a callback or operation belonging to the component being disabled.
            AZ::Outcome<void, AZStd::string> ActivateLutOnEntity(
                AZ::EntityId entityId, AZ::ComponentId hdrId, const AZ::Data::Asset<JE::Render::RPI::AnyAsset>& asset,
                AZ::Render::ShaperPresetType shaper, float minimumExposure, float maximumExposure)
            {
                if (!CanEditLutEntity(entityId) || !AzToolsFramework::EditorComponentAPIBus::HasHandlers() ||
                    !ValidLutShaper(shaper, minimumExposure, maximumExposure))
                {
                    return AZ::Failure(AZStd::string("Activate LUT requires an editable Editor entity and component composition services."));
                }
                AZ::Entity::ComponentArrayType components;
                AzToolsFramework::GetAllComponentsForEntity(entityId, components);
                AZ::Component* hdr = nullptr;
                EditorLookModificationComponent* look = nullptr;
                size_t lookCount = 0;
                for (auto* component : components)
                {
                    if (component->GetId() == hdrId && azrtti_cast<EditorHDRColorGradingComponent*>(component))
                    {
                        hdr = component;
                    }
                    if (auto* candidate = azrtti_cast<EditorLookModificationComponent*>(component))
                    {
                        look = candidate;
                        ++lookCount;
                    }
                }
                if (!hdr || lookCount > 1)
                {
                    return AZ::Failure(AZStd::string("The exact HDR component must exist and Look Modification must be unambiguous."));
                }
                const AZ::EntityComponentIdPair hdrPair(entityId, hdrId);
                bool hdrEnabled = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    hdrEnabled, &AzToolsFramework::EditorComponentAPIRequests::IsComponentEnabled, hdrPair);
                if (!hdrEnabled)
                {
                    return AZ::Failure(AZStd::string("The source HDR component must be active before LUT activation."));
                }
                JE::Render::Scene::LookModificationComponentConfig previousConfig;
                bool lookEnabled = false;
                bool lookPending = false;
                if (look)
                {
                    if (!look->GetConfiguration(previousConfig))
                    {
                        return AZ::Failure(AZStd::string("Could not snapshot the existing Look Modification configuration."));
                    }
                    AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                        lookEnabled, &AzToolsFramework::EditorComponentAPIRequests::IsComponentEnabled,
                        AZ::EntityComponentIdPair(entityId, look->GetId()));
                    AzToolsFramework::EditorPendingCompositionRequestBus::EventResult(
                        lookPending, entityId, &AzToolsFramework::EditorPendingCompositionRequests::IsComponentPending, look);
                    bool lookDisabled = false;
                    AzToolsFramework::EditorDisabledCompositionRequestBus::EventResult(
                        lookDisabled, entityId, &AzToolsFramework::EditorDisabledCompositionRequests::IsComponentDisabled, look);
                    if (!lookEnabled && !lookPending && !lookDisabled)
                    {
                        return AZ::Failure(AZStd::string("The existing Look Modification has an unsupported composition state."));
                    }
                }
                const bool createdLook = look == nullptr;
                bool changedLook = false;
                AzToolsFramework::ScopedUndoBatch undo("Activate HDR color grading LUT");
                const auto rollback = [&]()
                {
                    bool restored = true;
                    if (look)
                    {
                        const AZ::EntityComponentIdPair pair(entityId, look->GetId());
                        if (createdLook)
                        {
                            bool removed = false;
                            AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                                removed, &AzToolsFramework::EditorComponentAPIRequests::RemoveComponents,
                                AZStd::vector<AZ::EntityComponentIdPair>{ pair });
                            restored &= removed;
                        }
                        else
                        {
                            if (changedLook)
                            {
                                const bool hasHandler = JE::Render::Scene::LookModificationRequestBus::HasHandlers(entityId);
                                restored &= hasHandler;
                                if (hasHandler)
                                {
                                    JE::Render::Scene::LookModificationRequestBus::Event(
                                        entityId, &JE::Render::Scene::LookModificationRequests::SetColorGradingLut, previousConfig.m_colorGradingLut);
                                    JE::Render::Scene::LookModificationRequestBus::Event(
                                        entityId, &JE::Render::Scene::LookModificationRequests::SetShaperPresetType, previousConfig.m_shaperPresetType);
                                    JE::Render::Scene::LookModificationRequestBus::Event(
                                        entityId, &JE::Render::Scene::LookModificationRequests::SetCustomMinExposure, previousConfig.m_customMinExposure);
                                    JE::Render::Scene::LookModificationRequestBus::Event(
                                        entityId, &JE::Render::Scene::LookModificationRequests::SetCustomMaxExposure, previousConfig.m_customMaxExposure);
                                    JE::Render::Scene::LookModificationRequestBus::Event(
                                        entityId, &JE::Render::Scene::LookModificationRequests::SetEnabled, previousConfig.m_enabled);
                                    JE::Render::Scene::LookModificationComponentConfig restoredConfig;
                                    restored &= look->GetConfiguration(restoredConfig) &&
                                        restoredConfig.m_colorGradingLut.GetId() == previousConfig.m_colorGradingLut.GetId() &&
                                        restoredConfig.m_enabled == previousConfig.m_enabled &&
                                        restoredConfig.m_shaperPresetType == previousConfig.m_shaperPresetType &&
                                        restoredConfig.m_customMinExposure == previousConfig.m_customMinExposure &&
                                        restoredConfig.m_customMaxExposure == previousConfig.m_customMaxExposure;
                                }
                            }
                            if (!lookEnabled)
                            {
                                bool disabled = false;
                                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                                    disabled, &AzToolsFramework::EditorComponentAPIRequests::DisableComponents,
                                    AZStd::vector<AZ::EntityComponentIdPair>{ pair });
                                restored &= disabled;
                            }
                        }
                    }
                    if (hdrEnabled)
                    {
                        bool enabled = false;
                        AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                            enabled, &AzToolsFramework::EditorComponentAPIRequests::EnableComponents,
                            AZStd::vector<AZ::EntityComponentIdPair>{ hdrPair });
                        restored &= enabled;
                    }
                    if (look && !createdLook && lookPending)
                    {
                        // Re-enable against the restored HDR service to restore pending, not disabled.
                        bool enabled = false;
                        AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                            enabled, &AzToolsFramework::EditorComponentAPIRequests::EnableComponents,
                            AZStd::vector<AZ::EntityComponentIdPair>{ AZ::EntityComponentIdPair(entityId, look->GetId()) });
                        bool pending = false;
                        AzToolsFramework::EditorPendingCompositionRequestBus::EventResult(
                            pending, entityId, &AzToolsFramework::EditorPendingCompositionRequests::IsComponentPending, look);
                        restored &= !enabled && pending;
                    }
                    AZ_Error("EditorHDRColorGradingComponent", restored, "LUT activation rollback could not restore component composition.");
                    return restored;
                };
                bool disabled = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    disabled, &AzToolsFramework::EditorComponentAPIRequests::DisableComponents,
                    AZStd::vector<AZ::EntityComponentIdPair>{ hdrPair });
                if (!disabled)
                {
                    rollback();
                    return AZ::Failure(AZStd::string("Could not disable HDR Color Grading; no LUT activation was committed."));
                }
                if (createdLook)
                {
                    AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome added = AZ::Failure(AZStd::string());
                    AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                        added, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
                        AzToolsFramework::EntityIdList{ entityId }, AZ::ComponentTypeList{ azrtti_typeid<EditorLookModificationComponent>() });
                    // Find the newly-created component even on a failed composition result so rollback owns it.
                    components.clear();
                    AzToolsFramework::GetAllComponentsForEntity(entityId, components);
                    for (auto* component : components)
                    {
                        if (auto* candidate = azrtti_cast<EditorLookModificationComponent*>(component))
                        {
                            look = candidate;
                            break;
                        }
                    }
                    if (!added.IsSuccess() || !look)
                    {
                        rollback();
                        return AZ::Failure(AZStd::string("Could not create Look Modification; activation was not committed. See rollback diagnostics."));
                    }
                }
                const AZ::EntityComponentIdPair lookPair(entityId, look->GetId());
                bool enabled = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    enabled, &AzToolsFramework::EditorComponentAPIRequests::EnableComponents,
                    AZStd::vector<AZ::EntityComponentIdPair>{ lookPair });
                if (!enabled || !JE::Render::Scene::LookModificationRequestBus::HasHandlers(entityId))
                {
                    rollback();
                    return AZ::Failure(AZStd::string("Look Modification could not become active; activation was not committed. See rollback diagnostics."));
                }
                changedLook = true;
                JE::Render::Scene::LookModificationRequestBus::Event(entityId, &JE::Render::Scene::LookModificationRequests::SetColorGradingLut, asset);
                JE::Render::Scene::LookModificationRequestBus::Event(entityId, &JE::Render::Scene::LookModificationRequests::SetShaperPresetType, shaper);
                JE::Render::Scene::LookModificationRequestBus::Event(entityId, &JE::Render::Scene::LookModificationRequests::SetCustomMinExposure, minimumExposure);
                JE::Render::Scene::LookModificationRequestBus::Event(entityId, &JE::Render::Scene::LookModificationRequests::SetCustomMaxExposure, maximumExposure);
                JE::Render::Scene::LookModificationRequestBus::Event(entityId, &JE::Render::Scene::LookModificationRequests::SetEnabled, true);
                JE::Render::Scene::LookModificationComponentConfig applied;
                if (!look->GetConfiguration(applied) || applied.m_colorGradingLut.GetId() != asset.GetId() || !applied.m_enabled ||
                    applied.m_shaperPresetType != shaper || applied.m_customMinExposure != minimumExposure ||
                    applied.m_customMaxExposure != maximumExposure)
                {
                    rollback();
                    return AZ::Failure(AZStd::string("Look Modification did not retain the exact LUT and its encoding shaper; activation was not committed. See rollback diagnostics."));
                }
                AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityId);
                AzToolsFramework::PropertyEditorGUIMessagesBus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::Refresh_EntireTree);
                return AZ::Success();
            }
        }

        struct EditorHDRColorGradingComponent::LutOperation
        {
            enum class Phase { PrepareCapture, Capture, Asset };
            AZStd::mutex m_mutex;
            bool m_cancelled = false;
            bool m_received = false;
            JE::Render::RPI::AttachmentReadback::ReadbackResult m_readback{};
            bool m_metadataReceived = false;
            JE::ColorGrading::LutGenerationCaptureMetadata m_frameEncoding;
            AZ::Render::FrameCaptureId m_submittedCaptureId = AZ::Render::InvalidFrameCaptureId;
            Phase m_phase = Phase::PrepareCapture;
            bool m_waitOneFrame = true;
            bool m_activate = false;
            bool m_previousGenerateLut = false;
            AZ::Render::ShaperPresetType m_shaper = AZ::Render::ShaperPresetType::None;
            float m_minimumExposure = 0.0f;
            float m_maximumExposure = 0.0f;
            bool m_captureFinished = false;
            AZ::Render::FrameCaptureId m_captureId = AZ::Render::InvalidFrameCaptureId;
            AZ::Render::FrameCaptureResult m_captureResult = AZ::Render::FrameCaptureResult::None;
            AZStd::string m_captureInfo;
            AZStd::string m_outputBase;
            AZStd::string m_sourcePath;
            AZStd::string m_productPath;
            AZ::Data::Asset<JE::Render::RPI::AnyAsset> m_asset;
            AZStd::chrono::steady_clock::time_point m_deadline = AZStd::chrono::steady_clock::now() + LutTimeout;
        };

        void EditorHDRColorGradingComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorHDRColorGradingComponent, BaseClass>()
                    ->Version(3, &ConvertGeneratedLutMetadata)
                    ->Field("generatedLut", &EditorHDRColorGradingComponent::m_generatedLutAbsolutePath)
                    ->Field("generatedLutMetadataValid", &EditorHDRColorGradingComponent::m_generatedLutMetadataValid)
                    ->Field("generatedLutShaper", &EditorHDRColorGradingComponent::m_generatedLutShaper)
                    ->Field("generatedLutMinimumExposure", &EditorHDRColorGradingComponent::m_generatedLutMinimumExposure)
                    ->Field("generatedLutMaximumExposure", &EditorHDRColorGradingComponent::m_generatedLutMaximumExposure)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorHDRColorGradingComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "HDR Color Grading"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Tune and apply color grading in HDR."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/atom-guide/features/#post-processing-effects-postfx")
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Generation"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        // DPE group actions must precede its fields and use the empty group path.
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", QT_TRANSLATE_NOOP("AtomLyIntegration", "Generates a LUT from the scene's enabled color grading blend."))
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generate LUT"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::GenerateLut)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", QT_TRANSLATE_NOOP("AtomLyIntegration", "Use the generated LUT asset in a Look Modification component"))
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Activate LUT"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::ActivateLut)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorHDRColorGradingComponent::m_generatedLutAbsolutePath, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generated LUT Path"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Generated LUT Path"))
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ;

                    editContext->Class<JE::Render::Scene::HDRColorGradingComponentController>(
                        "HDRColorGradingComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::HDRColorGradingComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::HDRColorGradingComponentConfig>("HDRColorGradingComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::HDRColorGradingComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable HDR color grading"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable HDR color grading."))
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Color Adjustment"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorAdjustmentWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of color adjustments"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingExposure, QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingContrast, QT_TRANSLATE_NOOP("AtomLyIntegration", "Contrast"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Contrast Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingPreSaturation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Pre Saturation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Pre Saturation Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingFilterIntensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Intensity Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(AZ::Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingFilterMultiply, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Multiply"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Multiply Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorFilterSwatch, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Swatch"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Color Filter Swatch Value"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "White Balance"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_whiteBalanceWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of white balance"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_whiteBalanceKelvin, QT_TRANSLATE_NOOP("AtomLyIntegration", "Temperature"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Temperature in Kelvin"))
                            ->Attribute(AZ::Edit::Attributes::Min, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 40000.0f)
                            ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.165f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_whiteBalanceTint, QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_whiteBalanceLuminancePreservation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Luminance Preservation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulate the preservation of luminance"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Toning"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_splitToneWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulates the split toning effect."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_splitToneBalance, QT_TRANSLATE_NOOP("AtomLyIntegration", "Balance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Balance Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, -1.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_splitToneShadowsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Shadows Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_splitToneHighlightsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Highlights Color"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::HDRColorGradingComponentConfig::m_channelMixingRed, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Red"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Red Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::HDRColorGradingComponentConfig::m_channelMixingGreen, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Green"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Green Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::HDRColorGradingComponentConfig::m_channelMixingBlue, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Blue"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Blue Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow Midtones Highlights"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulates the SMH effect."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhShadowsStart, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Start"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows Start Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhShadowsEnd, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows End"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows End Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhHighlightsStart, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Start"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights Start Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhHighlightsEnd, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights End"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights End Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhShadowsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhMidtonesColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Midtones Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Midtones Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::HDRColorGradingComponentConfig::m_smhHighlightsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights Color"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Final Adjustment"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_finalAdjustmentWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of final adjustments"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingHueShift, QT_TRANSLATE_NOOP("AtomLyIntegration", "Hue Shift"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Hue Shift Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::HDRColorGradingComponentConfig::m_colorGradingPostSaturation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Post Saturation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Post Saturation Value"))
                            ->Attribute(AZ::Edit::Attributes::Min, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Generation"))
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::HDRColorGradingComponentConfig::m_lutResolution, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Resolution"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Resolution of generated LUT"))
                            ->EnumAttribute(AZ::Render::LutResolution::Lut16x16x16, QT_TRANSLATE_NOOP("AtomLyIntegration", "16x16x16"))
                            ->EnumAttribute(AZ::Render::LutResolution::Lut32x32x32, QT_TRANSLATE_NOOP("AtomLyIntegration", "32x32x32"))
                            ->EnumAttribute(AZ::Render::LutResolution::Lut64x64x64, QT_TRANSLATE_NOOP("AtomLyIntegration", "64x64x64"))
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::HDRColorGradingComponentConfig::m_shaperPresetType,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type."))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::None, QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::LinearCustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Linear Custom Range"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_48Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 48 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_1000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 1000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_2000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 2000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_4000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 4000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2CustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 Custom Range"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::PqSmpteSt2084, QT_TRANSLATE_NOOP("AtomLyIntegration", "PQ (SMPTE ST 2084)"))
                        ;
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorHDRColorGradingRequestBus>("EditorHDRColorGradingRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Event("GenerateLutAsync", &EditorHDRColorGradingRequests::GenerateLutAsync)
                    ->Event("ActivateLutAsync", &EditorHDRColorGradingRequests::ActivateLutAsync)
                    ;

                behaviorContext->EBus<EditorHDRColorGradingNotificationBus>("EditorHDRColorGradingNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Handler<JE::Editor::Render::Scene::Internal::EditorHDRColorGradingNotificationBusHandler>()
                    ->Event("OnGenerateLutCompleted", &EditorHDRColorGradingNotificationBus::Events::OnGenerateLutCompleted)
                    ->Event("OnActivateLutCompleted", &EditorHDRColorGradingNotificationBus::Events::OnActivateLutCompleted)
                    ;
            }
        }

        EditorHDRColorGradingComponent::EditorHDRColorGradingComponent(const JE::Render::Scene::HDRColorGradingComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorHDRColorGradingComponent::Activate()
        {
            BaseClass::Activate();
            EditorHDRColorGradingRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorHDRColorGradingComponent::Deactivate()
        {
            CancelOperation();
            EditorHDRColorGradingRequestBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorHDRColorGradingComponent::CancelOperation()
        {
            AZ::TickBus::Handler::BusDisconnect();
            JE::ColorGrading::LutGenerationCaptureNotificationBus::Handler::BusDisconnect();
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            if (m_operation)
            {
                {
                    AZStd::scoped_lock lock(m_operation->m_mutex);
                    m_operation->m_cancelled = true;
                }
                if (m_operation->m_captureId != AZ::Render::InvalidFrameCaptureId)
                {
                    AZ::Render::FrameCaptureRequestBus::Broadcast(
                        &AZ::Render::FrameCaptureRequests::CancelPendingCapture, m_operation->m_captureId);
                    m_operation->m_captureId = AZ::Render::InvalidFrameCaptureId;
                }
                if (!m_operation->m_activate)
                {
                    m_controller.SetGenerateLut(m_operation->m_previousGenerateLut);
                }
                m_operation.reset();
            }
            // Pending pass reservations are released above. A submitted GPU readback finishes
            // with cancelled shared state and cannot publish files or component changes.
        }

        void EditorHDRColorGradingComponent::FailOperation(const AZStd::string& error)
        {
            CancelOperation();
            AZ_Error("EditorHDRColorGradingComponent", false, "%s", error.c_str());
        }

        bool EditorHDRColorGradingComponent::BeginAssetWait(const AZStd::string& absolutePath, bool activate)
        {
            const auto relative = LutProductPath(absolutePath);
            if (!relative.IsSuccess())
            {
                FailOperation(relative.GetError());
                return false;
            }
            if (!m_operation)
            {
                m_operation = AZStd::make_shared<LutOperation>();
            }
            if (activate)
            {
                m_operation->m_shaper = m_generatedLutShaper;
                m_operation->m_minimumExposure = m_generatedLutMinimumExposure;
                m_operation->m_maximumExposure = m_generatedLutMaximumExposure;
            }
            m_operation->m_phase = LutOperation::Phase::Asset;
            m_operation->m_activate = activate;
            m_operation->m_sourcePath = absolutePath;
            m_operation->m_productPath = relative.GetValue();
            if (!AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusConnect();
            }
            return true;
        }

        void EditorHDRColorGradingComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            const auto operation = m_operation;
            if (!operation)
            {
                return;
            }
            if (!CanEditLutEntity(GetEntityId()))
            {
                FailOperation("LUT operation cancelled: the source entity is no longer editable in the Editor.");
                return;
            }
            if (AZStd::chrono::steady_clock::now() >= operation->m_deadline)
            {
                if (operation->m_phase == LutOperation::Phase::Asset)
                {
                    FailOperation("LUT operation timed out waiting for Asset Processor/catalogue and a ready LUT asset.");
                }
                else
                {
                    bool received = false;
                    {
                        AZStd::scoped_lock lock(operation->m_mutex);
                        received = operation->m_received;
                    }
                    auto* contexts = AZ::Interface<JE::Render::RPI::ViewportContextRequestsInterface>::Get();
                    const auto viewport = contexts ? contexts->GetDefaultViewportContext() : nullptr;
                    const auto pipeline = viewport ? viewport->GetCurrentPipeline() : nullptr;
                    FailOperation(AZStd::string::format(
                        "LUT operation timed out waiting for the viewport/LutGenerationPass readback. "
                        "phase=%s received=%d captureFinished=%d captureResult=%d viewport=%d pipeline=%d pipelineId=%s outputBase=%s generateLut=%d.",
                        operation->m_phase == LutOperation::Phase::PrepareCapture ? "PrepareCapture" : "Capture",
                        received, operation->m_captureFinished, static_cast<int>(operation->m_captureResult),
                        viewport != nullptr, pipeline != nullptr, pipeline ? pipeline->GetId().GetCStr() : "<none>",
                        operation->m_outputBase.c_str(), m_controller.m_configuration.m_generateLut));
                }
                return;
            }
            if (operation->m_phase == LutOperation::Phase::PrepareCapture)
            {
                if (operation->m_waitOneFrame)
                {
                    operation->m_waitOneFrame = false;
                    return;
                }
                auto* contexts = AZ::Interface<JE::Render::RPI::ViewportContextRequestsInterface>::Get();
                const auto viewport = contexts ? contexts->GetDefaultViewportContext() : nullptr;
                const auto pipeline = viewport ? viewport->GetCurrentPipeline() : nullptr;
                if (!pipeline)
                {
                    return;
                }
                AZ::Render::FrameCaptureOutcome outcome;
                AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                    outcome, &AZ::Render::FrameCaptureRequests::CapturePassAttachmentWithCallback,
                    [operation](const JE::Render::RPI::AttachmentReadback::ReadbackResult& result)
                    {
                        AZStd::scoped_lock lock(operation->m_mutex);
                        if (operation->m_cancelled || operation->m_received)
                        {
                            return;
                        }
                        operation->m_readback = result;
                        if (result.m_dataBuffer)
                        {
                            operation->m_readback.m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>(*result.m_dataBuffer);
                        }
                        for (auto& mip : operation->m_readback.m_mipDataBuffers)
                        {
                            if (mip.m_mipBuffer == result.m_dataBuffer)
                            {
                                mip.m_mipBuffer = operation->m_readback.m_dataBuffer;
                            }
                            else if (mip.m_mipBuffer)
                            {
                                mip.m_mipBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>(*mip.m_mipBuffer);
                            }
                        }
                        operation->m_received = true;
                    },
                    AZStd::vector<AZStd::string>{ pipeline->GetId().GetCStr(), "LutGenerationPass" },
                    AZStd::string("LutOutput"), JE::Render::RPI::PassAttachmentReadbackOption::Output);
                if (!outcome.IsSuccess())
                {
                    FailOperation(AZStd::string::format("LUT readback initialization failed: %s", outcome.GetError().m_errorMessage.c_str()));
                    return;
                }
                operation->m_captureId = outcome.GetValue();
                operation->m_submittedCaptureId = outcome.GetValue();
                operation->m_phase = LutOperation::Phase::Capture;
                // The request reserves a pass attachment; publication occurs in its subsequent FrameBegin.
                JE::ColorGrading::LutGenerationCaptureNotificationBus::Handler::BusConnect(outcome.GetValue());
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(outcome.GetValue());
                return;
            }
            if (operation->m_phase == LutOperation::Phase::Capture)
            {
                if (!operation->m_captureFinished)
                {
                    return;
                }
                if (operation->m_captureResult != AZ::Render::FrameCaptureResult::Success)
                {
                    FailOperation(AZStd::string::format("LUT readback failed: %s", operation->m_captureInfo.c_str()));
                    return;
                }
                JE::Render::RPI::AttachmentReadback::ReadbackResult readback{};
                JE::ColorGrading::LutGenerationCaptureMetadata encoding;
                bool metadataReceived = false;
                {
                    AZStd::scoped_lock lock(operation->m_mutex);
                    if (!operation->m_received)
                    {
                        return;
                    }
                    readback = operation->m_readback;
                    metadataReceived = operation->m_metadataReceived;
                    encoding = operation->m_frameEncoding;
                }
                if (!metadataReceived || !encoding.MatchesReadback(operation->m_submittedCaptureId, readback) ||
                    !ValidLutShaper(encoding.m_shaper, encoding.m_minimumExposure, encoding.m_maximumExposure))
                {
                    FailOperation("LUT capture has missing or incompatible effective frame encoding metadata (capture ID, dimensions or shaper). Regenerate the LUT.");
                    return;
                }
                operation->m_shaper = encoding.m_shaper;
                operation->m_minimumExposure = encoding.m_minimumExposure;
                operation->m_maximumExposure = encoding.m_maximumExposure;
                const auto written = JE::Editor::Render::Scene::WriteLutSource(readback, operation->m_outputBase);
                if (!written.IsSuccess())
                {
                    FailOperation(written.GetError());
                    return;
                }
                {
                    AZStd::scoped_lock lock(operation->m_mutex);
                    operation->m_readback = {};
                }
                m_controller.SetGenerateLut(operation->m_previousGenerateLut);
                if (!BeginAssetWait(written.GetValue(), false))
                {
                    return;
                }
            }
            if (!operation->m_asset.GetId().IsValid())
            {
                AZ::Data::AssetId assetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                    operation->m_productPath.c_str(), azrtti_typeid<JE::Render::RPI::AnyAsset>(), false);
                if (!assetId.IsValid())
                {
                    return; // New source: catalogue discovery is asynchronous and bounded by the deadline.
                }
                operation->m_asset = AZ::Data::AssetManager::Instance().GetAsset<JE::Render::RPI::AnyAsset>(assetId, AZ::Data::AssetLoadBehavior::QueueLoad);
            }
            if (operation->m_asset.IsError())
            {
                FailOperation("The LUT product failed to load. Check Asset Processor errors.");
                return;
            }
            if (!operation->m_asset.IsReady())
            {
                return;
            }
            const auto* lut = operation->m_asset->GetDataAs<AZ::Render::LookupTableAsset>();
            if (!lut)
            {
                FailOperation("The .azasset product does not contain LookupTableAsset.");
                return;
            }
            const auto valid = JE::Editor::Render::Scene::ValidateLookupTable(*lut);
            if (!valid.IsSuccess())
            {
                FailOperation(valid.GetError());
                return;
            }
            const auto entityId = GetEntityId();
            if (operation->m_activate)
            {
                const auto asset = operation->m_asset;
                const auto componentId = GetId();
                const auto shaper = operation->m_shaper;
                const auto minimumExposure = operation->m_minimumExposure;
                const auto maximumExposure = operation->m_maximumExposure;
                CancelOperation();
                const auto activated = ActivateLutOnEntity(entityId, componentId, asset, shaper, minimumExposure, maximumExposure);
                if (!activated.IsSuccess())
                {
                    AZ_Error("EditorHDRColorGradingComponent", false, "%s", activated.GetError().c_str());
                    return;
                }
                // The HDR component is now disabled. Use only local values after composition.
                EditorHDRColorGradingNotificationBus::Event(
                    entityId, &EditorHDRColorGradingNotification::OnActivateLutCompleted);
                return;
            }
            const auto sourcePath = operation->m_sourcePath;
            CancelOperation();
            {
                AzToolsFramework::ScopedUndoBatch undo("Generate HDR color grading LUT");
                m_generatedLutAbsolutePath = sourcePath;
                m_generatedLutShaper = operation->m_shaper;
                m_generatedLutMinimumExposure = operation->m_minimumExposure;
                m_generatedLutMaximumExposure = operation->m_maximumExposure;
                m_generatedLutMetadataValid = true;
                AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityId);
            }
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
            EditorHDRColorGradingNotificationBus::Event(
                entityId, &EditorHDRColorGradingNotification::OnGenerateLutCompleted, sourcePath);
        }

        void EditorHDRColorGradingComponent::OnLutGenerationCapture(
            const JE::ColorGrading::LutGenerationCaptureMetadata& metadata)
        {
            const auto operation = m_operation;
            if (!operation || operation->m_phase != LutOperation::Phase::Capture)
            {
                return;
            }
            AZStd::scoped_lock lock(operation->m_mutex);
            if (!operation->m_cancelled && !operation->m_metadataReceived &&
                metadata.m_captureId == operation->m_submittedCaptureId)
            {
                operation->m_frameEncoding = metadata;
                operation->m_metadataReceived = true;
            }
        }

        void EditorHDRColorGradingComponent::OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info)
        {
            JE::ColorGrading::LutGenerationCaptureNotificationBus::Handler::BusDisconnect();
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            if (m_operation && m_operation->m_phase == LutOperation::Phase::Capture)
            {
                m_operation->m_captureId = AZ::Render::InvalidFrameCaptureId;
                m_operation->m_captureResult = result;
                m_operation->m_captureInfo = info;
                m_operation->m_captureFinished = true;
            }
        }

        void EditorHDRColorGradingComponent::GenerateLut()
        {
            LutEntityEditState entityState;
            const bool canEdit = CanEditLutEntity(GetEntityId(), &entityState);
            if (m_operation || !canEdit || !m_controller.m_configuration.m_enabled)
            {
                AZ_Error("EditorHDRColorGradingComponent", false,
                    "Generate LUT requires enabled HDR Color Grading on an editable Editor entity and no pending LUT operation. "
                    "EntityId=%s ComponentId=%llu operation=%d enabled=%d editorEntity=%d editable=%d running=%d requesting=%d "
                    "entityExists=%d state=%d activeState=%d.",
                    GetEntityId().ToString().c_str(), static_cast<unsigned long long>(GetId()), static_cast<bool>(m_operation),
                    m_controller.m_configuration.m_enabled, entityState.m_editorEntity, entityState.m_editable,
                    entityState.m_running, entityState.m_requesting, entityState.m_entityExists, entityState.m_state,
                    static_cast<int>(AZ::Entity::State::Active));
                return;
            }
            const auto& config = m_controller.m_configuration;
            if (!ValidLutShaper(config.m_shaperPresetType, config.m_customMinExposure, config.m_customMaxExposure))
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "Generate LUT requires a valid shaper and finite custom exposure range.");
                return;
            }
            auto* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            char resolvedPath[AZ_MAX_PATH_LEN] = {};
            const AZStd::string output = "@projectroot@/" + AZStd::string::format(
                GeneratedLutRelativePath, AZ::Uuid::CreateRandom().ToString<AZStd::string>().c_str());
            if (!fileIO || !fileIO->ResolvePath(output.c_str(), resolvedPath, AZ_MAX_PATH_LEN))
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "Could not resolve a project-local LUT output path.");
                return;
            }
            const auto relative = LutProductPath(AZStd::string(resolvedPath) + ".azasset");
            if (!relative.IsSuccess())
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "%s", relative.GetError().c_str());
                return;
            }
            const AZ::IO::Path outputFolder = AZ::IO::Path(resolvedPath).ParentPath();
            if (!AZ::IO::SystemFile::CreateDir(outputFolder.c_str()))
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "Could not create the project-local LutGeneration output directory.");
                return;
            }
            m_operation = AZStd::make_shared<LutOperation>();
            m_operation->m_outputBase = resolvedPath;
            m_operation->m_previousGenerateLut = config.m_generateLut;
            m_operation->m_shaper = config.m_shaperPresetType;
            m_operation->m_minimumExposure = config.m_customMinExposure;
            m_operation->m_maximumExposure = config.m_customMaxExposure;
            m_controller.SetGenerateLut(true);
            AZ::TickBus::Handler::BusConnect();
        }

        AZ::u32 EditorHDRColorGradingComponent::ActivateLut()
        {
            if (m_operation || !CanEditLutEntity(GetEntityId()) || m_generatedLutAbsolutePath.empty())
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "Activate LUT requires a generated LUT and an editable Editor entity, with no pending operation.");
                return AZ::Edit::PropertyRefreshLevels::EntireTree;
            }
            if (!m_generatedLutMetadataValid ||
                !ValidLutShaper(m_generatedLutShaper, m_generatedLutMinimumExposure, m_generatedLutMaximumExposure))
            {
                AZ_Error("EditorHDRColorGradingComponent", false, "The generated LUT has no valid encoding metadata. Regenerate the LUT before activation.");
                return AZ::Edit::PropertyRefreshLevels::EntireTree;
            }
            BeginAssetWait(m_generatedLutAbsolutePath, true);
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        bool EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings()
        {
            return !m_generatedLutAbsolutePath.empty();
        }

        AZ::u32 EditorHDRColorGradingComponent::OnConfigurationChanged()
        {
            CancelOperation();
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //! EditorHDRColorGradingRequestBus overrides...
        void EditorHDRColorGradingComponent::GenerateLutAsync()
        {
            GenerateLut();
        }

        void EditorHDRColorGradingComponent::ActivateLutAsync()
        {
            ActivateLut();
        }
        ////////////////////////////////////////////////

    } // namespace Render
} // namespace AZ
