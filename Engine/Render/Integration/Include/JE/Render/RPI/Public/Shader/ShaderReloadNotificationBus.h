/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>

namespace JE::Render
{
    namespace RPI
    {
        class Shader;
        class ShaderAsset;
        class ShaderVariant;

        /**
         * Connect to this EBus to get notifications whenever a shader system class reinitializes itself.
         * The bus address is the AssetId of the ShaderAsset, even when the thing being reinitialized is a ShaderVariant or other shader related class.
         *
         * Be careful when using the parameters provided by these functions. The bus ID is an AssetId, and it's possible for the system to have
         * both *old* versions and *new reloaded* versions of the asset in memory at the same time, and they will have the same AssetId. Therefore
         * your bus Handlers could receive Reinitialized messages from multiple sources. It may be necessary to check the memory addresses of these
         * parameters against local members before using this data.
         */
        class JE_RENDER_INTEGRATION_API ShaderReloadNotifications
            : public AZ::EBusTraits
        {

        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZ::Data::AssetId              BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~ShaderReloadNotifications() {}

            //! Called when the ShaderAsset reinitializes itself in response to another asset being reloaded.
            virtual void OnShaderAssetReinitialized(const AZ::Data::Asset<ShaderAsset>& shaderAsset) { AZ_UNUSED(shaderAsset); }

            //! Called when the Shader instance reinitializes itself in response to the ShaderAsset being reloaded.
            virtual void OnShaderReinitialized(const Shader& shader) { AZ_UNUSED(shader); }

            //! Called when a particular shader variant is reinitialized.
            virtual void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) { AZ_UNUSED(shaderVariant); }
        };

        typedef AZ::EBus<ShaderReloadNotifications> ShaderReloadNotificationBus;

    } // namespace RPI
} //namespace AZ

AZ_DECLARE_EBUS_MULTI_ADDRESS(JE_RENDER_INTEGRATION_API, JE::Render::RPI::ShaderReloadNotifications);
