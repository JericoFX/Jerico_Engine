/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorCommonSystemComponent.h>
#include <Material/UseTextureFunctorSourceData.h>
#include <Material/SubsurfaceTransmissionParameterFunctorSourceData.h>
#include <Material/Transform2DFunctorSourceData.h>
#include <Material/ConvertEmissiveUnitFunctorSourceData.h>

#include <JE/Editor/Render/Features/Utils/EditorLightingPreset.h>
#include <JE/Editor/Render/Features/Utils/EditorModelPreset.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/Translation/TranslationDef.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/Application/Application.h>

#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Editor/Render/RPI/Edit/Material/LuaMaterialFunctorSourceData.h>


namespace JE::Editor::Render
{
    namespace Features
    {
        //! Main system component for the Atom Common Feature Gem's editor/tools module.
        void EditorCommonSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorCommonSystemComponent, AZ::Component>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorCommonSystemComponent>(QT_TRANSLATE_NOOP("Atom::Feature", "Common"), QT_TRANSLATE_NOOP("Atom::Feature", "Configures editor- and tool-specific functionality for common render features."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }

                JE::Editor::Render::Features::UseTextureFunctorSourceData::Reflect(context);
                JE::Editor::Render::Features::Transform2DFunctorSourceData::Reflect(context);
                JE::Editor::Render::Features::ConvertEmissiveUnitFunctorSourceData::Reflect(context);
                JE::Editor::Render::Features::SubsurfaceTransmissionParameterFunctorSourceData::Reflect(context);

                JE::Editor::Render::Features::EditorLightingPreset::Reflect(context);
                JE::Editor::Render::Features::EditorModelPreset::Reflect(context);
            }
        }

        void EditorCommonSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorCommonService"));
        }

        void EditorCommonSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorCommonService"));
        }

        void EditorCommonSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }

        void EditorCommonSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorCommonSystemComponent::Init()
        {
        }

        void EditorCommonSystemComponent::Activate()
        {
            JE::Render::RPI::MaterialFunctorSourceDataRegistration* materialFunctorRegistration = JE::Render::RPI::MaterialFunctorSourceDataRegistration::Get();
            if (!materialFunctorRegistration)
            {
                // On some host platforms shader processing is not supported and this interface is not available.
                return;
            }

            materialFunctorRegistration->RegisterMaterialFunctor("UseTexture", azrtti_typeid<UseTextureFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("Transform2D",              azrtti_typeid<Transform2DFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("ConvertEmissiveUnit",      azrtti_typeid<ConvertEmissiveUnitFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("HandleSubsurfaceScatteringParameters", azrtti_typeid<SubsurfaceTransmissionParameterFunctorSourceData>());
            materialFunctorRegistration->RegisterMaterialFunctor("Lua", azrtti_typeid<JE::Render::RPI::LuaMaterialFunctorSourceData>());
        }

        void EditorCommonSystemComponent::Deactivate()
        {
        }
    } // namespace Features
} // namespace JE::Editor::Render
