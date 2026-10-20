// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <JE/Editor/Render/Features/FeatureModule.h>
#include <FeatureModuleInternal.h>
#include <EditorCommonSystemComponent.h>
#include <Material/MaterialConverterSystemComponent.h>
#include <DiffuseProbeGrid/EditorComponents/EditorDiffuseProbeGridComponent.h>
#include <DiffuseProbeGrid/EditorComponents/EditorDiffuseGlobalIlluminationComponent.h>
#include <EditorModeFeedback/EditorModeFeedbackSystemComponent.h>

namespace JE::Editor::Render::Features
{
    class FeatureModule final : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(FeatureModule, "CommonModule", "{116699A4-176B-41BE-8D07-77590319687B}", AZ::Module);
        AZ_CLASS_ALLOCATOR(FeatureModule, AZ::SystemAllocator);

        FeatureModule()
        {
            JE::Render::Features::AppendOwnedRuntimeComponentDescriptors(m_descriptors);
            JE::Render::Features::AppendOwnedComponentDescriptor<EditorCommonSystemComponent>(m_descriptors);
            JE::Render::Features::AppendOwnedComponentDescriptor<MaterialConverterSystemComponent>(m_descriptors);
            JE::Render::Features::AppendOwnedComponentDescriptor<EditorDiffuseProbeGridComponent>(m_descriptors);
            JE::Render::Features::AppendOwnedComponentDescriptor<EditorDiffuseGlobalIlluminationComponent>(m_descriptors);
            JE::Render::Features::AppendOwnedComponentDescriptor<EditorModeFeedbackSystemComponent>(m_descriptors);
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            auto required = JE::Render::Features::GetRuntimeRequiredSystemComponents();
            required.push_back(azrtti_typeid<EditorCommonSystemComponent>());
            required.push_back(azrtti_typeid<EditorModeFeedbackSystemComponent>());
            return required;
        }
    };

    AZ::Module* CreateModule()
    {
        return aznew FeatureModule;
    }
}
