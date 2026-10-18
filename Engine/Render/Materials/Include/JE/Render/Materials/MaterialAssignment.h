/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Materials/Base.h>

#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/Materials/MaterialAssignmentId.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/unordered_map.h>

namespace JE
{
    namespace Render
    {
        using MaterialPropertyOverrideMap = AZStd::unordered_map<AZ::Name, AZStd::any>;

        struct JE_RENDER_MATERIALS_API MaterialAssignment final
        {
            AZ_RTTI_WITH_NAME(MaterialAssignment, "AZ::Render::MaterialAssignment", "{C66E5214-A24B-4722-B7F0-5991E6F8F163}");
            AZ_CLASS_ALLOCATOR(JE::Render::MaterialAssignment, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            MaterialAssignment() = default;

            MaterialAssignment(const AZ::Data::AssetId& materialAssetId);

            MaterialAssignment(const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& asset);

            MaterialAssignment(const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& asset, const AZ::Data::Instance<JE::Render::RPI::Material>& instance);

            //! Recreates the material instance from the asset if it has been loaded.
            //! If amy property overrides have been specified then a unique instance will be created.
            //! Otherwise an attempt will be made to find or create a shared instance.
            void RebuildInstance();

            //! Release asset and instance references
            void Release();

            //! Return true if contained assets have not been loaded
            bool RequiresLoading() const;

            //! Applies property overrides to material instance
            bool ApplyProperties();

            //! Returns a string composed of the asset path.
            AZStd::string ToString() const;

            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_materialAsset;
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_defaultMaterialAsset;
            AZ::Data::Instance<JE::Render::RPI::Material> m_materialInstance;
            MaterialPropertyOverrideMap m_propertyOverrides;
            JE::Render::RPI::MaterialModelUvOverrideMap m_matModUvOverrides;
            bool m_materialInstancePreCreated = false;
            bool m_materialInstanceMustBeUnique = false;
        };

        using MaterialAssignmentMap = AZStd::unordered_map<MaterialAssignmentId, MaterialAssignment>;
        using MaterialAssignmentLabelMap = AZStd::unordered_map<MaterialAssignmentId, AZStd::string>;
        extern JE_RENDER_MATERIALS_API const MaterialAssignment DefaultMaterialAssignment;
        extern JE_RENDER_MATERIALS_API const MaterialAssignmentId DefaultMaterialAssignmentId;
        extern JE_RENDER_MATERIALS_API const MaterialAssignmentMap DefaultMaterialAssignmentMap;

        //! Utility function for retrieving a material entry from a MaterialAssignmentMap
        JE_RENDER_MATERIALS_API const MaterialAssignment& GetMaterialAssignmentFromMap(const MaterialAssignmentMap& materials, const MaterialAssignmentId& id);

        //! Utility function for retrieving a material entry from a MaterialAssignmentMap, falling back to defaults for a particular asset
        //! or the entire model
        JE_RENDER_MATERIALS_API const MaterialAssignment& GetMaterialAssignmentFromMapWithFallback(
            const MaterialAssignmentMap& materials, const MaterialAssignmentId& id);

        //! Utility function for generating a set of available material assignments in a model
        JE_RENDER_MATERIALS_API MaterialAssignmentMap GetDefaultMaterialMapFromModelAsset(const AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset);

        //! Get material slot labels from a model
        JE_RENDER_MATERIALS_API MaterialAssignmentLabelMap GetMaterialSlotLabelsFromModelAsset(const AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset);

        //! Find an assignment id corresponding to the lod and label substring filters
        JE_RENDER_MATERIALS_API MaterialAssignmentId GetMaterialSlotIdFromModelAsset(
            const AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset,
            const MaterialAssignmentLodIndex lodFilter,
            const AZStd::string& labelFilter);

        //! Special case handling to convert script values to supported types
        JE_RENDER_MATERIALS_API JE::Render::RPI::MaterialPropertyValue ConvertMaterialPropertyValueFromScript(
            const JE::Render::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value);

        //! Converting material assignment map to a mesh feature processor custom material map
        JE_RENDER_MATERIALS_API JE::Render::Features::CustomMaterialMap ConvertToCustomMaterialMap(const JE::Render::MaterialAssignmentMap& materials);
    } // namespace Render
} // namespace JE
