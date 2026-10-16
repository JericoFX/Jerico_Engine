/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Assets/RenderBuilders/Model/ModelExporterContexts.h>
#include <AzCore/Serialization/Utils.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/SceneBuilderDependencyBus.h>

namespace JE::Assets::RenderBuilders
{
    /**
     * Export materials from Scene
     */
    class MaterialAssetBuilderComponent : public AZ::SceneAPI::SceneCore::ExportingComponent
    {

    public:
        AZ_COMPONENT(
            MaterialAssetBuilderComponent,
            "{777BB521-FCFE-4382-B8FD-E1EAF846F5C9}",
            AZ::SceneAPI::SceneCore::ExportingComponent);

        MaterialAssetBuilderComponent();
        ~MaterialAssetBuilderComponent() override = default;

        AZ::SceneAPI::Events::ProcessingResult BuildMaterials(MaterialAssetBuilderContext& context) const;

        // Return the material assetid for material id
        static uint32_t GetMaterialAssetSubId(uint64_t materialUid);

        // Required for ExportingComponent
        static void Reflect(AZ::ReflectContext* context);

    private:

        AZ::SceneAPI::Events::ProcessingResult ConvertMaterials(MaterialAssetBuilderContext& context) const;
        AZ::SceneAPI::Events::ProcessingResult AssignDefaultMaterials(MaterialAssetBuilderContext& context) const;

        AZ::Data::Asset<AZ::RPI::MaterialAsset> GetDefaultMaterialAsset() const;
    };

    /**
     * Report dependencies for the MaterialAssetBuilderComponent
     */
    class MaterialAssetDependenciesComponent
        : public AZ::Component
        , public AZ::SceneAPI::SceneBuilderDependencyBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialAssetDependenciesComponent, "{02163945-D7B4-45D4-9729-4376E1195505}");

        static void Reflect(AZ::ReflectContext* context);

        MaterialAssetDependenciesComponent() = default;
        ~MaterialAssetDependenciesComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

        // SceneAPI::SceneBuilderDependencyBus::Handler overrides...
        void ReportJobDependencies(AZ::SceneAPI::JobDependencyList& jobDependencyList, const char* platformIdentifier) override;
        void AddFingerprintInfo(AZStd::set<AZStd::string>& fingerprintInfo) override;
    };
} // namespace JE::Assets::RenderBuilders
