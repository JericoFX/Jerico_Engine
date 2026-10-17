/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <AzCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
    }

    }

namespace JE::Render::RPI
{
        /**
         * Wraps a RHI ShaderResourceGroupPool for use in the RPI to initialize ShaderResourceGroups.
         *
         * This class is a bit unusual in that InstanceData objects usually associate with an AssetData
         * class of corresponding type (for example, Material and MaterialAsset). In this case,
         * the ShaderResourceGroupPool instance corresponds to a specific ShaderResourceGroupAsset,
         * not a ShaderResourceGroup[Pool]Asset. This is because there is a 1:1 relationship with
         * ShaderResourceGroupAsset's ShaderResourceGroupLayout and no additional configuration data
         * is needed for the pool.
         *
         * User code should not need to access this pool directly; RPI::ShaderResourceGroup uses it internally.
         */
        class JE_RENDER_INTEGRATION_API ShaderResourceGroupPool final
            : public AZ::Data::InstanceData
        {
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(ShaderResourceGroupPool, "{D75561AF-C10A-41BC-BABF-63DBFA160388}");
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator);

            /**
             * Instantiates or returns an existing runtime pool for a given ShaderResourceGroup.
             * @param shaderResourceGroupPoolAsset The asset used to instantiate an instance of the streaming image pool.
             */
            static AZ::Data::Instance<ShaderResourceGroupPool> FindOrCreate(
                const AZ::Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroup> CreateRHIShaderResourceGroup();

            AZ::RHI::ShaderResourceGroupPool* GetRHIPool();
            const AZ::RHI::ShaderResourceGroupPool* GetRHIPool() const;

        private:
            ShaderResourceGroupPool() = default;

            // Standard RPI runtime instance initialization
            // @param anySrgInitParams Of Type RPI::ShaderResourceGroup::SrgInitParams.
            static AZ::Data::Instance<ShaderResourceGroupPool> CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* anySrgInitParams);

            AZ::RHI::ResultCode Init(ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName);

            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupPool> m_pool;
        };
    }

namespace AZ
{
}
