/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessor.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Culling.h>

namespace JE::Render
{
    namespace Features
    {
        void OcclusionCullingPlaneFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext
                    ->Class<OcclusionCullingPlaneFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void OcclusionCullingPlaneFeatureProcessor::Activate()
        {
            m_occlusionCullingPlanes.reserve(InitialOcclusionCullingPlanesAllocationSize);
            m_rpiOcclusionPlanes.reserve(InitialOcclusionCullingPlanesAllocationSize);

            EnableSceneNotification();
        }

        void OcclusionCullingPlaneFeatureProcessor::Deactivate()
        {
            AZ_Warning("OcclusionCullingPlaneFeatureProcessor", m_occlusionCullingPlanes.size() == 0,
                "Deactivating the OcclusionCullingPlaneFeatureProcessor, but there are still outstanding occlusion planes. Components\n"
                "using OcclusionCullingPlaneHandles should free them before the OcclusionCullingPlaneFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();
        }

        void OcclusionCullingPlaneFeatureProcessor::OnBeginPrepareRender()
        {
            if (m_rpiListNeedsUpdate)
            {
                // rebuild the RPI occlusion list
                m_rpiOcclusionPlanes.clear();

                for (auto& occlusionCullingPlane : m_occlusionCullingPlanes)
                {
                    if (!occlusionCullingPlane->GetEnabled())
                    {
                        continue;
                    }

                    JE::Render::RPI::CullingScene::OcclusionPlane rpiOcclusionPlane;

                    static const AZ::Vector3 BL = AZ::Vector3(-0.5f, 0.0f, -0.5f);
                    static const AZ::Vector3 TL = AZ::Vector3(-0.5f, 0.0f,  0.5f);
                    static const AZ::Vector3 TR = AZ::Vector3( 0.5f, 0.0f,  0.5f);
                    static const AZ::Vector3 BR = AZ::Vector3( 0.5f, 0.0f, -0.5f);

                    const AZ::Transform& transform = occlusionCullingPlane->GetTransform();

                    // convert corners to world space
                    rpiOcclusionPlane.m_cornerBL = transform.TransformPoint(BL);
                    rpiOcclusionPlane.m_cornerTL = transform.TransformPoint(TL);
                    rpiOcclusionPlane.m_cornerTR = transform.TransformPoint(TR);
                    rpiOcclusionPlane.m_cornerBR = transform.TransformPoint(BR);

                    // build world space AABB
                    AZ::Vector3 aabbMin = rpiOcclusionPlane.m_cornerBL.GetMin(rpiOcclusionPlane.m_cornerTR);
                    AZ::Vector3 aabbMax = rpiOcclusionPlane.m_cornerBL.GetMax(rpiOcclusionPlane.m_cornerTR);
                    rpiOcclusionPlane.m_aabb = AZ::Aabb::CreateFromMinMax(aabbMin, aabbMax);

                    m_rpiOcclusionPlanes.push_back(rpiOcclusionPlane);
                }

                GetParentScene()->GetCullingScene()->SetOcclusionPlanes(m_rpiOcclusionPlanes);

                m_rpiListNeedsUpdate = false;
            }
        }

        OcclusionCullingPlaneHandle OcclusionCullingPlaneFeatureProcessor::AddOcclusionCullingPlane(const AZ::Transform& transform)
        {
            AZStd::shared_ptr<OcclusionCullingPlane> occlusionCullingPlane = AZStd::make_shared<OcclusionCullingPlane>();
            occlusionCullingPlane->Init(GetParentScene());
            occlusionCullingPlane->SetTransform(transform);
            m_occlusionCullingPlanes.push_back(occlusionCullingPlane);
            m_rpiListNeedsUpdate = true;

            return occlusionCullingPlane;
        }

        void OcclusionCullingPlaneFeatureProcessor::RemoveOcclusionCullingPlane(OcclusionCullingPlaneHandle& occlusionCullingPlane)
        {
            AZ_Assert(occlusionCullingPlane.get(), "RemoveOcclusionCullingPlane called with an invalid handle");

            auto itEntry = AZStd::find_if(m_occlusionCullingPlanes.begin(), m_occlusionCullingPlanes.end(), [&](AZStd::shared_ptr<OcclusionCullingPlane> const& entry)
            {
                return (entry == occlusionCullingPlane);
            });

            AZ_Assert(itEntry != m_occlusionCullingPlanes.end(), "RemoveOcclusionCullingPlane called with an occlusion plane that is not in the occlusion plane list");
            m_occlusionCullingPlanes.erase(itEntry);
            occlusionCullingPlane = nullptr;
            m_rpiListNeedsUpdate = true;
        }

        void OcclusionCullingPlaneFeatureProcessor::SetTransform(const OcclusionCullingPlaneHandle& occlusionCullingPlane, const AZ::Transform& transform)
        {
            AZ_Assert(occlusionCullingPlane.get(), "SetTransform called with an invalid handle");
            occlusionCullingPlane->SetTransform(transform);
            m_rpiListNeedsUpdate = true;
        }

        void OcclusionCullingPlaneFeatureProcessor::SetEnabled(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool enabled)
        {
            AZ_Assert(occlusionCullingPlane.get(), "Enable called with an invalid handle");
            occlusionCullingPlane->SetEnabled(enabled);
            m_rpiListNeedsUpdate = true;
        }

        void OcclusionCullingPlaneFeatureProcessor::ShowVisualization(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool showVisualization)
        {
            AZ_Assert(occlusionCullingPlane.get(), "ShowVisualization called with an invalid handle");
            occlusionCullingPlane->ShowVisualization(showVisualization);
        }

        void OcclusionCullingPlaneFeatureProcessor::SetTransparentVisualization(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool transparentVisualization)
        {
            AZ_Assert(occlusionCullingPlane.get(), "SetTransparentVisualization called with an invalid handle");
            occlusionCullingPlane->SetTransparentVisualization(transparentVisualization);
        }
    } // namespace Features
} // namespace JE::Render
