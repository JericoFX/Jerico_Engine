/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Instance/Instance.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <JE/Render/RPI/Public/Base.h>

namespace JE::Render
{
    namespace RPI
    {
        class AttachmentImage;
        class RasterPass;
    }
}

namespace LyShine
{
    using AttachmentImages = AZStd::vector<AZ::Data::Instance<JE::Render::RPI::AttachmentImage>>;
    using AttachmentImageAndDependentsPair = AZStd::pair<AZ::Data::Instance<JE::Render::RPI::AttachmentImage>, AttachmentImages>;
    using AttachmentImagesAndDependencies = AZStd::vector<AttachmentImageAndDependentsPair>;
}

class LyShinePassRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = JE::Render::RPI::SceneId;

    //! Called when the number of render targets has changed and the LyShine pass needs to rebuild
    virtual void RebuildRttChildren() = 0;

    //! Returns a render to texture pass based on render target name
    virtual JE::Render::RPI::RasterPass* GetRttPass(const AZStd::string& name) = 0;

    //! Returns the final pass that renders the UI canvas contents
    virtual JE::Render::RPI::RasterPass* GetUiCanvasPass() = 0;
};
using LyShinePassRequestBus = AZ::EBus<LyShinePassRequests>;

class LyShinePassDataRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = JE::Render::RPI::SceneId;

    //! Get a list of render targets that require a render to texture pass, and any
    //! other render targets that are drawn on them
    virtual LyShine::AttachmentImagesAndDependencies GetRenderTargets() = 0;
};
using LyShinePassDataRequestBus = AZ::EBus<LyShinePassDataRequests>;
