/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/RadiusWeightModifier/EditorRadiusWeightModifierComponent.h>
#include <JE/Render/Scene/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConstants.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorRadiusWeightModifierComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorRadiusWeightModifierComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorRadiusWeightModifierComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "PostFX Radius Weight Modifier"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modifies PostFX override factor based on proximity of an influencer against this entity's bounding sphere"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/postfx-radius-weight-modifier/")
                        ;

                    editContext->Class<JE::Render::Scene::RadiusWeightModifierComponentController>("RadiusWeightModifierComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::RadiusWeightModifierComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::RadiusWeightModifierComponentConfig>("RadiusWeightModifierComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::Slider,
                            &JE::Render::Scene::RadiusWeightModifierComponentConfig::m_radius,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Radius"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Radius of PostFx Volume."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f);
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorRadiusWeightModifierComponent>()->RequestBus("PostFxWeightRequestBus");

                behaviorContext->ConstantProperty("EditorRadiusWeightModifierComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorRadiusWeightModifierComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorRadiusWeightModifierComponent::EditorRadiusWeightModifierComponent(const JE::Render::Scene::RadiusWeightModifierComponentConfig& config)
            : BaseClass(config)
        {

        }

        AZ::u32 EditorRadiusWeightModifierComponent::OnConfigurationChanged()
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}

