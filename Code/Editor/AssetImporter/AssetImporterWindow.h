#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Qt Interface
//
/////////////////////////////////////////////////////////////////////////////

#include <AzCore/Math/Guid.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/SceneSettingsCard.h>
#include <QFileSystemWatcher>
#include <QMainWindow>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>


namespace AzQtComponents
{
    class Card;
}

namespace AZStd
{
    class thread;
}

namespace Ui
{
    class AssetImporterWindow;
}

namespace AZ
{
    class Module;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IScriptProcessorRule;
        }
    }
}

class AssetImporterDocument;
class ImporterRootDisplayWidget;
class QCloseEvent;
class QMenu;
class QComboBox;
class QLineEdit;
class QPushButton;
class QAction;
class QVBoxLayout;
class QScrollArea;

class AssetImporterWindow
    : public QMainWindow
    , private AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
{
    Q_OBJECT
public:
    AssetImporterWindow();
    explicit AssetImporterWindow(QWidget* parent);
    ~AssetImporterWindow();

    // This implementation is required for unregister/register on "RegisterQtViewPane"
    static const GUID& GetClassID()
    {
        // {c50c09d6-5bfa-4d49-8542-e350656ed1bc}
        static const GUID guid = {
            0xc50c09d6, 0x5bfa, 0x4d49, { 0x85, 0x42, 0xe3, 0x50, 0x65, 0x6e, 0xd1, 0xbc }
        };
        return guid;
    }
    
    void OpenFile(const AZStd::string& filePath);
    
    bool CanClose();

public slots:
    void OnClearUnsavedChangesRequested();
    void OnSceneResetRequested();
    void OnAssignScript();
    void OnInspect();
    void SceneSettingsCardDestroyed();
    void SceneSettingsCardProcessingCompleted();

private:
    void Init();
    void GetAvailableModifiers(AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
        const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject& target) override;
    void ChangeSurfaceIdentity(AZ::SceneAPI::DataTypes::IManifestObject* owner,
        AZ::SceneAPI::DataTypes::IManifestObject* modifier, bool adding, bool& handled, bool& accepted);
    void RefreshRegionEditor() const;
    void SelectRegionEditorEntry() const;
    void ApplyRegionEditor();
    QWidget* m_regionPanel = nullptr;
    QComboBox* m_regionGroup = nullptr;
    QComboBox* m_regionSurface = nullptr;
    QComboBox* m_regionChoice = nullptr;
    QLineEdit* m_regionName = nullptr;
    QPushButton* m_regionApply = nullptr;
    AZ::u64 m_identityOwnerEpoch = 0;
    void OpenFileInternal(const AZStd::string& filePath);
    bool IsAllowedToChangeSourceFile();
    bool ShouldSaveBeforeClose();

    void HandleAssetLoadingCompleted();

    SceneSettingsCard* CreateSceneSettingsCard(
        QString fileName,
        SceneSettingsCard::Layout layout,
        SceneSettingsCard::State state);

    /// Reloads the currently loaded scene.
    /// warnUser: if true, always warns the user this operation is occuring. If false, only warn if there's a problem.
    void ReloadCurrentScene(bool warnUser);

private slots:
    void SaveClicked();
    
    void OverlayLayerAdded();
    void OverlayLayerRemoved();
    void UpdateDefaultSceneDisplay();
    void UpdateSceneDisplay(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene = {}) const;

    void FileChanged(QString path);

private:

    static const AZ::Uuid s_browseTag;
    static const char* s_documentationWebAddress;

    QScopedPointer<Ui::AssetImporterWindow> ui;
    QScopedPointer<AssetImporterDocument> m_assetImporterDocument;
    QScopedPointer<AZ::SceneAPI::UI::OverlayWidget> m_overlay;
    int m_openSceneSettingsCards = 0;
    int m_sceneSettingsCardOverlay = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;

    // Monitor the scene file, and scene settings file in case they are changed outside the scene settings tool.
    QFileSystemWatcher m_qtFileWatcher;

    AZ::SerializeContext* m_serializeContext;
    AZStd::string m_fullSourcePath;

    QScopedPointer<ImporterRootDisplayWidget> m_rootDisplay;
    bool m_isClosed;
    bool m_isSaving = false;

    AZStd::string m_scriptProcessorRuleFilename;
};
