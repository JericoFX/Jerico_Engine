/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once
#include <JE/Assets/RenderBuilders/RenderAssetBuildersExports.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>

namespace AZ::SceneAPI::Containers { class Scene; }
namespace AZ::SceneAPI::DataTypes { class IMeshGroup; }

namespace JE::Assets::RenderBuilders::SceneProcessing::Rules
{
    //! Authored identities, not ordinals, paths, slots, AssetIds or geometry hashes.
    struct JE_RENDER_ASSET_BUILDERS_API SurfaceIdentityReference
    {
        AZ_TYPE_INFO(SurfaceIdentityReference, "{B592030B-7CB9-48E1-804E-E438887F119D}");
        static void Reflect(AZ::ReflectContext* context);
        AZ::Uuid m_surfaceId = AZ::Uuid::CreateNull();
        AZ::Uuid m_faceId = AZ::Uuid::CreateNull();
        //! JSON wire v1 omits RegionId for absence; present RegionId is a non-null UUID string.
        //! Binary reflection and the optional C++ API retain their existing representation.
        AZStd::optional<AZ::Uuid> m_regionId;
        bool operator==(const SurfaceIdentityReference& rhs) const
        { return m_surfaceId == rhs.m_surfaceId && m_faceId == rhs.m_faceId && m_regionId == rhs.m_regionId; }
    };

    //! Authoring label in the same rule; runtime membership continues to use only its stable UUID.
    struct JE_RENDER_ASSET_BUILDERS_API SurfaceRegionDefinition
    {
        AZ_TYPE_INFO(SurfaceRegionDefinition, "{5EF07851-DA3F-4516-AB21-316F2ED56F40}");
        static void Reflect(AZ::ReflectContext* context);
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
        AZStd::string m_name;
        bool operator==(const SurfaceRegionDefinition& rhs) const { return m_id == rhs.m_id && m_name == rhs.m_name; }
    };

    //! Guard for the source triangle correspondence, not a public identity or full content revision.
    struct JE_RENDER_ASSET_BUILDERS_API SurfaceTopologyWitness
    {
        AZ_TYPE_INFO(SurfaceTopologyWitness, "{D5EF37AD-85B0-437C-90B7-35FC45E3CFA0}");
        static constexpr AZ::u32 Schema = 1;
        static void Reflect(AZ::ReflectContext* context);
        AZ::u32 m_schema = 0;
        AZ::u32 m_vertexCount = 0;
        AZ::u32 m_faceCount = 0;
        AZStd::array<AZ::u32, 5> m_digest{};
        bool operator==(const SurfaceTopologyWitness& rhs) const
        { return m_schema == rhs.m_schema && m_vertexCount == rhs.m_vertexCount && m_faceCount == rhs.m_faceCount && m_digest == rhs.m_digest; }
        bool operator!=(const SurfaceTopologyWitness& rhs) const { return !(*this == rhs); }
    };

    struct JE_RENDER_ASSET_BUILDERS_API SurfaceIdentityNode
    {
        AZ_TYPE_INFO(SurfaceIdentityNode, "{EEC52FFB-EFBB-48DE-9506-0BE90A54FA5B}");
        static void Reflect(AZ::ReflectContext* context);
        AZStd::string m_nodePath;
        SurfaceTopologyWitness m_topology;
        //! One reference per source triangle; a common authored Face may legitimately repeat.
        AZStd::vector<SurfaceIdentityReference> m_faces;
    };

    //! Persisted inside the existing MeshGroup RuleContainer in .assetinfo. Cook never generates UUIDs.
    class JE_RENDER_ASSET_BUILDERS_API SurfaceIdentityRule final : public AZ::SceneAPI::DataTypes::IRule
    {
    public:
        AZ_RTTI(SurfaceIdentityRule, "{292F5FE8-49D1-416D-A8B0-A2A6293CF4BB}", AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(SurfaceIdentityRule, AZ::SystemAllocator);
        static constexpr AZ::u32 Schema = 1;
        static constexpr const char* SourcePolygonMapKey = "JE.SurfaceIdentity.SourcePolygonMap.v1";
        static void Reflect(AZ::ReflectContext* context);
        static AZStd::optional<SurfaceTopologyWitness> ComputeTopologyWitness(const AZ::SceneAPI::DataTypes::IMeshData& mesh);
        //! Detached preparation for an explicit Scene Settings action only. Existing valid references are preserved.
        //! Source polygons available through IMeshData are triangles; this does not recover authored n-gons.
        static AZ::Outcome<SurfaceIdentityRule, AZStd::string> PrepareForMeshGroup(
            const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IMeshGroup& group);
        static bool MatchesMeshGroup(const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IMeshGroup& group, const SurfaceIdentityRule& rule);
        //! Explicit bulk authoring for an existing SurfaceId; nullopt clears, present null UUID is invalid.
        //! Does not generate identities, alter topology, or commit a document/Undo transaction.
        AZ::Outcome<bool, AZStd::string> SetRegionForSurface(
            const AZ::Uuid& surfaceId, const AZStd::optional<AZ::Uuid>& regionId);
        //! Creation is explicit. Deserialization/reflection never invents UUIDs or labels.
        AZ::Outcome<AZ::Uuid, AZStd::string> CreateRegion(AZStd::string_view name);
        //! Names an existing definition or a legacy referenced UUID, preserving that UUID.
        AZ::Outcome<bool, AZStd::string> SetRegionName(const AZ::Uuid& regionId, AZStd::string_view name);
        bool Validate() const;
        const SurfaceIdentityNode* Resolve(AZStd::string_view path, const AZ::SceneAPI::DataTypes::IMeshData& source) const;
        AZ::u32 m_schema = Schema;
        AZStd::vector<SurfaceIdentityNode> m_nodes;
        AZStd::vector<SurfaceRegionDefinition> m_regions;
    };
}
