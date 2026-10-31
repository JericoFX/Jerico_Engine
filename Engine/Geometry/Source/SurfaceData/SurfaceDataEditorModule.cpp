/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceDataEditorModule.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <Editor/EditorSurfaceDataSystemComponent.h>
#include <Editor/EditorSurfaceDataColliderComponent.h>
#include <Editor/EditorSurfaceDataShapeComponent.h>

namespace JE::Geometry::SurfaceData
{
    SurfaceDataEditorModule::SurfaceDataEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            EditorSurfaceDataSystemComponent::CreateDescriptor(),
            EditorSurfaceDataColliderComponent::CreateDescriptor(),
            EditorSurfaceDataShapeComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList SurfaceDataEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = SurfaceDataModule::GetRequiredSystemComponents();
        requiredComponents.push_back(azrtti_typeid<EditorSurfaceDataSystemComponent>());
        return requiredComponents;
    }

    AZ::Module* CreateAuthoringModule()
    {
        return aznew SurfaceDataEditorModule;
    }
}
