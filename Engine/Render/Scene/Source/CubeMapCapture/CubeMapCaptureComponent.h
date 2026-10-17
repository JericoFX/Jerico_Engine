/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CubeMapCapture/CubeMapCaptureComponentController.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace JE::Render
{
    namespace Scene
    {
        class CubeMapCaptureComponent final
            : public AzFramework::Components::ComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::CubeMapCaptureComponent, "AZ::Render::CubeMapCaptureComponent", CubeMapCaptureComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::CubeMapCaptureComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::CubeMapCaptureComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::CubeMapCaptureComponent, AZ::ComponentAllocator);

            CubeMapCaptureComponent() = default;
            CubeMapCaptureComponent(const CubeMapCaptureComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ
