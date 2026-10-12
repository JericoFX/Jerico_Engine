/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include "Rendering/WhiteBoxRenderData.h"
#include "JE/Geometry/WhiteBoxComponentBus.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>

namespace JE::Geometry
{
    class RenderMeshInterface;

    //! Runtime representation of White Box.
    class JE_GEOMETRY_API WhiteBoxComponent
        : public AZ::Component
        , public WhiteBoxComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::VisibleGeometryRequestBus::Handler
    {
    public:
        AZ_COMPONENT(WhiteBoxComponent, "{6CFD4D82-FA68-4C18-BE67-43FC2B755B64}", AZ::Component)
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        WhiteBoxComponent();
        WhiteBoxComponent(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent& operator=(const WhiteBoxComponent&) = delete;
        WhiteBoxComponent(WhiteBoxComponent&&);
        WhiteBoxComponent& operator=(WhiteBoxComponent&&);
        ~WhiteBoxComponent();

        // WhiteBoxComponentRequestBus ...
        bool WhiteBoxIsVisible() const override;

        void GenerateWhiteBoxMesh(const WhiteBoxRenderData& whiteBoxRenderData);

    private:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AzFramework::VisibleGeometryRequestBus::Handler overrides ...
        void BuildVisibleGeometry(const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const override;

        WhiteBoxRenderData m_whiteBoxRenderData; //!< Intermediate format to store White Box render data.
        AZStd::unique_ptr<RenderMeshInterface> m_renderMesh; //!< The render mesh to use for White Box rendering.
    };
} // namespace JE::Geometry
