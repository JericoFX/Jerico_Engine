/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "SurfaceDataTestFixtures.h"
#include <SurfaceData/SurfaceDataTestMocks.h>

#include <AzFramework/Components/TransformComponent.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataColliderComponent.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataShapeComponent.h>


namespace UnitTest
{
    void SurfaceDataTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "LmbrCentral" });

        AddComponentDescriptors({
            AzFramework::TransformComponent::CreateDescriptor(),

            JE::Geometry::SurfaceData::SurfaceDataSystemComponent::CreateDescriptor(),
            JE::Geometry::SurfaceData::SurfaceDataColliderComponent::CreateDescriptor(),
            JE::Geometry::SurfaceData::SurfaceDataShapeComponent::CreateDescriptor(),

            MockPhysicsColliderComponent::CreateDescriptor()
        });
    }
}

