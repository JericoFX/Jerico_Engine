/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <BuilderTestFixture.h>
#include <Model/ModelAssetBuilderComponent.h>
#include <JE/Render/RPI/Reflect/Model/MorphTargetDelta.h>
#include <JE/Assets/RenderBuilders/SceneProcessing/Rules/SurfaceIdentityRule.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/CustomPropertyData.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <AzCore/std/any.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace JE::Assets::RenderBuilders
{
    namespace Identity = SceneProcessing::Rules;
    class ModelSurfaceIdentityTests : public UnitTest::BuilderTestFixture
    {
    protected:
        using Rule = Identity::SurfaceIdentityRule;
        using Table = JE::Render::RPI::ModelSurfaceIdentityTable;
        using Views = AZStd::vector<ModelAssetBuilderComponent::ProductMeshView>;
        using CleanupSceneGenerics = void(*)();
        AZStd::array<AZStd::unique_ptr<AZ::DynamicModuleHandle>, 2> m_sceneModules;
        AZStd::array<CleanupSceneGenerics, 2> m_sceneCleanup{};
        void Reflect(AZ::ReflectContext* context) override
        {
            UnitTest::BuilderTestFixture::Reflect(context);
            if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                // Use the same public module reflection entry point as the native SceneProcessing owner.
                constexpr const char* names[]{"SceneCore", "SceneData"};
                constexpr const char* cleanup[]{"CleanUpSceneCoreGenericClassInfo", "CleanUpSceneDataGenericClassInfo"};
                using ReflectScene = void(*)(AZ::SerializeContext*);
                for (size_t i = 0; i != m_sceneModules.size(); ++i)
                {
                    m_sceneModules[i] = AZ::DynamicModuleHandle::Create(names[i]);
                    ASSERT_TRUE(m_sceneModules[i]);
                    ASSERT_TRUE(m_sceneModules[i]->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired));
                    const auto reflect = m_sceneModules[i]->GetFunction<ReflectScene>("Reflect");
                    ASSERT_NE(reflect, nullptr);
                    m_sceneCleanup[i] = m_sceneModules[i]->GetFunction<CleanupSceneGenerics>(cleanup[i]);
                    ASSERT_NE(m_sceneCleanup[i], nullptr);
                    reflect(sc); // Native modules guard types already reflected in this context.
                }
                ASSERT_NE(sc->FindClassData(azrtti_typeid<AZ::SceneAPI::Containers::RuleContainer>()), nullptr);
                ASSERT_NE(sc->FindClassData(azrtti_typeid<AZ::SceneAPI::SceneData::MeshGroup>()), nullptr);
                ASSERT_NE(sc->FindClassData(azrtti_typeid<AZ::SceneAPI::Containers::SceneManifest>()), nullptr);
            }
            Rule::Reflect(context);
        }
        void TearDown() override
        {
            // All reflected objects, SerializeContext and JSON registration die while module code is still resident.
            UnitTest::BuilderTestFixture::TearDown();
            for (size_t i = m_sceneModules.size(); i != 0;)
            {
                --i;
                if (m_sceneCleanup[i]) { m_sceneCleanup[i](); }
                m_sceneModules[i].reset(); // Native loader only uninitializes an initialization token it owns.
            }
        }
        static AZStd::shared_ptr<AZ::SceneData::GraphData::MeshData> Mesh(unsigned int firstMaterial = 7, unsigned int secondMaterial = 9)
        {
            auto mesh = AZStd::make_shared<AZ::SceneData::GraphData::MeshData>();
            mesh->AddPosition(AZ::Vector3(0, 0, 0)); mesh->AddPosition(AZ::Vector3(1, 0, 0));
            mesh->AddPosition(AZ::Vector3(0, 1, 0));
            mesh->AddPosition(AZ::Vector3(1, 1, 0)); mesh->AddPosition(AZ::Vector3(0, -1, 0));
            mesh->AddPosition(AZ::Vector3(-1, 0, 0));
            for (int i = 0; i != 6; ++i) { mesh->AddNormal(AZ::Vector3::CreateAxisZ()); }
            mesh->AddFace(0, 1, 2, firstMaterial); mesh->AddFace(1, 3, 2, secondMaterial);
            mesh->AddFace(0, 4, 1, firstMaterial); mesh->AddFace(2, 5, 0, secondMaterial);
            return mesh;
        }
        static AZStd::shared_ptr<Rule> Authored(const AZ::SceneAPI::DataTypes::IMeshData& mesh)
        {
            auto rule = AZStd::make_shared<Rule>();
            Identity::SurfaceIdentityNode node; node.m_nodePath = "mesh";
            const auto witness = Rule::ComputeTopologyWitness(mesh);
            EXPECT_TRUE(witness); if (!witness) { return rule; } node.m_topology = *witness;
            const AZ::Uuid surface("{10000000-0000-0000-0000-000000000001}");
            for (const char* id : {"{20000000-0000-0000-0000-000000000001}", "{20000000-0000-0000-0000-000000000002}",
                "{20000000-0000-0000-0000-000000000003}", "{20000000-0000-0000-0000-000000000004}"})
            { node.m_faces.push_back({surface, AZ::Uuid(id), AZStd::nullopt}); }
            rule->m_nodes.push_back(AZStd::move(node)); return rule;
        }
        static Table Resolve(const AZ::SceneAPI::Containers::SceneGraph& graph,
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex node, const AZ::SceneAPI::DataTypes::IMeshGroup& group)
        { return ModelAssetBuilderComponent::ResolveSurfaceIdentities(graph, node, group); }
        static ModelAssetBuilderComponent::ProductMeshContent Merge(ModelAssetBuilderComponent& builder,
            const ModelAssetBuilderComponent::ProductMeshContentList& meshes)
        { return builder.MergeMeshList(meshes, ModelAssetBuilderComponent::RemapIndices); }
        static auto Split(ModelAssetBuilderComponent& builder, const ModelAssetBuilderContext& context,
            const ModelAssetBuilderComponent::SourceMeshContentList& meshes)
        {
            AZStd::unordered_map<AZStd::string, uint16_t> joints;
            JE::Render::RPI::MorphTargetMetaAssetCreator morph;
            return builder.SourceMeshListToProductMeshList(context, meshes, joints, morph);
        }
        static auto View(ModelAssetBuilderComponent& builder, const ModelAssetBuilderComponent::ProductMeshContent& mesh)
        { return builder.CreateViewToEntireMesh(mesh); }
        static void CommonBuffers(ModelAssetBuilderComponent& builder, ModelAssetBuilderComponent::ProductMeshContentList& meshes,
            ModelAssetBuilderComponent::ProductMeshContent& output, Views& views)
        { builder.MergeMeshesToCommonBuffers(meshes, output, views); }
        static auto ByMaterial(ModelAssetBuilderComponent& builder, const ModelAssetBuilderComponent::ProductMeshContentList& meshes)
        { return builder.MergeMeshesByMaterialUid(meshes); }
    };

    TEST_F(ModelSurfaceIdentityTests, LegacyVersionOneObjectStreamLoadsWithItsOriginalRegionUuid)
    {
        // Reproduce the actual pre-change writer, not a v2 JSON document with Regions removed.
        // Its RTTI UUID, class version, bases and fields match ui-candidate-r2/before exactly.
        AZ::SerializeContext legacyContext;
        using ReflectScene = void(*)(AZ::SerializeContext*);
        for (const auto& module : m_sceneModules)
        {
            ASSERT_TRUE(module);
            const auto reflect = module->GetFunction<ReflectScene>("Reflect");
            ASSERT_NE(reflect, nullptr); reflect(&legacyContext);
        }
        ASSERT_NE(legacyContext.FindClassData(azrtti_typeid<AZ::SceneAPI::DataTypes::IRule>()), nullptr);
        Identity::SurfaceIdentityNode::Reflect(&legacyContext);
        legacyContext.Class<Rule, AZ::SceneAPI::DataTypes::IRule>()->Version(1)
            ->Field("Schema", &Rule::m_schema)->Field("Nodes", &Rule::m_nodes);
        const auto* oldType = legacyContext.FindClassData(azrtti_typeid<Rule>());
        const auto* newType = m_context->FindClassData(azrtti_typeid<Rule>());
        ASSERT_NE(oldType, nullptr); ASSERT_NE(newType, nullptr);
        ASSERT_EQ(oldType->m_version, 1); ASSERT_EQ(newType->m_version, 2);
        auto original = Authored(*Mesh());
        const AZ::Uuid legacyRegion("{30000000-0000-0000-0000-000000000001}");
        original->m_nodes[0].m_faces[0].m_regionId = legacyRegion;
        const auto source = original->m_nodes[0];
        AZStd::vector<char> oldBytes; AZ::IO::ByteContainerStream<AZStd::vector<char>> oldWriter(&oldBytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(oldWriter, AZ::DataStream::ST_BINARY, original.get(), &legacyContext));
        ASSERT_FALSE(oldBytes.empty());
        AZ::IO::MemoryStream oldReader(static_cast<const void*>(oldBytes.data()), oldBytes.size());
        Rule loaded;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(oldReader, loaded, m_context.get()));
        ASSERT_TRUE(loaded.Validate()); ASSERT_EQ(loaded.m_nodes.size(), 1);
        EXPECT_TRUE(loaded.m_regions.empty()); EXPECT_EQ(loaded.m_schema, 1);
        EXPECT_EQ(loaded.m_nodes[0].m_faces, source.m_faces); EXPECT_EQ(loaded.m_nodes[0].m_topology, source.m_topology);
        ASSERT_TRUE(loaded.SetRegionName(legacyRegion, "Interior").IsSuccess());
        ASSERT_EQ(loaded.m_regions.size(), 1); EXPECT_EQ(loaded.m_regions[0].m_id, legacyRegion);
        AZStd::vector<char> newBytes; AZ::IO::ByteContainerStream<AZStd::vector<char>> newWriter(&newBytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(newWriter, AZ::DataStream::ST_BINARY, &loaded, m_context.get()));
        AZ::IO::MemoryStream newReader(static_cast<const void*>(newBytes.data()), newBytes.size());
        Rule reopened;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(newReader, reopened, m_context.get()));
        ASSERT_TRUE(reopened.Validate()); EXPECT_EQ(reopened.m_regions, loaded.m_regions);
        EXPECT_EQ(reopened.m_nodes[0].m_faces, source.m_faces); EXPECT_EQ(reopened.m_nodes[0].m_topology, source.m_topology);
    }

    TEST_F(ModelSurfaceIdentityTests, NamedRegionCreateAssignClearAndSnapshotsPreserveTheAuthoredUuid)
    {
        auto rule = Authored(*Mesh()); const Rule before = *rule;
        const auto created = rule->CreateRegion("Outer shell");
        ASSERT_TRUE(created.IsSuccess()) << created.GetError().c_str(); const auto id = created.GetValue();
        ASSERT_FALSE(id.IsNull()); ASSERT_EQ(rule->m_regions.size(), 1);
        ASSERT_TRUE(rule->SetRegionForSurface(rule->m_nodes[0].m_faces[0].m_surfaceId, id).IsSuccess());
        const Rule applied = *rule;
        // Exercise the value snapshots used by the existing command; this is not a replacement for Editor Undo.
        *rule = before; EXPECT_TRUE(rule->m_regions.empty()); EXPECT_EQ(rule->m_nodes[0].m_faces, before.m_nodes[0].m_faces);
        *rule = applied; ASSERT_EQ(rule->m_regions.size(), 1); EXPECT_EQ(rule->m_regions[0].m_id, id);
        const auto renamed = rule->SetRegionName(id, "Outer finish");
        ASSERT_TRUE(renamed.IsSuccess()); EXPECT_TRUE(renamed.GetValue());
        EXPECT_EQ(rule->m_regions[0].m_id, id); EXPECT_EQ(rule->m_nodes[0].m_faces, applied.m_nodes[0].m_faces);
        ASSERT_TRUE(rule->SetRegionForSurface(rule->m_nodes[0].m_faces[0].m_surfaceId, AZStd::nullopt).IsSuccess());
        EXPECT_EQ(rule->m_regions[0].m_id, id); EXPECT_EQ(rule->m_regions[0].m_name, "Outer finish");
        EXPECT_EQ(rule->m_nodes[0].m_faces, before.m_nodes[0].m_faces);
    }

    TEST_F(ModelSurfaceIdentityTests, NamedRegionRejectsBlankDuplicateAndUnknownNamesWithoutMutation)
    {
        auto rule = Authored(*Mesh());
        EXPECT_FALSE(rule->CreateRegion("   ").IsSuccess()); EXPECT_TRUE(rule->m_regions.empty());
        const auto created = rule->CreateRegion("Room"); ASSERT_TRUE(created.IsSuccess());
        const auto id = created.GetValue(); const auto regions = rule->m_regions; const auto faces = rule->m_nodes[0].m_faces;
        EXPECT_FALSE(rule->CreateRegion("Room").IsSuccess());
        EXPECT_FALSE(rule->SetRegionName(id, "").IsSuccess());
        EXPECT_FALSE(rule->SetRegionName(AZ::Uuid::CreateNull(), "Other").IsSuccess());
        EXPECT_FALSE(rule->SetRegionName(AZ::Uuid("{30000000-0000-0000-0000-000000000099}"), "Other").IsSuccess());
        EXPECT_EQ(rule->m_regions, regions); EXPECT_EQ(rule->m_nodes[0].m_faces, faces);
        const auto second = rule->CreateRegion("Hall"); ASSERT_TRUE(second.IsSuccess()); EXPECT_NE(id, second.GetValue());
        const auto both = rule->m_regions;
        EXPECT_FALSE(rule->SetRegionName(id, "Hall").IsSuccess()); EXPECT_EQ(rule->m_regions, both);
        rule->m_regions.push_back({id, "Duplicate id"}); EXPECT_FALSE(rule->Validate());
    }

    TEST_F(ModelSurfaceIdentityTests, LegacyRegionWithoutDefinitionReopensAndCanBeNamedWithoutNewIdentity)
    {
        auto rule = Authored(*Mesh()); const AZ::Uuid legacy("{30000000-0000-0000-0000-000000000001}");
        rule->m_nodes[0].m_faces[0].m_regionId = legacy; const auto faces = rule->m_nodes[0].m_faces;
        AZ::JsonSerializerSettings store; store.m_serializeContext = m_context.get(); store.m_registrationContext = m_jsonRegistrationContext.get();
        AZ::JsonDeserializerSettings load; load.m_serializeContext = m_context.get(); load.m_registrationContext = m_jsonRegistrationContext.get();
        rapidjson::Document wire;
        ASSERT_EQ(AZ::JsonSerialization::Store(wire, wire.GetAllocator(), *rule, store).GetProcessing(), AZ::JsonSerializationResult::Processing::Completed);
        wire.RemoveMember("Regions"); // Old .assetinfo had only Schema/Nodes, including optional UUID memberships.
        Rule reopened;
        ASSERT_EQ(AZ::JsonSerialization::Load(reopened, wire, load).GetProcessing(), AZ::JsonSerializationResult::Processing::Completed);
        ASSERT_TRUE(reopened.Validate()); EXPECT_TRUE(reopened.m_regions.empty()); EXPECT_EQ(reopened.m_nodes[0].m_faces, faces);
        ASSERT_TRUE(reopened.SetRegionName(legacy, "Interior").IsSuccess()); ASSERT_EQ(reopened.m_regions.size(), 1);
        EXPECT_EQ(reopened.m_regions[0].m_id, legacy); EXPECT_EQ(reopened.m_nodes[0].m_faces, faces);
        rapidjson::Document named;
        ASSERT_EQ(AZ::JsonSerialization::Store(named, named.GetAllocator(), reopened, store).GetProcessing(), AZ::JsonSerializationResult::Processing::Completed);
        Rule renamedReopen;
        ASSERT_EQ(AZ::JsonSerialization::Load(renamedReopen, named, load).GetProcessing(), AZ::JsonSerializationResult::Processing::Completed);
        EXPECT_EQ(renamedReopen.m_regions, reopened.m_regions); EXPECT_EQ(renamedReopen.m_nodes[0].m_faces, faces);
    }

    TEST_F(ModelSurfaceIdentityTests, NamedRegionsManifestReopenAndClearRetainDefinitionsAndSourceData)
    {
        auto rule = Authored(*Mesh()); const auto source = rule->m_nodes[0];
        const auto created = rule->CreateRegion("Ceiling"); ASSERT_TRUE(created.IsSuccess()); const auto id = created.GetValue();
        auto group = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>(); group->GetRuleContainer().AddRule(rule);
        group->GetSceneNodeSelectionList().AddSelectedNode("mesh");
        AZ::SceneAPI::Containers::SceneManifest manifest; ASSERT_TRUE(manifest.AddEntry(group));
        for (const auto& value : {AZStd::optional<AZ::Uuid>(id), AZStd::optional<AZ::Uuid>()})
        {
            ASSERT_TRUE(rule->SetRegionForSurface(source.m_faces[0].m_surfaceId, value).IsSuccess());
            auto saved = manifest.SaveToJsonDocument(m_context.get(), m_jsonRegistrationContext.get()); ASSERT_TRUE(saved.IsSuccess());
            rapidjson::StringBuffer wire; rapidjson::Writer<rapidjson::StringBuffer> writer(wire); saved.GetValue().Accept(writer);
            AZ::SceneAPI::Containers::SceneManifest reopened;
            ASSERT_TRUE(reopened.LoadFromString(wire.GetString(), m_context.get(), m_jsonRegistrationContext.get()).IsSuccess());
            const auto loadedGroup = AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshGroup>(reopened.GetValue(0));
            ASSERT_TRUE(loadedGroup); const auto loaded = loadedGroup->GetRuleContainerConst().FindFirstByType<Rule>(); ASSERT_TRUE(loaded);
            ASSERT_TRUE(loaded->Validate()); EXPECT_EQ(loaded->m_regions, rule->m_regions);
            ASSERT_EQ(loaded->m_regions.size(), 1); EXPECT_EQ(loaded->m_regions[0].m_id, id); EXPECT_EQ(loaded->m_regions[0].m_name, "Ceiling");
            EXPECT_EQ(loaded->m_nodes[0].m_topology, source.m_topology); EXPECT_EQ(loaded->m_nodes[0].m_nodePath, source.m_nodePath);
            for (size_t i = 0; i != source.m_faces.size(); ++i)
            {
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_surfaceId, source.m_faces[i].m_surfaceId);
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_faceId, source.m_faces[i].m_faceId);
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_regionId, value);
            }
        }
    }

    TEST_F(ModelSurfaceIdentityTests, RegionAssignmentPreservesOtherSurfacesAndSourceIdentity)
    {
        auto rule = Authored(*Mesh());
        auto& node = rule->m_nodes[0];
        const AZ::Uuid otherSurface("{10000000-0000-0000-0000-000000000002}");
        node.m_faces[2].m_surfaceId = otherSurface; node.m_faces[3].m_surfaceId = otherSurface;
        const AZ::Uuid selected = node.m_faces[0].m_surfaceId;
        const auto before = node;
        const AZ::Uuid region("{30000000-0000-0000-0000-000000000001}");
        // Regions may span more than one existing surface; they do not create a surface or face.
        ASSERT_TRUE(rule->SetRegionForSurface(otherSurface, region).IsSuccess());
        const auto changed = rule->SetRegionForSurface(selected, region);
        ASSERT_TRUE(changed.IsSuccess()); EXPECT_TRUE(changed.GetValue());
        EXPECT_EQ(node.m_nodePath, before.m_nodePath); EXPECT_EQ(node.m_topology, before.m_topology);
        for (size_t i = 0; i != node.m_faces.size(); ++i)
        {
            EXPECT_EQ(node.m_faces[i].m_faceId, before.m_faces[i].m_faceId);
            EXPECT_EQ(node.m_faces[i].m_surfaceId, before.m_faces[i].m_surfaceId);
            EXPECT_EQ(node.m_faces[i].m_regionId, AZStd::optional<AZ::Uuid>(region));
        }
        const auto unchanged = rule->SetRegionForSurface(selected, region);
        ASSERT_TRUE(unchanged.IsSuccess()); EXPECT_FALSE(unchanged.GetValue());
        const auto cleared = rule->SetRegionForSurface(selected, AZStd::nullopt);
        ASSERT_TRUE(cleared.IsSuccess()); EXPECT_TRUE(cleared.GetValue());
        EXPECT_FALSE(node.m_faces[0].m_regionId); EXPECT_FALSE(node.m_faces[1].m_regionId);
        EXPECT_EQ(node.m_faces[2].m_regionId, AZStd::optional<AZ::Uuid>(region));
        EXPECT_EQ(node.m_faces[3].m_regionId, AZStd::optional<AZ::Uuid>(region));
    }

    TEST_F(ModelSurfaceIdentityTests, RegionAssignmentRejectsNullUnknownAndInvalidRuleWithoutMutation)
    {
        auto rule = Authored(*Mesh()); const auto before = rule->m_nodes[0];
        const AZ::Uuid region("{30000000-0000-0000-0000-000000000001}");
        const AZ::Uuid unknownSurface("{10000000-0000-0000-0000-000000000099}");
        EXPECT_FALSE(rule->SetRegionForSurface(AZ::Uuid::CreateNull(), region).IsSuccess());
        EXPECT_FALSE(rule->SetRegionForSurface(unknownSurface, region).IsSuccess());
        EXPECT_FALSE(rule->SetRegionForSurface(before.m_faces[0].m_surfaceId, AZ::Uuid::CreateNull()).IsSuccess());
        EXPECT_EQ(rule->m_nodes[0].m_faces, before.m_faces);
        EXPECT_EQ(rule->m_nodes[0].m_topology, before.m_topology);
        rule->m_nodes[0].m_faces.back().m_faceId = AZ::Uuid::CreateNull();
        const auto invalidBefore = rule->m_nodes[0].m_faces;
        EXPECT_FALSE(rule->SetRegionForSurface(before.m_faces[0].m_surfaceId, region).IsSuccess());
        EXPECT_EQ(rule->m_nodes[0].m_faces, invalidBefore);
    }

    TEST_F(ModelSurfaceIdentityTests, RegionAssignAndClearManifestReopenPreservesTopologyAndIds)
    {
        auto rule = Authored(*Mesh()); const auto original = rule->m_nodes[0];
        const AZ::Uuid region("{30000000-0000-0000-0000-000000000001}");
        auto group = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
        group->GetRuleContainer().AddRule(rule); group->GetSceneNodeSelectionList().AddSelectedNode("mesh");
        AZ::SceneAPI::Containers::SceneManifest manifest; ASSERT_TRUE(manifest.AddEntry(group));
        for (const auto& value : {AZStd::optional<AZ::Uuid>(region), AZStd::optional<AZ::Uuid>()})
        {
            ASSERT_TRUE(rule->SetRegionForSurface(original.m_faces[0].m_surfaceId, value).IsSuccess());
            const auto saved = manifest.SaveToJsonDocument(m_context.get(), m_jsonRegistrationContext.get());
            ASSERT_TRUE(saved.IsSuccess()) << saved.GetError().c_str();
            rapidjson::StringBuffer wire; rapidjson::Writer<rapidjson::StringBuffer> writer(wire); saved.GetValue().Accept(writer);
            if (!value) { EXPECT_EQ(AZStd::string_view(wire.GetString()).find("RegionId"), AZStd::string_view::npos); }
            AZ::SceneAPI::Containers::SceneManifest reopened;
            ASSERT_TRUE(reopened.LoadFromString(wire.GetString(), m_context.get(), m_jsonRegistrationContext.get()).IsSuccess());
            const auto loadedGroup = AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshGroup>(reopened.GetValue(0));
            ASSERT_TRUE(loadedGroup); const auto loaded = loadedGroup->GetRuleContainerConst().FindFirstByType<Rule>();
            ASSERT_TRUE(loaded); ASSERT_TRUE(loaded->Validate()); ASSERT_EQ(loaded->m_nodes.size(), 1);
            EXPECT_EQ(loaded->m_nodes[0].m_nodePath, original.m_nodePath);
            EXPECT_EQ(loaded->m_nodes[0].m_topology, original.m_topology);
            ASSERT_EQ(loaded->m_nodes[0].m_faces.size(), original.m_faces.size());
            for (size_t i = 0; i != original.m_faces.size(); ++i)
            {
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_faceId, original.m_faces[i].m_faceId);
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_surfaceId, original.m_faces[i].m_surfaceId);
                EXPECT_EQ(loaded->m_nodes[0].m_faces[i].m_regionId, value);
            }
        }
    }

    TEST_F(ModelSurfaceIdentityTests, RuleRoundtripPreservesExplicitAuthoredIdsAndTopologyWitness)
    {
        auto mesh = Mesh(); auto rule = Authored(*mesh); ASSERT_TRUE(rule->Validate());
        rule->m_nodes[0].m_faces[0].m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000001}");
        ASSERT_TRUE(rule->SetRegionName(*rule->m_nodes[0].m_faces[0].m_regionId, "Existing region").IsSuccess());
        AZStd::vector<char> bytes; AZ::IO::ByteContainerStream<AZStd::vector<char>> out(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(out, AZ::DataStream::ST_BINARY, rule.get(), m_context.get()));
        AZ::IO::MemoryStream in(static_cast<const void*>(bytes.data()), bytes.size()); Rule loaded;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(in, loaded, m_context.get()));
        ASSERT_TRUE(loaded.Validate()); ASSERT_EQ(loaded.m_nodes.size(), 1);
        EXPECT_EQ(loaded.m_nodes[0].m_faces, rule->m_nodes[0].m_faces);
        EXPECT_EQ(loaded.m_nodes[0].m_topology, rule->m_nodes[0].m_topology);
        EXPECT_EQ(loaded.m_regions, rule->m_regions);
        const auto* resolved = loaded.Resolve("mesh", *mesh); ASSERT_NE(resolved, nullptr);
        EXPECT_EQ(resolved->m_faces[0].m_faceId, rule->m_nodes[0].m_faces[0].m_faceId);
    }

    TEST_F(ModelSurfaceIdentityTests, ConnectivityWitnessExcludesShadingMaterialAndPositionButRejectsRetriangulation)
    {
        auto mesh = Mesh(); auto rule = Authored(*mesh); const auto baseline = Rule::ComputeTopologyWitness(*mesh);
        mesh->GetPositions()[0] = AZ::Vector3(7, 8, 9); mesh->GetNormals()[0] = AZ::Vector3::CreateAxisX();
        EXPECT_EQ(Rule::ComputeTopologyWitness(*mesh), baseline); ASSERT_NE(rule->Resolve("mesh", *mesh), nullptr);
        auto changedMaterial = Mesh(5, 6);
        EXPECT_EQ(Rule::ComputeTopologyWitness(*changedMaterial), baseline);
        AZStd::swap(mesh->GetFaces()[0], mesh->GetFaces()[1]);
        EXPECT_NE(Rule::ComputeTopologyWitness(*mesh), baseline); EXPECT_EQ(rule->Resolve("mesh", *mesh), nullptr);
        mesh->GetFaces()[0].vertexIndex[0] = 100;
        EXPECT_FALSE(Rule::ComputeTopologyWitness(*mesh));
    }

    TEST_F(ModelSurfaceIdentityTests, InvalidOrAmbiguousRuleNeverProducesInventedIds)
    {
        auto mesh = Mesh(); const auto valid = Authored(*mesh);
        for (int kind = 0; kind != 7; ++kind)
        {
            Rule invalid = *valid;
            if (kind == 0) { invalid.m_schema = 999; }
            if (kind == 1) { invalid.m_nodes.push_back(invalid.m_nodes[0]); }
            if (kind == 2) { invalid.m_nodes[0].m_faces.pop_back(); }
            if (kind == 3) { invalid.m_nodes[0].m_faces[0].m_faceId = AZ::Uuid::CreateNull(); }
            if (kind == 4) { invalid.m_nodes[0].m_faces[0].m_regionId = AZ::Uuid::CreateNull(); }
            if (kind == 5) { invalid.m_nodes[0].m_topology.m_schema = 99; }
            if (kind == 6)
            {
                invalid.m_nodes[0].m_faces[1].m_faceId = invalid.m_nodes[0].m_faces[0].m_faceId;
                invalid.m_nodes[0].m_faces[1].m_surfaceId = AZ::Uuid("{10000000-0000-0000-0000-000000000002}");
            }
            EXPECT_FALSE(invalid.Validate()) << kind; EXPECT_EQ(invalid.Resolve("mesh", *mesh), nullptr) << kind;
        }
        auto repeated = *valid; repeated.m_nodes[0].m_faces[1] = repeated.m_nodes[0].m_faces[0];
        EXPECT_TRUE(repeated.Validate()); EXPECT_NE(repeated.Resolve("mesh", *mesh), nullptr);
    }

    TEST_F(ModelSurfaceIdentityTests, OriginalAndOptimizedNodesResolveFacesOnlyThroughRecordedCorrespondence)
    {
        AZ::SceneAPI::Containers::Scene scene("identity"); auto& graph = scene.GetGraph(); auto mesh = Mesh();
        const auto original = graph.AddChild(graph.GetRoot(), "mesh", mesh);
        AZ::SceneAPI::SceneData::MeshGroup group; auto rule = Authored(*mesh); group.GetRuleContainer().AddRule(rule);
        const auto direct = Resolve(graph, original, group); ASSERT_EQ(direct.m_schema, 1); ASSERT_EQ(direct.m_entries.size(), 4);
        auto optimizedMesh = Mesh();
        optimizedMesh->GetFaces() = {mesh->GetFaces()[0], mesh->GetFaces()[2], mesh->GetFaces()[1], mesh->GetFaces()[3]};
        const auto optimized = graph.AddChild(graph.GetRoot(), "optimized", optimizedMesh);
        auto properties = AZStd::make_shared<AZ::SceneData::GraphData::CustomPropertyData>();
        properties->GetPropertyMap()[AZ::SceneAPI::Utilities::OriginalUnoptimizedMeshPropertyMapKey] = AZStd::make_any<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>(original);
        properties->GetPropertyMap()[Rule::SourcePolygonMapKey] = AZStd::make_any<AZStd::vector<AZ::u32>>(AZStd::vector<AZ::u32>{0, 2, 1, 3});
        graph.AddChild(optimized, "properties", properties);
        auto mapped = Resolve(graph, optimized, group); ASSERT_EQ(mapped.m_entries.size(), 4);
        const AZStd::array<size_t, 4> order{{0, 2, 1, 3}};
        for (size_t i = 0; i != 4; ++i) { EXPECT_EQ(mapped.m_entries[i], direct.m_entries[order[i]]); }
        properties->GetPropertyMap().erase(Rule::SourcePolygonMapKey);
        EXPECT_EQ(Resolve(graph, optimized, group).m_schema, 0); // No geometric matching fallback for coincident faces.
        properties->GetPropertyMap()[Rule::SourcePolygonMapKey] = AZStd::make_any<AZStd::vector<AZ::u32>>(AZStd::vector<AZ::u32>{0, 0, 1, 3});
        EXPECT_EQ(Resolve(graph, optimized, group).m_schema, 0); // Missing/duplicated provenance is ambiguous.
        properties->GetPropertyMap()[Rule::SourcePolygonMapKey] = AZStd::make_any<AZStd::vector<AZ::u32>>(AZStd::vector<AZ::u32>{0, 2, 1, 99});
        EXPECT_EQ(Resolve(graph, optimized, group).m_schema, 0);
        properties->GetPropertyMap()[Rule::SourcePolygonMapKey] = AZStd::make_any<bool>(true);
        EXPECT_EQ(Resolve(graph, optimized, group).m_schema, 0);
        group.GetRuleContainer().AddRule(Authored(*mesh)); EXPECT_EQ(Resolve(graph, original, group).m_schema, 0);
    }

    TEST_F(ModelSurfaceIdentityTests, IndistinguishableTrianglesRequireIdenticalSurfaceFaceAndRegionReferences)
    {
        auto mesh = Mesh();
        for (auto& face : mesh->GetFaces()) { face = {{0, 1, 2}}; }
        auto rule = Authored(*mesh); ASSERT_TRUE(rule->Validate());
        EXPECT_EQ(rule->Resolve("mesh", *mesh), nullptr); // No ordinal continuity claim across an invisible permutation.
        for (auto& face : rule->m_nodes[0].m_faces) { face.m_faceId = rule->m_nodes[0].m_faces[0].m_faceId; }
        ASSERT_TRUE(rule->Validate()); EXPECT_NE(rule->Resolve("mesh", *mesh), nullptr);
        rule->m_nodes[0].m_faces[1].m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000001}");
        ASSERT_TRUE(rule->Validate()); EXPECT_EQ(rule->Resolve("mesh", *mesh), nullptr); // Same Face does not disambiguate Region membership.
        for (auto& face : rule->m_nodes[0].m_faces) { face.m_regionId = rule->m_nodes[0].m_faces[1].m_regionId; }
        ASSERT_NE(rule->Resolve("mesh", *mesh), nullptr);
        rule->m_nodes[0].m_faces[2].m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000002}");
        EXPECT_EQ(rule->Resolve("mesh", *mesh), nullptr);
        for (auto& face : rule->m_nodes[0].m_faces) { face = rule->m_nodes[0].m_faces[0]; }
        EXPECT_NE(rule->Resolve("mesh", *mesh), nullptr);
    }

    TEST_F(ModelSurfaceIdentityTests, MaterialSplitMergeAndCommonBufferViewsPreserveParallelIdentityRows)
    {
        AZ::SceneAPI::Containers::Scene scene("identity"); auto mesh = Mesh();
        AZ::SceneAPI::SceneData::MeshGroup group; auto rule = Authored(*mesh); group.GetRuleContainer().AddRule(rule);
        const auto node = scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "mesh", mesh);
        ModelAssetBuilderComponent::SourceMeshContent source; source.m_meshData = mesh; source.m_materials.resize(10);
        source.m_materials[7] = 7; source.m_materials[9] = 9; source.m_surfaceIdentities = Resolve(scene.GetGraph(), node, group);
        MaterialAssetsByUid materials; AZ::Data::Asset<JE::Render::RPI::ModelAsset> output;
        AZ::Data::Asset<JE::Render::RPI::SkinMetaAsset> skin; AZ::Data::Asset<JE::Render::RPI::MorphTargetMetaAsset> morph;
        ModelAssetBuilderContext context(scene, group, {}, materials, output, skin, morph);
        ModelAssetBuilderComponent builder; const auto split = Split(builder, context, {source}); ASSERT_TRUE(split.IsSuccess());
        ASSERT_EQ(split.GetValue().size(), 4);
        for (size_t i = 0; i != 4; ++i)
        {
            ASSERT_EQ(split.GetValue()[i].m_surfaceIdentities.m_entries.size(), 1);
            EXPECT_EQ(split.GetValue()[i].m_surfaceIdentities.m_entries[0], source.m_surfaceIdentities.m_entries[i]);
        }
        const auto merged = Merge(builder, split.GetValue()); ASSERT_EQ(merged.m_surfaceIdentities.m_entries.size(), 4);
        EXPECT_EQ(merged.m_surfaceIdentities.m_entries, source.m_surfaceIdentities.m_entries);
        EXPECT_EQ(View(builder, merged).m_surfaceIdentities, merged.m_surfaceIdentities);
    }

    TEST_F(ModelSurfaceIdentityTests, CommonBuffersKeepKnownAndLegacyPerViewTablesSeparate)
    {
        ModelAssetBuilderComponent builder; ModelAssetBuilderComponent::ProductMeshContent known, legacy;
        known.m_indices = legacy.m_indices = {0, 1, 2}; known.m_positions = legacy.m_positions = {0,0,0,1,0,0,0,1,0};
        known.m_materialUid = legacy.m_materialUid = 7; known.m_vertexCount = legacy.m_vertexCount = 3;
        auto rule = Authored(*Mesh()); known.m_surfaceIdentities.m_schema = 1;
        known.m_surfaceIdentities.m_entries.push_back({rule->m_nodes[0].m_faces[0].m_surfaceId, rule->m_nodes[0].m_faces[0].m_faceId, AZStd::nullopt});
        ModelAssetBuilderComponent::ProductMeshContentList meshes{known, legacy};
        const auto byMaterial = ByMaterial(builder, meshes); ASSERT_TRUE(byMaterial.IsSuccess());
        ASSERT_EQ(byMaterial.GetValue().size(), 2); // Identity availability is not a vertex layout error or reason to lose known references.
        ModelAssetBuilderComponent::ProductMeshContent output; Views views;
        CommonBuffers(builder, meshes, output, views); ASSERT_EQ(views.size(), 2);
        EXPECT_EQ(views[0].m_surfaceIdentities, known.m_surfaceIdentities); EXPECT_EQ(views[1].m_surfaceIdentities.m_schema, 0);
        EXPECT_EQ(views[0].m_indexView.m_elementOffset, 0); EXPECT_EQ(views[1].m_indexView.m_elementOffset, 3);
    }


    TEST_F(ModelSurfaceIdentityTests, ReferenceJsonRoundtripDistinguishesAbsentAndPresentRegion)
    {
        using Processing = AZ::JsonSerializationResult::Processing;
        for (bool keepDefaults : {false, true})
        {
            for (bool hasRegion : {false, true})
            {
                auto value = Authored(*Mesh())->m_nodes[0].m_faces[0];
                if (hasRegion) { value.m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000001}"); }
                AZ::JsonSerializerSettings store;
                store.m_serializeContext = m_context.get(); store.m_registrationContext = m_jsonRegistrationContext.get();
                store.m_keepDefaults = keepDefaults;
                rapidjson::Document wire;
                ASSERT_EQ(AZ::JsonSerialization::Store(wire, wire.GetAllocator(), value, store).GetProcessing(), Processing::Completed);
                ASSERT_TRUE(wire.IsObject()); ASSERT_TRUE(wire.HasMember("SurfaceId")); ASSERT_TRUE(wire.HasMember("FaceId"));
                EXPECT_TRUE(wire["SurfaceId"].IsString()); EXPECT_TRUE(wire["FaceId"].IsString());
                EXPECT_EQ(wire.HasMember("RegionId"), hasRegion); EXPECT_EQ(wire.MemberCount(), hasRegion ? 3u : 2u);
                if (hasRegion) { EXPECT_TRUE(wire["RegionId"].IsString()); }
                // Load into an existing present region: an omitted field must clear it, not keep stale authoring.
                auto loaded = value; loaded.m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000002}");
                AZ::JsonDeserializerSettings load;
                load.m_serializeContext = m_context.get(); load.m_registrationContext = m_jsonRegistrationContext.get();
                ASSERT_EQ(AZ::JsonSerialization::Load(loaded, wire, load).GetProcessing(), Processing::Completed);
                EXPECT_EQ(loaded, value);
            }
        }
    }

    TEST_F(ModelSurfaceIdentityTests, ReferenceJsonRejectsMissingInvalidAndNonObjectInputsTransactionally)
    {
        using Processing = AZ::JsonSerializationResult::Processing;
        const Identity::SurfaceIdentityReference before{AZ::Uuid("{40000000-0000-0000-0000-000000000001}"),
            AZ::Uuid("{40000000-0000-0000-0000-000000000002}"), AZ::Uuid("{40000000-0000-0000-0000-000000000003}")};
        AZ::JsonDeserializerSettings load;
        load.m_serializeContext = m_context.get(); load.m_registrationContext = m_jsonRegistrationContext.get();
        load.m_reporting = [](AZStd::string_view, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view) { return result; };
        const auto reject = [&](const rapidjson::Value& wire, const char* label)
        {
            auto value = before;
            EXPECT_NE(AZ::JsonSerialization::Load(value, wire, load).GetProcessing(), Processing::Completed) << label;
            EXPECT_EQ(value, before) << label;
        };
        constexpr const char* valid = R"({"SurfaceId":"{10000000-0000-0000-0000-000000000001}","FaceId":"{20000000-0000-0000-0000-000000000001}","RegionId":"{30000000-0000-0000-0000-000000000001}"})";
        for (const char* field : {"SurfaceId", "FaceId"})
        {
            rapidjson::Document wire; wire.Parse(valid); ASSERT_FALSE(wire.HasParseError()); wire.RemoveMember(field);
            reject(wire, field);
        }
        for (const char* field : {"SurfaceId", "FaceId", "RegionId"})
        {
            for (const char* invalid : {"null", "\"{00000000-0000-0000-0000-000000000000}\"", "\"not-a-uuid\"", "{}", "42"})
            {
                rapidjson::Document wire; wire.Parse(valid); ASSERT_FALSE(wire.HasParseError());
                rapidjson::Document bad; bad.Parse(invalid); ASSERT_FALSE(bad.HasParseError());
                wire[field].CopyFrom(bad, wire.GetAllocator()); reject(wire, field);
            }
        }
        for (const char* invalid : {"null", "[]", "\"not-an-object\"", "42", "{}"})
        {
            rapidjson::Document wire; wire.Parse(invalid); ASSERT_FALSE(wire.HasParseError()); reject(wire, invalid);
        }
    }

    TEST_F(ModelSurfaceIdentityTests, ReferenceJsonCannotStoreNullRequiredOrPresentNullRegionIds)
    {
        using Processing = AZ::JsonSerializationResult::Processing;
        AZ::JsonSerializerSettings store;
        store.m_serializeContext = m_context.get(); store.m_registrationContext = m_jsonRegistrationContext.get();
        store.m_reporting = [](AZStd::string_view, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view) { return result; };
        for (int invalidField = 0; invalidField != 3; ++invalidField)
        {
            auto value = Authored(*Mesh())->m_nodes[0].m_faces[0];
            if (invalidField == 0) { value.m_surfaceId = AZ::Uuid::CreateNull(); }
            if (invalidField == 1) { value.m_faceId = AZ::Uuid::CreateNull(); }
            if (invalidField == 2) { value.m_regionId = AZ::Uuid::CreateNull(); }
            rapidjson::Document wire;
            EXPECT_NE(AZ::JsonSerialization::Store(wire, wire.GetAllocator(), value, store).GetProcessing(), Processing::Completed)
                << invalidField;
        }
    }

    TEST_F(ModelSurfaceIdentityTests, ReferenceJsonSerializerRegistrationUsesOwnerReflectionLifetime)
    {
        AZ::JsonRegistrationContext context;
        const auto type = azrtti_typeid<Identity::SurfaceIdentityReference>();
        EXPECT_EQ(context.GetSerializerForType(type), nullptr);
        Rule::Reflect(&context);
        ASSERT_NE(context.GetSerializerForType(type), nullptr);
        context.EnableRemoveReflection(); Rule::Reflect(&context); context.DisableRemoveReflection();
        EXPECT_EQ(context.GetSerializerForType(type), nullptr);
    }

    TEST_F(ModelSurfaceIdentityTests, MeshGroupManifestJsonReopenRetainsIdsWithoutCookingOrRandomAssignment)
    {
        auto rule = Authored(*Mesh());
        rule->m_nodes[0].m_faces[0].m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000001}");
        ASSERT_FALSE(rule->m_nodes[0].m_faces[1].m_regionId);
        auto group = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
        group->GetRuleContainer().AddRule(rule); group->GetSceneNodeSelectionList().AddSelectedNode("mesh");
        AZ::SceneAPI::Containers::SceneManifest manifest; ASSERT_TRUE(manifest.AddEntry(group));
        auto saved = manifest.SaveToJsonDocument(m_context.get(), m_jsonRegistrationContext.get());
        AZStd::string jsonIssues;
        if (!saved.IsSuccess())
        {
            // Repeat only the failed Store with identical settings, adding a reporter that preserves every ResultCode.
            AZ::JsonSerializerSettings diagnostics;
            diagnostics.m_serializeContext = m_context.get();
            diagnostics.m_registrationContext = m_jsonRegistrationContext.get();
            diagnostics.m_reporting = [&jsonIssues](AZStd::string_view message,
                AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
            {
                result.AppendToString(jsonIssues, path);
                jsonIssues += ": ";
                if (!message.empty()) { jsonIssues.append(message.data(), message.size()); }
                jsonIssues += '\n';
                return result;
            };
            rapidjson::Document diagnosticDocument;
            const auto result = AZ::JsonSerialization::Store(
                diagnosticDocument, diagnosticDocument.GetAllocator(), manifest, diagnostics);
            jsonIssues += "Repeat Store result: "; result.AppendToString(jsonIssues, "");
        }
        ASSERT_TRUE(saved.IsSuccess()) << saved.GetError().c_str() << '\n' << jsonIssues.c_str();
        rapidjson::StringBuffer text; rapidjson::Writer<rapidjson::StringBuffer> writer(text); saved.GetValue().Accept(writer);
        AZ::SceneAPI::Containers::SceneManifest reopened;
        ASSERT_TRUE(reopened.LoadFromString(text.GetString(), m_context.get(), m_jsonRegistrationContext.get()).IsSuccess());
        ASSERT_EQ(reopened.GetEntryCount(), 1);
        const auto loadedGroup = AZStd::rtti_pointer_cast<AZ::SceneAPI::DataTypes::IMeshGroup>(reopened.GetValue(0));
        ASSERT_TRUE(loadedGroup); const auto loaded = loadedGroup->GetRuleContainerConst().FindFirstByType<Rule>();
        ASSERT_TRUE(loaded); ASSERT_TRUE(loaded->Validate());
        EXPECT_EQ(loaded->m_nodes[0].m_faces, rule->m_nodes[0].m_faces);
        EXPECT_EQ(loaded->m_nodes[0].m_topology, rule->m_nodes[0].m_topology);
        EXPECT_NE(loaded->Resolve("mesh", *Mesh()), nullptr);
    }


    TEST_F(ModelSurfaceIdentityTests, ExplicitAuthoringCreatesDetachedIdsOnlyForSelectedSourceMeshes)
    {
        AZ::SceneAPI::Containers::Scene scene("authoring");
        auto meshA = Mesh(); auto meshB = Mesh();
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "sourceA", meshA);
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "sourceB", meshB);
        auto groupA = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
        auto groupB = AZStd::make_shared<AZ::SceneAPI::SceneData::MeshGroup>();
        groupA->SetName("Shaderball"); groupB->SetName("Shaderball"); // A display name is not a document owner.
        groupA->GetSceneNodeSelectionList().AddSelectedNode("sourceA");
        groupA->GetSceneNodeSelectionList().RemoveSelectedNode("sourceB");
        groupB->GetSceneNodeSelectionList().AddSelectedNode("sourceB");
        groupB->GetSceneNodeSelectionList().RemoveSelectedNode("sourceA");
        ASSERT_TRUE(scene.GetManifest().AddEntry(groupA)); ASSERT_TRUE(scene.GetManifest().AddEntry(groupB));
        ASSERT_NE(groupA->GetId(), groupB->GetId());
        const auto prepared = Rule::PrepareForMeshGroup(scene, *groupA);
        ASSERT_TRUE(prepared.IsSuccess()) << prepared.GetError().c_str();
        const auto& rule = prepared.GetValue(); ASSERT_TRUE(rule.Validate()); ASSERT_EQ(rule.m_nodes.size(), 1);
        EXPECT_EQ(rule.m_nodes[0].m_nodePath, "sourceA"); ASSERT_EQ(rule.m_nodes[0].m_faces.size(), 4);
        EXPECT_EQ(rule.m_nodes[0].m_topology.m_vertexCount, 6); EXPECT_EQ(rule.m_nodes[0].m_topology.m_faceCount, 4);
        for (size_t i = 0; i != 4; ++i)
        {
            const auto& reference = rule.m_nodes[0].m_faces[i];
            EXPECT_FALSE(reference.m_surfaceId.IsNull()); EXPECT_FALSE(reference.m_faceId.IsNull()); EXPECT_FALSE(reference.m_regionId);
            EXPECT_EQ(reference.m_surfaceId, rule.m_nodes[0].m_faces[0].m_surfaceId);
            for (size_t j = 0; j != i; ++j) { EXPECT_NE(reference.m_faceId, rule.m_nodes[0].m_faces[j].m_faceId); }
        }
        // Preparation is detached: no rule, manifest, other group, or source geometry is mutated.
        EXPECT_EQ(groupA->GetRuleContainer().GetRuleCount(), 0); EXPECT_EQ(groupB->GetRuleContainer().GetRuleCount(), 0);
        EXPECT_EQ(scene.GetManifest().GetEntryCount(), 2); EXPECT_EQ(meshA->GetFaceCount(), 4);
        EXPECT_TRUE(Rule::MatchesMeshGroup(scene, *groupA, rule)); EXPECT_FALSE(Rule::MatchesMeshGroup(scene, *groupB, rule));
    }

    TEST_F(ModelSurfaceIdentityTests, ExplicitAuthoringUsesDifferentSurfacesForTwoSelectedSourceMeshes)
    {
        AZ::SceneAPI::Containers::Scene scene("authoring");
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "sourceA", Mesh());
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "sourceB", Mesh());
        AZ::SceneAPI::SceneData::MeshGroup group;
        group.GetSceneNodeSelectionList().AddSelectedNode("sourceA");
        group.GetSceneNodeSelectionList().AddSelectedNode("sourceB");
        const auto prepared = Rule::PrepareForMeshGroup(scene, group);
        ASSERT_TRUE(prepared.IsSuccess()) << prepared.GetError().c_str();
        ASSERT_EQ(prepared.GetValue().m_nodes.size(), 2);
        const auto& a = prepared.GetValue().m_nodes[0]; const auto& b = prepared.GetValue().m_nodes[1];
        ASSERT_EQ(a.m_faces.size(), 4); ASSERT_EQ(b.m_faces.size(), 4);
        EXPECT_NE(a.m_nodePath, b.m_nodePath); EXPECT_NE(a.m_faces[0].m_surfaceId, b.m_faces[0].m_surfaceId);
        for (const auto& reference : a.m_faces) { EXPECT_EQ(reference.m_surfaceId, a.m_faces[0].m_surfaceId); }
        for (const auto& reference : b.m_faces) { EXPECT_EQ(reference.m_surfaceId, b.m_faces[0].m_surfaceId); }
        EXPECT_EQ(group.GetRuleContainer().GetRuleCount(), 0);
    }

    TEST_F(ModelSurfaceIdentityTests, ExplicitAuthoringPreservesValidExistingIdsAndOptionalRegions)
    {
        AZ::SceneAPI::Containers::Scene scene("authoring"); auto mesh = Mesh();
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "mesh", mesh);
        AZ::SceneAPI::SceneData::MeshGroup group; group.GetSceneNodeSelectionList().AddSelectedNode("mesh");
        auto existing = Authored(*mesh);
        existing->m_nodes[0].m_faces[0].m_regionId = AZ::Uuid("{30000000-0000-0000-0000-000000000001}");
        group.GetRuleContainer().AddRule(existing);
        const auto prepared = Rule::PrepareForMeshGroup(scene, group);
        ASSERT_TRUE(prepared.IsSuccess()) << prepared.GetError().c_str();
        ASSERT_EQ(prepared.GetValue().m_nodes.size(), 1);
        EXPECT_EQ(prepared.GetValue().m_nodes[0].m_faces, existing->m_nodes[0].m_faces);
        EXPECT_EQ(prepared.GetValue().m_nodes[0].m_topology, existing->m_nodes[0].m_topology);
        EXPECT_EQ(group.GetRuleContainer().GetRule(0).get(), existing.get());
        EXPECT_EQ(group.GetRuleContainer().GetRuleCount(), 1);
    }

    TEST_F(ModelSurfaceIdentityTests, ExplicitAuthoringRejectsStaleOrMultipleRulesInsteadOfRegenerating)
    {
        AZ::SceneAPI::Containers::Scene scene("authoring"); auto mesh = Mesh();
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "mesh", mesh);
        AZ::SceneAPI::SceneData::MeshGroup group; group.GetSceneNodeSelectionList().AddSelectedNode("mesh");
        auto existing = Authored(*mesh); group.GetRuleContainer().AddRule(existing);
        AZStd::swap(mesh->GetFaces()[0], mesh->GetFaces()[1]);
        EXPECT_FALSE(Rule::PrepareForMeshGroup(scene, group).IsSuccess());
        EXPECT_EQ(group.GetRuleContainer().GetRule(0).get(), existing.get());
        AZStd::swap(mesh->GetFaces()[0], mesh->GetFaces()[1]);
        group.GetRuleContainer().AddRule(Authored(*mesh));
        EXPECT_FALSE(Rule::PrepareForMeshGroup(scene, group).IsSuccess()); EXPECT_EQ(group.GetRuleContainer().GetRuleCount(), 2);
    }

    TEST_F(ModelSurfaceIdentityTests, ExplicitAuthoringRejectsIndistinguishableSourcesAndEmptySelectionWithoutMutation)
    {
        AZ::SceneAPI::Containers::Scene scene("authoring"); auto mesh = Mesh();
        mesh->GetFaces()[2] = mesh->GetFaces()[0];
        scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "mesh", mesh);
        AZ::SceneAPI::SceneData::MeshGroup group; group.GetSceneNodeSelectionList().AddSelectedNode("mesh");
        EXPECT_FALSE(Rule::PrepareForMeshGroup(scene, group).IsSuccess());
        EXPECT_EQ(group.GetRuleContainer().GetRuleCount(), 0); EXPECT_EQ(mesh->GetFaceCount(), 4);
        AZ::SceneAPI::Containers::Scene empty("empty");
        EXPECT_FALSE(Rule::PrepareForMeshGroup(empty, group).IsSuccess());
        EXPECT_EQ(group.GetRuleContainer().GetRuleCount(), 0);
    }

    TEST_F(ModelSurfaceIdentityTests, MissingOrStaleSourceRuleKeepsGeometryWithoutPublicIdentity)
    {
        AZ::SceneAPI::Containers::Scene scene("identity"); auto mesh = Mesh();
        const auto node = scene.GetGraph().AddChild(scene.GetGraph().GetRoot(), "mesh", mesh);
        AZ::SceneAPI::SceneData::MeshGroup group;
        EXPECT_EQ(Resolve(scene.GetGraph(), node, group).m_schema, 0);
        auto rule = Authored(*mesh); group.GetRuleContainer().AddRule(rule);
        ASSERT_EQ(Resolve(scene.GetGraph(), node, group).m_schema, 1);
        AZStd::swap(mesh->GetFaces()[0], mesh->GetFaces()[1]);
        EXPECT_EQ(Resolve(scene.GetGraph(), node, group).m_schema, 0);
        EXPECT_EQ(mesh->GetFaceCount(), 4); EXPECT_EQ(mesh->GetVertexCount(), 6);
        EXPECT_EQ(mesh->GetFaceInfo(0).vertexIndex[0], 1); // Rejection never edits/replaces the valid geometry.
    }
}
