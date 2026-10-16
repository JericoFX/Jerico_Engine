/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/RenderBuilders/Model/ModelExporterContexts.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>

namespace JE::Assets::RenderBuilders
{
    ModelAssetBuilderContext::ModelAssetBuilderContext(
        const AZ::SceneAPI::Containers::Scene& scene,
        const AZ::SceneAPI::DataTypes::IMeshGroup& group,
        const AZ::SceneAPI::CoordinateSystemConverter coordSysConverter,
        const MaterialAssetsByUid& materialsByUid,
        AZ::Data::Asset<AZ::RPI::ModelAsset>& outputModelAsset,
        AZ::Data::Asset<AZ::RPI::SkinMetaAsset>& outputSkinMetaAsset,
        AZ::Data::Asset<AZ::RPI::MorphTargetMetaAsset>& outputMorphTargetMetaAsset)
        : m_scene(scene)
        , m_group(group)
        , m_coordSysConverter(coordSysConverter)
        , m_materialsByUid(materialsByUid)
        , m_outputModelAsset(outputModelAsset)
        , m_outputSkinMetaAsset(outputSkinMetaAsset)
        , m_outputMorphTargetMetaAsset(outputMorphTargetMetaAsset)
    {}

    MaterialAssetBuilderContext::MaterialAssetBuilderContext(
        const AZ::SceneAPI::Containers::Scene& scene,
        MaterialAssetsByUid& outputMaterialsByUid)
        : m_scene(scene)
        , m_outputMaterialsByUid(outputMaterialsByUid)
    {}
} // namespace JE::Assets::RenderBuilders
