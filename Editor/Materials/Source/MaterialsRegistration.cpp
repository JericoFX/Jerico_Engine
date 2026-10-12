/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <JE/Editor/Materials/MaterialsRegistration.h>
#include <JE/Render/Materials/MaterialsRegistration.h>
#include <AzCore/Module/Module.h>
#include <JE/Editor/Materials/EditorMaterialComponent.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponent.h>

namespace JE::Editor
{
    namespace
    {
        class MaterialsModule final : public AZ::Module
        {
        public:
            AZ_RTTI(MaterialsModule, "{3B036BED-4F24-420F-BB55-4BEB736A17FC}", AZ::Module);
            AZ_CLASS_ALLOCATOR(MaterialsModule, AZ::SystemAllocator);

            MaterialsModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    JE::Render::CreateMaterialComponentDescriptor(),
                    CreateEditorMaterialComponentDescriptor(),
                    CreateEditorMaterialSystemComponentDescriptor(),
                });
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return { azrtti_typeid<EditorMaterialSystemComponent>() };
            }
        };
    }

    AZ::ComponentDescriptor* CreateEditorMaterialComponentDescriptor()
    {
        return EditorMaterialComponent::CreateDescriptor();
    }

    AZ::ComponentDescriptor* CreateEditorMaterialSystemComponentDescriptor()
    {
        return EditorMaterialSystemComponent::CreateDescriptor();
    }

    AZ::Module* CreateMaterialsModule()
    {
        return aznew MaterialsModule;
    }
}
