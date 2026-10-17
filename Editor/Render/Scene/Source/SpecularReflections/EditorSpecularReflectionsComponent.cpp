/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <SpecularReflections/EditorSpecularReflectionsComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorSpecularReflectionsComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorSpecularReflectionsComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorSpecularReflectionsComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular Reflections"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular Reflections configuration"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://")
                    ;

                    editContext->Class<JE::Render::Scene::SpecularReflectionsComponentController>(
                        "SpecularReflectionsComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::SpecularReflectionsComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                    editContext->Class<JE::Render::Scene::SpecularReflectionsComponentSSRConfig>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Screen Space Reflections (SSR)"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Screen Space Reflections (SSR) Configuration"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::SpecularReflectionsComponentSSRConfig::m_options, QT_TRANSLATE_NOOP("AtomLyIntegration", "SSR Options"), "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                    editContext->Class<JE::Render::Scene::SpecularReflectionsComponentConfig>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular Reflections Component"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Configures specular reflection options for the level"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::SpecularReflectionsComponentConfig::m_ssr, QT_TRANSLATE_NOOP("AtomLyIntegration", "SSR configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorSpecularReflectionsComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorSpecularReflectionsComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorSpecularReflectionsComponent::EditorSpecularReflectionsComponent(const JE::Render::Scene::SpecularReflectionsComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorSpecularReflectionsComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
