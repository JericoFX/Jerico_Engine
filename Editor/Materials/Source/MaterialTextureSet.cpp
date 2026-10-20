/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialTextureSet.h"

#include <JE/Editor/Render/RPI/Edit/Common/JsonReportingHelper.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/algorithm.h>

namespace JE::Editor
{
    namespace
    {
        using Role = MaterialTextureRole;
        using Channel = MaterialTextureChannel;
        using Convention = MaterialNormalConvention;

        AZStd::string Lower(AZStd::string text)
        {
            for (char& character : text)
            {
                if (character >= 'A' && character <= 'Z')
                {
                    character += 'a' - 'A';
                }
            }
            return text;
        }

        bool IsImage(const AZ::IO::Path& path)
        {
            const auto extension = path.Extension().Native();
            const auto lower = Lower(AZStd::string(extension.data(), extension.size()));
            for (const char* supported : {".png", ".jpg", ".jpeg", ".tif", ".tiff", ".tga", ".bmp", ".exr", ".dds"})
            {
                if (lower == supported)
                {
                    return true;
                }
            }
            return false;
        }

        bool IsResolution(const AZStd::string& token)
        {
            if (token.size() < 2 || token.back() != 'k')
            {
                return false;
            }
            return AZStd::all_of(token.begin(), token.end() - 1, [](char c) { return c >= '0' && c <= '9'; });
        }

        struct ParsedName
        {
            AZStd::string identity;
            AZStd::string resolution;
            AZStd::string token;
            Role role = Role::Unknown;
            Convention convention = Convention::Unspecified;
            bool packed = false;
            bool packedHeight = false;
        };

        ParsedName ParseName(const AZ::IO::Path& path)
        {
            struct Token { AZStd::string value; size_t end; };
            const auto stemView = path.Stem().Native();
            const AZStd::string stem(stemView.data(), stemView.size());
            AZStd::vector<Token> tokens;
            size_t offset = 0;
            while (offset < stem.size())
            {
                if (stem[offset] == '_' || stem[offset] == '-' || stem[offset] == ' ')
                {
                    ++offset;
                    continue;
                }
                const size_t begin = offset;
                while (offset < stem.size() && stem[offset] != '_' && stem[offset] != '-' && stem[offset] != ' ')
                {
                    ++offset;
                }
                tokens.push_back({Lower(stem.substr(begin, offset - begin)), offset});
            }
            ParsedName parsed;
            size_t end = tokens.size();
            if (end && IsResolution(tokens[end - 1].value))
            {
                parsed.resolution = tokens[--end].value;
            }
            struct Suffix { const char* token; Role role; Convention convention = Convention::Unspecified; };
            const Suffix suffixes[] = {
                {"nor_gl", Role::Normal, Convention::OpenGL}, {"nor_dx", Role::Normal, Convention::DirectX},
                {"normal_gl", Role::Normal, Convention::OpenGL}, {"normal_dx", Role::Normal, Convention::DirectX},
                {"norgl", Role::Normal, Convention::OpenGL}, {"nordx", Role::Normal, Convention::DirectX},
                {"normalgl", Role::Normal, Convention::OpenGL}, {"normaldx", Role::Normal, Convention::DirectX},
                {"base_color", Role::BaseColor}, {"ambient_occlusion", Role::Occlusion},
                {"basecolor", Role::BaseColor}, {"albedo", Role::BaseColor}, {"diffuse", Role::BaseColor},
                {"diff", Role::BaseColor}, {"color", Role::BaseColor}, {"col", Role::BaseColor},
                {"roughness", Role::Roughness}, {"rough", Role::Roughness}, {"rou", Role::Roughness},
                {"metallic", Role::Metallic}, {"metalness", Role::Metallic}, {"metal", Role::Metallic},
                {"ambientocclusion", Role::Occlusion}, {"occlusion", Role::Occlusion}, {"ao", Role::Occlusion}, {"occ", Role::Occlusion},
                {"normalmap", Role::Normal}, {"normals", Role::Normal}, {"normal", Role::Normal},
                {"nmap", Role::Normal}, {"norm", Role::Normal}, {"nor", Role::Normal},
                {"emissive", Role::Emissive}, {"emission", Role::Emissive}, {"emit", Role::Emissive},
                {"displacement", Role::Height}, {"heightmap", Role::Height}, {"height", Role::Height}, {"displ", Role::Height}, {"disp", Role::Height},
                {"arm", Role::Occlusion}, {"orm", Role::Occlusion}, {"ordp", Role::Occlusion},
                {"glossiness", Role::Unknown}, {"gloss", Role::Unknown}, {"specular", Role::Unknown}
            };
            for (const auto& suffix : suffixes)
            {
                size_t consumed = 0;
                if (end >= 2 && tokens[end - 2].value + "_" + tokens[end - 1].value == suffix.token)
                {
                    consumed = 2;
                }
                else if (end && tokens[end - 1].value == suffix.token)
                {
                    consumed = 1;
                }
                if (consumed)
                {
                    end -= consumed;
                    parsed.token = suffix.token;
                    parsed.role = suffix.role;
                    parsed.convention = suffix.convention;
                    parsed.packed = parsed.token == "arm" || parsed.token == "orm" || parsed.token == "ordp";
                    parsed.packedHeight = parsed.token == "ordp";
                    break;
                }
            }
            if (end && IsResolution(tokens[end - 1].value))
            {
                if (!parsed.resolution.empty())
                {
                    return {}; // More than one resolution token is not a safe set identity.
                }
                parsed.resolution = tokens[--end].value;
            }
            if (end)
            {
                parsed.identity = stem.substr(0, tokens[end - 1].end);
            }
            return parsed;
        }

        AZStd::string ProvenanceTag(const MaterialTextureSet& set)
        {
            return "JE.MaterialTextureSet/1 " + AZ::IO::Path(set.directory).LexicallyNormal().AsPosix() + " | " +
                Lower(set.identity) + " | " + Lower(set.resolution);
        }

        const char* RoleName(Role role)
        {
            switch (role)
            {
            case Role::BaseColor: return "Base color";
            case Role::Normal: return "Normal";
            case Role::Roughness: return "Roughness";
            case Role::Metallic: return "Metallic";
            case Role::Occlusion: return "Occlusion";
            case Role::Emissive: return "Emission";
            case Role::Height: return "Height";
            default: return "Unknown";
            }
        }

        const char* ChannelName(Channel channel)
        {
            switch (channel)
            {
            case Channel::Red: return "Red";
            case Channel::Green: return "Green";
            case Channel::Blue: return "Blue";
            case Channel::Alpha: return "Alpha";
            default: return "RGB";
            }
        }

        AZ::Outcome<JE::Render::RPI::MaterialSourceData, AZStd::string> BlankSource(
            const AZStd::string& typePath, const AZStd::string& target)
        {
            const AZStd::string source = JE::Editor::Tools::GetPathWithoutAlias(typePath);
            if (!AZ::IO::PathView(source).IsAbsolute() || !source.ends_with(".materialtype") ||
                !AZ::IO::PathView(target).IsAbsolute() || !target.ends_with(".material"))
            {
                return AZ::Failure(AZStd::string("Select an absolute .materialtype source and a .material destination."));
            }
            if (AZ::IO::SystemFile::Exists(target.c_str()))
            {
                return AZ::Failure(AZStd::string("The material destination already exists."));
            }
            auto type = JE::Render::RPI::MaterialUtils::LoadMaterialTypeSourceData(source);
            if (!type.IsSuccess())
            {
                return AZ::Failure(AZStd::string("Failed to load the selected material type."));
            }
            JE::Render::RPI::MaterialSourceData data;
            data.m_materialTypeVersion = type.GetValue().m_version;
            data.m_materialType = JE::Editor::Tools::GetPathToExteralReference(target, source);
            data.m_parentMaterial.clear();
            return AZ::Success(AZStd::move(data));
        }

        AZ::Outcome<JE::Render::RPI::MaterialSourceData, AZStd::string> StandardPbrSource(const AZStd::string& target)
        {
            // This authoring path only supports the bundled StandardPBR. Full type/functor validation remains with its builder.
            const AZStd::string source = JE::Editor::Tools::GetPathWithoutAlias(
                "@engroot@/Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype");
            if (!AZ::IO::PathView(source).IsAbsolute() || !AZ::IO::PathView(target).IsAbsolute() || !target.ends_with(".material"))
            {
                return AZ::Failure(AZStd::string("Select an absolute .material destination and a valid engine source root."));
            }
            auto json = AZ::JsonSerializationUtils::ReadJsonFile(source);
            if (!json.IsSuccess())
            {
                return AZ::Failure(json.GetError());
            }
            const auto& document = json.GetValue();
            if (!document.IsObject() || !document.HasMember("version") || !document["version"].IsUint() ||
                document["version"].GetUint() == 0)
            {
                return AZ::Failure(AZStd::string("The bundled StandardPBR source has an invalid or missing version."));
            }
            JE::Render::RPI::MaterialSourceData data;
            data.m_materialTypeVersion = document["version"].GetUint();
            data.m_materialType = JE::Editor::Tools::GetPathToExteralReference(target, source);
            data.m_parentMaterial.clear();
            return AZ::Success(AZStd::move(data));
        }
    }

    bool IsMaterialTextureSource(const AZStd::string& sourcePath)
    {
        const AZ::IO::Path path = AZ::IO::Path(JE::Editor::Tools::GetPathWithoutAlias(sourcePath)).LexicallyNormal();
        if (!path.IsAbsolute() || !IsImage(path))
        {
            return false;
        }
        bool exists = false;
        AZ::IO::SystemFile::FindFiles(path.c_str(), [&exists](const char*, bool isFile)
        {
            exists = isFile;
            return false;
        });
        return exists;
    }

    AZ::Outcome<MaterialTextureSet, AZStd::string> AnalyzeMaterialTextureSet(const AZStd::string& anchorPath)
    {
        const AZ::IO::Path anchor = AZ::IO::Path(JE::Editor::Tools::GetPathWithoutAlias(anchorPath)).LexicallyNormal();
        if (!IsMaterialTextureSource(anchor.Native()))
        {
            return AZ::Failure(AZStd::string("Select an existing local texture source."));
        }
        const AZ::IO::Path directory(anchor.ParentPath());
        const auto directoryName = directory.Filename().Native();
        const auto folderName = Lower(AZStd::string(directoryName.data(), directoryName.size()));
        if (folderName == "thumbs" || folderName == "thumbnails" || folderName == "previews")
        {
            return AZ::Failure(AZStd::string("Preview and thumbnail directories are not a texture set."));
        }
        const auto parsedAnchor = ParseName(anchor);
        if (parsedAnchor.identity.empty())
        {
            return AZ::Failure(AZStd::string("The texture name does not identify a unique set and resolution."));
        }
        MaterialTextureSet set;
        set.anchorPath = anchor.Native();
        set.directory = directory.Native();
        set.identity = parsedAnchor.identity;
        set.resolution = parsedAnchor.resolution;
        set.suggestedMaterialPath = (directory / (set.identity + (set.resolution.empty() ? "" : "_" + set.resolution) + ".material")).Native();
        const AZ::IO::Path filter = directory / "*";
        AZ::IO::SystemFile::FindFiles(filter.c_str(), [&](const char* name, bool isFile)
        {
            const AZ::IO::Path path = (AZ::IO::PathView(name).IsAbsolute() ? AZ::IO::Path(name) : directory / name).LexicallyNormal();
            if (!isFile || !IsImage(path) || !AZ::IO::SystemFile::Exists(path.c_str()))
            {
                return true;
            }
            const auto parsed = ParseName(path);
            if (Lower(parsed.identity) != Lower(set.identity) || parsed.resolution != set.resolution)
            {
                return true;
            }
            auto add = [&](Role role, Channel channel, bool supported)
            {
                set.candidates.push_back({path.Native(), parsed.token, role, channel, parsed.convention, supported});
            };
            if (parsed.packed)
            {
                add(Role::Occlusion, Channel::Red, true);
                add(Role::Roughness, Channel::Green, true);
                add(parsed.packedHeight ? Role::Height : Role::Metallic, Channel::Blue, !parsed.packedHeight);
            }
            else
            {
                const bool scalar = parsed.role == Role::Roughness || parsed.role == Role::Metallic ||
                    parsed.role == Role::Occlusion || parsed.role == Role::Height;
                add(parsed.role, scalar ? Channel::Red : Channel::RGB, parsed.role != Role::Unknown);
            }
            return true;
        });
        AZStd::sort(set.candidates.begin(), set.candidates.end(), [](const auto& a, const auto& b)
        {
            return a.path == b.path ? a.role < b.role : a.path < b.path;
        });
        for (Role role : {Role::BaseColor, Role::Normal, Role::Roughness, Role::Metallic, Role::Occlusion, Role::Emissive, Role::Height})
        {
            AZStd::vector<size_t> matches;
            for (size_t i = 0; i < set.candidates.size(); ++i)
            {
                if (set.candidates[i].role == role && set.candidates[i].supported)
                {
                    matches.push_back(i);
                }
            }
            if (matches.size() == 1)
            {
                set.selected.push_back(matches.front());
            }
            else if (matches.empty())
            {
                set.missing.push_back(role);
            }
            else
            {
                set.warnings.push_back(AZStd::string::format("%s has multiple candidates; select one explicitly.", RoleName(role)));
            }
        }
        for (const auto& candidate : set.candidates)
        {
            if (candidate.role == Role::Normal)
            {
                set.warnings.push_back(candidate.normalConvention == Convention::Unspecified ?
                    "Normal convention is unspecified; StandardPBR's existing flip setting is preserved." :
                    "Normal convention was inferred from its filename. Flip Y follows the documented StandardPBR convention; verify the source.");
                set.warnings.push_back("Verify the image processing Normal preset; recognizing a texture name does not change its cook settings.");
            }
            if (!candidate.supported)
            {
                set.warnings.push_back(candidate.role == Role::Height ?
                    "ORDp Blue is height, not metallic. This channel is not connected; its source is preserved." :
                    "An unrecognized texture is preserved as a candidate without an automatic assignment.");
            }
            else if (candidate.role == Role::Height)
            {
                set.warnings.push_back("Height is linked with displacement disabled; review scale before enabling it.");
            }
        }
        return AZ::Success(AZStd::move(set));
    }

    AZ::Outcome<JE::Render::RPI::MaterialSourceData, AZStd::string> BuildTextureMaterialSource(
        const MaterialTextureSet& set, const AZStd::vector<size_t>& selected, const AZStd::string& materialPath)
    {
        auto result = StandardPbrSource(materialPath);
        if (!result.IsSuccess())
        {
            return result;
        }
        auto& data = result.GetValue();
        data.m_description = "Created from a local texture set. Unassigned candidates are not silently connected.\n" + ProvenanceTag(set) + "\n";
        for (const auto& candidate : set.candidates)
        {
            const char* convention = candidate.normalConvention == Convention::OpenGL ? "OpenGL" :
                candidate.normalConvention == Convention::DirectX ? "DirectX" : "unspecified";
            data.m_description += AZStd::string::format("Candidate: %s | %s | %s | %s | %s%s\n", candidate.path.c_str(),
                candidate.token.c_str(), RoleName(candidate.role), ChannelName(candidate.channel), convention,
                candidate.supported ? "" : " | NOT CONNECTED (unsupported channel or role)");
        }
        for (const auto& warning : set.warnings)
        {
            data.m_description += "Review: " + warning + "\n";
        }
        AZStd::vector<Role> assigned;
        for (size_t index : selected)
        {
            if (index >= set.candidates.size())
            {
                return AZ::Failure(AZStd::string("A selected texture candidate no longer exists."));
            }
            const auto& candidate = set.candidates[index];
            const AZ::IO::Path path = AZ::IO::Path(candidate.path).LexicallyNormal();
            if (!candidate.supported || candidate.role == Role::Unknown ||
                (candidate.role == Role::Height && candidate.channel != Channel::Red) ||
                !path.IsAbsolute() || !IsImage(path) || !AZ::IO::SystemFile::Exists(path.c_str()) ||
                AZ::IO::Path(path.ParentPath()).Compare(AZ::IO::Path(set.directory).LexicallyNormal()) != 0)
            {
                return AZ::Failure(AZStd::string("A selected texture is missing, outside the set, or has an unsupported role/channel."));
            }
            const auto parsed = ParseName(path);
            if (Lower(parsed.identity) != Lower(set.identity) || parsed.resolution != set.resolution ||
                AZStd::find(assigned.begin(), assigned.end(), candidate.role) != assigned.end())
            {
                return AZ::Failure(AZStd::string("Select at most one texture per role from the same set and resolution."));
            }
            assigned.push_back(candidate.role);
            const char* property = nullptr;
            const char* channelProperty = nullptr;
            switch (candidate.role)
            {
            case Role::BaseColor: property = "baseColor.textureMap"; break;
            case Role::Normal: property = "normal.textureMap"; break;
            case Role::Roughness: property = "roughness.textureMap"; channelProperty = "roughness.textureMapChannel"; break;
            case Role::Metallic: property = "metallic.textureMap"; channelProperty = "metallic.textureMapChannel"; break;
            case Role::Occlusion: property = "occlusion.diffuseTextureMap"; channelProperty = "occlusion.diffuseTextureMapChannel"; break;
            case Role::Emissive: property = "emissive.textureMap"; break;
            case Role::Height: property = "parallax.textureMap"; break;
            default: break;
            }
            if (channelProperty && candidate.channel == Channel::RGB)
            {
                return AZ::Failure(AZStd::string("A scalar texture requires an explicit RGBA channel."));
            }
            data.SetPropertyValue(AZ::Name(property), JE::Editor::Tools::GetPathToExteralReference(materialPath, candidate.path));
            if (channelProperty)
            {
                data.SetPropertyValue(AZ::Name(channelProperty), AZStd::string(ChannelName(candidate.channel)));
            }
            if (candidate.role == Role::Normal && candidate.normalConvention != Convention::Unspecified)
            {
                // StandardPBR's documented authoring convention: OpenGL normal maps use Flip Y; DirectX maps do not.
                data.SetPropertyValue(AZ::Name("normal.flipY"), candidate.normalConvention == Convention::OpenGL);
            }
            if (candidate.role == Role::Height)
            {
                data.SetPropertyValue(AZ::Name("parallax.useTexture"), false);
            }
            if (candidate.role == Role::Emissive)
            {
                data.SetPropertyValue(AZ::Name("emissive.enable"), true);
            }
        }
        return result;
    }

    AZ::Outcome<void, AZStd::string> WriteMaterialSourceFileNew(
        const JE::Render::RPI::MaterialSourceData& materialData, const AZStd::string& materialPath)
    {
        if (!AZ::IO::PathView(materialPath).IsAbsolute() || !materialPath.ends_with(".material"))
        {
            return AZ::Failure(AZStd::string("Select an absolute .material destination."));
        }
        if (AZ::IO::SystemFile::Exists(materialPath.c_str()))
        {
            return AZ::Failure(AZStd::string("The material destination already exists."));
        }
        rapidjson::Document document;
        document.SetObject();
        AZ::JsonSerializerSettings settings;
        JE::Render::RPI::JsonReportingHelper reporting;
        reporting.Attach(settings);
        AZ::JsonSerialization::Store(document, document.GetAllocator(), materialData, settings);
        if (reporting.ErrorsReported())
        {
            return AZ::Failure(AZStd::string("Failed to serialize the material source."));
        }
        AZStd::string contents;
        auto serialized = AZ::JsonSerializationUtils::WriteJsonString(document, contents);
        if (!serialized.IsSuccess())
        {
            return serialized;
        }
        AZ::IO::SystemFile output;
        if (!output.Open(materialPath.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_NEW))
        {
            return AZ::Failure(AZStd::string("Failed to create the material destination. Existing files are not overwritten."));
        }
        const auto written = output.Write(contents.data(), contents.size());
        output.Close();
        if (written != contents.size())
        {
            AZ::IO::SystemFile::Delete(materialPath.c_str());
            return AZ::Failure(AZStd::string("Failed to write the complete material source."));
        }
        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> CreateMaterialSourceFile(const AZStd::string& materialTypePath, const AZStd::string& materialPath)
    {
        auto source = BlankSource(materialTypePath, materialPath);
        if (!source.IsSuccess())
        {
            return AZ::Failure(source.GetError());
        }
        return WriteMaterialSourceFileNew(source.GetValue(), materialPath);
    }

    bool IsMaterialForTextureSet(const MaterialTextureSet& set, const AZStd::string& materialPath)
    {
        if (!AZ::IO::SystemFile::Exists(materialPath.c_str()))
        {
            return false;
        }
        // A manual destination may not even be JSON. Collision checks must return inline, without a document-load diagnostic.
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(materialPath);
        if (!json.IsSuccess() || !json.GetValue().IsObject() || !json.GetValue().HasMember("description") ||
            !json.GetValue()["description"].IsString())
        {
            return false;
        }
        const auto& description = json.GetValue()["description"];
        return AZStd::string(description.GetString(), description.GetStringLength()).find("\n" + ProvenanceTag(set) + "\n") !=
            AZStd::string::npos;
    }
}
