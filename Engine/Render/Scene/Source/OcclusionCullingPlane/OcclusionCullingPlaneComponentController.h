/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <JE/Render/Features/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>

namespace JE::Editor::Render::Scene
{
    class EditorOcclusionCullingPlaneComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class OcclusionCullingPlaneComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::OcclusionCullingPlaneComponentConfig, "AZ::Render::OcclusionCullingPlaneComponentConfig", "{D0E107CA-5AFB-4675-BC97-94BCA5F248DB}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(OcclusionCullingPlaneComponentConfig, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            bool m_showVisualization = true;
            bool m_transparentVisualization = false;

            OcclusionCullingPlaneComponentConfig() = default;
        };

        class OcclusionCullingPlaneComponentController final
            : public AZ::Data::AssetBus::MultiHandler
            , private AZ::TransformNotificationBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent;

            AZ_CLASS_ALLOCATOR(OcclusionCullingPlaneComponentController, AZ::SystemAllocator);
            AZ_RTTI_WITH_NAME(JE::Render::Scene::OcclusionCullingPlaneComponentController, "AZ::Render::OcclusionCullingPlaneComponentController", "{8EDA3C7D-5171-4843-9969-4D84DB13F221}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            OcclusionCullingPlaneComponentController() = default;
            OcclusionCullingPlaneComponentController(const OcclusionCullingPlaneComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const OcclusionCullingPlaneComponentConfig& config);
            const OcclusionCullingPlaneComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(OcclusionCullingPlaneComponentController);

            // TransformNotificationBus overrides
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // handle for this occlusion plane in the feature processor
            JE::Render::Features::OcclusionCullingPlaneHandle m_handle;

            JE::Render::Features::OcclusionCullingPlaneFeatureProcessorInterface* m_featureProcessor = nullptr;
            AZ::TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            OcclusionCullingPlaneComponentConfig m_configuration;
        };
    } // namespace Render
} // namespace AZ
