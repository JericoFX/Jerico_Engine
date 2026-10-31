// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <JE/Editor/Tools/Module.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AtomToolsFrameworkSystemComponent.h>
#include <Window/AtomToolsMainWindowSystemComponent.h>
#include <PerformanceMonitor/PerformanceMonitorSystemComponent.h>
#include <PreviewRenderer/PreviewRendererSystemComponent.h>
#include <GraphModel/GraphModelSystemComponent.h>
#include <GraphCanvas/GraphCanvas.h>
#include <GraphCanvas/GraphCanvasModuleRegistration.h>

namespace JE::Editor::Tools
{
    class AtomToolsFrameworkModule final : public AZ::Module
    {
    public:
        AZ_RTTI(AtomToolsFrameworkModule, "{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomToolsFrameworkModule, AZ::SystemAllocator);

        AtomToolsFrameworkModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                AtomToolsFrameworkSystemComponent::CreateDescriptor(),
                AtomToolsMainWindowSystemComponent::CreateDescriptor(),
                PerformanceMonitorSystemComponent::CreateDescriptor(),
                PreviewRendererSystemComponent::CreateDescriptor(),
                GraphModel::GraphModelSystemComponent::CreateDescriptor(),
            });
            GraphCanvas::AppendGraphCanvasDescriptors(m_descriptors);
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return {
                azrtti_typeid<AtomToolsFrameworkSystemComponent>(),
                azrtti_typeid<AtomToolsMainWindowSystemComponent>(),
                azrtti_typeid<PerformanceMonitorSystemComponent>(),
                azrtti_typeid<PreviewRendererSystemComponent>(),
                azrtti_typeid<GraphModel::GraphModelSystemComponent>(),
                azrtti_typeid<GraphCanvas::GraphCanvasSystemComponent>(),
            };
        }
    };

    AZ::Module* CreateModule()
    {
        return aznew AtomToolsFrameworkModule();
    }
}
