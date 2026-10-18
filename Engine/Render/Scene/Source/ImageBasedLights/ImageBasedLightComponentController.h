/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <JE/Render/Features/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentConfig.h>

namespace JE::Editor::Render::Scene
{
    class EditorImageBasedLightComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        //! Controls behavior of an image based light that affects a scene
        class ImageBasedLightComponentController final
            : ImageBasedLightComponentRequestBus::Handler
            , AZ::Data::AssetBus::MultiHandler
            , AZ::TransformNotificationBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorImageBasedLightComponent;

            AZ_CLASS_ALLOCATOR(ImageBasedLightComponentController, AZ::SystemAllocator);
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ImageBasedLightComponentController, "AZ::Render::ImageBasedLightComponentController", "{73DBD008-4E77-471C-B7DE-F2217A256FE2}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            ImageBasedLightComponentController() = default;
            ImageBasedLightComponentController(const ImageBasedLightComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ImageBasedLightComponentConfig& config);
            const ImageBasedLightComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(ImageBasedLightComponentController);

            //! Data::AssetBus interface
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            //! ImageBasedLightComponentRequestBus overrides...
            void SetSpecularImageAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) override;
            void SetDiffuseImageAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) override;
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetSpecularImageAsset() const override;
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetDiffuseImageAsset() const override;
            void SetSpecularImageAssetId(const AZ::Data::AssetId imageAssetId) override;
            void SetDiffuseImageAssetId(const AZ::Data::AssetId imageAssetId) override;
            AZ::Data::AssetId GetSpecularImageAssetId() const override;
            AZ::Data::AssetId GetDiffuseImageAssetId() const override;
            void SetSpecularImageAssetPath(const AZStd::string path) override;
            void SetDiffuseImageAssetPath(const AZStd::string path) override;
            AZStd::string GetSpecularImageAssetPath() const override;
            AZStd::string GetDiffuseImageAssetPath() const override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;

            // TransformNotificationBus::Handler overrides...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            //! Queues a load of an image Asset
            void LoadImage(AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset);
            
            //! Releases all image assest.
            void ReleaseImages();
            
            //! Handles all Data::AssetBus calls in a unified way
            void UpdateWithAsset(AZ::Data::Asset<AZ::Data::AssetData> updatedAsset);
            
            //! Validates individual asset updates, returns true for valid ready assets.
            bool HandleAssetUpdate(AZ::Data::Asset<AZ::Data::AssetData> updatedAsset, AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& configAsset);

            AZ::EntityId m_entityId;
            ImageBasedLightComponentConfig m_configuration;
            JE::Render::Features::ImageBasedLightFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    } // namespace Render
} // namespace AZ
