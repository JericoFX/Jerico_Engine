/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/vector.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Describes the set of outputs required by a shader, which the render pipeline must bind.
        struct JE_RENDER_INTEGRATION_API ShaderOutputContract
        {
            AZ_TYPE_INFO(ShaderOutputContract, "{CE2A07A4-9911-4F98-AA1E-3391D80E6526}");
            static void Reflect(AZ::ReflectContext* context);

            struct JE_RENDER_INTEGRATION_API ColorAttachmentInfo
            {
                AZ_TYPE_INFO(ColorAttachmentInfo, "{BDFDB9E7-1DF2-441D-BCC5-23E806F845DF}");

                uint32_t m_componentCount = 0; //!< Expected number of texel components. Corresponds to RHI::GetFormatComponentCount(Format).
            };

            AZStd::vector<ColorAttachmentInfo> m_requiredColorAttachments;

            AZ::HashValue64 GetHash() const;
        };
    } // namespace RPI
} // namespace AZ
