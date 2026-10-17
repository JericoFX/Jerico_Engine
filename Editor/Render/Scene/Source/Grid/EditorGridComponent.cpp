/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Grid/EditorGridComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorGridComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorGridComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorGridComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Grid"),
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Adds grid to the scene"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Other")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/grid/")
                        ;

                    editContext->Class<JE::Render::Scene::GridComponentController>(
                        "GridComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::GridComponentController::m_configuration,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::GridComponentConfig>(
                        "GridComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::GridComponentConfig::m_gridSize,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Grid Size"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Grid width and depth"))
                            ->Attribute(AZ::Edit::Attributes::Min, JE::Render::Scene::GridComponentController::MinGridSize)
                            ->Attribute(AZ::Edit::Attributes::Max, JE::Render::Scene::GridComponentController::MaxGridSize)
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                " m")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::GridComponentConfig::m_primarySpacing,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Primary Grid Spacing"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Amount of space between grid lines"))
                            ->Attribute(AZ::Edit::Attributes::Min, JE::Render::Scene::GridComponentController::MinSpacing)
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                " m")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::GridComponentConfig::m_secondarySpacing,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Secondary Grid Spacing"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Amount of space between sub-grid lines"))
                            ->Attribute(AZ::Edit::Attributes::Min, JE::Render::Scene::GridComponentController::MinSpacing)
                            ->Attribute(AZ::Edit::Attributes::Suffix,
                                " m")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::GridComponentConfig::m_axisColor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Axis Color"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color of the grid axis"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::GridComponentConfig::m_primaryColor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Primary Color"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color of the primary grid lines"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::GridComponentConfig::m_secondaryColor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Secondary Color"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Color of the secondary grid lines"))
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorGridComponent>()->RequestBus("GridComponentRequestBus");

                behaviorContext->ConstantProperty("EditorGridComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorGridComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorGridComponent::EditorGridComponent(const JE::Render::Scene::GridComponentConfig& config)
            : BaseClass(config)
        {
        }

    } // namespace Render
} // namespace AZ
