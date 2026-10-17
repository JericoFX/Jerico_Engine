/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Atom/Feature/LookupTable/LookupTableAsset.h>
#include <JE/Render/RPI/Public/Pass/AttachmentReadback.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

namespace JE::Editor::Render::Scene
{
    //! Write the baked float LUT directly to the existing .3dl and .azasset source formats.
    //! The base must be absolute, with an existing parent directory. Existing outputs are never overwritten.
    AZ::Outcome<AZStd::string, AZStd::string> WriteLutSource(
        const JE::Render::RPI::AttachmentReadback::ReadbackResult& readback, const AZStd::string& absoluteOutputBase);

    //! Validate the native LUT data before the display mapper indexes its RGB triplets.
    AZ::Outcome<void, AZStd::string> ValidateLookupTable(const AZ::Render::LookupTableAsset& lookupTable);
}
