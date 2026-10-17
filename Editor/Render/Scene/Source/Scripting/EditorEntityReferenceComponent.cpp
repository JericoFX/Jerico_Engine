/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Scripting/EditorEntityReferenceComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorEntityReferenceComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEntityReferenceComponent, BaseClass>()
                    ->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorEntityReferenceComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Entity Reference"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Contains a reference list to other entities"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Miscellaneous")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/entity-reference/")
                        ;

                    editContext->Class<JE::Render::Scene::EntityReferenceComponentController>("EntityReferenceComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::EntityReferenceComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::EntityReferenceComponentConfig>("EntityReferenceComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::EntityReferenceComponentConfig::m_entityIdReferences, QT_TRANSLATE_NOOP("AtomLyIntegration", "EntityIdReferences"), QT_TRANSLATE_NOOP("AtomLyIntegration", "List of references to other entities."))
                        ;
                    ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorEntityReferenceComponent>()->RequestBus("EntityReferenceRequestBus");

                behaviorContext->ConstantProperty("EditorEntityReferenceComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorEntityReferenceComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorEntityReferenceComponent::EditorEntityReferenceComponent(const JE::Render::Scene::EntityReferenceComponentConfig& config)
            : BaseClass(config)
        {

        }

        AZ::u32 EditorEntityReferenceComponent::OnConfigurationChanged()
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
