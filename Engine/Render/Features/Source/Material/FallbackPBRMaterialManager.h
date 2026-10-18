/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>
#include <JE/Render/Features/Mesh/MeshInfo.h>
#include <JE/Render/Features/Mesh/MeshInfoBus.h>
#include <JE/Render/Features/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <JE/Render/Features/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Buffer/RingBuffer.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <AzCore/std/parallel/mutex.h>


namespace JE::Render::Features
{
    namespace FallbackPBR
    {
        class MaterialManager : public JE::Render::Features::MeshInfoNotificationBus::Handler
        {
        public:
            MaterialManager();
            void Activate(JE::Render::RPI::Scene* scene);
            void Deactivate();
            void Update();

            // integration with the MeshFeatureProcessor
            const AZ::RHI::Ptr<MaterialEntry> GetFallbackPBRMaterialEntry(const JE::Render::Features::MeshInfoHandle handle);
            void UpdateFallbackPBRMaterialEntry(const JE::Render::Features::MeshInfoHandle handle, AZStd::function<bool(MaterialEntry*)> updateFunction);
            const AZ::Data::Instance<JE::Render::RPI::Buffer>& GetFallbackPBRMaterialBuffer() const;

            void UpdateReflectionProbes(const JE::Render::Features::TransformServiceFeatureProcessorInterface::ObjectId& objectId, const AZ::Aabb& aabbWS);

        protected:
            // --------------------------------------------------------------------------------------
            // MeshInfoNotificationBus::Handler overrides
            void OnAcquireMeshInfoEntry(const JE::Render::Features::MeshInfoHandle meshInfoHandle) override;
            void OnReleaseMeshInfoEntry(const JE::Render::Features::MeshInfoHandle meshInfoHandle) override;
            void OnPopulateMeshInfoEntry(
                const JE::Render::Features::MeshInfoHandle meshInfoHandle,
                JE::Render::Features::ModelDataInstanceInterface* modelData,
                const size_t lodIndex,
                const size_t lodMeshIndex) override;
            // --------------------------------------------------------------------------------------

        private:
            bool m_isEnabled = false;

            void UpdateFallbackPBRMaterial();
            void UpdateFallbackPBRMaterialBuffer();
            void InitNullCubeMap();

            // reflection probe
            struct ReflectionProbe
            {
                AZ::Transform m_modelToWorld;
                AZ::Vector3 m_outerObbHalfLengths;
                AZ::Vector3 m_innerObbHalfLengths;
                bool m_useParallaxCorrection = false;
                float m_exposure = 0.0f;

                AZ::Data::Instance<JE::Render::RPI::Image> m_reflectionProbeCubeMap;
            };
            AZStd::unordered_map<JE::Render::Features::TransformServiceFeatureProcessorInterface::ObjectId, ReflectionProbe> m_reflectionProbeData;

            AZStd::mutex m_mutex;
            AZStd::vector<AZ::RHI::Ptr<MaterialEntry>> m_materialData;
            JE::Render::RPI::RingBuffer m_materialDataBuffer;
            bool m_bufferNeedsUpdate;

            AZ::RHI::ShaderInputNameIndex m_fallbackPBRMaterialIndex = "m_fallbackPBRMaterial";
            AZ::RHI::ShaderInputNameIndex m_nullCubeMapIndex = "m_nullCubeMapIndex";
            AZ::Data::Instance<JE::Render::RPI::Image> m_nullCubeMapTexture;
            JE::Render::RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
            JE::Render::Features::ReflectionProbeFeatureProcessorInterface* m_rpfp = nullptr;
        };
    } // namespace FallbackPBR
} // namespace JE::Render::Features