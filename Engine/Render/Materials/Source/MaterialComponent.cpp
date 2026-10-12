/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Materials/MaterialComponent.h>
#include <JE/Render/Materials/MaterialsRegistration.h>

namespace JE
{
    namespace Render
    {
        MaterialComponent::MaterialComponent(const MaterialComponentConfig& config)
            : BaseClass(config)
        {
        }

        void MaterialComponent::Reflect(AZ::ReflectContext* context)
        {
            ReflectMaterialTypes(context);
            BaseClass::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialComponent, BaseClass>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("MaterialComponentTypeId", BehaviorConstant(AZ::Uuid(MaterialComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);
            }
        }
    } // namespace Render
} // namespace JE
