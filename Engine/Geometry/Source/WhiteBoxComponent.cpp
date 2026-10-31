/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Rendering/WhiteBoxMaterial.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Rendering/WhiteBoxRenderDataUtil.h>
#include <Rendering/WhiteBoxRenderMeshInterface.h>
#include <JE/Geometry/WhiteBoxBus.h>

namespace JE::Geometry
{
    void WhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxRenderData::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxComponent, AZ::Component>()->Version(1)->Field(
                "WhiteBoxRenderData", &WhiteBoxComponent::m_whiteBoxRenderData);
        }
    }

    void WhiteBoxComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialConsumerService"));
    }

    void WhiteBoxComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialConsumerService"));
    }

    WhiteBoxComponent::WhiteBoxComponent() = default;
    WhiteBoxComponent::WhiteBoxComponent(WhiteBoxComponent&&) = default;
    WhiteBoxComponent& WhiteBoxComponent::operator=(WhiteBoxComponent&&) = default;
    WhiteBoxComponent::~WhiteBoxComponent() = default;

    void WhiteBoxComponent::Activate()
    {
        WhiteBoxRequestBus::BroadcastResult(m_renderMesh, &WhiteBoxRequests::CreateRenderMeshInterface, GetEntityId());

        const AZ::EntityId entityId = GetEntityId();

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        // generate the mesh
        m_renderMesh->BuildMesh(m_whiteBoxRenderData, worldFromLocal);
        m_renderMesh->UpdateMaterial(m_whiteBoxRenderData.m_material);
        m_renderMesh->SetVisiblity(m_whiteBoxRenderData.m_material.m_visible);

        AzFramework::VisibleGeometryRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        WhiteBoxComponentRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(entityId, GetId()));
        m_surfaceDataProvider = AZStd::make_unique<WhiteBoxSurfaceDataProvider>(AZ::EntityComponentIdPair(entityId, GetId()));
        m_surfaceDataProvider->Publish(m_whiteBoxRenderData, worldFromLocal);
    }

    void WhiteBoxComponent::Deactivate()
    {
        m_surfaceDataProvider.reset();
        WhiteBoxComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzFramework::VisibleGeometryRequestBus::Handler::BusDisconnect();

        m_renderMesh.reset();
    }

    void WhiteBoxComponent::GenerateWhiteBoxMesh(const WhiteBoxRenderData& whiteBoxRenderData)
    {
        m_whiteBoxRenderData = whiteBoxRenderData;
        if (m_renderMesh)
        {
            AZ::Transform world = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(world, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            // Keep the active renderer and query snapshot on the same runtime data, including replacement/empty data.
            m_renderMesh->BuildMesh(m_whiteBoxRenderData, world);
            m_renderMesh->UpdateMaterial(m_whiteBoxRenderData.m_material);
            m_renderMesh->SetVisiblity(m_whiteBoxRenderData.m_material.m_visible);
            if (m_surfaceDataProvider) { m_surfaceDataProvider->Publish(m_whiteBoxRenderData, world); }
        }
    }

    void WhiteBoxComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_renderMesh->UpdateTransform(world);
        if (m_surfaceDataProvider) { m_surfaceDataProvider->UpdateTransform(world); }
    }

    void WhiteBoxComponent::BuildVisibleGeometry(
        [[maybe_unused]] const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const
    {
        // Convert the white box render data into visible geometry data
        const AzFramework::VisibleGeometry geometry = BuildVisibleGeometryFromWhiteBoxRenderData(GetEntityId(), m_whiteBoxRenderData);

        if (!geometry.m_indices.empty() && !geometry.m_vertices.empty())
        {
            geometryContainer.push_back(geometry);
        }
    }

    bool WhiteBoxComponent::WhiteBoxIsVisible() const
    {
        return m_renderMesh->IsVisible();
    }
} // namespace JE::Geometry
