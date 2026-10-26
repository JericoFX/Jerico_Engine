/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Config
{
    class SoftNameBehavior
        : public AZ::SceneAPI::SceneCore::BehaviorComponent
        , protected AZ::SceneAPI::Events::GraphMetaInfoBus::Handler
    {
    public:
        AZ_COMPONENT(SoftNameBehavior, "{C2A9D207-485F-4752-B37B-388B0A52A956}", AZ::SceneAPI::SceneCore::BehaviorComponent);

        ~SoftNameBehavior() override = default;

        void Activate() override;
        void Deactivate() override;

        void GetVirtualTypes(
            AZ::SceneAPI::Events::GraphMetaInfo::VirtualTypesSet& types,
            const AZ::SceneAPI::Containers::Scene& scene,
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex node) override;
        void GetVirtualTypeName(AZStd::string& name, AZ::Crc32 type) override;
        void GetAllVirtualTypes(AZ::SceneAPI::Events::GraphMetaInfo::VirtualTypesSet& types) override;

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace JE::Assets::RenderBuilders::SceneProcessing::Config
