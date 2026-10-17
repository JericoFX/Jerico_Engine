/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/ExposureControl/EditorExposureControlComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorExposureControlComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorExposureControlComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorExposureControlComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure Control"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure component control exposure value for rendered scene."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/exposure-control/") // [TODO ATOM-2672][PostFX] need create page for PostProcessing.
                        ;

                    editContext->Class<JE::Render::Scene::ExposureControlComponentController>(
                        "ExposureControlComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::ExposureControlComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::ExposureControlComponentConfig>("ExposureControlComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::ExposureControlComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable exposure control."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox,
                            &JE::Render::Scene::ExposureControlComponentConfig::m_exposureControlType,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Control Type"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "How to control a exposure value."))
                        ->EnumAttribute(AZ::Render::ExposureControl::ExposureControlType::ManualOnly, QT_TRANSLATE_NOOP("AtomLyIntegration", "Manual Only"))
                        ->EnumAttribute(AZ::Render::ExposureControl::ExposureControlType::EyeAdaptation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Eye Adaptation"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ExposureControlComponentConfig::m_manualCompensationValue, QT_TRANSLATE_NOOP("AtomLyIntegration", "Manual Compensation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Manual exposure compensation value."))
                        ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Eye Adaptation"))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::ExposureControlComponentConfig::IsEyeAdaptation)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ExposureControlComponentConfig::m_autoExposureMin, QT_TRANSLATE_NOOP("AtomLyIntegration", "Minimum Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Minimum exposure value for the auto exposure."))
                        ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ExposureControlComponentConfig::m_autoExposureMax, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum exposure value for the auto exposure."))
                        ->Attribute(AZ::Edit::Attributes::Min, -16.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 16.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ExposureControlComponentConfig::m_autoExposureSpeedUp, QT_TRANSLATE_NOOP("AtomLyIntegration", "Speed Up"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The speed at which auto exposure adapts to bright scenes."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ExposureControlComponentConfig::m_autoExposureSpeedDown, QT_TRANSLATE_NOOP("AtomLyIntegration", "Speed Down"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The speed at which auto exposure adapts to dark scenes."))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ExposureControlComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::ExposureControlComponentConfig::m_heatmapEnabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Heatmap"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Areas below minimum exposure will be highlighted in blue. Areas above in red."))
                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::ExposureControlComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorExposureControlComponent>()->RequestBus("ExposureControlRequestBus");

                behaviorContext->ConstantProperty("EditorExposureControlComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorExposureControlComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorExposureControlComponent::EditorExposureControlComponent(const JE::Render::Scene::ExposureControlComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorExposureControlComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
