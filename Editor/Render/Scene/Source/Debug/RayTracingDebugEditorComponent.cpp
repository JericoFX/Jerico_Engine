/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RayTracingDebugEditorComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render::Scene
{
    void RayTracingDebugEditorComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serializeContext{ azrtti_cast<AZ::SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugEditorComponent, BaseClass>()
                ->Version(0)
            ;
            // clang-format on

            if (auto* editContext{ serializeContext->GetEditContext() })
            {
                // clang-format off
                editContext->Class<RayTracingDebugEditorComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Ray Tracing"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls for debugging ray tracing."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Debugging")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;

                editContext->Class<JE::Render::Scene::RayTracingDebugComponentController>("RayTracingDebugComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::RayTracingDebugComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::RayTracingDebugComponentController::IsRayTracingSupported)
                    ->UIElement(AZ::Edit::UIHandlers::LineEdit, QT_TRANSLATE_NOOP("AtomLyIntegration", "Ray Tracing is not supported on this device."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::RayTracingDebugComponentController::IsRayTracingNotSupported)
                ;

                editContext->Class<JE::Render::Scene::RayTracingDebugComponentConfig>("RayTracingDebugComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::RayTracingDebugComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Ray Tracing Debugging"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Ray Tracing Debugging."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::RayTracingDebugComponentConfig::m_debugViewMode, QT_TRANSLATE_NOOP("AtomLyIntegration", "View mode"), QT_TRANSLATE_NOOP("AtomLyIntegration", "What property to output to the view"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::InstanceIndex, QT_TRANSLATE_NOOP("AtomLyIntegration", "Instance Index"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::InstanceID, QT_TRANSLATE_NOOP("AtomLyIntegration", "Instance ID"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::ClusterID, QT_TRANSLATE_NOOP("AtomLyIntegration", "Cluster ID"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::PrimitiveIndex, QT_TRANSLATE_NOOP("AtomLyIntegration", "Primitive Index"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::Barycentrics, QT_TRANSLATE_NOOP("AtomLyIntegration", "Barycentric Coordinates"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::Normals, QT_TRANSLATE_NOOP("AtomLyIntegration", "Normals"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::UVs, QT_TRANSLATE_NOOP("AtomLyIntegration", "UV Coordinates"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::BaseColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Base Color"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::EmissiveColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Emissive Color"))
                        ->EnumAttribute(JE::Render::Features::RayTracingDebugViewMode::IrradianceColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Irradiance Color"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::RayTracingDebugComponentConfig::GetEnabled)
                ;
                // clang-format on
            }
        }
    }

    AZ::u32 RayTracingDebugEditorComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace AZ::Render
