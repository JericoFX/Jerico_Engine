/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/FeatureProcessorFactory.h>
#include <Components/DiffuseProbeGridSystemComponent.h>
#include <Components/DiffuseProbeGridComponentController.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseGlobalIlluminationFeatureProcessor.h>
#include <Render/DiffuseProbeGridBlendDistancePass.h>
#include <Render/DiffuseProbeGridBlendIrradiancePass.h>
#include <Render/DiffuseProbeGridBorderUpdatePass.h>
#include <Render/DiffuseProbeGridClassificationPass.h>
#include <Render/DiffuseProbeGridDownsamplePass.h>
#include <Render/DiffuseProbeGridPreparePass.h>
#include <Render/DiffuseProbeGridQueryPass.h>
#include <Render/DiffuseProbeGridQueryFullscreenPass.h>
#include <Render/DiffuseProbeGridQueryFullscreenPassData.h>
#include <Render/DiffuseProbeGridRayTracingPass.h>
#include <Render/DiffuseProbeGridRelocationPass.h>
#include <Render/DiffuseProbeGridRenderPass.h>
#include <Render/DiffuseProbeGridVisualizationAccelerationStructurePass.h>
#include <Render/DiffuseProbeGridVisualizationCompositePass.h>
#include <Render/DiffuseProbeGridVisualizationPreparePass.h>
#include <Render/DiffuseProbeGridVisualizationRayTracingPass.h>

namespace JE::Render
{
    namespace Features
    {
        void DiffuseProbeGridSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<DiffuseProbeGridSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }

            DiffuseProbeGridFeatureProcessor::Reflect(context);
            DiffuseGlobalIlluminationFeatureProcessor::Reflect(context);
            DiffuseProbeGridQueryFullscreenPassData::Reflect(context);
        }

        void DiffuseProbeGridSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("DiffuseProbeGrid"));
            provided.push_back(AZ_CRC_CE("DiffuseGlobalIllumination"));
        }

        void DiffuseProbeGridSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("DiffuseProbeGrid"));
            incompatible.push_back(AZ_CRC_CE("DiffuseGlobalIllumination"));
        }

        void DiffuseProbeGridSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void DiffuseProbeGridSystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = JE::Render::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            const char* passTemplatesFile = "Passes/DiffuseProbeGridTemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }

        void DiffuseProbeGridSystemComponent::Activate()
        {
            // register feature processors
            JE::Render::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            JE::Render::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DiffuseGlobalIlluminationFeatureProcessor>();

            auto* passSystem = JE::Render::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "PassSystemInterface is null");

            // setup handler for pass templates mappings
            m_loadTemplatesHandler = JE::Render::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            passSystem->ConnectEvent(m_loadTemplatesHandler);

            // load the DiffuseProbeGrid pass classes
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridPreparePass"), &DiffuseProbeGridPreparePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridRayTracingPass"), &DiffuseProbeGridRayTracingPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridBlendIrradiancePass"), &DiffuseProbeGridBlendIrradiancePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridBlendDistancePass"), &DiffuseProbeGridBlendDistancePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridBorderUpdatePass"), &DiffuseProbeGridBorderUpdatePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridRelocationPass"), &DiffuseProbeGridRelocationPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridClassificationPass"), &DiffuseProbeGridClassificationPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridDownsamplePass"), &DiffuseProbeGridDownsamplePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridRenderPass"), &DiffuseProbeGridRenderPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridVisualizationPreparePass"), &DiffuseProbeGridVisualizationPreparePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridVisualizationAccelerationStructurePass"), &DiffuseProbeGridVisualizationAccelerationStructurePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridVisualizationRayTracingPass"), &DiffuseProbeGridVisualizationRayTracingPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridVisualizationCompositePass"), &DiffuseProbeGridVisualizationCompositePass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridQueryPass"), &DiffuseProbeGridQueryPass::Create);
            passSystem->AddPassCreator(AZ::Name("DiffuseProbeGridQueryFullscreenPass"), &DiffuseProbeGridQueryFullscreenPass::Create);
        }

        void DiffuseProbeGridSystemComponent::Deactivate()
        {
            JE::Render::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            JE::Render::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DiffuseGlobalIlluminationFeatureProcessor>();

            m_loadTemplatesHandler.Disconnect();
        }
    }
}
