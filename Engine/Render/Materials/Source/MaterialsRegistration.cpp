/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <JE/Render/Materials/MaterialsRegistration.h>
#include <JE/Render/Materials/MaterialAssignment.h>
#include <JE/Render/Materials/MaterialComponent.h>

namespace JE::Render
{
    void ReflectMaterialTypes(AZ::ReflectContext* context)
    {
        MaterialAssignment::Reflect(context);
    }

    AZ::ComponentDescriptor* CreateMaterialComponentDescriptor()
    {
        return MaterialComponent::CreateDescriptor();
    }
}
