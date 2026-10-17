/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/AssetCreator.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>

#include <AzCore/Asset/AssetCommon.h>

namespace UnitTest
{
    class PassBuilderTests;
}

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        //! An asset that describes the root of a pass by having a PassTemplate.
        //! By adding PassRequests to the template you can describe an entire tree of passes.
        class JE_RENDER_INTEGRATION_API PassAsset final
            : public AZ::Data::AssetData
        {
            friend class PassSystem;
            friend class UnitTest::PassBuilderTests;
        public:
            AZ_RTTI(PassAsset, "{FBAF94C2-6617-491E-8269-55DBC9845539}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(PassAsset, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            static constexpr const char* DisplayName{ "Pass" };
            static constexpr const char* Group{ "RenderingPipeline" };
            static constexpr const char* Extension{ "pass" };

            //! Retrieves the underlying PassTemplate
            const AZStd::unique_ptr<PassTemplate>& GetPassTemplate() const;

        private:
            // Sets the pass template on this pass asset. Use for testing only.
            void SetPassTemplateForTestingOnly(const PassTemplate& passTemplate);

            AZStd::unique_ptr<PassTemplate> m_passTemplate = nullptr;
        };

        using PassAssetHandler = AssetHandler<PassAsset>;

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
