/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/LookModification/EditorLookModificationComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorLookModificationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLookModificationComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorLookModificationComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Look Modification"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The look modification process."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/look-modification/") // [TODO ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<JE::Render::Scene::LookModificationComponentController>(
                        "LookModificationComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::LookModificationComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::LookModificationComponentConfig>("LookModificationComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::LookModificationComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable look modification"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable look modification."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::LookModificationComponentConfig::m_colorGradingLut, QT_TRANSLATE_NOOP("AtomLyIntegration", "Color Grading LUT"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Color grading LUT"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox,
                            &JE::Render::Scene::LookModificationComponentConfig::m_shaperPresetType, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type."))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::None, QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::LinearCustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Linear Custom Range"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_48Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 48 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_1000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 1000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_2000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 2000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2_4000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 4000 nits"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::Log2CustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 Custom Range"))
                            ->EnumAttribute(AZ::Render::ShaperPresetType::PqSmpteSt2084, QT_TRANSLATE_NOOP("AtomLyIntegration", "PQ (SMPTE ST 2084)"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::LookModificationComponentConfig::m_customMinExposure, QT_TRANSLATE_NOOP("AtomLyIntegration", "Minimum Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The minimum exposure this LUT supports. Values smaller than this will be clamped to 0."))
                            ->Attribute(AZ::Edit::Attributes::Min, -50.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::LookModificationComponentConfig::IsUsingCustomShaper)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::LookModificationComponentConfig::m_customMaxExposure, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The maximum exposure this LUT supports. Values larger than this will be clamped."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::LookModificationComponentConfig::IsUsingCustomShaper)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::LookModificationComponentConfig::m_colorGradingLutIntensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Blend intensity of this LUT."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::LookModificationComponentConfig::ArePropertiesReadOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::LookModificationComponentConfig::m_colorGradingLutOverride, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Override"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Blend intensity of this LUT."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::LookModificationComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::LookModificationComponentConfig
#include <JE/Render/Features/ParamMacros/StartOverrideEditorContext.inl>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorLookModificationComponent>()->RequestBus("LookModificationRequestBus");

                behaviorContext->ConstantProperty("EditorLookModificationComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorLookModificationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorLookModificationComponent::EditorLookModificationComponent(const JE::Render::Scene::LookModificationComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorLookModificationComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
