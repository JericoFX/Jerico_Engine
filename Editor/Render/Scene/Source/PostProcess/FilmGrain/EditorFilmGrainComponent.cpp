/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/FilmGrain/EditorFilmGrainComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorFilmGrainComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorFilmGrainComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorFilmGrainComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Film Grain"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Film Grain"))
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
                            "https://o3de.org/docs/user-guide/components/reference/atom/FilmGrain/") // Create documentation page for this effect
                        ;

                    editContext->Class<JE::Render::Scene::FilmGrainComponentController>("FilmGrainComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::FilmGrainComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<JE::Render::Scene::FilmGrainComponentConfig>("FilmGrainComponentConfig", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::FilmGrainComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Film Grain"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Film Grain."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::FilmGrainComponentConfig::m_intensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity of effect"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::FilmGrainComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::FilmGrainComponentConfig::m_luminanceDampening, QT_TRANSLATE_NOOP("AtomLyIntegration", "Luminance Dampening"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Factor for dampening effect in areas of both high and low luminance"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::FilmGrainComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::FilmGrainComponentConfig::m_tilingScale, QT_TRANSLATE_NOOP("AtomLyIntegration", "Tiling Scale"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Factor for tiling the pregenerated noise"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::FilmGrainComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::FilmGrainComponentConfig
#include <JE/Render/Features/ParamMacros/StartOverrideEditorContext.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorFilmGrainComponent>()->RequestBus("FilmGrainRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorFilmGrainComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::FilmGrain::EditorFilmGrainComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorFilmGrainComponent::EditorFilmGrainComponent(const JE::Render::Scene::FilmGrainComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorFilmGrainComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
