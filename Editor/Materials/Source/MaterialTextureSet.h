/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace JE::Editor
{
    enum class MaterialTextureRole { BaseColor, Normal, Roughness, Metallic, Occlusion, Emissive, Height, Unknown };
    enum class MaterialTextureChannel { RGB, Red, Green, Blue, Alpha };
    enum class MaterialNormalConvention { Unspecified, OpenGL, DirectX };

    struct MaterialTextureCandidate
    {
        AZStd::string path;
        AZStd::string token;
        MaterialTextureRole role = MaterialTextureRole::Unknown;
        MaterialTextureChannel channel = MaterialTextureChannel::RGB;
        MaterialNormalConvention normalConvention = MaterialNormalConvention::Unspecified;
        bool supported = true;
    };

    struct MaterialTextureSet
    {
        AZStd::string anchorPath;
        AZStd::string directory;
        AZStd::string identity;
        AZStd::string resolution;
        AZStd::string suggestedMaterialPath;
        AZStd::vector<MaterialTextureCandidate> candidates;
        AZStd::vector<size_t> selected;
        AZStd::vector<MaterialTextureRole> missing;
        AZStd::vector<AZStd::string> warnings;
    };

    //! Enumerates real files in the anchor directory; no recursive discovery or metadata downloads.
    JE_EDITOR_MATERIALS_API bool IsMaterialTextureSource(const AZStd::string& path);
    JE_EDITOR_MATERIALS_API AZ::Outcome<MaterialTextureSet, AZStd::string> AnalyzeMaterialTextureSet(const AZStd::string& anchorPath);

    //! Uses actual StandardPBR source/version and relative references. Selection indices are explicit, never first-match.
    JE_EDITOR_MATERIALS_API AZ::Outcome<JE::Render::RPI::MaterialSourceData, AZStd::string> BuildTextureMaterialSource(
        const MaterialTextureSet& textureSet, const AZStd::vector<size_t>& selected, const AZStd::string& materialPath);

    //! Sole exclusive-create writer for both texture-generated sources and the existing materialtype creation action.
    JE_EDITOR_MATERIALS_API AZ::Outcome<void, AZStd::string> WriteMaterialSourceFileNew(
        const JE::Render::RPI::MaterialSourceData& source, const AZStd::string& materialPath);
    JE_EDITOR_MATERIALS_API AZ::Outcome<void, AZStd::string> CreateMaterialSourceFile(
        const AZStd::string& materialTypePath, const AZStd::string& materialPath);

    //! Opens an existing generated material only if its persisted description identifies this exact set; never overwrites it.
    JE_EDITOR_MATERIALS_API bool IsMaterialForTextureSet(const MaterialTextureSet& textureSet, const AZStd::string& materialPath);
}
