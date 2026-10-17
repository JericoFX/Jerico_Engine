/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class AuxGeomDrawQueue;
        class DynamicPrimitiveProcessor;
        class FixedShapeProcessor;

        /**
         * FeatureProcessor for Auxiliary Geometry
         */
        class AuxGeomFeatureProcessor final
            : public JE::Render::RPI::AuxGeomFeatureProcessorInterface
        {
        public: // functions
            AZ_CLASS_ALLOCATOR(AuxGeomFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::AuxGeomFeatureProcessor, "{75E17417-C8E3-4B64-8469-7662D1E0904A}", JE::Render::RPI::AuxGeomFeatureProcessorInterface);
            AZ_FEATURE_PROCESSOR(AuxGeomFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            AuxGeomFeatureProcessor() = default;
            virtual ~AuxGeomFeatureProcessor() = default;

            // RPI::FeatureProcessor
            void Activate() override;
            void Deactivate() override;
            void Render(const FeatureProcessor::RenderPacket& fpPacket) override;            
            void OnRenderEnd() override;

            // RPI::AuxGeomFeatureProcessorInterface
            JE::Render::RPI::AuxGeomDrawPtr GetDrawQueue() override; // returns the scene DrawQueue
            JE::Render::RPI::AuxGeomDrawPtr GetDrawQueueForView(const JE::Render::RPI::View* view) override;

            JE::Render::RPI::AuxGeomDrawPtr GetOrCreateDrawQueueForView(const JE::Render::RPI::View* view) override;
            void ReleaseDrawQueueForView(const JE::Render::RPI::View* view) override;

            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* pipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        private: // functions

            AuxGeomFeatureProcessor(const AuxGeomFeatureProcessor&) = delete;
            void OnSceneRenderPipelinesChanged();

        private: // data

            static const char* s_featureProcessorName;
            
            //! Cache a pointer to the AuxGeom draw queue for our scene
            JE::Render::RPI::AuxGeomDrawPtr m_sceneDrawQueue = nullptr;

            //! Map used to store the AuxGeomDrawQueue for each view
            AZStd::map<const JE::Render::RPI::View*, JE::Render::RPI::AuxGeomDrawPtr> m_viewDrawDataMap; // using View* as key to not hold a reference to the view

            //! The object that handles the dynamic primitive geometry data
            AZStd::unique_ptr<DynamicPrimitiveProcessor> m_dynamicPrimitiveProcessor;

            //! The object that handles fixed shape geometry data
            AZStd::unique_ptr<FixedShapeProcessor> m_fixedShapeProcessor;
        };

        inline JE::Render::RPI::AuxGeomDrawPtr AuxGeomFeatureProcessor::GetDrawQueue()
        {
            return m_sceneDrawQueue;
        }

    } // namespace Render
} // namespace AZ
