/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawPacket.h>

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicBuffer.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicDrawContext.h>
#include <JE/Render/RPI/Public/Material/Material.h>

namespace JE::Render
{
    namespace RPI
    {
        class Scene;
        class RenderPipeline;
        class Pass;

        //! Geometry data which uses triangle lists 
        struct GeometryData
        {
            const void* m_vertexData = nullptr;
            uint32_t m_vertexDataSize = 0;
            uint32_t m_vertexCount = 0;
            const void* m_indexData = nullptr;
            uint32_t m_indexDataSize = 0;
            uint32_t m_indexCount = 0;
        };

        //! Interface of dynamic draw system which provide access to system dynamic buffer
        //! and some draw functions
        class JE_RENDER_INTEGRATION_API DynamicDrawInterface
        {
        public:
            AZ_RTTI(DynamicDrawInterface, "{0883B8A7-9D25-418A-8F98-B33C52FF21CC}");

            DynamicDrawInterface() = default;
            virtual ~DynamicDrawInterface() = default;

            static DynamicDrawInterface* Get();

            AZ_DISABLE_COPY_MOVE(DynamicDrawInterface);

            //! Create a DynamicDrawContext
            //! The created DynamicDrawContext is managed by dynamic draw system.
            virtual AZ::RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext() = 0;

            //! Get a DynamicBuffer from DynamicDrawSystem.
            //! The returned buffer will be invalidated every time the RPISystem's RenderTick is called
            virtual AZ::RHI::Ptr<DynamicBuffer> GetDynamicBuffer(uint32_t size, uint32_t alignment) = 0;

            //! Draw a geometry to a scene with a given material
            virtual void DrawGeometry(AZ::Data::Instance<Material> material, const GeometryData& geometry, ScenePtr scene) = 0;

            //! Deprecated. Please use AddDrawPacket(Scene* scene, ConstPtr<RHI::DrawPacket> drawPacket) instead
            //! Submits a DrawPacket to the renderer.
            //! Note that ownership of the DrawPacket pointer is passed to the dynamic draw system.
            //! (it will be cleaned up correctly since the DrawPacket keeps track of the allocator that was used when it was built)
            virtual void AddDrawPacket(Scene* scene, AZStd::unique_ptr<const AZ::RHI::DrawPacket> drawPacket) = 0;
            
            //! Submits a DrawPacket to the scene.
            //! The dynamic draw system will keep a reference for the DrawPacket until it's rendered.
            virtual void AddDrawPacket(Scene* scene, ConstPtr<AZ::RHI::DrawPacket> drawPacket) = 0;

            //! Get DrawLists from any DynamicDrawContext which output to the specified RasterPass.
            virtual AZStd::vector<AZ::RHI::DrawListView> GetDrawListsForPass(const RasterPass* pass) = 0;
        };

        //! Global function to query the DynamicDrawInterface.
        inline DynamicDrawInterface* GetDynamicDraw()
        {
            return DynamicDrawInterface::Get();
        }
    }
}
