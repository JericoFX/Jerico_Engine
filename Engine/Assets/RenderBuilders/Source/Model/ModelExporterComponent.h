/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Utils.h>

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ::SceneAPI::Events
{
    class ExportEventContext;
}

namespace JE::Assets::RenderBuilders
{
    /**
     * This is the central component that drive the process of exporting a scene to Model
     * and Material assets. It delegates asset-build duties to other components like
     * ModelAssetBuilderComponent and MaterialAssetBuilderComponent via export events.
     */
    class ModelExporterComponent : public AZ::SceneAPI::SceneCore::ExportingComponent
    {
    public:
        AZ_COMPONENT(
            ModelExporterComponent,
            "{AE42AB62-A4D6-4147-88A0-692549EE5427}",
            AZ::SceneAPI::SceneCore::ExportingComponent);
        static void Reflect(AZ::ReflectContext* context);

        ModelExporterComponent();
        ~ModelExporterComponent() override = default;

        AZ::SceneAPI::Events::ProcessingResult ExportModel(AZ::SceneAPI::Events::ExportEventContext& context) const;

    private:
        struct AssetExportContext
        {
            AssetExportContext() = default;
            AssetExportContext(
                AZStd::string_view relativeFileName,
                AZStd::string_view extension,
                AZ::Uuid sourceUuid,
                AZ::DataStream::StreamType dataStreamType);

            AZStd::string_view m_relativeFileName;
            AZStd::string_view m_extension;
            const AZ::Uuid m_sourceUuid = AZ::Uuid::CreateNull();
            const AZ::DataStream::StreamType m_dataStreamType = AZ::DataStream::ST_BINARY;
        };

        template<class T>
        bool ExportAsset(
            const AZ::Data::Asset<T>& asset,
            const AssetExportContext& assetContext,
            AZ::SceneAPI::Events::ExportEventContext& eventContext,
            const char* assetTypeDebugName) const;
    };
} // namespace JE::Assets::RenderBuilders
