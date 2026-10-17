/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/EditorDirectionalLightComponent.h>

#include <JE/Editor/Render/RPI/Edit/Common/ColorUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/Feature/CoreLights/PhotometricValue.h>

#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        EditorDirectionalLightComponent::EditorDirectionalLightComponent(const JE::Render::Scene::DirectionalLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorDirectionalLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDirectionalLightComponent, BaseClass>()
                    ->Version(3, ConvertToEditorRenderComponentAdapter<2>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDirectionalLightComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Directional Light"),
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "A directional light to cast a shadow of meshes onto meshes."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/directional-light/")
                        ;

                    editContext->Class<JE::Render::Scene::DirectionalLightComponentController>(
                        "DirectionalLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentController::m_configuration,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::DirectionalLightComponentConfig>("DirectionalLightComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::DirectionalLightComponentConfig::m_color,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color of the light"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute("ColorEditorConfiguration", JE::Render::RPI::ColorUtils::GetLinearRgbEditorConfig())
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_intensityMode,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity mode"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Allows specifying light values in lux or Ev100"))
                            ->EnumAttribute(AZ::Render::PhotometricUnit::Lux,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Lux"))
                            ->EnumAttribute(AZ::Render::PhotometricUnit::Ev100Illuminance, "Ev100")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_intensity,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity of the light in the set photometric unit."))
                            ->Attribute(AZ::Edit::Attributes::Min, &JE::Render::Scene::DirectionalLightComponentConfig::GetIntensityMin)
                            ->Attribute(AZ::Edit::Attributes::Max, &JE::Render::Scene::DirectionalLightComponentConfig::GetIntensityMax)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, &JE::Render::Scene::DirectionalLightComponentConfig::GetIntensitySoftMin)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, &JE::Render::Scene::DirectionalLightComponentConfig::GetIntensitySoftMax)
                            ->Attribute(AZ::Edit::Attributes::Suffix, &JE::Render::Scene::DirectionalLightComponentConfig::GetIntensitySuffix)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_angularDiameter,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Angular diameter"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Angular diameter of the directional light in degrees. The sun is about 0.5."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", " deg"))
                            
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentConfig::m_lightingChannelConfig,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Lighting Channels"), "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            
                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowEnabled,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Shadow"),
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Shadow for this directional light. Only one directional light can have shadow"))
                        ->DataElement(AZ::Edit::UIHandlers::EntityId, &JE::Render::Scene::DirectionalLightComponentConfig::m_cameraEntityId,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Camera"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Entity of the camera for cascaded shadowmap view frustum."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowFarClipDistance,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow far clip"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow specific far clip distance."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &JE::Render::Scene::DirectionalLightComponentConfig::UpdateCascadeFarDepths)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowmapSize,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadowmap size"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Width/Height of shadowmap"))
                            ->EnumAttribute(AZ::Render::ShadowmapSize::Size256, " 256")
                            ->EnumAttribute(AZ::Render::ShadowmapSize::Size512, " 512")
                            ->EnumAttribute(AZ::Render::ShadowmapSize::Size1024, "1024")
                            ->EnumAttribute(AZ::Render::ShadowmapSize::Size2048, "2048")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_cascadeCount,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Cascade count"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Number of cascades"))
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, AZ::Render::Shadow::MaxNumberOfCascades)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &JE::Render::Scene::DirectionalLightComponentConfig::UpdateCascadeFarDepths)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentConfig::m_isShadowmapFrustumSplitAutomatic,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Automatic splitting"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Switch splitting of shadowmap frustum to cascades automatically or not."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &JE::Render::Scene::DirectionalLightComponentConfig::UpdateCascadeFarDepths)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowmapFrustumSplitSchemeRatio,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Split ratio"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Ratio to lerp between the two types of frustum splitting scheme.\n"
                            "0 = Uniform scheme which will split the frustum evenly across all cascades.\n"
                            "1 = Logarithmic scheme which is designed to split the frustum in a logarithmic fashion "
                            "in order to enable us to produce a more optimal perspective aliasing across the frustum."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsSplitManual)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Vector4, &JE::Render::Scene::DirectionalLightComponentConfig::m_cascadeFarDepths,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Far depth cascade"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Far depth of each cascade.  The value of the index greater than or equal to cascade count is ignored."))
                            ->Attribute(AZ::Edit::Attributes::Min, JE::Render::Scene::DirectionalLightConstants::MIN_CASCADE_FAR_DEPTH)
                            ->Attribute(AZ::Edit::Attributes::Max, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowFarClipDistance)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsSplitAutomatic)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DirectionalLightComponentConfig::m_groundHeight,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Ground height"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Height of the ground. Used to correct position of cascades."))
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                " m")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsCascadeCorrectionDisabled)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::DirectionalLightComponentConfig::m_isCascadeCorrectionEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Cascade correction"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable position correction of cascades to optimize the appearance for certain camera positions."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::DirectionalLightComponentConfig::m_isDebugColoringEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug coloring"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable coloring to see how cascades places 0:red, 1:green, 2:blue, 3:yellow."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowFilterMethod,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow filter method"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Filtering method of edge-softening of shadows.\n"
                            "  None: No filtering\n"
                            "  PCF: Percentage-closer filtering\n"
                            "  ESM: Exponential shadow maps\n"
                            "  ESM+PCF: ESM with a PCF fallback\n"
                            "For BehaviorContext (or TrackView), None=0, PCF=1, ESM=2, ESM+PCF=3"))
                            ->EnumAttribute(AZ::Render::ShadowFilterMethod::None,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                            ->EnumAttribute(AZ::Render::ShadowFilterMethod::Pcf, "PCF")
                            ->EnumAttribute(AZ::Render::ShadowFilterMethod::Esm, "ESM")
                            ->EnumAttribute(AZ::Render::ShadowFilterMethod::EsmPcf, "ESM+PCF")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_filteringSampleCount,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Filtering sample count"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "This is used only when the pixel is predicted to be on the boundary.\n"
                            "Specific to PCF and ESM+PCF."))
                            ->Attribute(AZ::Edit::Attributes::Min, 4)
                            ->Attribute(AZ::Edit::Attributes::Max, 64)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowPcfDisabled)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_receiverPlaneBiasEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow Receiver Plane Bias Enable"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "This reduces shadow acne when using large pcf kernels."))
                          ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                          ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowPcfDisabled) 
                          ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                         ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_shadowBias,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow Bias"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Reduces acne by applying a fixed bias along z in shadow-space.\n"
                            "If this is 0, no biasing is applied.")) 
                           ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                           ->Attribute(AZ::Edit::Attributes::Max, 0.2) 
                           ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                           ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(
                           AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_normalShadowBias,
                           QT_TRANSLATE_NOOP("AtomLyIntegration", "Normal Shadow Bias"),
                           QT_TRANSLATE_NOOP("AtomLyIntegration", "Reduces acne by biasing the shadowmap lookup along the geometric normal.\n"
                           "If this is 0, no biasing is applied."))
                           ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                           ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                           ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                           ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_cascadeBlendingEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Blend between cascades"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enables smooth blending between shadow map cascades."))
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowPcfDisabled)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)

                        // Fullscreen Shadow Blur...
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_fullscreenBlurEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fullscreen Blur"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enables fullscreen blur on fullscreen sunlight shadows."))
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_fullscreenBlurConstFalloff,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Fullscreen Blur Strength"),
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Affects how strong the fullscreen shadow blur is. Recommended value is 0.67"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.95f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_fullscreenBlurDepthFalloffStrength,
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Fullscreen Blur Sharpness"),
                                QT_TRANSLATE_NOOP("AtomLyIntegration", "Affects how sharp the fullscreen shadow blur appears around edges. Recommended value is 50"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 400.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::DirectionalLightComponentConfig::IsShadowDisabled)

                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Global Illumination"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DirectionalLightComponentConfig::m_affectsGI,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Affects GI"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls whether this light affects diffuse global illumination."))
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DirectionalLightComponentConfig::m_affectsGIFactor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Factor"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Multiplier on the amount of contribution to diffuse global illumination."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDirectionalLightComponent>()->RequestBus("DirectionalLightRequestBus");

                behaviorContext->ConstantProperty("EditorDirectionalLightComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorDirectionalLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            }
        }

        void EditorDirectionalLightComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AZ::Render::ShadowingDirectionalLightNotificationsBus::Handler::BusConnect();
        }

        void EditorDirectionalLightComponent::Deactivate()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
            AZ::Render::ShadowingDirectionalLightNotificationsBus::Handler::BusDisconnect();
        }

        void EditorDirectionalLightComponent::DisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            static const auto forward = AZ::Vector3::CreateAxisY();
            static const auto up = AZ::Vector3::CreateAxisZ();
            static const auto right = AZ::Vector3::CreateAxisX();
            static const float arrowScale = 0.5f;
            static const float arrowLength = 1.5f;
            static const float arrowOffset = 0.75f;
            static const float originScale = 0.2f;
            static const float originScale0 = 0.1f;
            static const float originScale1 = 0.05f;

            auto transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                transform,
                GetEntityId(),
                &AZ::TransformBus::Events::GetWorldTM);

            transform.ExtractUniformScale();
            const AZ::Vector3 origin = transform.GetTranslation();
            const AZ::Vector3 originOffset = origin - (transform.TransformVector(forward) * arrowOffset);
            const AZ::Vector3 target = origin - (transform.TransformVector(forward) * (arrowLength + arrowOffset));

            const AZ::Vector3 originPozZ = originOffset + (transform.TransformVector(up) * (originScale + originScale1));
            const AZ::Vector3 targetPosZ = target + (transform.TransformVector(up) * (originScale + originScale1));
            const AZ::Vector3 originPosX = originOffset + (transform.TransformVector(right) * (originScale + originScale1));
            const AZ::Vector3 targetPosX = target + (transform.TransformVector(right) * (originScale + originScale1));
            const AZ::Vector3 originNegX = originOffset - (transform.TransformVector(right) * (originScale + originScale1));
            const AZ::Vector3 targetNegX = target - (transform.TransformVector(right) * (originScale + originScale1));
            const AZ::Vector3 originNegZ = originOffset - (transform.TransformVector(up) * (originScale + originScale1));
            const AZ::Vector3 targetNegZ = target - (transform.TransformVector(up) * (originScale + originScale1));

            debugDisplay.SetColor(m_controller.GetColor());
            debugDisplay.DrawWireDisk(origin, transform.TransformVector(-forward), originScale + originScale0);
            debugDisplay.DrawArrow(targetPosZ, originPozZ, arrowScale);
            debugDisplay.DrawArrow(targetNegZ, originNegZ, arrowScale);
            debugDisplay.DrawArrow(targetPosX, originPosX, arrowScale);
            debugDisplay.DrawArrow(targetNegX, originNegX, arrowScale);
            debugDisplay.DrawDisk(origin, transform.TransformVector(forward), originScale);
        }

        void EditorDirectionalLightComponent::OnShadowingDirectionalLightChanged(const AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle& handle)
        {
            if (m_controller.m_lightHandle != handle)
            {
                m_controller.m_configuration.m_shadowEnabled = false;
                SetDirty();
            }
        }

        AZ::u32 EditorDirectionalLightComponent::OnConfigurationChanged()
        {
            // Set the intenstiy of the photometric unit in case the controller is disabled. This is needed to correctly convert between photometric units.
            m_controller.m_photometricValue.SetIntensity(m_controller.m_configuration.m_intensity);

            // If the intensity mode changes in the editor, convert the photometric value and update the intensity
            if (m_controller.m_configuration.m_intensityMode != m_controller.m_photometricValue.GetType())
            {
                m_controller.m_photometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_intensity = m_controller.m_photometricValue.GetIntensity();
            }

            m_controller.m_configurationChangedEvent.Signal();

            BaseClass::OnConfigurationChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

    } // namespace Render
} // namespace AZ
