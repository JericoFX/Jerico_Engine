/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Pass/PassData.h>

namespace JE::Render
{
    namespace Features
    {
        //! Custom data for EsmShadowmapsPass.  Should be specified in the PassRequest.
        struct EsmShadowmapsPassData
            : public JE::Render::RPI::PassData
        {
            using Base = JE::Render::RPI::PassData;
            AZ_RTTI(EsmShadowmapsPassData, "9B2265DF-2234-4DA9-A79F-7D34F6474160", Base);
            AZ_CLASS_ALLOCATOR(EsmShadowmapsPassData, AZ::SystemAllocator);

            EsmShadowmapsPassData() = default;
            virtual ~EsmShadowmapsPassData() = default;

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EsmShadowmapsPassData, Base>()
                        ->Version(0)
                        ->Field("LightType", &EsmShadowmapsPassData::m_lightType)
                        ;
                }
            }

            AZ::Name m_lightType;
        };
    } // namespace Rendr
} // namespace JE::Render
