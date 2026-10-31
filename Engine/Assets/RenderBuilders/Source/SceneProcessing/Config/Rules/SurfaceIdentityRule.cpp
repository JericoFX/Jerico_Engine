/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <JE/Assets/RenderBuilders/SceneProcessing/Rules/SurfaceIdentityRule.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/limits.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Rules
{
    namespace
    {
        constexpr char TopologyDomain[] = "JE.Surface.SourceTopology";
        constexpr AZ::u64 TopologyHeaderBytes = sizeof(TopologyDomain) - 1 + 12;
        constexpr AZ::u64 TopologyMaxFaces = (AZStd::numeric_limits<AZ::u32>::max() / 8ull - TopologyHeaderBytes) / 12;

        // JSON wire v1: required UUID strings, with RegionId omitted when no region was authored.
        // Keep the binary/reflected optional field intact; AZStd::optional intentionally has no JSON representation.
        class JsonSurfaceIdentityReferenceSerializer final : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonSurfaceIdentityReferenceSerializer, "{4917050A-4FB9-4AB6-9726-53F99C0FA94C}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR(JsonSurfaceIdentityReferenceSerializer, AZ::SystemAllocator);

            OperationFlags GetOperationsFlags() const override { return OperationFlags::ManualDefault; }

            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputType,
                const rapidjson::Value& input, AZ::JsonDeserializerContext& context) override
            {
                namespace JSR = AZ::JsonSerializationResult;
                if (!outputValue || outputType != azrtti_typeid<SurfaceIdentityReference>() || !input.IsObject())
                {
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Invalid,
                        "Surface identity must be an object with explicit SurfaceId and FaceId UUID strings.");
                }
                SurfaceIdentityReference staged;
                const auto loadUuid = [&](const char* name, AZ::Uuid& value) -> JSR::ResultCode
                {
                    AZ::ScopedContextPath path(context, name);
                    const auto field = input.FindMember(name);
                    if (field == input.MemberEnd() || !field->value.IsString())
                    {
                        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Invalid,
                            "An authored identity requires a non-null UUID string; omit RegionId for no region.");
                    }
                    const auto result = ContinueLoading(&value, azrtti_typeid<AZ::Uuid>(), field->value, context);
                    if (result.GetProcessing() != JSR::Processing::Completed || value.IsNull())
                    {
                        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Invalid,
                            "An authored identity UUID must be valid and non-null.");
                    }
                    return result;
                };
                auto result = loadUuid("SurfaceId", staged.m_surfaceId);
                if (result.GetProcessing() != JSR::Processing::Completed) { return context.Report(result, "Invalid SurfaceId."); }
                result = loadUuid("FaceId", staged.m_faceId);
                if (result.GetProcessing() != JSR::Processing::Completed) { return context.Report(result, "Invalid FaceId."); }
                if (input.HasMember("RegionId"))
                {
                    AZ::Uuid region;
                    result = loadUuid("RegionId", region);
                    if (result.GetProcessing() != JSR::Processing::Completed) { return context.Report(result, "Invalid RegionId."); }
                    staged.m_regionId = region;
                }
                *static_cast<SurfaceIdentityReference*>(outputValue) = AZStd::move(staged);
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Loaded authored surface identity.");
            }

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& output, const void* inputValue,
                const void* defaultValue, const AZ::Uuid& inputType, AZ::JsonSerializerContext& context) override
            {
                namespace JSR = AZ::JsonSerializationResult;
                AZ_UNUSED(defaultValue); // References are self-contained, never patches against a default identity.
                if (!inputValue || inputType != azrtti_typeid<SurfaceIdentityReference>())
                {
                    return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Invalid, "Invalid surface identity input.");
                }
                const auto& value = *static_cast<const SurfaceIdentityReference*>(inputValue);
                if (value.m_surfaceId.IsNull() || value.m_faceId.IsNull() || (value.m_regionId && value.m_regionId->IsNull()))
                {
                    return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Invalid, "Cannot store null authored identity UUIDs.");
                }
                rapidjson::Value staged(rapidjson::kObjectType);
                auto result = ContinueStoringToJsonObjectField(staged, "SurfaceId", &value.m_surfaceId, nullptr,
                    azrtti_typeid<AZ::Uuid>(), context);
                result.Combine(ContinueStoringToJsonObjectField(staged, "FaceId", &value.m_faceId, nullptr,
                    azrtti_typeid<AZ::Uuid>(), context));
                if (value.m_regionId)
                {
                    result.Combine(ContinueStoringToJsonObjectField(staged, "RegionId", &*value.m_regionId, nullptr,
                        azrtti_typeid<AZ::Uuid>(), context));
                }
                if (result.GetProcessing() != JSR::Processing::Completed) { return context.Report(result, "Cannot store authored identity."); }
                output = AZStd::move(staged);
                return context.Report(result, "Stored authored surface identity.");
            }
        };
    }
    void SurfaceIdentityReference::Reflect(AZ::ReflectContext* context)
    {
        if (auto* json = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            json->Serializer<JsonSurfaceIdentityReferenceSerializer>()->HandlesType<SurfaceIdentityReference>();
        }
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SurfaceIdentityReference>()->Version(1)
                ->Field("SurfaceId", &SurfaceIdentityReference::m_surfaceId)
                ->Field("FaceId", &SurfaceIdentityReference::m_faceId)
                ->Field("RegionId", &SurfaceIdentityReference::m_regionId);
        }
    }
    void SurfaceTopologyWitness::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SurfaceTopologyWitness>()->Version(1)
                ->Field("Schema", &SurfaceTopologyWitness::m_schema)
                ->Field("VertexCount", &SurfaceTopologyWitness::m_vertexCount)
                ->Field("FaceCount", &SurfaceTopologyWitness::m_faceCount)
                ->Field("Digest", &SurfaceTopologyWitness::m_digest);
        }
    }
    void SurfaceIdentityNode::Reflect(AZ::ReflectContext* context)
    {
        SurfaceIdentityReference::Reflect(context); SurfaceTopologyWitness::Reflect(context);
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SurfaceIdentityNode>()->Version(1)
                ->Field("NodePath", &SurfaceIdentityNode::m_nodePath)
                ->Field("Topology", &SurfaceIdentityNode::m_topology)
                ->Field("Faces", &SurfaceIdentityNode::m_faces);
        }
    }
    void SurfaceRegionDefinition::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SurfaceRegionDefinition>()->Version(1)
                ->Field("Id", &SurfaceRegionDefinition::m_id)->Field("Name", &SurfaceRegionDefinition::m_name);
        }
    }
    void SurfaceIdentityRule::Reflect(AZ::ReflectContext* context)
    {
        SurfaceIdentityNode::Reflect(context);
        SurfaceRegionDefinition::Reflect(context);
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SurfaceIdentityRule, AZ::SceneAPI::DataTypes::IRule>()->Version(2)
                ->Field("Schema", &SurfaceIdentityRule::m_schema)
                ->Field("Nodes", &SurfaceIdentityRule::m_nodes)
                ->Field("Regions", &SurfaceIdentityRule::m_regions);
            if (auto* edit = sc->GetEditContext())
            {
                edit->Class<SurfaceIdentityRule>("Surface Identity", "Persistent authored surface and face references.");
            }
        }
    }

    namespace
    {
        struct AuthoringMesh { AZStd::string m_path; const AZ::SceneAPI::DataTypes::IMeshData* m_mesh; };
        AZStd::vector<AuthoringMesh> GetAuthoringMeshes(const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IMeshGroup& group)
        {
            using Selector = AZ::SceneAPI::Utilities::SceneGraphSelector;
            const auto& graph = scene.GetGraph();
            const auto selected = Selector::GenerateTargetNodes(graph, group.GetSceneNodeSelectionList(), Selector::IsMesh,
                Selector::RemapToOriginalUnoptimizedMesh);
            AZStd::vector<AuthoringMesh> result; AZStd::unordered_set<AZStd::string> paths;
            for (const auto& path : selected)
            {
                const auto node = graph.Find(path);
                if (!node.IsValid()) { return {}; }
                const auto* mesh = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshData*>(graph.GetNodeContent(node).get());
                if (!mesh) { return {}; }
                if (paths.insert(path).second) { result.push_back({path, mesh}); }
            }
            return result;
        }
    }
    bool SurfaceIdentityRule::MatchesMeshGroup(const AZ::SceneAPI::Containers::Scene& scene,
        const AZ::SceneAPI::DataTypes::IMeshGroup& group, const SurfaceIdentityRule& rule)
    {
        const auto meshes = GetAuthoringMeshes(scene, group);
        if (meshes.empty() || meshes.size() != rule.m_nodes.size() || !rule.Validate()) { return false; }
        for (const auto& source : meshes) { if (!rule.Resolve(source.m_path, *source.m_mesh)) { return false; } }
        return true;
    }
    AZ::Outcome<SurfaceIdentityRule, AZStd::string> SurfaceIdentityRule::PrepareForMeshGroup(
        const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IMeshGroup& group)
    {
        AZStd::shared_ptr<const SurfaceIdentityRule> existing;
        const auto& rules = group.GetRuleContainerConst();
        for (size_t i = 0; i != rules.GetRuleCount(); ++i)
        {
            const auto rule = AZStd::rtti_pointer_cast<const SurfaceIdentityRule>(rules.GetRule(i));
            if (rule)
            {
                if (existing) { return AZ::Failure(AZStd::string("Multiple Surface Identity rules require an explicit repair.")); }
                existing = rule;
            }
        }
        if (existing)
        {
            if (!MatchesMeshGroup(scene, group, *existing))
            { return AZ::Failure(AZStd::string("Existing Surface Identity is stale or ambiguous; references were preserved unchanged.")); }
            return AZ::Success(*existing);
        }
        const auto meshes = GetAuthoringMeshes(scene, group);
        if (meshes.empty()) { return AZ::Failure(AZStd::string("Select at least one source mesh before adding Surface Identity.")); }
        SurfaceIdentityRule result;
        // Validate all correspondence before generating IDs or committing any document state.
        for (const auto& source : meshes)
        {
            const auto witness = ComputeTopologyWitness(*source.m_mesh);
            if (!witness) { return AZ::Failure(AZStd::string("Source topology cannot support persistent identity.")); }
            AZStd::unordered_set<AZ::SceneAPI::DataTypes::IMeshData::Face> faces;
            for (AZ::u32 i = 0; i != source.m_mesh->GetFaceCount(); ++i)
            {
                if (!faces.insert(source.m_mesh->GetFaceInfo(i)).second)
                { return AZ::Failure(AZStd::string("Indistinguishable source triangles require authored correspondence; no IDs were generated.")); }
            }
            SurfaceIdentityNode node; node.m_nodePath = source.m_path; node.m_topology = *witness;
            result.m_nodes.push_back(AZStd::move(node));
        }
        for (auto& node : result.m_nodes)
        {
            const AZ::Uuid surface = AZ::Uuid::CreateRandom();
            node.m_faces.reserve(node.m_topology.m_faceCount);
            for (AZ::u32 i = 0; i != node.m_topology.m_faceCount; ++i)
            { node.m_faces.push_back({surface, AZ::Uuid::CreateRandom(), AZStd::nullopt}); }
        }
        if (!result.Validate()) { return AZ::Failure(AZStd::string("Unable to prepare valid authored surface references.")); }
        return AZ::Success(AZStd::move(result));
    }

    AZStd::optional<SurfaceTopologyWitness> SurfaceIdentityRule::ComputeTopologyWitness(const AZ::SceneAPI::DataTypes::IMeshData& mesh)
    {
        // AZ::Sha1's bit counter is u32; reject before traversal rather than wrap the canonical message length.
        const auto faceCount = mesh.GetFaceCount(); const auto vertexCount = mesh.GetVertexCount();
        if (faceCount == 0 || vertexCount == 0 || static_cast<AZ::u64>(faceCount) > TopologyMaxFaces)
        { return AZStd::nullopt; }
        AZ::Sha1 hash; hash.ProcessBytes(reinterpret_cast<const AZStd::byte*>(TopologyDomain), sizeof(TopologyDomain) - 1);
        const auto word = [&hash](AZ::u32 value)
        {
            const AZ::u8 bytes[]{static_cast<AZ::u8>(value), static_cast<AZ::u8>(value >> 8),
                static_cast<AZ::u8>(value >> 16), static_cast<AZ::u8>(value >> 24)};
            hash.ProcessBytes(reinterpret_cast<const AZStd::byte*>(bytes), sizeof(bytes));
        };
        word(SurfaceTopologyWitness::Schema); word(vertexCount); word(faceCount);
        for (AZ::u32 face = 0; face != faceCount; ++face)
        {
            for (AZ::u32 vertex : mesh.GetFaceInfo(face).vertexIndex)
            {
                if (vertex >= vertexCount) { return AZStd::nullopt; }
                word(vertex);
            }
        }
        SurfaceTopologyWitness result; result.m_schema = SurfaceTopologyWitness::Schema;
        result.m_vertexCount = vertexCount; result.m_faceCount = faceCount;
        AZ::u32 digest[5]; hash.GetDigest(digest);
        for (size_t i = 0; i != result.m_digest.size(); ++i) { result.m_digest[i] = digest[i]; }
        return result;
    }
    AZ::Outcome<bool, AZStd::string> SurfaceIdentityRule::SetRegionForSurface(
        const AZ::Uuid& surfaceId, const AZStd::optional<AZ::Uuid>& regionId)
    {
        if (surfaceId.IsNull() || (regionId && regionId->IsNull()) || !Validate())
        {
            return AZ::Failure(AZStd::string("The surface, region UUID or Surface Identity rule is invalid."));
        }
        bool found = false; bool changed = false;
        for (const auto& node : m_nodes)
        {
            for (const auto& face : node.m_faces)
            {
                if (face.m_surfaceId == surfaceId)
                {
                    found = true; changed = changed || face.m_regionId != regionId;
                }
            }
        }
        if (!found) { return AZ::Failure(AZStd::string("The selected SurfaceId no longer exists in this rule.")); }
        if (!changed) { return AZ::Success(false); }
        for (auto& node : m_nodes)
        {
            for (auto& face : node.m_faces)
            {
                if (face.m_surfaceId == surfaceId) { face.m_regionId = regionId; }
            }
        }
        return AZ::Success(true);
    }
    namespace
    {
        AZStd::string RegionLabel(AZStd::string_view name)
        {
            const auto first = name.find_first_not_of(" \t\r\n");
            if (first == AZStd::string_view::npos || name.find('\0') != AZStd::string_view::npos) { return {}; }
            for (const unsigned char c : name) { if (c < 0x20) { return {}; } }
            return AZStd::string(name.substr(first, name.find_last_not_of(" \t\r\n") - first + 1));
        }
    }
    AZ::Outcome<AZ::Uuid, AZStd::string> SurfaceIdentityRule::CreateRegion(AZStd::string_view name)
    {
        const auto label = RegionLabel(name);
        if (label.empty() || !Validate()) { return AZ::Failure(AZStd::string("A current rule and a non-empty region name are required.")); }
        for (const auto& region : m_regions)
        {
            if (region.m_name == label) { return AZ::Failure(AZStd::string("That region name already exists in this mesh group.")); }
        }
        AZStd::unordered_set<AZ::Uuid> used;
        for (const auto& region : m_regions) { used.insert(region.m_id); }
        for (const auto& node : m_nodes)
        {
            for (const auto& face : node.m_faces) { if (face.m_regionId) { used.insert(*face.m_regionId); } }
        }
        AZ::Uuid id;
        do { id = AZ::Uuid::CreateRandom(); } while (id.IsNull() || used.find(id) != used.end());
        m_regions.push_back({id, label});
        return AZ::Success(id);
    }
    AZ::Outcome<bool, AZStd::string> SurfaceIdentityRule::SetRegionName(const AZ::Uuid& id, AZStd::string_view name)
    {
        const auto label = RegionLabel(name);
        if (id.IsNull() || label.empty() || !Validate()) { return AZ::Failure(AZStd::string("The region or name is invalid.")); }
        SurfaceRegionDefinition* existing = nullptr;
        for (auto& region : m_regions)
        {
            if (region.m_id == id) { existing = &region; }
            else if (region.m_name == label) { return AZ::Failure(AZStd::string("That region name already exists in this mesh group.")); }
        }
        if (existing)
        {
            if (existing->m_name == label) { return AZ::Success(false); }
            existing->m_name = label; return AZ::Success(true);
        }
        for (const auto& node : m_nodes)
        {
            for (const auto& face : node.m_faces)
            {
                if (face.m_regionId && *face.m_regionId == id)
                {
                    m_regions.push_back({id, label}); return AZ::Success(true);
                }
            }
        }
        return AZ::Failure(AZStd::string("The region no longer belongs to this rule."));
    }
    bool SurfaceIdentityRule::Validate() const
    {
        if (m_schema != Schema || m_nodes.empty()) { return false; }
        AZStd::unordered_set<AZ::Uuid> regionIds;
        AZStd::unordered_set<AZStd::string> regionNames;
        for (const auto& region : m_regions)
        {
            if (region.m_id.IsNull() || RegionLabel(region.m_name) != region.m_name || region.m_name.empty() ||
                !regionIds.insert(region.m_id).second || !regionNames.insert(region.m_name).second) { return false; }
        }
        AZStd::unordered_set<AZStd::string> paths;
        AZStd::unordered_map<AZ::Uuid, AZ::Uuid> owners;
        for (const auto& node : m_nodes)
        {
            if (node.m_nodePath.empty() || !paths.insert(node.m_nodePath).second ||
                node.m_topology.m_schema != SurfaceTopologyWitness::Schema || node.m_topology.m_vertexCount == 0 ||
                node.m_topology.m_faceCount == 0 || node.m_faces.size() != node.m_topology.m_faceCount ||
                node.m_topology.m_faceCount > TopologyMaxFaces) { return false; }
            for (const auto& face : node.m_faces)
            {
                if (face.m_surfaceId.IsNull() || face.m_faceId.IsNull() || (face.m_regionId && face.m_regionId->IsNull())) { return false; }
                const auto inserted = owners.emplace(face.m_faceId, face.m_surfaceId);
                if (!inserted.second && inserted.first->second != face.m_surfaceId) { return false; }
            }
        }
        return true;
    }
    const SurfaceIdentityNode* SurfaceIdentityRule::Resolve(AZStd::string_view path, const AZ::SceneAPI::DataTypes::IMeshData& source) const
    {
        if (!Validate()) { return nullptr; }
        for (const auto& node : m_nodes)
        {
            if (node.m_nodePath == path)
            {
                const auto witness = ComputeTopologyWitness(source);
                if (!witness || *witness != node.m_topology) { return nullptr; }
                // The connectivity witness cannot tell apart equal source triangles across reimports.
                // Only identical authored references are safe; different Face or Region references need stronger lineage.
                AZStd::unordered_map<AZ::SceneAPI::DataTypes::IMeshData::Face, SurfaceIdentityReference> occurrences;
                for (AZ::u32 index = 0; index != source.GetFaceCount(); ++index)
                {
                    const auto& reference = node.m_faces[index];
                    const auto inserted = occurrences.emplace(source.GetFaceInfo(index), reference);
                    if (!inserted.second && !(inserted.first->second == reference)) { return nullptr; }
                }
                return &node;
            }
        }
        return nullptr;
    }
}
