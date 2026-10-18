/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Features
    {
        class SkinnedMeshVertexStreamProperties;
        class SkinnedMeshOutputStreamManager;

        class SkinnedMeshSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(SkinnedMeshSystemComponent, "{5B36DCDC-9120-4C12-8594-8D0F8E9A7197}");

            SkinnedMeshSystemComponent();
            ~SkinnedMeshSystemComponent();

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        protected:
        
            ////////////////////////////////////////////////////////////////////////
            // Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            AZStd::unique_ptr<SkinnedMeshVertexStreamProperties> m_vertexStreamProperties;
            AZStd::unique_ptr<SkinnedMeshOutputStreamManager> m_outputStreamManager;
        };
    } // End Render namespace
} // End AZ namespace
