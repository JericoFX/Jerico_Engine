/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshStatsCollector.h>
#include <JE/Render/Features/SkinnedMesh/SkinnedMeshFeatureProcessorInterface.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

#include <JE/Render/RPI/Public/Culling.h>
#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <Atom/Utils/StableDynamicArray.h>

#include <AzCore/base.h>
#include <AzCore/std/parallel/concurrency_checker.h>

namespace JE::Render
{
    namespace Features
    {
        class SkinnedMeshComputePass;
        class MorphTargetComputePass;
        class MeshFeatureProcessor;

        class SkinnedMeshFeatureProcessor final
            : public SkinnedMeshFeatureProcessorInterface
        {
            friend class SkinnedMeshStatsCollector;
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI_WITH_NAME(JE::Render::Features::SkinnedMeshFeatureProcessor, "AZ::Render::SkinnedMeshFeatureProcessor", "{D1F44963-913F-4210-92E1-945FA306BED4}", JE::Render::Features::SkinnedMeshFeatureProcessorInterface);
            AZ_FEATURE_PROCESSOR(SkinnedMeshFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            SkinnedMeshFeatureProcessor() = default;
            virtual ~SkinnedMeshFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;
            void OnRenderEnd() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(JE::Render::RPI::RenderPipeline* renderPipeline, JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType) override;
            void OnBeginPrepareRender() override;

            // SkinnedMeshFeatureProcessorInterface overrides ...
            SkinnedMeshHandle AcquireSkinnedMesh(const SkinnedMeshHandleDescriptor& desc) override;
            bool ReleaseSkinnedMesh(SkinnedMeshHandle& handle) override;
            void SetSkinningMatrices(const SkinnedMeshHandle& handle, const AZStd::vector<float>& data) override;
            void SetMorphTargetWeights(const SkinnedMeshHandle& handle, uint32_t lodIndex, const AZStd::vector<float>& weights) override;
            void EnableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) override;
            void DisableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) override;

            AZ::Data::Instance<JE::Render::RPI::Shader> GetSkinningShader() const;
            JE::Render::RPI::ShaderOptionGroup CreateSkinningShaderOptionGroup(const SkinnedMeshShaderOptions shaderOptions, SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedHandler);
            void OnSkinningShaderReinitialized(const AZ::Data::Instance<JE::Render::RPI::Shader> skinningShader);
            void SubmitSkinningDispatchItems(const AZ::RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex);
            void SetupSkinningScope(AZ::RHI::FrameGraphInterface frameGraph, int deviceIndex);

            AZ::Data::Instance<JE::Render::RPI::Shader> GetMorphTargetShader() const;
            void SubmitMorphTargetDispatchItems(const AZ::RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex);
            void SetupMorphTargetScope(AZ::RHI::FrameGraphInterface frameGraph, int deviceIndex);

        private:
            AZ_DISABLE_COPY_MOVE(SkinnedMeshFeatureProcessor);

            void InitSkinningAndMorphPass(JE::Render::RPI::RenderPipeline* renderPipeline);

            static const char* s_featureProcessorName;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_skinningShader;
            CachedSkinnedMeshShaderOptions m_cachedSkinningShaderOptions;

            AZ::Data::Instance<JE::Render::RPI::Shader> m_morphTargetShader;

            AZStd::concurrency_checker m_renderProxiesChecker;
            AZ::StableDynamicArray<SkinnedMeshRenderProxy> m_renderProxies;
            AZStd::unique_ptr<SkinnedMeshStatsCollector> m_statsCollector;

            AZStd::unordered_set<const AZ::RHI::DispatchItem*> m_skinningDispatches;
            AZ::RHI::MultiDevice::DeviceMask m_alreadyCreatedSkinningScopeThisFrame;

            AZStd::unordered_set<const AZ::RHI::DispatchItem*> m_morphTargetDispatches;
            AZ::RHI::MultiDevice::DeviceMask m_alreadyCreatedMorphTargetScopeThisFrame;

            AZStd::mutex m_dispatchItemMutex;

        };
    } // namespace Features
} // namespace JE::Render
