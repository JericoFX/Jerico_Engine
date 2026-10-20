// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <JE/Render/Features/FeatureModule.h>
#include <FeatureModuleInternal.h>
#include <CommonSystemComponent.h>
#include <FrameCaptureSystemComponent.h>
#include <ProfilingCaptureSystemComponent.h>
#include <ImGui/ImGuiSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshSystemComponent.h>
#include <CoreLights/CoreLightsSystemComponent.h>
#include <DiffuseProbeGrid/Components/DiffuseProbeGridSystemComponent.h>
#include <DiffuseProbeGrid/Components/DiffuseProbeGridComponent.h>
#include <DiffuseProbeGrid/Components/DiffuseGlobalIlluminationComponent.h>

namespace JE::Render::Features
{
    void AppendOwnedRuntimeComponentDescriptors(AZStd::list<AZ::ComponentDescriptor*>& descriptors)
    {
        AppendOwnedComponentDescriptor<CoreLightsSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<CommonSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<FrameCaptureSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<ProfilingCaptureSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<ImGuiSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<SkinnedMeshSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<DiffuseProbeGridSystemComponent>(descriptors);
        AppendOwnedComponentDescriptor<DiffuseProbeGridComponent>(descriptors);
        AppendOwnedComponentDescriptor<DiffuseGlobalIlluminationComponent>(descriptors);
    }

    AZ::ComponentTypeList GetRuntimeRequiredSystemComponents()
    {
        return {
            azrtti_typeid<CommonSystemComponent>(),
            azrtti_typeid<FrameCaptureSystemComponent>(),
            azrtti_typeid<ProfilingCaptureSystemComponent>(),
            azrtti_typeid<CoreLightsSystemComponent>(),
            azrtti_typeid<ImGuiSystemComponent>(),
            azrtti_typeid<SkinnedMeshSystemComponent>(),
            azrtti_typeid<DiffuseProbeGridSystemComponent>()
        };
    }

    class FeatureModule final : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(FeatureModule, "CommonModule", "{116699A4-176B-41BE-8D07-77590319687B}", AZ::Module);
        AZ_CLASS_ALLOCATOR(FeatureModule, AZ::SystemAllocator);

        FeatureModule()
        {
            AppendOwnedRuntimeComponentDescriptors(m_descriptors);
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return GetRuntimeRequiredSystemComponents();
        }
    };

    AZ::Module* CreateModule()
    {
        return aznew FeatureModule;
    }
}
