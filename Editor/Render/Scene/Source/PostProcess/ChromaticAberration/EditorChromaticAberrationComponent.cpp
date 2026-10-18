/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ChromaticAberration/EditorChromaticAberrationComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorChromaticAberrationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorChromaticAberrationComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorChromaticAberrationComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Chromatic Aberration"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Chromatic Aberration"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(
                            AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(
                            AZ::Edit::Attributes::ViewportIcon,
                            "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL,
                            "https://docs.o3de.org/docs/user-guide/components/reference/atom/chromatic-aberration/")
                        ;

                    editContext->Class<JE::Render::Scene::ChromaticAberrationComponentController>("ChromaticAberrationComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::ChromaticAberrationComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<JE::Render::Scene::ChromaticAberrationComponentConfig>("ChromaticAberrationComponentConfig", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::ChromaticAberrationComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Chromatic Aberration"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Chromatic Aberration."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ChromaticAberrationComponentConfig::m_strength, QT_TRANSLATE_NOOP("AtomLyIntegration", "Strength"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Strength of effect"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ChromaticAberrationComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::ChromaticAberrationComponentConfig::m_blend, QT_TRANSLATE_NOOP("AtomLyIntegration", "Blend"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Factor for additive blending with original image"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::ChromaticAberrationComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::ChromaticAberrationComponentConfig
#include <JE/Render/Features/ParamMacros/StartOverrideEditorContext.inl>
#include <JE/Render/Features/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorChromaticAberrationComponent>()->RequestBus("ChromaticAberrationRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorChromaticAberrationComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::ChromaticAberration::EditorChromaticAberrationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorChromaticAberrationComponent::EditorChromaticAberrationComponent(const JE::Render::Scene::ChromaticAberrationComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorChromaticAberrationComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
