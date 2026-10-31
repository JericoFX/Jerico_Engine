/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Geometry/SurfaceData/SurfaceDataModule.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataColliderComponent.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataShapeComponent.h>

namespace JE::Geometry::SurfaceData
{
    SurfaceDataModule::SurfaceDataModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            SurfaceDataSystemComponent::CreateDescriptor(),
            SurfaceDataColliderComponent::CreateDescriptor(),
            SurfaceDataShapeComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList SurfaceDataModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<SurfaceDataSystemComponent>(),
        };
    }

    AZ::Module* CreateRuntimeModule()
    {
        return aznew SurfaceDataModule;
    }
}
