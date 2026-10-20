/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>

#include <Components/DiffuseProbeGridComponentController.h>
#include <Components/DiffuseProbeGridComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Features
    {
        class JE_RENDER_FEATURES_API DiffuseProbeGridComponent final
            : public AzFramework::Components::ComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponentConfig>;
            AZ_RTTI_WITH_NAME(DiffuseProbeGridComponent, "AZ::Render::DiffuseProbeGridComponent", DiffuseProbeGridComponentTypeId, BaseClass, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(DiffuseProbeGridComponent)
            AZ_COMPONENT_BASE(DiffuseProbeGridComponent)
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponent, AZ::ComponentAllocator);

            DiffuseProbeGridComponent() = default;
            DiffuseProbeGridComponent(const DiffuseProbeGridComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Features
} // namespace JE::Render
