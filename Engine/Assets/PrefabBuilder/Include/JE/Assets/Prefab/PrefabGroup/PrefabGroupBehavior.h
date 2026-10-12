/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Assets/Prefab/PrefabBuilderExports.h>

#include <JE/Assets/Prefab/PrefabGroup/PrefabGroup.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/IO/Path/Path.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <AzCore/JSON/rapidjson.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::Events
{
    class PreExportEventContext;
}

namespace JE::Assets::Prefab
{
    class JE_PREFAB_BUILDER_API PrefabGroupBehavior
        : public AZ::SceneAPI::SceneCore::BehaviorComponent
    {
    public:
        AZ_COMPONENT(PrefabGroupBehavior, "{13DC2819-CAC2-4977-91D7-C870087072AB}", AZ::SceneAPI::SceneCore::BehaviorComponent);

        ~PrefabGroupBehavior() override = default;

        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::SceneAPI::Events::ProcessingResult OnPrepareForExport(AZ::SceneAPI::Events::PreExportEventContext& context) const;
        AZStd::unique_ptr<rapidjson::Document> CreateProductAssetData(const JE::Assets::Prefab::PrefabGroup* prefabGroup, const AZ::IO::Path& relativePath) const;

        bool WriteOutProductAsset(
            AZ::SceneAPI::Events::PreExportEventContext& context,
            const JE::Assets::Prefab::PrefabGroup* prefabGroup,
            const rapidjson::Document& doc) const;

        bool WriteOutProductAssetFile(
            const AZStd::string& filePath,
            AZ::SceneAPI::Events::PreExportEventContext& context,
            const JE::Assets::Prefab::PrefabGroup* prefabGroup,
            const rapidjson::Document& doc,
            bool debug) const;

        struct ExportEventHandler;
        AZStd::shared_ptr<ExportEventHandler> m_exportEventHandler;

        static constexpr int s_prefabGroupPreferredTabOrder{ 6 };
    };
}

