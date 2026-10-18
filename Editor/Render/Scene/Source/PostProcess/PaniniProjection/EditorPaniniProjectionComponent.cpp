/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PaniniProjection/EditorPaniniProjectionComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorPaniniProjectionComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPaniniProjectionComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorPaniniProjectionComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Panini Projection"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Panini Projection"))
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
                            "https://o3de.org/docs/user-guide/components/reference/atom/PaniniProjection/") // Create documentation page for this effect
                        ;

                    editContext->Class<JE::Render::Scene::PaniniProjectionComponentController>("PaniniProjectionComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::PaniniProjectionComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<JE::Render::Scene::PaniniProjectionComponentConfig>("PaniniProjectionComponentConfig", "")
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::PaniniProjectionComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Panini Projection"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Panini Projection."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::PaniniProjectionComponentConfig::m_depth, QT_TRANSLATE_NOOP("AtomLyIntegration", "Depth"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Depth of focal point"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::PaniniProjectionComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::PaniniProjectionComponentConfig
#include <JE/Render/Features/ParamMacros/StartOverrideEditorContext.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorPaniniProjectionComponent>()->RequestBus("PaniniProjectionRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorPaniniProjectionComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::PaniniProjection::EditorPaniniProjectionComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorPaniniProjectionComponent::EditorPaniniProjectionComponent(const JE::Render::Scene::PaniniProjectionComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorPaniniProjectionComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
