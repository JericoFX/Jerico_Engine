/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessor.h>
#include <AzCore/Math/Random.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>

namespace JE::Render
{
    namespace Features
    {
        OcclusionCullingPlane::~OcclusionCullingPlane()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_meshFeatureProcessor->ReleaseMesh(m_visualizationMeshHandle);
        }

        void OcclusionCullingPlane::Init(JE::Render::RPI::Scene* scene)
        {
            AZ_Assert(scene, "OcclusionCullingPlane::Init called with a null Scene pointer");

            m_meshFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::MeshFeatureProcessorInterface>();

            // load visualization plane model and material
            m_visualizationModelAsset = JE::Render::RPI::AssetUtils::LoadCriticalAsset<JE::Render::RPI::ModelAsset>(
                "Models/OcclusionCullingPlane.fbx.azmodel",
                JE::Render::RPI::AssetUtils::TraceLevel::Assert);

            m_visualizationMeshHandle = m_meshFeatureProcessor->AcquireMesh(MeshHandleDescriptor(m_visualizationModelAsset));
            m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(m_visualizationMeshHandle, true);
            m_meshFeatureProcessor->SetRayTracingEnabled(m_visualizationMeshHandle, false);
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, AZ::Transform::CreateIdentity());

            SetVisualizationMaterial();
        }

        void OcclusionCullingPlane::SetVisualizationMaterial()
        {
            AZStd::string materialAssetPath;
            if (m_transparentVisualization)
            {
                materialAssetPath = "Materials/OcclusionCullingPlane/OcclusionCullingPlaneTransparentVisualization.azmaterial";
            }
            else
            {
                materialAssetPath = "Materials/OcclusionCullingPlane/OcclusionCullingPlaneVisualization.azmaterial";
            }

            JE::Render::RPI::AssetUtils::TraceLevel traceLevel = JE::Render::RPI::AssetUtils::TraceLevel::Assert;
            m_visualizationMaterialAsset = JE::Render::RPI::AssetUtils::LoadCriticalAsset<JE::Render::RPI::MaterialAsset>(materialAssetPath.c_str(), traceLevel);
            m_visualizationMaterialAsset.QueueLoad();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_visualizationMaterialAsset.GetId());
        }

        void OcclusionCullingPlane::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            if (m_visualizationMaterialAsset.GetId() == asset.GetId())
            {
                m_visualizationMaterialAsset = asset;
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

                m_visualizationMaterial = JE::Render::RPI::Material::FindOrCreate(m_visualizationMaterialAsset);
                m_meshFeatureProcessor->SetCustomMaterials(m_visualizationMeshHandle, m_visualizationMaterial);
            }
        }

        void OcclusionCullingPlane::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZ_Error("OcclusionCullingPlane", false, "Failed to load OcclusionCullingPlane visualization asset %s", asset.ToString<AZStd::string>().c_str());
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        }

        void OcclusionCullingPlane::SetTransform(const AZ::Transform& transform)
        {
            m_transform = transform;

            // update visualization plane transform
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, transform);
        }

        void OcclusionCullingPlane::ShowVisualization(bool showVisualization)
        {
            if (m_showVisualization != showVisualization)
            {
                m_meshFeatureProcessor->SetVisible(m_visualizationMeshHandle, showVisualization);
                SetVisualizationMaterial();
            }
        }

        void OcclusionCullingPlane::SetTransparentVisualization(bool transparentVisualization)
        {
            if (m_transparentVisualization != transparentVisualization)
            {
                m_transparentVisualization = transparentVisualization;
                SetVisualizationMaterial();
            }
        }

    } // namespace Features
} // namespace JE::Render
