/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <EditorComponents/EditorDiffuseGlobalIlluminationComponent.h>

namespace JE::Editor::Render
{
    namespace Features
    {
        void EditorDiffuseGlobalIlluminationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDiffuseGlobalIlluminationComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDiffuseGlobalIlluminationComponent>(
                        QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Diffuse Global Illumination"),
                        QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Diffuse Global Illumination configuration"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://")
                        ;

                    editContext->Class<JE::Render::Features::DiffuseGlobalIlluminationComponentController>(
                        "ToneMapperComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Features::DiffuseGlobalIlluminationComponentController::m_configuration,
                            QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Features::DiffuseGlobalIlluminationComponentConfig>("DiffuseGlobalIlluminationComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Features::DiffuseGlobalIlluminationComponentConfig::m_qualityLevel,
                            QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Quality Level"),
                            QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Quality Level"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->EnumAttribute(JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::Low,
                                QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Low"))
                            ->EnumAttribute(JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::Medium,
                                QT_TRANSLATE_NOOP("DiffuseProbeGrid", "Medium"))
                            ->EnumAttribute(JE::Render::Features::DiffuseGlobalIlluminationQualityLevel::High,
                                QT_TRANSLATE_NOOP("DiffuseProbeGrid", "High"))
                    ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorDiffuseGlobalIlluminationComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Features::EditorDiffuseGlobalIlluminationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDiffuseGlobalIlluminationComponent::EditorDiffuseGlobalIlluminationComponent(const JE::Render::Features::DiffuseGlobalIlluminationComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorDiffuseGlobalIlluminationComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
