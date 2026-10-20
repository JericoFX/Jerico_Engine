/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>

namespace JE::Render
{
    namespace Features
    {
        class DiffuseProbeGridSystemComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(DiffuseProbeGridSystemComponent, "{8635A450-FBEC-49E2-A5E5-D8429352530B}");

            DiffuseProbeGridSystemComponent() = default;
            ~DiffuseProbeGridSystemComponent() = default;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        private:
            //! Loads the pass templates mapping file 
            void LoadPassTemplateMappings();

            ////////////////////////////////////////////////////////////////////////
            // Component interface implementation
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            //! Used for loading the pass templates of the hair gem.
            JE::Render::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
        };
    }
}
