/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyBox/EditorPhysicalSkyComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorPhysicalSkyComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPhysicalSkyComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorPhysicalSkyComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Physical Sky"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Physical Sky render the background of your scene with physical simulation"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Environment")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/physical-sky/")
                        ;

                    editContext->Class<JE::Render::Scene::PhysicalSkyComponentController>(
                        "PhysicalSkyComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::PhysicalSkyComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::PhysicalSkyComponentConfig>(
                        "PhysicalSkyComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::PhysicalSkyComponentConfig::m_intensityMode, QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity Mode"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Specifying the light unit"))
                                ->EnumAttribute(JE::Render::Features::PhotometricUnit::Ev100Luminance, QT_TRANSLATE_NOOP("AtomLyIntegration", "Ev100"))
                                ->EnumAttribute(JE::Render::Features::PhotometricUnit::Nit, QT_TRANSLATE_NOOP("AtomLyIntegration", "Nit"))
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::PhysicalSkyComponentConfig::m_skyIntensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Sky Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Brightness of the sky"))
                                ->Attribute(AZ::Edit::Attributes::Min, &JE::Render::Scene::PhysicalSkyComponentConfig::GetSkyIntensityMin)
                                ->Attribute(AZ::Edit::Attributes::Max, &JE::Render::Scene::PhysicalSkyComponentConfig::GetSkyIntensityMax)
                                ->Attribute(AZ::Edit::Attributes::Suffix, &JE::Render::Scene::PhysicalSkyComponentConfig::GetIntensitySuffix)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::PhysicalSkyComponentConfig::m_sunIntensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Sun Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Brightness of the sun"))
                                ->Attribute(AZ::Edit::Attributes::Min, &JE::Render::Scene::PhysicalSkyComponentConfig::GetSunIntensityMin)
                                ->Attribute(AZ::Edit::Attributes::Max, &JE::Render::Scene::PhysicalSkyComponentConfig::GetSunIntensityMax)
                                ->Attribute(AZ::Edit::Attributes::Suffix, &JE::Render::Scene::PhysicalSkyComponentConfig::GetIntensitySuffix)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::PhysicalSkyComponentConfig::m_sunRadiusFactor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Sun Radius Factor"), QT_TRANSLATE_NOOP("AtomLyIntegration", "A factor for Physical sun radius in millions of km. 1 unit is 695,508 km"))
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Max, 2.f)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::PhysicalSkyComponentConfig::m_turbidity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Turbidity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "A measure of the aerosol content in the air. Default is 1."))
                                ->Attribute(AZ::Edit::Attributes::Min, 1)
                                ->Attribute(AZ::Edit::Attributes::Max, 10)
                                ->Attribute(AZ::Edit::Attributes::Step, 1)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::PhysicalSkyComponentConfig::m_skyBoxFogSettings, QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog settings for rendering on top of physical sky"))
                                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorPhysicalSkyComponent>()
                    ->RequestBus("PhysicalSkyRequestBus")
                    ->RequestBus("SkyBoxFogRequestBus");

                behaviorContext->ConstantProperty("EditorPhysicalSkyComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Features::EditorPhysicalSkyComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorPhysicalSkyComponent::EditorPhysicalSkyComponent(const JE::Render::Scene::PhysicalSkyComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorPhysicalSkyComponent::OnConfigurationChanged()
        {
            // If the intensity mode changes in the editor, convert the photometric value and update the intensity
            if (m_controller.m_configuration.m_intensityMode != m_controller.m_skyPhotometricValue.GetType())
            {
                m_controller.m_skyPhotometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_skyIntensity = m_controller.m_skyPhotometricValue.GetIntensity();


                m_controller.m_sunPhotometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_sunIntensity = m_controller.m_sunPhotometricValue.GetIntensity();
            }

            BaseClass::OnConfigurationChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
