/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Document hosts FBX back-end data storage and access,
// loading and saving APIs.
//
/////////////////////////////////////////////////////////////////////////////

#include <string>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/function/function_fwd.h>
#include <JE/Assets/RenderBuilders/SceneProcessing/Rules/SurfaceIdentityRule.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AssetImporter/SaveUtilities/AsyncSaveRunner.h>

class QWidget;
class QObject;

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISceneNodeGroup;
            class IMeshGroup;
            class ISkeletonGroup;
            class ISkinGroup;
            class IAnimationGroup;
            class IMaterialRule;
            class IActorGroup;
            class IEFXMotionGroup;
        }
    }
}

class AssetImporterDocument
{
public:
    AssetImporterDocument();
    virtual ~AssetImporterDocument() = default;

    bool LoadScene(const AZStd::string& sceneFullPath);
    void SaveScene(AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete);
    void ClearScene();

    AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& GetScene();
    AZ::u64 GetEpoch() const { return m_epoch.load(); }
    using SurfaceRule = JE::Assets::RenderBuilders::SceneProcessing::Rules::SurfaceIdentityRule;
    using SurfaceIdentityChanged = AZStd::function<void(const AZ::SceneAPI::Containers::Scene&,
        const AZ::SceneAPI::DataTypes::IMeshGroup&)>;
    AZ::Outcome<bool, AZStd::string> ApplySurfaceIdentityChange(const AZ::Uuid& groupId,
        const AZStd::optional<SurfaceRule>& proposed, QObject* owner, AZStd::function<bool()> ownerIsCurrent,
        SurfaceIdentityChanged changed);
    
protected:
    void SaveManifest();

    AZStd::atomic<AZ::u64> m_epoch{0}; // LoadScene also runs on the existing import worker.
    AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> m_scene;
    AZStd::shared_ptr<AZ::AsyncSaveRunner> m_saveRunner;
};
