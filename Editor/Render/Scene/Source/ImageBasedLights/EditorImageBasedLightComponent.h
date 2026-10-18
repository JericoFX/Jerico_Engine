/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ImageBasedLights/ImageBasedLightComponent.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentBus.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        //! In-editor image based lighting component
        class EditorImageBasedLightComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::ImageBasedLightComponentController, JE::Render::Scene::ImageBasedLightComponent, JE::Render::Scene::ImageBasedLightComponentConfig>
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::ImageBasedLightComponentController, JE::Render::Scene::ImageBasedLightComponent, JE::Render::Scene::ImageBasedLightComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorImageBasedLightComponent, "AZ::Render::EditorImageBasedLightComponent", JE::Render::Scene::EditorImageBasedLightComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorImageBasedLightComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorImageBasedLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorImageBasedLightComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorImageBasedLightComponent() = default;
            EditorImageBasedLightComponent(const JE::Render::Scene::ImageBasedLightComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;

            AZ::u32 OnDiffuseImageAssetChanged();
            AZ::u32 OnSpecularImageAssetChanged();
            AZ::u32 OnExposureChanged();

        private:

            bool UpdateImageAsset(AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& asset1, const char* asset1Suffix, const char* asset1Name,
                                  AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& asset2, const char* asset2Suffix, const char* asset2Name);

            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> m_specularImageAsset;
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> m_diffuseImageAsset;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
