/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "JEWhiteBoxSurfaceMaterials.h"
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/limits.h>
#include <cmath>

namespace JE::BSP
{
    namespace
    {
        namespace Api = JE::Geometry::Api;
        using Assignment = JE::Render::MaterialAssignment;
        using AssignmentId = JE::Render::MaterialAssignmentId;
        constexpr uint32_t InvalidSlot = AZStd::numeric_limits<uint32_t>::max();

        bool ValidBindings(const SurfaceMaterialBindings& bindings)
        {
            AZStd::unordered_set<AZ::Uuid> keys;
            for (const auto& binding : bindings)
            {
                if (binding.m_surfaceId.IsNull() || binding.m_materialSlot == InvalidSlot ||
                    !keys.insert(binding.m_surfaceId).second) { return false; }
            }
            return true;
        }

        bool HasAssignment(const Assignment& assignment)
        {
            return assignment.m_materialAsset.GetId().IsValid() || !assignment.m_propertyOverrides.empty() ||
                !assignment.m_matModUvOverrides.empty() || assignment.m_materialInstancePreCreated;
        }

        Assignment ResolveAssignment(const JE::Render::MaterialAssignmentMap& materials, uint32_t slot,
            const JE::Geometry::WhiteBoxMaterial& legacy, bool* explicitAssignment = nullptr)
        {
            if (explicitAssignment) { *explicitAssignment = false; }
            // Resolve serialized authoring data even while native material instances are loading.
            for (const auto& id : {AssignmentId::CreateFromLodAndStableId(0, slot),
                AssignmentId::CreateFromStableIdOnly(slot), AssignmentId::CreateDefault()})
            {
                const auto it = materials.find(id);
                // A present slot reset also has meaning: Atom resolves it against the model default asset.
                if (it != materials.end() && (!id.IsDefault() || HasAssignment(it->second)))
                {
                    if (explicitAssignment) { *explicitAssignment = true; }
                    return it->second;
                }
            }
            Assignment fallback;
            fallback.m_propertyOverrides[AZ::Name("baseColor.color")] = AZ::Color(legacy.m_tint);
            fallback.m_propertyOverrides[AZ::Name("baseColor.useTexture")] = legacy.m_useTexture;
            return fallback;
        }

        bool EqualAssignments(const Assignment& lhs, const Assignment& rhs)
        {
            if (lhs.m_materialAsset.GetId() != rhs.m_materialAsset.GetId() ||
                lhs.m_matModUvOverrides != rhs.m_matModUvOverrides ||
                lhs.m_propertyOverrides.size() != rhs.m_propertyOverrides.size()) { return false; }
            // A pre-created instance cannot be reconstructed from serialized asset/override data.
            if (lhs.m_materialInstancePreCreated || rhs.m_materialInstancePreCreated) { return false; }
            for (const auto& [name, value] : lhs.m_propertyOverrides)
            {
                const auto it = rhs.m_propertyOverrides.find(name);
                if (it == rhs.m_propertyOverrides.end() || value.type() != it->second.type()) { return false; }
                if (value.is<AZ::Name>())
                {
                    if (AZStd::any_cast<AZ::Name>(value) != AZStd::any_cast<AZ::Name>(it->second)) { return false; }
                    continue;
                }
                const auto a = AZ::RPI::MaterialPropertyValue::FromAny(value);
                const auto b = AZ::RPI::MaterialPropertyValue::FromAny(it->second);
                // Unsupported values are conflicts, never silently treated as equal.
                if (!a.IsValid() || !b.IsValid() || a != b) { return false; }
            }
            return true;
        }

        bool SelectedFaces(const JE::Geometry::WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons, Api::FaceHandles& output)
        {
            if (polygons.empty()) { return false; }
            const auto validFaces = Api::MeshFaceHandles(mesh);
            const auto validPolygons = Api::MeshPolygonHandles(mesh);
            for (const auto& polygon : polygons)
            {
                if (AZStd::find(validPolygons.begin(), validPolygons.end(), polygon) == validPolygons.end()) { return false; }
                for (const auto face : polygon.m_faceHandles)
                {
                    if (AZStd::find(validFaces.begin(), validFaces.end(), face) == validFaces.end()) { return false; }
                    if (AZStd::find(output.begin(), output.end(), face) == output.end()) { output.push_back(face); }
                }
            }
            return !output.empty();
        }
    }

    bool MergeSurfaceMaterials(
        const JE::Geometry::WhiteBoxMesh& target, const SurfaceMaterialBindings& targetBindings,
        const JE::Render::MaterialAssignmentMap& targetMaterials, const JE::Geometry::WhiteBoxMaterial& targetLegacy,
        const JE::Geometry::WhiteBoxMesh& source, const SurfaceMaterialBindings& sourceBindings,
        const JE::Render::MaterialAssignmentMap& sourceMaterials, const JE::Geometry::WhiteBoxMaterial& sourceLegacy,
        bool targetHasMaterialOwner, SurfaceMaterialMerge& output, AZStd::string& error)
    {
        if (!ValidBindings(targetBindings) || !ValidBindings(sourceBindings))
        {
            error = "Invalid or duplicate surface-to-material bindings; the current mesh is preserved.";
            return false;
        }
        SurfaceMaterialMerge candidate{targetBindings, targetMaterials};
        AZStd::map<uint32_t, Assignment> materialSlots;
        AZStd::map<uint32_t, bool> explicitSlots;
        materialSlots[0] = ResolveAssignment(targetMaterials, 0, targetLegacy, &explicitSlots[0]);
        AZStd::unordered_map<AZ::Uuid, Assignment> surfaces;
        for (const auto face : Api::MeshFaceHandles(target))
        {
            const auto id = Api::FaceSurfaceId(target, face);
            if (id.IsNull()) { error = "Missing destination surface UUID; the current mesh is preserved."; return false; }
            const auto slot = FindSurfaceMaterialSlot(targetBindings, id);
            materialSlots[slot] = ResolveAssignment(targetMaterials, slot, targetLegacy, &explicitSlots[slot]);
            surfaces[id] = materialSlots[slot];
            if (AZStd::none_of(candidate.m_bindings.begin(), candidate.m_bindings.end(), [&](const auto& b){ return b.m_surfaceId == id; }))
            {
                candidate.m_bindings.push_back({id, slot});
            }
        }
        // Reserve scene slots even when their original geometry is temporarily absent (Undo/live CSG).
        for (const auto& binding : targetBindings)
        {
            materialSlots[binding.m_materialSlot] = ResolveAssignment(targetMaterials, binding.m_materialSlot, targetLegacy, &explicitSlots[binding.m_materialSlot]);
            surfaces[binding.m_surfaceId] = materialSlots[binding.m_materialSlot];
        }
        for (const auto& entry : targetMaterials)
        {
            const auto& id = entry.first;
            if (id.m_materialSlotStableId != InvalidSlot)
            {
                materialSlots[id.m_materialSlotStableId] = ResolveAssignment(targetMaterials, id.m_materialSlotStableId, targetLegacy,
                    &explicitSlots[id.m_materialSlotStableId]);
            }
        }
        AZStd::unordered_map<uint32_t, uint32_t> sourceSlotRemap;
        AZStd::unordered_set<AZ::Uuid> visited;
        for (const auto face : Api::MeshFaceHandles(source))
        {
            const auto id = Api::FaceSurfaceId(source, face);
            if (id.IsNull()) { error = "Missing source surface UUID; the current mesh is preserved."; return false; }
            if (!visited.insert(id).second) { continue; }
            const auto sourceSlot = FindSurfaceMaterialSlot(sourceBindings, id);
            bool sourceExplicit = false;
            const auto assignment = ResolveAssignment(sourceMaterials, sourceSlot, sourceLegacy, &sourceExplicit);
            const auto inherited = surfaces.find(id);
            if (inherited != surfaces.end() && !EqualAssignments(inherited->second, assignment))
            {
                error = "The same surface UUID has incompatible material assets, properties or UV overrides; the current mesh is preserved.";
                return false;
            }
            if (sourceExplicit && !targetHasMaterialOwner)
            {
                error = "Add the Atom Material component to the destination to preserve the source material assignments; the current mesh is preserved.";
                return false;
            }
            if (inherited != surfaces.end())
            {
                const auto inheritedSlot = FindSurfaceMaterialSlot(candidate.m_bindings, id);
                if (sourceExplicit && !explicitSlots[inheritedSlot])
                {
                    candidate.m_materials[AssignmentId::CreateFromStableIdOnly(inheritedSlot)] = assignment;
                    explicitSlots[inheritedSlot] = true;
                }
                sourceSlotRemap.try_emplace(sourceSlot, inheritedSlot);
                continue;
            }
            const auto mapped = sourceSlotRemap.find(sourceSlot);
            auto compatible = AZStd::find_if(materialSlots.begin(), materialSlots.end(), [&](const auto& entry)
            {
                return explicitSlots[entry.first] == sourceExplicit && EqualAssignments(entry.second, assignment);
            });
            uint32_t slot = 0;
            if (mapped != sourceSlotRemap.end()) { slot = mapped->second; }
            else if (compatible != materialSlots.end()) { slot = compatible->first; }
            else
            {
                if (!targetHasMaterialOwner)
                {
                    error = "Add the Atom Material component to the destination to preserve the source material assignments; the current mesh is preserved.";
                    return false;
                }
                if (assignment.m_materialInstancePreCreated)
                {
                    error = "The source has a pre-created material instance without a persistable assignment; the current mesh is preserved.";
                    return false;
                }
                const auto last = materialSlots.rbegin()->first;
                if (last >= InvalidSlot - 1) { error = "Material slot capacity exhausted; the current mesh is preserved."; return false; }
                slot = last + 1;
                materialSlots[slot] = assignment;
                explicitSlots[slot] = true;
                candidate.m_materials[AssignmentId::CreateFromStableIdOnly(slot)] = assignment;
            }
            candidate.m_bindings.push_back({id, slot});
            sourceSlotRemap[sourceSlot] = slot;
            surfaces[id] = assignment;
        }
        output = AZStd::move(candidate);
        error.clear();
        return true;
    }

    bool MaterialAssignmentsEquivalent(const JE::Render::MaterialAssignmentMap& lhs,
        const JE::Render::MaterialAssignmentMap& rhs)
    {
        if (lhs.size() != rhs.size()) { return false; }
        for (const auto& [id, assignment] : lhs)
        {
            const auto it = rhs.find(id);
            if (it == rhs.end() || !EqualAssignments(assignment, it->second)) { return false; }
        }
        return true;
    }
    bool AssignPolygonMaterial(const JE::Geometry::WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons,
        uint32_t slot, SurfaceMaterialBindings& bindings)
    {
        Api::FaceHandles faces;
        if (slot == InvalidSlot || !ValidBindings(bindings) || !SelectedFaces(mesh, polygons, faces)) { return false; }
        auto candidate = bindings;
        for (const auto face : faces)
        {
            const auto id = Api::FaceSurfaceId(mesh, face);
            if (id.IsNull()) { return false; }
            const auto it = AZStd::find_if(candidate.begin(), candidate.end(), [&](const auto& binding){ return binding.m_surfaceId == id; });
            if (it == candidate.end()) { candidate.push_back({id, slot}); }
            else { it->m_materialSlot = slot; }
        }
        bindings = AZStd::move(candidate);
        return true;
    }

    bool PreparePolygonMaterialAssignment(const JE::Geometry::WhiteBoxMesh& mesh,
        const Api::PolygonHandles& polygons, const SurfaceMaterialBindings& bindings,
        const JE::Render::MaterialAssignmentMap& materials, const AZ::Data::Asset<AZ::RPI::MaterialAsset>& material,
        const AZStd::unordered_set<AZ::Name>& modelUvNames, bool hasMaterialOwner,
        SurfaceMaterialMerge& output, AZStd::string& error)
    {
        if (!hasMaterialOwner)
        {
            error = "Add the Atom Material component to this entity before assigning face materials.";
            return false;
        }
        if (!material.IsReady() || !material->GetMaterialTypeAsset().IsReady() || !material->GetMaterialPropertiesLayout())
        {
            error = "The selected Atom material and its material type must finish loading before assignment.";
            return false;
        }
        Api::FaceHandles faces;
        if (!ValidBindings(bindings) || !SelectedFaces(mesh, polygons, faces))
        {
            error = "Select a current polygon in Default component mode; invalid surface bindings cannot be replaced.";
            return false;
        }

        SurfaceMaterialMerge candidate{bindings, materials};
        AZStd::map<uint32_t, Assignment> materialSlots;
        const auto rememberSlot = [&](uint32_t slot)
        {
            auto effective = ResolveAssignment(materials, slot, {});
            if (!effective.m_materialAsset.GetId().IsValid())
            {
                effective.m_materialAsset = effective.m_defaultMaterialAsset;
            }
            materialSlots[slot] = AZStd::move(effective);
        };
        rememberSlot(0);
        for (const auto& binding : bindings) { rememberSlot(binding.m_materialSlot); }
        for (const auto& entry : materials)
        {
            if (entry.first.m_materialSlotStableId != InvalidSlot) { rememberSlot(entry.first.m_materialSlotStableId); }
        }
        uint32_t lastSlot = materialSlots.rbegin()->first;
        AZStd::unordered_map<uint32_t, uint32_t> sourceSlotRemap;
        AZStd::unordered_set<AZ::Uuid> visited;
        const auto* layout = material->GetMaterialPropertiesLayout();
        const auto uvNames = material->GetMaterialTypeAsset()->GetUvNameMap();
        for (const auto face : faces)
        {
            const auto id = Api::FaceSurfaceId(mesh, face);
            if (id.IsNull()) { error = "The selected surface is missing its complete UUID."; return false; }
            if (!visited.insert(id).second) { continue; }
            const auto oldSlot = FindSurfaceMaterialSlot(bindings, id);
            bool explicitAssignment = false;
            auto assignment = ResolveAssignment(materials, oldSlot, {}, &explicitAssignment);
            // An explicit material choice replaces the legacy Tint/UseTexture fallback. It does not
            // manufacture those properties as overrides on an unrelated material type.
            if (!explicitAssignment) { assignment = {}; }
            if (assignment.m_materialInstancePreCreated)
            {
                error = "A pre-created material instance cannot be preserved as a scene assignment.";
                return false;
            }
            for (const auto& [name, value] : assignment.m_propertyOverrides)
            {
                const auto index = layout->FindPropertyIndex(name);
                if (index.IsNull())
                {
                    error = AZStd::string::format("The selected material has no property '%s'; existing overrides are preserved.", name.GetCStr());
                    return false;
                }
                const auto* descriptor = layout->GetPropertyDescriptor(index);
                AZ::RPI::MaterialPropertyValue property;
                if (descriptor->GetDataType() == AZ::RPI::MaterialPropertyDataType::Enum)
                {
                    // Native MaterialComponent accepts enum names as well as UInt values. Do not
                    // coerce arbitrary numeric types (or accept an out-of-range enum index).
                    if (value.is<AZ::Name>() || value.is<AZStd::string>())
                    {
                        property = JE::Render::ConvertMaterialPropertyValueFromScript(descriptor, value);
                    }
                    else if (value.is<uint32_t>())
                    {
                        property = AZ::RPI::MaterialPropertyValue::FromAny(value);
                        const auto oldMaterial = assignment.m_materialAsset.GetId().IsValid() ?
                            assignment.m_materialAsset : assignment.m_defaultMaterialAsset;
                        const auto* oldLayout = oldMaterial.IsReady() ? oldMaterial->GetMaterialPropertiesLayout() : nullptr;
                        const auto oldIndex = oldLayout ? oldLayout->FindPropertyIndex(name) : AZ::RPI::MaterialPropertyIndex{};
                        const auto* oldDescriptor = oldLayout && !oldIndex.IsNull() ? oldLayout->GetPropertyDescriptor(oldIndex) : nullptr;
                        if (!oldDescriptor || oldDescriptor->GetDataType() != AZ::RPI::MaterialPropertyDataType::Enum ||
                            oldDescriptor->GetEnumName(property.GetValue<uint32_t>()) != descriptor->GetEnumName(property.GetValue<uint32_t>()))
                        {
                            error = AZStd::string::format("Numeric enum override '%s' cannot retain its meaning in the selected material.", name.GetCStr());
                            return false;
                        }
                    }
                    if (!property.IsValid() || property.GetTypeId() != azrtti_typeid<uint32_t>() ||
                        descriptor->GetEnumName(property.GetValue<uint32_t>()).IsEmpty())
                    {
                        error = AZStd::string::format("Enum override '%s' is incompatible with the selected material.", name.GetCStr());
                        return false;
                    }
                }
                else
                {
                    // Do not round a double (or reinterpret a numeric type) to fit another material.
                    if (value.is<double>() || value.is<AZ::Name>())
                    {
                        error = AZStd::string::format("Override '%s' has an incompatible native value type.", name.GetCStr());
                        return false;
                    }
                    property = AZ::RPI::MaterialPropertyValue::FromAny(value);
                }
                if (!property.IsValid() || !AZ::RPI::ValidateMaterialPropertyDataType(property.GetTypeId(), descriptor,
                    [&](const char* message) { error = message; }))
                {
                    if (error.empty()) { error = AZStd::string::format("Unsupported override '%s' cannot be preserved.", name.GetCStr()); }
                    return false;
                }
            }
            for (const auto& [semantic, uvName] : assignment.m_matModUvOverrides)
            {
                if (modelUvNames.find(uvName) == modelUvNames.end() ||
                    AZStd::none_of(uvNames.begin(), uvNames.end(), [&](const auto& entry) { return entry.m_shaderInput == semantic; }))
                {
                    error = "The selected material cannot preserve an existing UV reference; no assignment was changed.";
                    return false;
                }
            }
            assignment.m_materialAsset = material;
            assignment.m_defaultMaterialAsset = {};
            assignment.m_materialInstance = {};
            const auto equivalent = [&](const Assignment& existing)
            {
                return existing.m_materialInstanceMustBeUnique == assignment.m_materialInstanceMustBeUnique &&
                    EqualAssignments(existing, assignment);
            };
            // Prefer the current slot for idempotence. Unique instances may retain their own slot,
            // but cannot coalesce with another slot (even with equal assets/properties/UV references).
            auto compatible = materialSlots.find(oldSlot);
            if (compatible != materialSlots.end() && !equivalent(compatible->second)) { compatible = materialSlots.end(); }
            if (compatible == materialSlots.end() && !assignment.m_materialInstanceMustBeUnique)
            {
                compatible = AZStd::find_if(materialSlots.begin(), materialSlots.end(),
                    [&](const auto& entry) { return equivalent(entry.second); });
            }
            const auto remapped = sourceSlotRemap.find(oldSlot);
            uint32_t slot;
            if (remapped != sourceSlotRemap.end()) { slot = remapped->second; }
            else if (compatible != materialSlots.end()) { slot = compatible->first; }
            else
            {
                if (lastSlot >= InvalidSlot - 1) { error = "Material slot capacity exhausted; no assignment was changed."; return false; }
                slot = ++lastSlot;
                materialSlots.emplace(slot, assignment);
                candidate.m_materials[AssignmentId::CreateFromStableIdOnly(slot)] = assignment;
            }
            sourceSlotRemap[oldSlot] = slot;
            const auto binding = AZStd::find_if(candidate.m_bindings.begin(), candidate.m_bindings.end(),
                [&](const auto& entry) { return entry.m_surfaceId == id; });
            if (binding == candidate.m_bindings.end()) { candidate.m_bindings.push_back({id, slot}); }
            else { binding->m_materialSlot = slot; }
        }
        output = AZStd::move(candidate);
        error.clear();
        return true;
    }

    bool TransformPolygonUVs(JE::Geometry::WhiteBoxMesh& mesh, const Api::PolygonHandles& polygons,
        const AZ::Vector2& scale, const AZ::Vector2& offset, float rotationDegrees)
    {
        Api::FaceHandles faces;
        if (!scale.IsFinite() || !offset.IsFinite() || !std::isfinite(rotationDegrees) ||
            !SelectedFaces(mesh, polygons, faces)) { return false; }
        const float radians = AZ::DegToRad(rotationDegrees);
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        AZStd::vector<AZStd::pair<Api::HalfedgeHandle, AZ::Vector2>> transformed;
        for (const auto face : faces)
        {
            for (const auto halfedge : Api::FaceHalfedgeHandles(mesh, face))
            {
                const auto uv = Api::HalfedgeUV(mesh, halfedge);
                const float u = uv.GetX() * scale.GetX();
                const float v = uv.GetY() * scale.GetY();
                const AZ::Vector2 result(cosine * u - sine * v + offset.GetX(), sine * u + cosine * v + offset.GetY());
                if (!result.IsFinite()) { return false; }
                transformed.emplace_back(halfedge, result);
            }
        }
        for (const auto& [halfedge, uv] : transformed) { Api::SetHalfedgeUV(mesh, halfedge, uv); }
        return true;
    }
}
