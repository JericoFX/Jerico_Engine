/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/AmbientOcclusion/EditorAoComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        using AoMethodComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<AZ::Render::Ao::AoMethodType>>;
        static AoMethodComboBoxVec PopulateAoMethodList()
        {
            return AoMethodComboBoxVec
            {
                { AZ::Render::Ao::AoMethodType::SSAO, "SSAO" },
                { AZ::Render::Ao::AoMethodType::GTAO, "GTAO" },
            };
        }

        using GtaoQualityLevelComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<AZ::Render::Ao::GtaoQualityLevel>>;
        static GtaoQualityLevelComboBoxVec PopulateGtaoQualityLevelList()
        {
            return GtaoQualityLevelComboBoxVec
            {
                { AZ::Render::Ao::GtaoQualityLevel::SuperLow, "Super Low" },
                { AZ::Render::Ao::GtaoQualityLevel::Low, "Low" },
                { AZ::Render::Ao::GtaoQualityLevel::Medium, "Medium" },
                { AZ::Render::Ao::GtaoQualityLevel::High, "High" },
                { AZ::Render::Ao::GtaoQualityLevel::SuperHigh, "Super High" },
            };
        }

        void EditorAoComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorAoComponent, BaseClass>()
                    ->Version(2);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorAoComponent>(
                        "Ambient Occlusion", "Controls Ambient Occlusion.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/PostFX")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/ssao/") // [GFX TODO][ATOM-2672][PostFX] need create page for PostProcessing.
                        ;

                    editContext->Class<JE::Render::Scene::AoComponentController>(
                        "AoComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Scene::AoComponentController::m_configuration, "Configuration", "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<JE::Render::Scene::AoComponentConfig>("AoComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::AoComponentConfig::m_enabled,
                            "Enable AO",
                            "Enable AO.")

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox,
                            &JE::Render::Scene::AoComponentConfig::m_aoMethod,
                            "AO Method",
                            "The method used for AO calculation.")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateAoMethodList)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_ssaoStrength,
                            "SSAO Strength",
                            "Multiplier for how much strong SSAO appears.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsSsaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_ssaoSamplingRadius,
                            "SSAO Sampling Radius",
                            "The sampling radius of the SSAO effect in screen UV space")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.25f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsSsaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox,
                            &JE::Render::Scene::AoComponentConfig::m_gtaoQuality,
                            "GTAO Quality",
                            "The quality level for the GTAO effect.")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateGtaoQualityLevelList)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_gtaoStrength,
                            "GTAO Strength",
                            "Multiplier for how much strong GTAO appears.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_gtaoPower,
                            "GTAO Power",
                            "Power factor for how much strong GTAO appears.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_gtaoWorldRadius,
                            "GTAO World Radius",
                            "Sampling radius in world units.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_gtaoThicknessBlend,
                            "GTAO Thickness Blend",
                            "Blend factor for thickness.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_gtaoMaxDepth,
                            "GTAO Max Depth",
                            "Max depth for GTAO effect.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &JE::Render::Scene::AoComponentConfig::IsGtaoInactive)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::AoComponentConfig::m_enableBlur,
                            "Enable Blur",
                            "Enables AO Blur")

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_blurConstFalloff,
                            "Blur Strength",
                            "Affects how strong the blur is. Recommended value is 0.67")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.95f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_blurDepthFalloffStrength,
                            "Blur Sharpness",
                            "Affects how sharp the SSAO blur appears around edges. Recommended value is 50")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 400.0f)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &JE::Render::Scene::AoComponentConfig::m_blurDepthFalloffThreshold,
                            "Blur Edge Threshold",
                            "Affects the threshold needed for the blur algorithm to detect an edge. Recommended to be left at 0.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox,
                            &JE::Render::Scene::AoComponentConfig::m_enableDownsample,
                            "Enable Downsample",
                            "Enables depth downsampling before SSAO. Slightly lower quality but 2x as fast as regular SSAO.")


                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS JE::Render::Scene::AoComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorAoComponent>()->RequestBus("AoRequestBus");

                behaviorContext->ConstantProperty("EditorAoComponentTypeId", BehaviorConstant(AZ::Uuid(JE::Editor::Render::Scene::Ao::EditorAoComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorAoComponent::EditorAoComponent(const JE::Render::Scene::AoComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorAoComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
