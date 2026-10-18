/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AuxGeomDrawProcessorShared.h"

namespace JE::Render
{
    namespace Features
    {
        AZ::RHI::CullMode ConvertToRHICullMode(AuxGeomFaceCullMode faceCull)
        {
            switch(faceCull)
            {
                case FaceCull_None: return AZ::RHI::CullMode::None;
                case FaceCull_Front: return AZ::RHI::CullMode::Front;
                case FaceCull_Back: return AZ::RHI::CullMode::Back;
            }
            return AZ::RHI::CullMode::None;
        }

        AZ::RHI::DepthWriteMask ConvertToRHIDepthWriteMask(AuxGeomDepthWriteType depthWrite)
        {
            switch (depthWrite)
            {
            case DepthWrite_On:  return AZ::RHI::DepthWriteMask::All;
            case DepthWrite_Off: return AZ::RHI::DepthWriteMask::Zero;
            }
            AZ_Assert(false, "Invalid AuxGeom DepthWriteType value passed");
            return AZ::RHI::DepthWriteMask::All;
        }

    }
}
