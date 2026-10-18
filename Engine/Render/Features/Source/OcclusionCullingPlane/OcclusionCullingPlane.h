/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h>
#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>

namespace JE::Render
{
    namespace Features
    {
        //! This class represents an OcclusionCullingPlane which is used to cull meshes that are inside the view frustum
        class OcclusionCullingPlane final
            : public AZ::Data::AssetBus::MultiHandler
        {
        public:
            OcclusionCullingPlane() = default;
            ~OcclusionCullingPlane();

            void Init(JE::Render::RPI::Scene* scene);

            void SetTransform(const AZ::Transform& transform);
            const AZ::Transform& GetTransform() const { return m_transform; }

            void SetEnabled(bool enabled) { m_enabled = enabled; }
            bool GetEnabled() const { return m_enabled; }

            // enables or disables rendering of the visualization plane
            void ShowVisualization(bool showVisualization);

            // sets the visualization to transparent mode
            void SetTransparentVisualization(bool transparentVisualization);

        private:

            void SetVisualizationMaterial();

            // AZ::Data::AssetBus::Handler overrides...
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            AZ::Transform m_transform;
            bool m_enabled = true;
            bool m_showVisualization = true;
            bool m_transparentVisualization = false;

            // visualization
            JE::Render::Features::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            AZ::Data::Asset<JE::Render::RPI::ModelAsset> m_visualizationModelAsset;
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_visualizationMaterialAsset;
            AZ::Data::Instance<JE::Render::RPI::Material> m_visualizationMaterial;
            JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle m_visualizationMeshHandle;
        };
    } // namespace Features
} // namespace JE::Render
