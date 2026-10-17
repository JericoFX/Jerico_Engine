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
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <JE/Render/Scene/SkyBox/HDRiSkyboxComponentConfig.h>
#include <JE/Render/Scene/SkyBox/HDRiSkyboxBus.h>

namespace JE::Editor::Render::Scene
{
    class EditorHDRiSkyboxComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class HDRiSkyboxComponentController final
            : public AZ::TransformNotificationBus::Handler
            , public HDRiSkyboxRequestBus::Handler
            , AZ::Data::AssetBus::MultiHandler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorHDRiSkyboxComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::HDRiSkyboxComponentController, "AZ::Render::HDRiSkyboxComponentController", "{D01C123D-4EA1-4A9B-A7D9-47EF26A55CD0}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            HDRiSkyboxComponentController() = default;
            HDRiSkyboxComponentController(const HDRiSkyboxComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const HDRiSkyboxComponentConfig& config);
            const HDRiSkyboxComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(HDRiSkyboxComponentController);

            //! Data::AssetBus interface
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // HDRiSkyboxRequestBus::Handler overrides ...
            AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetCubemapAsset() const override;
            void SetCubemapAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& cubemapAsset) override;
            void SetCubemapAssetPath(const AZStd::string& path) override;
            void SetCubemapAssetId(AZ::Data::AssetId) override;
            AZ::Data::AssetId GetCubemapAssetId() const override;
            AZStd::string GetCubemapAssetPath() const override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;

            //! Queues a load of an image Asset
            void LoadImage(AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset);

            //! Handles all Data::AssetBus calls in a unified way
            void UpdateWithAsset(AZ::Data::Asset<AZ::Data::AssetData> updatedAsset);

            //! Validates individual asset updates, returns true for valid ready assets.
            bool IsAssetValid(AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& asset);

            //! Get the inverse matrix from entity transfom, without scale and translate
            AZ::Matrix4x4 GetInverseTransform(const AZ::Transform& world);

            AZ::TransformInterface* m_transformInterface = nullptr;
            AZ::Render::SkyBoxFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
            HDRiSkyboxComponentConfig m_configuration;
            AZ::EntityId m_entityId;
            bool m_isActive = false;
        };
    }
}
