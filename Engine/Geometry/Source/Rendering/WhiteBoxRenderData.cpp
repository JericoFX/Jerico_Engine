/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderData.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Sha1.h>
#include <cstring>
#include <cmath>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <JE/Geometry/SurfaceData/SurfaceDataProviderRequestBus.h>
#include <JE/Geometry/SurfaceData/SurfaceDataSystemRequestBus.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace JE::Geometry
{
    AZStd::optional<SurfaceData::SurfaceSourceRevision> ComputeWhiteBoxSourceRevision(const WhiteBoxRenderData& renderData)
    {
        // Keep the existing Sha1 implementation's high-32-bit-zero length encoding valid; no payload allocation.
        constexpr char domain[] = "JE.WhiteBox.PublishedContent";
        constexpr size_t headerBytes = sizeof(domain) - 1 + 4 + 8 + 3 * 4 + 2;
        constexpr size_t faceBytes = 3 * 5 * 4 + 3 * 4 + 16 + 4;
        constexpr size_t maxBytes = AZStd::numeric_limits<AZ::u32>::max() / 8;
        if (renderData.m_faces.size() > (maxBytes - headerBytes) / faceBytes || !renderData.m_material.m_tint.IsFinite())
        {
            return AZStd::nullopt;
        }
        // Validate before encoding. A bad later face cannot yield a plausible partial-content revision.
        for (const auto& face : renderData.m_faces)
        {
            if (!face.m_v1.m_position.IsFinite() || !face.m_v2.m_position.IsFinite() || !face.m_v3.m_position.IsFinite() ||
                !face.m_v1.m_uv.IsFinite() || !face.m_v2.m_uv.IsFinite() || !face.m_v3.m_uv.IsFinite() || !face.m_normal.IsFinite())
            {
                return AZStd::nullopt;
            }
        }
        AZ::Sha1 hash;
        const auto byte = [&hash](AZ::u8 value) { hash.ProcessByte(static_cast<AZStd::byte>(value)); };
        const auto word = [&byte](AZ::u32 value)
        {
            for (unsigned shift = 0; shift != 32; shift += 8) { byte(static_cast<AZ::u8>(value >> shift)); }
        };
        const auto scalar = [&word](float value)
        {
            static_assert(sizeof(float) == sizeof(AZ::u32) && AZStd::numeric_limits<float>::is_iec559);
            AZ::u32 bits;
            std::memcpy(&bits, &value, sizeof(bits));
            if ((bits & 0x7fffffffu) == 0) { bits = 0; } // Canonicalize both IEEE zero encodings explicitly.
            word(bits);
        };
        const auto vector = [&scalar](const AZ::Vector3& value)
        {
            scalar(value.GetX()); scalar(value.GetY()); scalar(value.GetZ());
        };
        for (size_t index = 0; index != sizeof(domain) - 1; ++index) { byte(static_cast<AZ::u8>(domain[index])); }
        constexpr AZ::u32 schema = 1; // WhiteBox published local content, not an imported-model/cook identity schema.
        word(schema);
        const AZ::u64 count = renderData.m_faces.size();
        word(static_cast<AZ::u32>(count)); word(static_cast<AZ::u32>(count >> 32));
        for (const auto& face : renderData.m_faces)
        {
            for (const auto* vertex : {&face.m_v1, &face.m_v2, &face.m_v3})
            {
                vector(vertex->m_position); scalar(vertex->m_uv.GetX()); scalar(vertex->m_uv.GetY());
            }
            vector(face.m_normal);
            for (const auto value : face.m_surfaceId) { hash.ProcessByte(value); }
            word(face.m_materialSlot);
        }
        vector(renderData.m_material.m_tint);
        byte(renderData.m_material.m_useTexture ? 1 : 0);
        byte(renderData.m_material.m_visible ? 1 : 0);
        SurfaceData::SurfaceSourceRevision token;
        token.m_schema = schema;
        AZ::u32 digest[5];
        hash.GetDigest(digest);
        for (size_t index = 0; index != token.m_digest.size(); ++index) { token.m_digest[index] = digest[index]; }
        return token;
    }

    bool UpdateWhiteBoxSourceRevision(WhiteBoxRenderData& renderData)
    {
        const auto revision = ComputeWhiteBoxSourceRevision(renderData);
        renderData.m_sourceRevision = revision.value_or(SurfaceData::SurfaceSourceRevision{});
        return revision.has_value();
    }

    class WhiteBoxSurfaceDataProvider::State final : public SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        explicit State(const AZ::EntityComponentIdPair& owner) : m_owner(owner) {}
        ~State() override { Stop(); }

        bool Publish(const WhiteBoxRenderData& renderData, const AZ::Transform& world)
        {
            if (m_stopping) { return false; }
            if (renderData.m_faces.empty()) { Stop(); return true; }
            WhiteBoxRenderData snapshot = renderData;
            if (snapshot.m_sourceRevision.m_schema != 0)
            {
                const auto actual = ComputeWhiteBoxSourceRevision(snapshot);
                if (!actual || *actual != snapshot.m_sourceRevision) { snapshot.m_sourceRevision = {}; }
            }
            else { snapshot.m_sourceRevision = {}; } // Legacy data is not silently upgraded at runtime.
            const AZ::Aabb bounds = Bounds(snapshot, world);
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
                m_data = AZStd::move(snapshot);
                m_worldFromLocal = world;
                m_bounds = bounds;
            }
            return PublishEntry(bounds); // Never acquire registry locks while holding the snapshot lock.
        }

        bool UpdateTransform(const AZ::Transform& world)
        {
            if (m_stopping) { return false; }
            AZ::Aabb bounds;
            bool empty;
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
                m_worldFromLocal = world;
                empty = m_data.m_faces.empty();
                bounds = Bounds(m_data, world);
                m_bounds = bounds;
            }
            if (empty) { Stop(); return true; }
            return PublishEntry(bounds);
        }

        SurfaceData::SurfaceRayQueryResult QueryRay(
            const SurfaceData::SurfaceRayRequest& request, AZStd::span<SurfaceData::SurfaceRayHit> output) const override
        {
            if ((request.m_filter != SurfaceData::SurfaceRayQueryFilter::AllRegisteredProviders &&
                    request.m_filter != SurfaceData::SurfaceRayQueryFilter::ExactOwner) ||
                (request.m_filter == SurfaceData::SurfaceRayQueryFilter::ExactOwner &&
                    (!request.m_owner.GetEntityId().IsValid() || request.m_owner.GetComponentId() == AZ::InvalidComponentId)))
            {
                SurfaceData::SurfaceRayQueryResult invalid;
                invalid.m_status = SurfaceData::SurfaceRayQueryStatus::InvalidInput;
                return invalid;
            }
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
            if (m_data.m_faces.empty() ||
                (request.m_filter == SurfaceData::SurfaceRayQueryFilter::ExactOwner && request.m_owner != m_owner))
            {
                SurfaceData::SurfaceRayQueryResult unsupported;
                unsupported.m_status = SurfaceData::SurfaceRayQueryStatus::Unsupported;
                unsupported.m_unsupportedProviderCount = 1;
                return unsupported;
            }
            auto result = IntersectWhiteBoxRenderData(m_data, m_worldFromLocal,
                request.m_startWorldPosition, request.m_endWorldPosition, request.m_sidedness, output);
            StampPublishedSource(result, output);
            return result;
        }

        SurfaceData::SurfaceRayQueryResult QueryVolume(const SurfaceData::SurfaceVolumeRequest& request,
            AZStd::span<SurfaceData::SurfaceRayHit> output) const override
        {
            if (!request.IsValid())
            {
                SurfaceData::SurfaceRayQueryResult invalid;
                invalid.m_status = SurfaceData::SurfaceRayQueryStatus::InvalidInput;
                return invalid;
            }
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
            if (m_data.m_faces.empty() ||
                (request.m_filter == SurfaceData::SurfaceRayQueryFilter::ExactOwner && request.m_owner != m_owner))
            {
                SurfaceData::SurfaceRayQueryResult unsupported;
                unsupported.m_status = SurfaceData::SurfaceRayQueryStatus::Unsupported;
                unsupported.m_unsupportedProviderCount = 1;
                return unsupported;
            }
            auto result = IntersectWhiteBoxRenderDataVolume(m_data, m_worldFromLocal, request, output);
            StampPublishedSource(result, output);
            return result;
        }

        AZStd::optional<AZ::Aabb> GetSurfaceRayBounds() const override
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
            return m_bounds; // Same publication as geometry/pose; no registry mutation or per-query geometry scan.
        }

        void GetSurfacePoints(const AZ::Vector3&, SurfaceData::SurfacePointList&) const override
        {
            // This owner advertises geometry only; the XY dispatcher must not invent point samples.
        }

    private:
        // Called with m_snapshotMutex already shared-locked; metadata and geometry come from one publication.
        void StampPublishedSource(SurfaceData::SurfaceRayQueryResult& result, AZStd::span<SurfaceData::SurfaceRayHit> output) const
        {
            if (result.m_status == SurfaceData::SurfaceRayQueryStatus::InvalidInput) { return; }
            result.m_supportedProviderCount = 1;
            for (size_t index = 0; index < result.m_hitCount; ++index)
            {
                output[index].m_entityAndComponent = m_owner;
                output[index].m_sourceRevision = m_data.m_sourceRevision;
                output[index].m_revisionState = m_data.m_sourceRevision.m_schema != 0
                    ? SurfaceData::SurfaceSourceRevisionState::Content : SurfaceData::SurfaceSourceRevisionState::Unavailable;
            }
        }

        static AZ::Aabb Bounds(const WhiteBoxRenderData& data, const AZ::Transform& world)
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            for (const auto& face : data.m_faces)
            {
                for (const auto& point : {face.m_v1.m_position, face.m_v2.m_position, face.m_v3.m_position})
                {
                    const auto transformed = world.TransformPoint(point);
                    if (!transformed.IsFinite()) { return AZ::Aabb::CreateNull(); }
                    bounds.AddPoint(transformed);
                }
            }
            return bounds;
        }

        bool PublishEntry(const AZ::Aabb& bounds)
        {
            auto* system = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get();
            if (m_handle != SurfaceData::InvalidSurfaceDataRegistryHandle && system != m_registeredSystem)
            {
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
                m_handle = SurfaceData::InvalidSurfaceDataRegistryHandle;
                m_registeredSystem = nullptr;
            }
            if (!system) { return false; } // Do not start a system or activate an excluded host to register this provider.
            SurfaceData::SurfaceDataRegistryEntry entry;
            entry.m_entityId = m_owner.GetEntityId();
            entry.m_componentId = m_owner.GetComponentId();
            entry.m_bounds = bounds;
            entry.m_queryCapabilities = SurfaceData::SurfaceDataQueryCapabilities::RaysAndVolumes;
            entry.m_maxPointsCreatedPerInput = 0;
            if (m_handle == SurfaceData::InvalidSurfaceDataRegistryHandle)
            {
                m_registeredSystem = system;
                m_handle = system->RegisterSurfaceDataProvider(entry);
                if (m_handle == SurfaceData::InvalidSurfaceDataRegistryHandle) { return false; }
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_handle);
            }
            else { system->UpdateSurfaceDataProvider(m_handle, entry); }
            return true;
        }

        void Stop()
        {
            if (m_stopping) { return; }
            m_stopping = true;
            // Shared-dispatch bus disconnect waits for its callbacks; do not hold the snapshot lock here.
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
            const auto handle = m_handle;
            auto* registeredSystem = m_registeredSystem;
            m_handle = SurfaceData::InvalidSurfaceDataRegistryHandle;
            m_registeredSystem = nullptr;
            if (handle != SurfaceData::InvalidSurfaceDataRegistryHandle &&
                registeredSystem == AZ::Interface<SurfaceData::SurfaceDataSystem>::Get())
            {
                registeredSystem->UnregisterSurfaceDataProvider(handle);
            }
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_snapshotMutex);
                m_data.m_faces.clear();
                m_bounds = AZ::Aabb::CreateNull();
                m_worldFromLocal = AZ::Transform::CreateIdentity();
            }
            m_stopping = false;
        }

        const AZ::EntityComponentIdPair m_owner;
        mutable AZStd::shared_mutex m_snapshotMutex;
        WhiteBoxRenderData m_data;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity();
        SurfaceData::SurfaceDataRegistryHandle m_handle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystem* m_registeredSystem = nullptr; // Used only while it is still the current Interface.
        bool m_stopping = false; // Owner-thread reentrancy guard during unregister notifications.
    };

    WhiteBoxSurfaceDataProvider::WhiteBoxSurfaceDataProvider(const AZ::EntityComponentIdPair& owner)
        : m_state(AZStd::make_unique<State>(owner)) {}
    WhiteBoxSurfaceDataProvider::~WhiteBoxSurfaceDataProvider() = default;
    bool WhiteBoxSurfaceDataProvider::Publish(const WhiteBoxRenderData& data, const AZ::Transform& world)
    { return m_state->Publish(data, world); }
    bool WhiteBoxSurfaceDataProvider::UpdateTransform(const AZ::Transform& world)
    { return m_state->UpdateTransform(world); }

    void WhiteBoxRenderData::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxFace::Reflect(context);
        WhiteBoxMaterial::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SurfaceData::SurfaceSourceRevision>()
                ->Version(1)
                ->Field("Schema", &SurfaceData::SurfaceSourceRevision::m_schema)
                ->Field("Digest", &SurfaceData::SurfaceSourceRevision::m_digest);
            serializeContext->Class<WhiteBoxRenderData>()
                ->Version(4)
                ->Field("Faces", &WhiteBoxRenderData::m_faces)
                ->Field("Material", &WhiteBoxRenderData::m_material)
                ->Field("SourceRevision", &WhiteBoxRenderData::m_sourceRevision);
        }
    }

    void WhiteBoxVertex::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxVertex>()
                ->Version(1)
                ->Field("Position", &WhiteBoxVertex::m_position)
                ->Field("UV", &WhiteBoxVertex::m_uv);
        }
    }

    void WhiteBoxFace::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxVertex::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxFace>()
                ->Version(2)
                ->Field("Vertex1", &WhiteBoxFace::m_v1)
                ->Field("Vertex2", &WhiteBoxFace::m_v2)
                ->Field("Vertex3", &WhiteBoxFace::m_v3)
                ->Field("Normal", &WhiteBoxFace::m_normal)
                ->Field("SurfaceId", &WhiteBoxFace::m_surfaceId)
                ->Field("MaterialSlot", &WhiteBoxFace::m_materialSlot);
        }
    }

    namespace
    {
        bool ValidWhiteBoxQueryGeometry(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal)
        {
            if (!worldFromLocal.IsFinite() || worldFromLocal.GetUniformScale() == 0.0f) { return false; }
            for (const auto& face : renderData.m_faces)
            {
                const AZ::Vector3 a = worldFromLocal.TransformPoint(face.m_v1.m_position);
                const AZ::Vector3 b = worldFromLocal.TransformPoint(face.m_v2.m_position);
                const AZ::Vector3 c = worldFromLocal.TransformPoint(face.m_v3.m_position);
                const AZ::Vector3 localNormal = (face.m_v2.m_position - face.m_v1.m_position).Cross(
                    face.m_v3.m_position - face.m_v1.m_position);
                const float localAreaSquared = 0.5f * localNormal.GetLengthSq();
                const float worldNormalLengthSq = (b - a).Cross(c - a).GetLengthSq();
                if (!a.IsFinite() || !b.IsFinite() || !c.IsFinite() || !std::isfinite(localAreaSquared) ||
                    !std::isfinite(worldNormalLengthSq) ||
                    (localAreaSquared > DegenerateTriangleAreaSquareEpsilon && worldNormalLengthSq <= 0.0f))
                {
                    return false;
                }
            }
            return true;
        }
    }

    WhiteBoxRayQueryResult IntersectWhiteBoxRenderData(
        const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& startWorld, const AZ::Vector3& endWorld,
        WhiteBoxRaySidedness sidedness, AZStd::span<WhiteBoxRayHit> output)
    {
        WhiteBoxRayQueryResult result;
        const AZ::Vector3 segment = endWorld - startWorld;
        const float segmentLength = segment.GetLength();
        if (!worldFromLocal.IsFinite() || worldFromLocal.GetUniformScale() == 0.0f ||
            !startWorld.IsFinite() || !endWorld.IsFinite() ||
            !std::isfinite(segmentLength) || segmentLength <= 0.0f ||
            (sidedness != WhiteBoxRaySidedness::FrontFace && sidedness != WhiteBoxRaySidedness::DoubleSided))
        {
            result.m_status = WhiteBoxRayQueryStatus::InvalidInput;
            return result;
        }

        if (!ValidWhiteBoxQueryGeometry(renderData, worldFromLocal))
        {
            result.m_status = WhiteBoxRayQueryStatus::InvalidInput;
            return result;
        }

        AZ::Intersect::SegmentTriangleHitTester tester(startWorld, endWorld);
        for (size_t primitive = 0; primitive < renderData.m_faces.size(); ++primitive)
        {
            const auto& face = renderData.m_faces[primitive];
            const AZ::Vector3 a = worldFromLocal.TransformPoint(face.m_v1.m_position);
            const AZ::Vector3 b = worldFromLocal.TransformPoint(face.m_v2.m_position);
            const AZ::Vector3 c = worldFromLocal.TransformPoint(face.m_v3.m_position);
            // Match the renderer's LOCAL culling criterion; world scaling must not remove a retained face.
            const AZ::Vector3 localNormal = (face.m_v2.m_position - face.m_v1.m_position).Cross(
                face.m_v3.m_position - face.m_v1.m_position);
            const float localAreaSquared = 0.5f * localNormal.GetLengthSq();
            if (localAreaSquared <= DegenerateTriangleAreaSquareEpsilon)
            {
                continue;
            }
            AZ::Vector3 normal = (b - a).Cross(c - a);
            const float normalLength = normal.GetLength();

            AZ::Vector3 facingNormal;
            float fraction;
            const bool intersects = sidedness == WhiteBoxRaySidedness::FrontFace
                ? tester.IntersectSegmentTriangleCCW(a, b, c, facingNormal, fraction)
                : tester.IntersectSegmentTriangle(a, b, c, facingNormal, fraction);
            if (!intersects || !std::isfinite(fraction) || fraction < 0.0f || fraction > 1.0f)
            {
                continue;
            }

            const float distance = fraction * segmentLength;
            size_t insertion = 0;
            // Enumeration is by primitive index, so equal-distance hits retain that deterministic order.
            while (insertion < result.m_hitCount && output[insertion].m_distance <= distance)
            {
                ++insertion;
            }
            if (result.m_hitCount == output.size())
            {
                ++result.m_discardedHitCount;
                result.m_status = WhiteBoxRayQueryStatus::Truncated;
                if (insertion == output.size())
                {
                    continue;
                }
            }
            else
            {
                ++result.m_hitCount;
            }
            for (size_t index = result.m_hitCount - 1; index > insertion; --index)
            {
                output[index] = output[index - 1];
            }
            // The double-sided tester faces its normal toward the ray; this contract retains source winding.
            normal /= normalLength;
            output[insertion] = {startWorld + segment * fraction, normal, distance,
                face.m_surfaceId, face.m_materialSlot, primitive};
        }
        return result;
    }

    WhiteBoxRayQueryResult IntersectWhiteBoxRenderDataVolume(const WhiteBoxRenderData& renderData,
        const AZ::Transform& worldFromLocal, const SurfaceData::SurfaceVolumeRequest& request, AZStd::span<WhiteBoxRayHit> output)
    {
        WhiteBoxRayQueryResult result;
        if (!request.IsValid() || !ValidWhiteBoxQueryGeometry(renderData, worldFromLocal))
        {
            result.m_status = WhiteBoxRayQueryStatus::InvalidInput;
            return result;
        }
        const auto localFromWorld = worldFromLocal.GetInverse();
        if (!localFromWorld.IsFinite()) { result.m_status = WhiteBoxRayQueryStatus::InvalidInput; return result; }
        const auto& bounds = request.m_worldBounds;
        AZStd::array<AZ::Vector3, 8> worldCorners, localCorners;
        for (size_t index = 0; index < worldCorners.size(); ++index)
        {
            worldCorners[index] = {(index & 1) ? bounds.GetMax().GetX() : bounds.GetMin().GetX(),
                (index & 2) ? bounds.GetMax().GetY() : bounds.GetMin().GetY(),
                (index & 4) ? bounds.GetMax().GetZ() : bounds.GetMin().GetZ()};
            localCorners[index] = localFromWorld.TransformPoint(worldCorners[index]);
            if (!localCorners[index].IsFinite()) { result.m_status = WhiteBoxRayQueryStatus::InvalidInput; return result; }
        }
        AZStd::array<AZStd::array<size_t, 2>, 12> edgeCorners;
        AZStd::fixed_vector<AZ::Intersect::SegmentTriangleHitTester, 12> edgeTesters;
        for (size_t corner = 0; corner < worldCorners.size(); ++corner)
        {
            for (int32_t axis = 0; axis != 3; ++axis)
            {
                const size_t bit = size_t{1} << axis;
                if (corner & bit) { continue; }
                const size_t other = corner | bit;
                const float lengthSq = (localCorners[other] - localCorners[corner]).GetLengthSq();
                if (!std::isfinite(lengthSq) || lengthSq <= 0.0f)
                { result.m_status = WhiteBoxRayQueryStatus::InvalidInput; return result; }
                edgeCorners[edgeTesters.size()] = {corner, other};
                edgeTesters.emplace_back(localCorners[corner], localCorners[other]);
            }
        }
        const auto clipEdge = [&](const AZ::Vector3& start, const AZ::Vector3& end, AZ::Vector3& point)
        {
            // The native clipper uses absolute parallel epsilon. Affine conditioning makes every nonzero
            // segment component +/-1, retaining the exact segment parameter under supported uniform scales.
            AZ::Vector3 scaledMin, scaledMax, direction;
            for (int32_t axis = 0; axis != 3; ++axis)
            {
                const double origin = start.GetElement(axis);
                const double delta = static_cast<double>(end.GetElement(axis)) - origin;
                if (delta == 0.0)
                {
                    if (origin < bounds.GetMin().GetElement(axis) || origin > bounds.GetMax().GetElement(axis)) { return false; }
                    scaledMin.SetElement(axis, -1.0f); scaledMax.SetElement(axis, 1.0f); direction.SetElement(axis, 0.0f);
                }
                else
                {
                    const auto normalized = [&](float bound)
                    {
                        // A unit segment cannot reach coordinates outside [-1,1]; saturation keeps finite intermediates.
                        return static_cast<float>(AZStd::max(-2.0, AZStd::min(2.0, (static_cast<double>(bound) - origin) / std::fabs(delta))));
                    };
                    scaledMin.SetElement(axis, normalized(bounds.GetMin().GetElement(axis)));
                    scaledMax.SetElement(axis, normalized(bounds.GetMax().GetElement(axis)));
                    direction.SetElement(axis, delta > 0.0 ? 1.0f : -1.0f);
                }
            }
            auto clippedStart = AZ::Vector3::CreateZero();
            float enter, exit;
            if (!AZ::Intersect::ClipRayWithAabb(AZ::Aabb::CreateFromMinMax(scaledMin, scaledMax), clippedStart, direction, enter, exit) ||
                !std::isfinite(enter) || !std::isfinite(exit) || enter < 0.0f || enter > 1.0f || exit < enter) { return false; }
            // Any point in this convex intersection is valid. Prefer the interior of the clipped interval,
            // avoiding a rounded entry point just outside a slab without expanding the box or adding epsilon.
            const double fraction = (static_cast<double>(enter) + AZStd::min(1.0, static_cast<double>(exit))) * 0.5;
            for (int32_t axis = 0; axis != 3; ++axis)
            {
                const double origin = start.GetElement(axis);
                point.SetElement(axis, static_cast<float>(origin + (static_cast<double>(end.GetElement(axis)) - origin) * fraction));
            }
            return bounds.Contains(point);
        };
        // Convex intersection is exhaustive: triangle vertex in box, triangle edge through box face,
        // or box edge through triangle. Enumerate ALL 3+12 edges, not sampled rays or candidate AABB contacts.
        const auto pointInIntersection = [&](const WhiteBoxFace& face, const AZStd::array<AZ::Vector3, 3>& world,
            AZ::Vector3& point)
        {
            for (const auto& vertex : world)
            {
                if (bounds.Contains(vertex)) { point = vertex; return true; }
            }
            for (size_t edge = 0; edge != 3; ++edge)
            {
                if (clipEdge(world[edge], world[(edge + 1) % 3], point)) { return true; }
            }
            for (size_t edge = 0; edge != edgeTesters.size(); ++edge)
            {
                AZ::Vector3 facingNormal;
                float fraction;
                if (!edgeTesters[edge].IntersectSegmentTriangle(face.m_v1.m_position, face.m_v2.m_position,
                        face.m_v3.m_position, facingNormal, fraction) ||
                    !std::isfinite(fraction) || fraction < 0.0f || fraction > 1.0f) { continue; }
                const auto& edgeIndices = edgeCorners[edge];
                point = worldCorners[edgeIndices[0]] + (worldCorners[edgeIndices[1]] - worldCorners[edgeIndices[0]]) * fraction;
                if (bounds.Contains(point)) { return true; }
            }
            return false;
        };
        for (size_t primitive = 0; primitive < renderData.m_faces.size(); ++primitive)
        {
            const auto& face = renderData.m_faces[primitive];
            const auto localNormal = (face.m_v2.m_position - face.m_v1.m_position).Cross(face.m_v3.m_position - face.m_v1.m_position);
            if (0.5f * localNormal.GetLengthSq() <= DegenerateTriangleAreaSquareEpsilon) { continue; }
            const AZStd::array<AZ::Vector3, 3> world{worldFromLocal.TransformPoint(face.m_v1.m_position),
                worldFromLocal.TransformPoint(face.m_v2.m_position), worldFromLocal.TransformPoint(face.m_v3.m_position)};
            const auto triangleBounds = AZ::Aabb::CreateFromMinMax(world[0].GetMin(world[1]).GetMin(world[2]),
                world[0].GetMax(world[1]).GetMax(world[2]));
            if (!triangleBounds.Overlaps(bounds)) { continue; }
            AZ::Vector3 point;
            if (!pointInIntersection(face, world, point)) { continue; }
            if (result.m_hitCount == output.size())
            {
                ++result.m_discardedHitCount;
                result.m_status = WhiteBoxRayQueryStatus::Truncated;
                continue;
            }
            auto& hit = output[result.m_hitCount++];
            hit = {};
            hit.m_worldPosition = point;
            const auto normal = (world[1] - world[0]).Cross(world[2] - world[0]);
            hit.m_worldNormal = normal / std::sqrt(normal.GetLengthSq()); // Source winding, not facing-normal from edge tests.
            hit.m_distance = (point - bounds.GetCenter()).GetLength(); // Representative point, NOT penetration/minimum distance.
            hit.m_surfaceId = face.m_surfaceId;
            hit.m_materialSlot = face.m_materialSlot;
            hit.m_primitiveIndex = primitive;
        }
        return result;
    }

    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& inFaces)
    {
        // actual face count can be less than the original face count if any degenerate
        // faces are detected and removed
        const size_t inFaceCount = inFaces.size();
        size_t outFaceCount = 0;

        WhiteBoxFaces outFaces;

        // resize to the worst case number of faces
        outFaces.resize(inFaceCount);

        for (size_t inFaceIndex = 0; inFaceIndex < inFaceCount; ++inFaceIndex)
        {
            const WhiteBoxFace face = inFaces[inFaceIndex];
            const AZ::Vector3& vertex1 = face.m_v1.m_position;
            const AZ::Vector3& vertex2 = face.m_v2.m_position;
            const AZ::Vector3& vertex3 = face.m_v3.m_position;

            const auto areaSquared = 0.5f * ((vertex2 - vertex1).Cross(vertex3 - vertex1)).GetLengthSq();

            // only copy non-degenerate triangles (area is less than zero)
            if (areaSquared > DegenerateTriangleAreaSquareEpsilon)
            {
                outFaces[outFaceCount] = face;
                outFaceCount++;
            }
        }

        // now that we know the number of faces we need we can safely resize the vector without any
        // additional allocations as the number of faces will be equal to or less than the original size
        outFaces.resize(outFaceCount);

        return outFaces;
    }
} // namespace JE::Geometry

namespace JE::BSP
{
    void SurfaceMaterialBinding::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SurfaceMaterialBinding>()->Version(1)
                ->Field("SurfaceId", &SurfaceMaterialBinding::m_surfaceId)
                ->Field("MaterialSlot", &SurfaceMaterialBinding::m_materialSlot);
        }
    }

    uint32_t FindSurfaceMaterialSlot(const SurfaceMaterialBindings& bindings, const AZ::Uuid& surfaceId)
    {
        for (const auto& binding : bindings)
        {
            if (binding.m_surfaceId == surfaceId) { return binding.m_materialSlot; }
        }
        return 0;
    }

    AZStd::vector<MaterialIndexRange> PartitionFacesByMaterial(JE::Geometry::WhiteBoxFaces& faces)
    {
        if (faces.size() > AZStd::numeric_limits<uint32_t>::max() / 3 ||
            AZStd::any_of(faces.begin(), faces.end(), [](const auto& face)
                { return face.m_materialSlot == AZStd::numeric_limits<uint32_t>::max(); })) { return {}; }
        AZStd::stable_sort(faces.begin(), faces.end(), [](const auto& a, const auto& b)
        {
            return a.m_materialSlot < b.m_materialSlot;
        });
        AZStd::vector<MaterialIndexRange> ranges;
        for (size_t face = 0; face < faces.size(); ++face)
        {
            if (ranges.empty() || ranges.back().m_materialSlot != faces[face].m_materialSlot)
            {
                ranges.push_back({faces[face].m_materialSlot, static_cast<uint32_t>(face * 3), 0});
            }
            ranges.back().m_indexCount += 3;
        }
        return ranges;
    }
}