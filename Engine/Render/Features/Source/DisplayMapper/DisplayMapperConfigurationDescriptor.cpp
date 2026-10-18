/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Render/Features/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

namespace AZ::Render
{
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(DisplayMapperOperationType);
}

namespace JE::Render
{
    namespace Features
    {
        void AcesParameterOverrides::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Enum<AZ::Render::OutputDeviceTransformType>()
                    ->Version(0)
                    ->Value("48Nits", AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_48Nits)
                    ->Value("1000Nits", AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_1000Nits)
                    ->Value("2000Nits", AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_2000Nits)
                    ->Value("4000Nits", AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_4000Nits)
                    ->Value("NumOutputDeviceTransformTypes", AZ::Render::OutputDeviceTransformType::NumOutputDeviceTransformTypes)
                    ;

                serializeContext->Class<AcesParameterOverrides>()
                    ->Version(0)
                    ->Field("OverrideDefaults", &AcesParameterOverrides::m_overrideDefaults)
                    ->Field("AlterSurround", &AcesParameterOverrides::m_alterSurround)
                    ->Field("ApplyDesaturation", &AcesParameterOverrides::m_applyDesaturation)
                    ->Field("ApplyCATD60toD65", &AcesParameterOverrides::m_applyCATD60toD65)
                    ->Field("PresetODT", &AcesParameterOverrides::m_preset)
                    ->Field("CinemaLimitsBlack", &AcesParameterOverrides::m_cinemaLimitsBlack)
                    ->Field("CinemaLimitsWhite", &AcesParameterOverrides::m_cinemaLimitsWhite)
                    ->Field("MinPoint", &AcesParameterOverrides::m_minPoint)
                    ->Field("MidPoint", &AcesParameterOverrides::m_midPoint)
                    ->Field("MaxPoint", &AcesParameterOverrides::m_maxPoint)
                    ->Field("SurroundGamma", &AcesParameterOverrides::m_surroundGamma)
                    ->Field("Gamma", &AcesParameterOverrides::m_gamma);
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<AZ::Render::OutputDeviceTransformType>("OutputDeviceTransformType")
                    ->Enum<AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_48Nits>("OutputDeviceTransformType_48Nits")
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Enum<AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_1000Nits>("OutputDeviceTransformType_100Nits")
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Enum<AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_2000Nits>("OutputDeviceTransformType_2000Nits")
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Enum<AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_4000Nits>("OutputDeviceTransformType_4000Nits")
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Enum<AZ::Render::OutputDeviceTransformType::NumOutputDeviceTransformTypes>("OutputDeviceTransformType_NumOutputDeviceTransformTypes")
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ;

                behaviorContext->Class<AcesParameterOverrides>("AcesParameterOverrides")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Method("LoadPreset", &AcesParameterOverrides::LoadPreset)
                    ->Property("overrideDefaults", BehaviorValueProperty(&AcesParameterOverrides::m_overrideDefaults))
                    ->Property("preset", BehaviorValueProperty(&AcesParameterOverrides::m_preset))
                    ->Enum<aznumeric_cast<int>(AZ::Render::OutputDeviceTransformType::NumOutputDeviceTransformTypes)>(
                        "OutputDeviceTransformType_NumOutputDeviceTransformTypes")
                    ->Enum<aznumeric_cast<int>(AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_48Nits)>(
                        "OutputDeviceTransformType_48Nits")
                    ->Enum<aznumeric_cast<int>(AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_1000Nits)>(
                        "OutputDeviceTransformType_1000Nits")
                    ->Enum<aznumeric_cast<int>(AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_2000Nits)>(
                        "OutputDeviceTransformType_2000Nits")
                    ->Enum<aznumeric_cast<int>(AZ::Render::OutputDeviceTransformType::OutputDeviceTransformType_4000Nits)>(
                        "OutputDeviceTransformType_4000Nits")
                    ->Property("alterSurround", BehaviorValueProperty(&AcesParameterOverrides::m_alterSurround))
                    ->Property("applyDesaturation", BehaviorValueProperty(&AcesParameterOverrides::m_applyDesaturation))
                    ->Property("applyCATD60toD65", BehaviorValueProperty(&AcesParameterOverrides::m_applyCATD60toD65))
                ;
            }
        }

        void AcesParameterOverrides::LoadPreset()
        {
            DisplayMapperParameters displayMapperParameters;
            AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&displayMapperParameters, m_preset);

            m_alterSurround = (displayMapperParameters.m_OutputDisplayTransformFlags & AcesDisplayMapperFeatureProcessor::AlterSurround) != 0;
            m_applyDesaturation = (displayMapperParameters.m_OutputDisplayTransformFlags & AcesDisplayMapperFeatureProcessor::ApplyDesaturation) != 0;
            m_applyCATD60toD65 = (displayMapperParameters.m_OutputDisplayTransformFlags & AcesDisplayMapperFeatureProcessor::ApplyCATD60toD65) != 0;
            m_cinemaLimitsBlack = displayMapperParameters.m_cinemaLimits[0];
            m_cinemaLimitsWhite = displayMapperParameters.m_cinemaLimits[1];
            m_minPoint = displayMapperParameters.m_acesSplineParams.minPoint[0];
            m_midPoint = displayMapperParameters.m_acesSplineParams.midPoint[0];
            m_maxPoint = displayMapperParameters.m_acesSplineParams.maxPoint[0];
            m_surroundGamma = displayMapperParameters.m_surroundGamma;
            m_gamma = displayMapperParameters.m_gamma;
        }

        void DisplayMapperConfigurationDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                AZ::Render::DisplayMapperOperationTypeReflect(*serializeContext);
                
                serializeContext->Class<DisplayMapperConfigurationDescriptor>()
                    ->Version(3)
                    ->Field("Name", &DisplayMapperConfigurationDescriptor::m_name)
                    ->Field("OperationType", &DisplayMapperConfigurationDescriptor::m_operationType)
                    ->Field("LdrGradingLutEnabled", &DisplayMapperConfigurationDescriptor::m_ldrGradingLutEnabled)
                    ->Field("LdrColorGradingLut", &DisplayMapperConfigurationDescriptor::m_ldrColorGradingLut)
                    ->Field("AcesParameterOverrides", &DisplayMapperConfigurationDescriptor::m_acesParameterOverrides)
                ;

            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                AZ::Render::DisplayMapperOperationTypeReflect(*behaviorContext);
            }
        }

        void DisplayMapperPassData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperPassData, Base>()
                    ->Version(0)
                    ->Field("DisplayMapperConfig", &DisplayMapperPassData::m_config)
                    ->Field("MergeLdrGradingLut", &DisplayMapperPassData::m_mergeLdrGradingLut)
                    ->Field("OutputTransformShaderOverride", &DisplayMapperPassData::m_outputTransformOverride)
                    ;
            }
        }

    } // namespace Features
} // namespace JE::Render
