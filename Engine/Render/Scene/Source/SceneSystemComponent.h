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
    namespace Scene
    {
        class SceneSystemComponent
            : public AZ::Component
        {
        public:
            AZ_RTTI_WITH_NAME(SceneSystemComponent, "AtomLyIntegrationCommonFeaturesSystemComponent", "{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}", AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(SceneSystemComponent)
            AZ_COMPONENT_BASE(SceneSystemComponent)
            AZ_CLASS_ALLOCATOR(SceneSystemComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

        };
    } // namespace Render
} // namespace AZ
