/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageBasedLights/ImageBasedLightComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace JE::Render
{
    namespace Scene
    {
        ImageBasedLightComponent::ImageBasedLightComponent(const ImageBasedLightComponentConfig& config)
            : BaseClass(config)
        {
        }

        void ImageBasedLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImageBasedLightComponent, BaseClass>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ImageBasedLightComponent>()->RequestBus("ImageBasedLightComponentRequestBus");

                behaviorContext->ConstantProperty("ImageBasedLightComponentTypeId", BehaviorConstant(AZ::Uuid(ImageBasedLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace AZ
