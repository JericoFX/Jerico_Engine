/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>

#include <Atom/RHI/FreeListAllocator.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class BufferAsset;
        class Buffer;
    }

namespace JE::Render
{

    namespace Features
    {
        class SkinnedMeshOutputStreamManager
            : public SkinnedMeshOutputStreamManagerInterface
            , private AZ::SystemTickBus::Handler
        {
        public:
            AZ_RTTI_WITH_NAME(SkinnedMeshOutputStreamManager, "AZ::Render::SkinnedMeshOutputStreamManager", "{3107EC84-DDF6-46FD-8B2C-00431D1BB67C}", JE::Render::Features::SkinnedMeshOutputStreamManagerInterface);

            SkinnedMeshOutputStreamManager();
            ~SkinnedMeshOutputStreamManager();
            void Init();

            // SkinnedMeshOutputStreamManagerInterface
            AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> Allocate(size_t byteCount) override;
            void DeAllocate(AZ::RHI::VirtualAddress allocation) override;
            void DeAllocateNoSignal(AZ::RHI::VirtualAddress allocation) override;
            AZ::Data::Asset<JE::Render::RPI::BufferAsset> GetBufferAsset() override;
            AZ::Data::Instance<JE::Render::RPI::Buffer> GetBuffer() override;

        private:
            // SystemTickBus
            void OnSystemTick() override;

            void EnsureInit();
            void GarbageCollect();
            void CalculateAlignment();
            void CreateBufferAsset();

            AZ::Data::Asset<JE::Render::RPI::BufferAsset> m_bufferAsset;
            AZ::Data::Instance<JE::Render::RPI::Buffer> m_buffer;
            AZ::RHI::FreeListAllocator m_freeListAllocator;
            AZStd::mutex m_allocatorMutex;
            uint64_t m_alignment = 1;
            size_t m_sizeInBytes = 0;
            bool m_memoryWasFreed = false;
            bool m_broadcastMemoryAvailableEvent = false;
            bool m_needsInit = true;
        };
    } // namespace Features
} // namespace JE::Render
