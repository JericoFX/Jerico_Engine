/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>

#include <JE/Render/RPI/Public/Buffer/BufferSystem.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <JE/Render/RPI/Public/Buffer/BufferPool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <JE/Render/RPI/Reflect/Buffer/BufferAssetCreator.h>
#include <JE/Render/RPI/Reflect/Buffer/BufferAssetView.h>

#include <AzCore/Instance/InstanceDatabase.h>
#include <AzCore/Interface/Interface.h>

namespace JE::Render
{
    namespace RPI
    {
        void BufferSystem::Reflect(AZ::ReflectContext* context)
        {
            BufferAsset::Reflect(context);
            BufferAssetView::Reflect(context);
        }

        void BufferSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<BufferAssetHandler>());
        }

        void BufferSystem::Init()
        {
            {
                AZ::Data::InstanceHandler<Buffer> handler;
                handler.m_createFunction = [](AZ::Data::AssetData* bufferAsset)
                {
                    return Buffer::CreateInternal(*(azrtti_cast<BufferAsset*>(bufferAsset)));
                };
                AZ::Data::InstanceDatabase<Buffer>::Create(azrtti_typeid<BufferAsset>(), handler);
            }

            {
                AZ::Data::InstanceHandler<BufferPool> handler;
                handler.m_createFunction = [](AZ::Data::AssetData* poolAsset)
                {
                    return BufferPool::CreateInternal(*(azrtti_cast<ResourcePoolAsset*>(poolAsset)));
                };
                AZ::Data::InstanceDatabase<BufferPool>::Create(azrtti_typeid<ResourcePoolAsset>(), handler);
            }
            AZ::Interface<BufferSystemInterface>::Register(this);

            m_initialized = true;
        }

        void BufferSystem::Shutdown()
        {
            if (!m_initialized)
            {
                return;
            }
            for (uint8_t index = 0; index < static_cast<uint8_t>(CommonBufferPoolType::Count); index++)
            {
                m_commonPools[index] = nullptr;
            }
            AZ::Interface<BufferSystemInterface>::Unregister(this);
            AZ::Data::InstanceDatabase<Buffer>::Destroy();
            AZ::Data::InstanceDatabase<BufferPool>::Destroy();
            m_initialized = false;
        }
        
        AZ::RHI::Ptr<AZ::RHI::BufferPool> BufferSystem::GetCommonBufferPool(CommonBufferPoolType poolType)
        {
            const uint8_t index = static_cast<uint8_t>(poolType);
            if (!m_commonPools[index])
            {
                CreateCommonBufferPool(poolType);
            }

            return m_commonPools[index];
        }

        bool BufferSystem::CreateCommonBufferPool(CommonBufferPoolType poolType)
        {
            if (!m_initialized)
            {
                return false;
            }

            AZ::RHI::Ptr<AZ::RHI::BufferPool> bufferPool = aznew AZ::RHI::BufferPool;

            AZ::RHI::BufferPoolDescriptor bufferPoolDesc;
            switch (poolType)
            {
            case CommonBufferPoolType::Constant:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::Constant;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::StaticInputAssembly:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::DynamicInputAssembly:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::DynamicInputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadBack:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Read;
                break;
            case CommonBufferPoolType::Staging:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::CopyRead;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadWrite:
                // Add CopyRead flag too since it's often we need to read back GPU attachment buffers.
                bufferPoolDesc.m_bindFlags =
//                  [To Do] - the following line (and possibly InputAssembly / DynamicInputAssembly) will need to
//                  be added to support future indirect buffer usage for GPU driven render pipeline
//                    RHI::BufferBindFlags::Indirect |  
                    AZ::RHI::BufferBindFlags::ShaderWrite | AZ::RHI::BufferBindFlags::ShaderRead | AZ::RHI::BufferBindFlags::CopyRead;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::ReadOnly:
//                  [To Do] - the following line (and possibly InputAssembly / DynamicInputAssembly) will need to
//                  be added to support future indirect buffer usage for GPU driven render pipeline
                bufferPoolDesc.m_bindFlags = // RHI::BufferBindFlags::Indirect |
                    AZ::RHI::BufferBindFlags::ShaderRead;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            case CommonBufferPoolType::Indirect:
                bufferPoolDesc.m_bindFlags = AZ::RHI::BufferBindFlags::ShaderReadWrite | AZ::RHI::BufferBindFlags::Indirect |
                    AZ::RHI::BufferBindFlags::CopyRead | AZ::RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
                bufferPoolDesc.m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
                break;
            default:
                AZ_Error("BufferSystem", false, "Unknown common buffer pool type: %d", poolType);
                return false;
            }

            bufferPool->SetName(AZ::Name(AZStd::string::format("RPI::CommonBufferPool_%i", static_cast<uint32_t>(poolType))));
            AZ::RHI::ResultCode resultCode = bufferPool->Init(bufferPoolDesc);
            if (resultCode != AZ::RHI::ResultCode::Success)
            {
                AZ_Error("BufferSystem", false, "Failed to create buffer pool: %d", poolType);
                return false;
            }

            m_commonPools[static_cast<uint8_t>(poolType)] = bufferPool;
            return true;
        }

        AZ::Data::Instance<Buffer> BufferSystem::CreateBufferFromCommonPool(const CommonBufferDescriptor& descriptor)
        {            
            AZ::Uuid bufferId;
            if (descriptor.m_isUniqueName)
            {
                bufferId = AZ::Uuid::CreateName(descriptor.m_bufferName);
                // Report error if there is a buffer with same name.
                // Note: this shouldn't return the existing buffer because users are expecting a newly created buffer.
                if (AZ::Data::InstanceDatabase<Buffer>::Instance().Find(AZ::Data::InstanceId::CreateUuid(bufferId)))
                {
                    AZ_Error("BufferSystem", false, "Buffer with same name '%s' already exist", descriptor.m_bufferName.c_str());
                    return nullptr;
                }
            }
            else
            {
                bufferId = AZ::Uuid::CreateRandom();
            }

            AZ::RHI::Ptr<AZ::RHI::BufferPool> bufferPool = GetCommonBufferPool(descriptor.m_poolType);

            if (!bufferPool)
            {
                AZ_Error("BufferSystem", false, "Common buffer pool type %d doesn't exist", descriptor.m_poolType);
                return nullptr;
            }

            AZ::RHI::BufferDescriptor bufferDesc;
            bufferDesc.m_alignment = descriptor.m_elementSize;
            bufferDesc.m_bindFlags = bufferPool->GetDescriptor().m_bindFlags;
            bufferDesc.m_byteCount = descriptor.m_byteCount;
            bufferDesc.m_ownerDeviceIndex = descriptor.m_ownerDeviceIndex;

            AZ::Data::Asset<BufferAsset> bufferAsset;
            BufferAssetCreator creator;
            creator.Begin(bufferId);
            creator.SetBufferName(descriptor.m_bufferName);
            creator.SetBuffer(descriptor.m_bufferData, descriptor.m_bufferData? descriptor.m_byteCount : 0, bufferDesc);
            creator.SetUseCommonPool(descriptor.m_poolType);

            AZ::RHI::BufferViewDescriptor viewDescriptor;
            if (descriptor.m_elementFormat != AZ::RHI::Format::Unknown)
            {
                viewDescriptor = AZ::RHI::BufferViewDescriptor::CreateTyped(
                    0,
                    aznumeric_cast<uint32_t>(bufferDesc.m_byteCount / descriptor.m_elementSize),
                    descriptor.m_elementFormat);
            }
            else
            {
                viewDescriptor = AZ::RHI::BufferViewDescriptor::CreateStructured(
                    0, aznumeric_cast<uint32_t>(bufferDesc.m_byteCount / descriptor.m_elementSize), descriptor.m_elementSize);
            }
            creator.SetBufferViewDescriptor(viewDescriptor);

            if (creator.End(bufferAsset))
            {
                return AZ::Data::InstanceDatabase<Buffer>::Instance().FindOrCreate(AZ::Data::InstanceId::CreateUuid(bufferId), bufferAsset);
            }

            return nullptr;
        }

        AZ::Data::Instance<Buffer> BufferSystem::FindCommonBuffer(AZStd::string_view uniqueBufferName)
        {
            const AZ::Uuid bufferId = AZ::Uuid::CreateName(uniqueBufferName);
            return AZ::Data::InstanceDatabase<Buffer>::Instance().Find(AZ::Data::InstanceId::CreateUuid(bufferId));
        }
    } // namespace RPI
} // namespace AZ
