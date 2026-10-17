/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/Grid/GridComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void GridComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GridComponentConfig, AZ::ComponentConfig>()
                    ->Version(0)
                    ->Field("gridSize", &GridComponentConfig::m_gridSize)
                    ->Field("axisColor", &GridComponentConfig::m_axisColor)
                    ->Field("primarySpacing", &GridComponentConfig::m_primarySpacing)
                    ->Field("primaryColor", &GridComponentConfig::m_primaryColor)
                    ->Field("secondarySpacing", &GridComponentConfig::m_secondarySpacing)
                    ->Field("secondaryColor", &GridComponentConfig::m_secondaryColor)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
