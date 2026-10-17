/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/Scene/CoreLights/CoreLightsConstants.h>
#include <JE/Render/Scene/Mesh/MeshComponentConstants.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneSystemComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Render
{
    namespace Scene
    {
        void SceneSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            CoreLightConstantsReflect(context);

            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<SceneSystemComponent, AZ::Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<SceneSystemComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Common"), QT_TRANSLATE_NOOP("AtomLyIntegration", "[Description of functionality provided by this System Component]"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void SceneSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LyIntegrationCommonFeaturesService"));
        }

        void SceneSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LyIntegrationCommonFeaturesService"));
        }

        void SceneSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void SceneSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void SceneSystemComponent::Init()
        {
        }

        void SceneSystemComponent::Activate()
        {
            auto modelAssetHandler = azrtti_cast<JE::Render::RPI::ModelAssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<JE::Render::RPI::ModelAsset>()));
            modelAssetHandler->m_componentTypeId = EditorMeshComponentTypeId;
        }

        void SceneSystemComponent::Deactivate()
        {
        }
    } // namespace Render
} // namespace AZ
