/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

namespace ImageProcessingAtomEditor
{
    class ImageProcessingEditorRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        /////////////////////////////////////////////////////////////////////////

        //! Open single texture file
        virtual void OpenSourceTextureFile(const AZ::Uuid& textureSourceID) = 0;

        //! Call only after confirming authoring. True creates sourcePath + ".assetinfo" exclusively; false reuses compatible inference or existing settings unchanged.
        //! Existing sidecars are strictly read; malformed or incompatible settings require review and are never modified. Failures return inline diagnostics without opening the texture editor.
        virtual AZ::Outcome<bool, AZStd::string> EnsureTexturePresetIfUnset(
            const AZStd::string& sourcePath, const AZStd::string& presetName) = 0;
    };

    using ImageProcessingEditorRequestBus = AZ::EBus<ImageProcessingEditorRequests>;
}//namespace ImageProcessingAtomEditor

