/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Instance/Instance.h>

#include <JE/Render/RPI/Public/FeatureProcessor.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>

#include <SharedBuffer.h>
#include <MultiDispatchComputePass.h>
#include <MeshletsRenderPass.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class RenderPipeline;
    }

namespace AZ
{

    namespace Meshlets
    {
        class MeshletsRenderObject;


        // Not used yet - should be developed to support instancing
        struct RenderObjectInstanceData
        {
            Render::TransformServiceFeatureProcessorInterface::ObjectId m_objectId;
            MeshletsRenderObject* meshletsRenderObject;
        };

        class MeshletsFeatureProcessor final
            : public JE::Render::RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
            Name MeshletsComputePassName;
            Name MeshletsRenderPassName;

        public:
            AZ_RTTI(MeshletsFeatureProcessor, "{1D93DE27-2DC4-4E9B-90B3-DCDCB941C920}", JE::Render::RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            MeshletsFeatureProcessor();
            virtual ~MeshletsFeatureProcessor();

            void Init(JE::Render::RPI::RenderPipeline* pipeline);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline) override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            bool InitComputePass(const Name& passName);
            bool InitRenderPass(const Name& passName);

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            void SetTransform(
                const Render::TransformServiceFeatureProcessorInterface::ObjectId objectId,
                const AZ::Transform& transform);
            Render::TransformServiceFeatureProcessorInterface::ObjectId AddMeshletsRenderObject(
                MeshletsRenderObject* meshletsRenderObject);
            void RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject);

            Data::Instance<JE::Render::RPI::Shader> GetComputeShader() { return m_computeShader; }
            Data::Instance<JE::Render::RPI::Shader> GetRenderShader() { return m_renderShader; }

        protected:
            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* pipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            bool BuildDrawPacket(
                ModelLodDataArray& lodRenderDataArray,
                Render::TransformServiceFeatureProcessorInterface::ObjectId objectId);

            bool HasMeshletPasses(JE::Render::RPI::RenderPipeline* renderPipeline);
            bool AddMeshletsPassesToPipeline(JE::Render::RPI::RenderPipeline* renderPipeline);

            void CreateResources();
            void CleanResources();
            void CleanPasses();

            void DeletePendingMeshletsRenderObjects();

        private:
            AZ_DISABLE_COPY_MOVE(MeshletsFeatureProcessor);

            AZStd::unique_ptr<Meshlets::SharedBuffer> m_sharedBuffer;  // used for all meshlets geometry buffers.

            // Cache the pass request data for creating a parent pass and associating it in the pipeline
            AZ::Data::Asset<JE::Render::RPI::AnyAsset> m_passRequestAsset;

            Data::Instance<MultiDispatchComputePass> m_computePass;
            Data::Instance<MeshletsRenderPass> m_renderPass;

            AZStd::list<MeshletsRenderObject*> m_meshletsRenderObjects;

            //! The render pipeline is acquired and set when a pipeline is created or changed
            //! and accordingly the passes and the feature processor are associated.
            //! Notice that scene can contain several pipelines all using the same feature
            //! processor.  On the pass side, it will acquire the scene and request the FP, 
            //! but on the FP side, it will only associate to the latest pass hence such a case
            //! might still be a problem.  If needed, it can be resolved using a map for each
            //! pass name per pipeline.
            JE::Render::RPI::RenderPipeline* m_renderPipeline = nullptr;

            Render::TransformServiceFeatureProcessorInterface* m_transformServiceFeatureProcessor = nullptr;

            AZStd::list<MeshletsRenderObject*> m_renderObjectsMarkedForDeletion;

            Data::Instance<JE::Render::RPI::Shader> m_renderShader;
            Data::Instance<JE::Render::RPI::Shader> m_computeShader;
            RHI::GeometryView m_geometryView { RHI::MultiDevice::AllDevices };
        };
    } // namespace Meshlets
} // namespace AZ
