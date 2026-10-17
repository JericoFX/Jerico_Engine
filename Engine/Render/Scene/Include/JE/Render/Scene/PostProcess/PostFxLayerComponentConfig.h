/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostFxLayerCategoriesConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class PostFxLayerComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(PostFxLayerComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PostFxLayerComponentConfig, "AZ::Render::PostFxLayerComponentConfig", "{D9D31439-BD33-43AA-B341-4F47C669F843}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string GetPriorityLabel();

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
            void CopySettingsTo(AZ::Render::PostProcessSettingsInterface* settings) const;

            // Returns a sorted list of priority-to-layer-categories
            AZStd::vector<AZStd::pair<int, AZStd::string>> BuildLayerCategories() const;
            int m_layerCategoryValue = AZ::Render::PostFx::DefaultLayerCategoryValue;

            // If entity containing the tags have cameras, PostFx will limit the effect to those specific entities
            AZStd::vector<AZStd::string> m_cameraTags;

            // If entity contains the following tags, PostFx will ignore those entities
            AZStd::vector<AZStd::string> m_exclusionTags;
        };
    }
}
