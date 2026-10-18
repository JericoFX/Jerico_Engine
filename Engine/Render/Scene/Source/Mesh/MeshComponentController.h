/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/Features/LightingChannel/LightingChannelConfiguration.h>
#include <JE/Render/Debug/AtomImGuiToolsBus.h>
#include <JE/Render/Materials/MaterialAssignment.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Scene/Mesh/MeshComponentBus.h>
#include <JE/Render/Scene/Mesh/MeshHandleStateBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Instance/InstanceDatabase.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>

namespace JE::Editor::Render::Scene
{
    class EditorMeshComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        //! A configuration structure for the MeshComponentController
        class MeshComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::MeshComponentConfig, "AZ::Render::MeshComponentConfig", "{63737345-51B1-472B-9355-98F99993909B}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(MeshComponentConfig, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            // Editor helper functions
            bool IsAssetSet();
            bool LodTypeIsScreenCoverage();
            bool LodTypeIsSpecificLOD();
            bool ShowLodConfig();
            AZStd::vector<AZStd::pair<JE::Render::RPI::Cullable::LodOverride, AZStd::string>> GetLodOverrideValues();

            AZ::Data::Asset<JE::Render::RPI::ModelAsset> m_modelAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
            AZ::RHI::DrawItemSortKey m_sortKey = 0;
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_isAlwaysDynamic = false;
            bool m_useForwardPassIblSpecular = false;
            bool m_isRayTracingEnabled = true;
            // the ModelAsset should support ray intersection if any one of the two flags are enabled.
            bool m_editorRayIntersection = false;       // Set to true if the config is for EditorMeshComponent so the ModelAsset always support ray intersection 
            bool m_enableRayIntersection = false;       // Set to true if a mesh need ray intersection support at runtime. 
            JE::Render::RPI::Cullable::LodType m_lodType = JE::Render::RPI::Cullable::LodType::Default;
            JE::Render::RPI::Cullable::LodOverride m_lodOverride = aznumeric_cast<JE::Render::RPI::Cullable::LodOverride>(0);
            float m_minimumScreenCoverage = 1.0f / 1080.0f;
            float m_qualityDecayRate = 0.5f;

            JE::Render::Features::LightingChannelConfiguration m_lightingChannelConfig;
        };

        class MeshComponentController final
            : private MeshComponentRequestBus::Handler
            , private MeshHandleStateRequestBus::Handler
            , private JE::Render::Debug::AtomImGuiMeshCallbackBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
            , public AzFramework::VisibleGeometryRequestBus::Handler
            , public AzFramework::RenderGeometry::IntersectionRequestBus::Handler
            , private AZ::TransformNotificationBus::Handler
            , private JE::Render::MaterialConsumerRequestBus::Handler
            , private JE::Render::MaterialComponentNotificationBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorMeshComponent;

            AZ_CLASS_ALLOCATOR(MeshComponentController, AZ::SystemAllocator);
            AZ_RTTI_WITH_NAME(JE::Render::Scene::MeshComponentController, "AZ::Render::MeshComponentController", "{D0F35FAC-4194-4C89-9487-D000DDB8B272}");

            ~MeshComponentController();

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            MeshComponentController() = default;
            MeshComponentController(const MeshComponentConfig& config);

            void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
            void Deactivate();
            void SetConfiguration(const MeshComponentConfig& config);
            const MeshComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(MeshComponentController);

            // MeshComponentRequestBus overrides ...
            void SetModelAsset(AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset) override;
            AZ::Data::Asset<const JE::Render::RPI::ModelAsset> GetModelAsset() const override;
            void SetModelAssetId(AZ::Data::AssetId modelAssetId) override;
            AZ::Data::AssetId GetModelAssetId() const override;
            void SetModelAssetPath(const AZStd::string& modelAssetPath) override;
            AZStd::string GetModelAssetPath() const override;
            AZ::Data::Instance<JE::Render::RPI::Model> GetModel() const override;

            // JE::Render::Debug::AtomImGuiMeshCallbackBus::Handler overrides ...
            const JE::Render::RPI::MeshDrawPacketLods* GetDrawPackets() const override;

            // MeshHandleStateRequestBus overrides ...
            const JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle* GetMeshHandle() const override;

            void SetSortKey(AZ::RHI::DrawItemSortKey sortKey) override;
            AZ::RHI::DrawItemSortKey GetSortKey() const override;

            void SetIsAlwaysDynamic(bool isAlwaysDynamic) override;
            bool GetIsAlwaysDynamic() const override;

            void SetLodType(JE::Render::RPI::Cullable::LodType lodType) override;
            JE::Render::RPI::Cullable::LodType GetLodType() const override;

            void SetLodOverride(JE::Render::RPI::Cullable::LodOverride lodOverride) override;
            JE::Render::RPI::Cullable::LodOverride GetLodOverride() const override;

            void SetMinimumScreenCoverage(float minimumScreenCoverage) override;
            float GetMinimumScreenCoverage() const override;

            void SetQualityDecayRate(float qualityDecayRate) override;
            float GetQualityDecayRate() const override;

            void SetVisibility(bool visible) override;
            bool GetVisibility() const override;

            void SetRayTracingEnabled(bool enabled) override;
            bool GetRayTracingEnabled() const override;

            void SetExcludeFromReflectionCubeMaps(bool excludeFromReflectionCubeMaps) override;
            bool GetExcludeFromReflectionCubeMaps() const override;

            // AzFramework::BoundsRequestBus::Handler overrides ...
            AZ::Aabb GetWorldBounds() const override;
            AZ::Aabb GetLocalBounds() const override;

            // AzFramework::VisibleGeometryRequestBus::Handler overrides ...
            void BuildVisibleGeometry(const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const override;

            // Searches all shader items referenced by the material for one with a matching draw list tag.
            // Returns true if a matching tag was found. Otherwise, false.
            bool DoesMaterialUseDrawListTag(
                const AZ::Data::Instance<JE::Render::RPI::Material> material, const AZ::RHI::DrawListTag searchDrawListTag) const;

            // IntersectionRequestBus overrides ...
            AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) const override;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // MaterialConsumerRequestBus::Handler overrides ...
            JE::Render::MaterialAssignmentId FindMaterialAssignmentId(
                const JE::Render::MaterialAssignmentLodIndex lod, const AZStd::string& label) const override;
            JE::Render::MaterialAssignmentLabelMap GetMaterialLabels() const override;
            JE::Render::MaterialAssignmentMap GetDefaultMaterialMap() const override;
            AZStd::unordered_set<AZ::Name> GetModelUvNames() const override;

            // MaterialComponentNotificationBus::Handler overrides ...
            void OnMaterialsUpdated(const JE::Render::MaterialAssignmentMap& materials) override;
            void OnMaterialPropertiesUpdated(const JE::Render::MaterialAssignmentMap& materials) override;

            //! Check if the model asset requires to be cloned (e.g. cloth) for unique model instances.
            //! @param modelAsset The model asset to check.
            //! @result True in case the model asset needs to be cloned before creating the model. False if there is a 1:1 relationship between
            //! the model asset and the model and it is static and shared. In the second case the m_originalModelAsset of the mesh handle is
            //! equal to the model asset that the model is linked to.
            static bool RequiresCloning(const AZ::Data::Asset<JE::Render::RPI::ModelAsset>& modelAsset);

            void HandleModelChange(const AZ::Data::Instance<JE::Render::RPI::Model>& model);
            void HandleObjectSrgCreate(const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& objectSrg);
            void RegisterModel();
            void UnregisterModel();
            void RefreshModelRegistration();

            JE::Render::RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;

            void HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale);

            void LightingChannelMaskChanged();

            JE::Render::Features::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
            AZ::TransformInterface* m_transformInterface = nullptr;
            AZ::EntityComponentIdPair m_entityComponentIdPair;
            bool m_isVisible = true;
            MeshComponentConfig m_configuration;
            AZ::Vector3 m_cachedNonUniformScale = AZ::Vector3::CreateOne();
            //! Cached bus to use to notify RenderGeometry::Intersector the entity/component has changed.
            AzFramework::RenderGeometry::IntersectionNotificationBus::BusPtr m_intersectionNotificationBus;

            JE::Render::Features::MeshHandleDescriptor::ModelChangedEvent::Handler m_modelChangedEventHandler
            {
                [&](const AZ::Data::Instance<JE::Render::RPI::Model>& model) { HandleModelChange(model); }
            };
            
            JE::Render::Features::MeshHandleDescriptor::ObjectSrgCreatedEvent::Handler m_objectSrgCreatedHandler
            {
                [&](const AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& objectSrg) { HandleObjectSrgCreate(objectSrg); }
            };

            AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler
            {
                [&](const AZ::Vector3& nonUniformScale) { HandleNonUniformScaleChange(nonUniformScale); }
            };
        };
    } // namespace Render
} // namespace AZ
