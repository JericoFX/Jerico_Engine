/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScreenSpace/EditorDeferredFogComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        void EditorDeferredFogComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDeferredFogComponent, BaseClass>()
                    ->Version(2);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDeferredFogComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Deferred Fog"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Deferred Fog"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Environment")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/deferred-fog/") // [TODO][ATOM-13427] Create Wiki for Deferred Fog
                        ;

                    editContext->Class<JE::Render::Scene::DeferredFogComponentController>(
                        "DeferredFogComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::DeferredFogComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::DeferredFogComponentConfig>(
                        "DeferredFogComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::DeferredFogComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Deferred Fog"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Deferred Fog."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DeferredFogComponentConfig::m_enableFogLayerShaderOption,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Fog Layer"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Fog Layer"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                         ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Scene::DeferredFogComponentConfig::m_useNoiseTextureShaderOption,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Turbulence Properties"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Turbulence Properties"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &JE::Render::Scene::DeferredFogComponentConfig::m_fogColor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The fog color."))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Scene::DeferredFogComponentConfig::m_fogMode,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Mode"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Which formula to use for calculating the fog."))
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<JE::Render::Features::FogMode>())

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Distance"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogStartDistance,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Start Distance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The distance from the viewer when the fog starts"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogEndDistance,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog End Distance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "At what distance from the viewer does the fog take over and mask the background scene out."))
                            ->Attribute(AZ::Edit::Attributes::Min, &JE::Render::Scene::DeferredFogComponentConfig::m_fogStartDistance)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::DeferredFogComponentConfig::SupportsFogEnd)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Density Control"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogDensity,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Density"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Density of the fog that can range from 0.0 to 1.0"))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::DeferredFogComponentConfig::SupportsFogDensity)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogDensityClamp,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Density Clamp"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "The maximum density that the fog can reach. This enables the sky, horizon, and other bright, distant objects to be visible through dense fog."))
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        // Fog layer properties
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Layer"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::DeferredFogComponentConfig::GetEnableFogLayerShaderOption)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogMinHeight,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Bottom Height"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The height at which the fog layer starts"))
                            ->Attribute(AZ::Edit::Attributes::Min, -5000.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)


                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_fogMaxHeight,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Fog Max Height"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The height of the fog layer top"))
                            ->Attribute(AZ::Edit::Attributes::Min, -5000.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        // Fog turbulence properties
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Turbulence"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Scene::DeferredFogComponentConfig::GetUseNoiseTextureShaderOption)
                        ->DataElement(AZ::Edit::UIHandlers::LineEdit, &JE::Render::Scene::DeferredFogComponentConfig::m_noiseTexture,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Noise Texture"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The noise texture used for creating the fog turbulence"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        // First noise octave
                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &JE::Render::Scene::DeferredFogComponentConfig::m_noiseScaleUV,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Noise Texture First Octave Scale"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The scale of the first noise octave - higher indicates higher frequency / repetition"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &JE::Render::Scene::DeferredFogComponentConfig::m_noiseVelocityUV,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Noise Texture First Octave Velocity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The velocity of the first noise octave UV coordinates"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        // Second noise octave
                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &JE::Render::Scene::DeferredFogComponentConfig::m_noiseScaleUV2,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Noise Texture Second Octave Scale"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The scale of the second noise octave - higher indicates higher frequency / repetition"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &JE::Render::Scene::DeferredFogComponentConfig::m_noiseVelocityUV2,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Noise Texture Second Octave Velocity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The velocity of the second noise octave UV coordinates"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::DeferredFogComponentConfig::m_octavesBlendFactor,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Octaves Blend Factor"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The blend factor between the noise octaves"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDeferredFogComponent>()->RequestBus("DeferredFogRequestsBus");

                behaviorContext->ConstantProperty("EditorDeferredFogComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::DeferredFog::EditorDeferredFogComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDeferredFogComponent::EditorDeferredFogComponent(const JE::Render::Scene::DeferredFogComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorDeferredFogComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
