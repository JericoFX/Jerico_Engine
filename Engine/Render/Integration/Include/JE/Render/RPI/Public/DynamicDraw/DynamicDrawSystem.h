/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicDrawInterface.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicBufferAllocator.h>
#include <JE/Render/RPI/Reflect/RPISystemDescriptor.h>

namespace JE::Render
{
    namespace RPI
    {
        //! DynamicDrawSystem is the dynamic draw system in RPI system which implements DynamicDrawInterface
        //! It contains the dynamic buffer allocator which is used to allocate dynamic buffers.
        //! It also responses to submit all the dynamic draw data to the rendering passes.
        class JE_RENDER_INTEGRATION_API DynamicDrawSystem final : public DynamicDrawInterface
        {
            friend class RPISystem;
        public:
            AZ_TYPE_INFO(DynamicDrawSystem, "{49D23FE9-352F-4AB0-B9BB-A3B75592023B}");

            // Dynamic draw system initialization and shutdown
            void Init(const DynamicDrawSystemDescriptor& descriptor);
            void Shutdown();

            // DynamicDrawInterface overrides...
            AZ::RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext() override;
            AZ::RHI::Ptr<DynamicBuffer> GetDynamicBuffer(uint32_t size, uint32_t alignment) override;
            void DrawGeometry(AZ::Data::Instance<Material> material, const GeometryData& geometry, ScenePtr scene) override;
            void AddDrawPacket(Scene* scene, AZStd::unique_ptr<const AZ::RHI::DrawPacket> drawPacket) override;
            void AddDrawPacket(Scene* scene, ConstPtr<AZ::RHI::DrawPacket> drawPacket) override;
            AZStd::vector<AZ::RHI::DrawListView> GetDrawListsForPass(const RasterPass* pass) override;

            // Submit draw data for selected scene and pipeline
            void SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views);

        protected:

            void FrameEnd();

        private:
            AZStd::mutex m_mutexBufferAlloc;
            AZStd::unique_ptr<DynamicBufferAllocator> m_bufferAlloc;

            AZStd::mutex m_mutexDrawContext;
            AZStd::list<AZ::RHI::Ptr<DynamicDrawContext>> m_dynamicDrawContexts;

            AZStd::mutex m_mutexDrawPackets;
            AZStd::map<Scene*, AZStd::vector<ConstPtr<const AZ::RHI::DrawPacket>>> m_drawPackets;
        };
    }
}
