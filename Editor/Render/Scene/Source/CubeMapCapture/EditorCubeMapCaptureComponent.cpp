/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/EditorCubeMapCaptureComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorCubeMapCaptureComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorCubeMapCaptureComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorCubeMapCaptureComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "CubeMap Capture"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The CubeMap Capture component captures a specular or diffuse cubemap at a specific position in the level"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->UIElement(AZ::Edit::UIHandlers::Button, QT_TRANSLATE_NOOP("AtomLyIntegration", "Capture CubeMap"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Capture CubeMap"))
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Capture CubeMap"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCubeMapCaptureComponent::CaptureCubeMap)
                        ;

                    editContext->Class<JE::Render::Scene::CubeMapCaptureComponentController>(
                        "CubeMapCaptureComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::CubeMapCaptureComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::CubeMapCaptureComponentConfig>(
                        "CubeMapCaptureComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::CubeMapCaptureComponentConfig::m_exposure, QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure to use when capturing the cubemap"))
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::CubeMapCaptureComponentConfig::m_captureType, QT_TRANSLATE_NOOP("AtomLyIntegration", "Capture Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The type of cubemap to capture"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapCaptureType::Specular, QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular IBL"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapCaptureType::Diffuse, QT_TRANSLATE_NOOP("AtomLyIntegration", "Diffuse IBL"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &JE::Render::Scene::CubeMapCaptureComponentConfig::OnCaptureTypeChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::CubeMapCaptureComponentConfig::m_specularQualityLevel, QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular IBL CubeMap Quality"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Resolution of the Specular IBL cubemap"))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::CubeMapCaptureComponentConfig::GetSpecularQualityVisibilitySetting)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &JE::Render::Scene::CubeMapCaptureComponentConfig::OnSpecularQualityChanged)
                            ->EnumAttribute(JE::Render::Scene::CubeMapSpecularQualityLevel::VeryLow, QT_TRANSLATE_NOOP("AtomLyIntegration", "Very Low"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapSpecularQualityLevel::Low, QT_TRANSLATE_NOOP("AtomLyIntegration", "Low"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapSpecularQualityLevel::Medium, QT_TRANSLATE_NOOP("AtomLyIntegration", "Medium"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapSpecularQualityLevel::High, QT_TRANSLATE_NOOP("AtomLyIntegration", "High"))
                            ->EnumAttribute(JE::Render::Scene::CubeMapSpecularQualityLevel::VeryHigh, QT_TRANSLATE_NOOP("AtomLyIntegration", "Very High"))
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &JE::Render::Scene::CubeMapCaptureComponentConfig::m_relativePath, QT_TRANSLATE_NOOP("AtomLyIntegration", "CubeMap Path"), QT_TRANSLATE_NOOP("AtomLyIntegration", "CubeMap Path"))
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<JE::Render::Scene::EditorCubeMapCaptureBus>("EditorCubeMapCaptureBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("CaptureCubeMap", &JE::Render::Scene::EditorCubeMapCaptureInterface::CaptureCubeMap)
                    ;
                
                behaviorContext->ConstantProperty("EditorCubeMapCaptureComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Render::Scene::EditorCubeMapCaptureComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorCubeMapCaptureComponent::EditorCubeMapCaptureComponent(const JE::Render::Scene::CubeMapCaptureComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorCubeMapCaptureComponent::Activate()
        {
            BaseClass::Activate();
            JE::Render::Scene::EditorCubeMapCaptureBus::Handler::BusConnect(GetEntityId());
        }

        void EditorCubeMapCaptureComponent::Deactivate()
        {
            JE::Render::Scene::EditorCubeMapCaptureBus::Handler::BusDisconnect(GetEntityId());
            BaseClass::Deactivate();
        }

        AZ::u32 EditorCubeMapCaptureComponent::CaptureCubeMap()
        {
            JE::Render::Scene::CubeMapCaptureComponentConfig& configuration = m_controller.m_configuration;

            return RenderCubeMap(
                [&](AZ::Render::RenderCubeMapCallback callback, AZStd::string& relativePath) { m_controller.RenderCubeMap(callback, relativePath); },
                "Capturing Cubemap...",
                GetEntity(),
                "CubeMapCaptures",
                configuration.m_relativePath,
                configuration.m_captureType,
                configuration.m_specularQualityLevel);
        }
    } // namespace Render
} // namespace AZ
