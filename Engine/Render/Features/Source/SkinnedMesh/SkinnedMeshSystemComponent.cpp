/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <SkinnedMesh/SkinnedMeshVertexStreamProperties.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>
#include <MorphTargets/MorphTargetComputePass.h>

#include <JE/Render/RPI/Public/FeatureProcessorFactory.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>

#include <AzCore/Serialization/SerializeContext.h>


namespace JE::Render
{
    namespace Features
    {
        SkinnedMeshSystemComponent::SkinnedMeshSystemComponent() = default;
        SkinnedMeshSystemComponent::~SkinnedMeshSystemComponent() = default;

        void SkinnedMeshSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<SkinnedMeshSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
            
            SkinnedMeshFeatureProcessor::Reflect(context);
        }

        void SkinnedMeshSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SkinnedMeshService"));
        }

        void SkinnedMeshSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SkinnedMeshService"));
        }

        void SkinnedMeshSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void SkinnedMeshSystemComponent::Init()
        {
        }

        void SkinnedMeshSystemComponent::Activate()
        {
            m_vertexStreamProperties = AZStd::make_unique<SkinnedMeshVertexStreamProperties>();
            m_outputStreamManager = AZStd::make_unique<SkinnedMeshOutputStreamManager>();

            JE::Render::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SkinnedMeshFeatureProcessor>();

            auto* passSystem = JE::Render::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");
            passSystem->AddPassCreator(AZ::Name("MorphTargetComputePass"), &MorphTargetComputePass::Create);
            passSystem->AddPassCreator(AZ::Name("SkinnedMeshComputePass"), &SkinnedMeshComputePass::Create);
        }

        void SkinnedMeshSystemComponent::Deactivate()
        {
            JE::Render::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkinnedMeshFeatureProcessor>();
            m_outputStreamManager.reset();
            m_vertexStreamProperties.reset();
        }
    } // End Render namespace
} // End AZ namespace

