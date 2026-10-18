/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/MotionBlur/EditorMotionBlurComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        using MotionBlurQualityComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<JE::Render::Features::MotionBlur::SampleQuality>>;
        static MotionBlurQualityComboBoxVec PopulateMotionBlurQualityList()
        {
            return MotionBlurQualityComboBoxVec
            {
                { JE::Render::Features::MotionBlur::SampleQuality::Low, "Low" },
                { JE::Render::Features::MotionBlur::SampleQuality::Medium, "Medium" },
                { JE::Render::Features::MotionBlur::SampleQuality::High, "High" },
                { JE::Render::Features::MotionBlur::SampleQuality::Ultra, "Ultra" },
            };
        }

        void EditorMotionBlurComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMotionBlurComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMotionBlurComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Motion Blur"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Motion Blur"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(
                            AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // Create better icons for this effect
                        ->Attribute(
                            AZ::Edit::Attributes::ViewportIcon,
                            "Icons/Components/Viewport/Component_Placeholder.svg") // Create better icons for this effect
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL,
                            "https://o3de.org/docs/user-guide/components/reference/atom/MotionBlur/") // Create documentation page for this effect
                        ;

                    editContext->Class<JE::Render::Scene::MotionBlurComponentController>("MotionBlurComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::MotionBlurComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<JE::Render::Scene::MotionBlurComponentConfig>("MotionBlurComponentConfig", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::MotionBlurComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Motion Blur"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Motion Blur."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::MotionBlurComponentConfig::m_strength, QT_TRANSLATE_NOOP("AtomLyIntegration", "Strength"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Strength of the Effect"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::MotionBlurComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::MotionBlurComponentConfig::m_sampleQuality, "Sample Quality", "Quality of the effect")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateMotionBlurQualityList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::MotionBlurComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::MotionBlurComponentConfig
#include <JE/Render/Features/ParamMacros/StartOverrideEditorContext.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorMotionBlurComponent>()->RequestBus("MotionBlurRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorMotionBlurComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::MotionBlur::EditorMotionBlurComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorMotionBlurComponent::EditorMotionBlurComponent(const JE::Render::Scene::MotionBlurComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorMotionBlurComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
