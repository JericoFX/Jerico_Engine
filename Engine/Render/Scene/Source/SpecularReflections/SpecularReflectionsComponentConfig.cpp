/*7
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SpecularReflections/SpecularReflectionsComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Render
{
    namespace Scene
    {
        void SpecularReflectionsComponentSSRConfig::Reflect(AZ::ReflectContext* context)
        {
            JE::Render::Features::SSROptions::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponentSSRConfig>()
                    ->Version(1)
                    ->Field("Options", &SpecularReflectionsComponentSSRConfig::m_options)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<JE::Render::Features::SSROptions>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Screen Space Reflections (SSR)"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Screen Space Reflections (SSR) Configuration"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Features::SSROptions::m_enable, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable SSR"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Screen Space Reflections (SSR)"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_maxRayDistance, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Ray Distance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The maximum length of the rays to consider for hit detection"))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_maxDepthThreshold, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Depth Threshold"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The maximum delta between the ray depth and depth buffer value which will be considered a hit.  Also known as thickness."))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_maxRoughness, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Roughness"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Surfaces at or below this roughness value will have SSR applied"))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_roughnessBias, QT_TRANSLATE_NOOP("AtomLyIntegration", "Roughness Bias"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Bias applied to the surface roughness"))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Features::SSROptions::m_halfResolution, QT_TRANSLATE_NOOP("AtomLyIntegration", "Half Resolution"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Use half resolution in the reflected image, improves performance but may increase artifacts during camera motion"))
                            ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &JE::Render::Features::SSROptions::m_reflectionMethod, QT_TRANSLATE_NOOP("AtomLyIntegration", "Reflection Method"),
                                      QT_TRANSLATE_NOOP("AtomLyIntegration",
                                      "Screen-space: Use screen-space reflections only\n\n"
                                      "Hybrid SSR-RT: Use ray tracing for hit detection and screen-space data for surface evaluation\n\n"
                                      "Hybrid SSR-RT + Ray Tracing fallback: Use screen-space reflection data when available and ray tracing when not\n\n"
                                      "Ray Tracing: Use hardware ray tracing only"))
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                                ->EnumAttribute(JE::Render::Features::SSROptions::ReflectionMethod::ScreenSpace, QT_TRANSLATE_NOOP("AtomLyIntegration", "Screen Space"))
                                ->EnumAttribute(JE::Render::Features::SSROptions::ReflectionMethod::Hybrid, QT_TRANSLATE_NOOP("AtomLyIntegration", "Hybrid SSR-RT"))
                                ->EnumAttribute(JE::Render::Features::SSROptions::ReflectionMethod::HybridWithFallback, QT_TRANSLATE_NOOP("AtomLyIntegration", "Hybrid SSR-RT + Ray Tracing fallback"))
                                ->EnumAttribute(JE::Render::Features::SSROptions::ReflectionMethod::RayTracing, QT_TRANSLATE_NOOP("AtomLyIntegration", "Ray Tracing"))
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Features::SSROptions::m_rayTraceFallbackSpecular, QT_TRANSLATE_NOOP("AtomLyIntegration", "Apply Fallback Specular Lighting"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Apply specular lighting in the fallback image, improves fallback image accuracy but may reduce performance and increase artifacts"))
                                ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsRayTracingFallbackEnabled)
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Temporal Filtering"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Features::SSROptions::m_temporalFiltering, QT_TRANSLATE_NOOP("AtomLyIntegration", "Temporal Filtering"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Reduce artifacts with temporal filtering over multiple frames"))
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_temporalFilteringStrength, QT_TRANSLATE_NOOP("AtomLyIntegration", "Temporal Filtering Strength"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Higher strength reduces motion artifacts but increases temporal lag"))
                                ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsTemporalFilteringEnabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 1)
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Luminance"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &JE::Render::Features::SSROptions::m_luminanceClamp, QT_TRANSLATE_NOOP("AtomLyIntegration", "Luminance Clamp"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Reduce specular artifacts by clamping the luminance to a maximum value"))
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsEnabled)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Features::SSROptions::m_maxLuminance, QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum Luminance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Maximum luminance value"))
                                ->Attribute(AZ::Edit::Attributes::Visibility, &JE::Render::Features::SSROptions::IsLuminanceClampEnabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 3)
                    ;
                }
            }
        }

        void SpecularReflectionsComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            SpecularReflectionsComponentSSRConfig::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponentConfig, AZ::ComponentConfig>()
                    ->Version(1)
                    ->Field("SSR", &SpecularReflectionsComponentConfig::m_ssr)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
