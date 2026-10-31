/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporter/AssetImporterWindow.h>
#include <AssetImporter/ui_AssetImporterWindow.h>
#include <AssetImporter/AssetImporterTool.h>
#include <AssetImporter/ImporterRootDisplay.h>

#include <QCloseEvent>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QMessageBox>
#include <QTimer>
#include <QPointer>
#include <QThread>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>

class IXMLDOMDocumentPtr; // Needed for settings.h
class CXTPDockingPaneLayout; // Needed for settings.h
#include <Settings.h>

#include <QScrollArea>

#include <AssetImporter/ActionOutput.h>
#include <AssetImporter/AssetImporterDocument.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <Util/PathUtil.h>

#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Rules/ScriptProcessorRule.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphInspectWidget.h>

const char* AssetImporterWindow::s_documentationWebAddress = "https://www.o3de.org/docs/user-guide/assets/scene-settings/";
const AZ::Uuid AssetImporterWindow::s_browseTag = AZ::Uuid::CreateString("{C240D2E1-BFD2-4FFA-BB5B-CC0FA389A5D3}");

AssetImporterWindow::AssetImporterWindow()
    : AssetImporterWindow(nullptr)
{
}

AssetImporterWindow::AssetImporterWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AssetImporterWindow)
    , m_assetImporterDocument(new AssetImporterDocument())
    , m_serializeContext(nullptr)
    , m_rootDisplay(nullptr)
    , m_overlay(nullptr)
    , m_isClosed(false)
{
    Init();
}

AssetImporterWindow::~AssetImporterWindow()
{
    m_isClosed = true; ++m_identityOwnerEpoch;
    AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
    while (QLayoutItem* cardToDelete = ui->m_cardAreaLayout->takeAt(0))
    {
        delete cardToDelete->widget();
        delete cardToDelete;
    }
}

void AssetImporterWindow::OpenFile(const AZStd::string& filePath)
{
    if (m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex)
    {
        QMessageBox::warning(this, "In progress", "Please wait for the previous task to complete before opening a new file.");
        return;
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }

    // Make sure we are not browsing *over* a current editing operation
    if (!IsAllowedToChangeSourceFile())
    {
        // Issue will already have been reported to the user.
        return;
    }

    if (!m_overlay->PopAllLayers())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }

    OpenFileInternal(filePath);
}

bool AssetImporterWindow::CanClose()
{
    if (m_isClosed)
    {
        return true;
    }

    if (m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex)
    {
        QMessageBox::critical(this, "Processing In Progress", "Please wait until processing has completed to try again.",
            QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::critical(this, "Unable to close", "Unable to close one or more windows at this time.",
            QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }

    if (ShouldSaveBeforeClose())
    {
        return false;
    }

    m_isClosed = true; ++m_identityOwnerEpoch;
    return true;
}

void AssetImporterWindow::Init()
{
    // Serialization and reflection framework setup
    AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(m_serializeContext, "Serialization context not available");

    // Load the style sheets
    AzQtComponents::StylesheetPreprocessor styleSheetProcessor(nullptr);

    auto mainWindowQSSPath = AZ::IO::Path(AZ::Utils::GetEnginePath()) / "Assets";
    mainWindowQSSPath /= "Editor/Styles/AssetImporterWindow.qss";
    mainWindowQSSPath.MakePreferred();
    QFile mainWindowStyleSheetFile(mainWindowQSSPath.c_str());
    if (mainWindowStyleSheetFile.open(QFile::ReadOnly))
    {
        setStyleSheet(styleSheetProcessor.ProcessStyleSheet(mainWindowStyleSheetFile.readAll()));
    }

    ui->setupUi(this);
        
    // Setup the overlay system, and set the root to be the root display. The root display has the browse,
    //  the Import button & the cancel button, which are handled here by the window.
    m_overlay.reset(aznew AZ::SceneAPI::UI::OverlayWidget(this));
    m_rootDisplay.reset(aznew ImporterRootDisplayWidget(m_serializeContext));
    connect(m_rootDisplay->GetManifestWidget(), &AZ::SceneAPI::UI::ManifestWidget::RequestModifierChange,
        this, &AssetImporterWindow::ChangeSurfaceIdentity, Qt::DirectConnection);
    AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
    connect(
        m_rootDisplay.data()->GetManifestWidget(),
        &AZ::SceneAPI::UI::ManifestWidget::SaveClicked,
        this,
        &AssetImporterWindow::SaveClicked);
    connect(
        m_rootDisplay.data()->GetManifestWidget(),
        &AZ::SceneAPI::UI::ManifestWidget::OnInspect,
        this,
        &AssetImporterWindow::OnInspect);
    connect(
        m_rootDisplay.data()->GetManifestWidget(),
        &AZ::SceneAPI::UI::ManifestWidget::OnSceneResetRequested,
        this,
        &AssetImporterWindow::OnSceneResetRequested);
    connect(
        m_rootDisplay.data()->GetManifestWidget(),
        &AZ::SceneAPI::UI::ManifestWidget::OnClearUnsavedChangesRequested,
        this,
        &AssetImporterWindow::OnClearUnsavedChangesRequested);
    connect(
        m_rootDisplay.data()->GetManifestWidget(),
        &AZ::SceneAPI::UI::ManifestWidget::OnAssignScript,
        this,
        &AssetImporterWindow::OnAssignScript);

    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerAdded, this, &AssetImporterWindow::OverlayLayerAdded);
    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerRemoved, this, &AssetImporterWindow::OverlayLayerRemoved);

    m_overlay->SetRoot(m_rootDisplay.data());
    ui->m_settingsAreaLayout->addWidget(m_overlay.data());
    // Inline authoring in Scene Settings: display names, never editable UUIDs.
    m_regionPanel = new QGroupBox(tr("Surface Identity — Regions"), this);
    auto* regionLayout = new QFormLayout(m_regionPanel);
    regionLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_regionGroup = new QComboBox(m_regionPanel); m_regionSurface = new QComboBox(m_regionPanel);
    m_regionChoice = new QComboBox(m_regionPanel); m_regionName = new QLineEdit(m_regionPanel);
    m_regionApply = new QPushButton(tr("Apply"), m_regionPanel);
    m_regionGroup->setObjectName("surfaceRegionMeshGroup"); m_regionSurface->setObjectName("surfaceRegionSurface");
    m_regionChoice->setObjectName("surfaceRegionChoice"); m_regionName->setObjectName("surfaceRegionName");
    m_regionApply->setObjectName("surfaceRegionApply");
    regionLayout->addRow(tr("Mesh group"), m_regionGroup); regionLayout->addRow(tr("Source surface"), m_regionSurface);
    regionLayout->addRow(tr("Region"), m_regionChoice); regionLayout->addRow(tr("Name"), m_regionName);
    regionLayout->addRow(m_regionApply);
    ui->m_settingsAreaLayout->addWidget(m_regionPanel);
    connect(m_regionGroup, &QComboBox::currentIndexChanged, this, [this]() { RefreshRegionEditor(); });
    connect(m_regionSurface, &QComboBox::currentIndexChanged, this, [this]() { RefreshRegionEditor(); });
    connect(m_regionChoice, &QComboBox::currentIndexChanged, this, [this]() { SelectRegionEditorEntry(); });
    connect(m_regionApply, &QPushButton::clicked, this, &AssetImporterWindow::ApplyRegionEditor);
    RefreshRegionEditor();

    // Filling the initial browse prompt text to be programmatically set from available extensions
    AZStd::unordered_set<AZStd::string> extensions;
    AZ::SceneAPI::Events::AssetImportRequestBus::Broadcast(
        &AZ::SceneAPI::Events::AssetImportRequestBus::Events::GetSupportedFileExtensions, extensions);
    AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, !extensions.empty(), "No file extensions defined for assets.");
    if (!extensions.empty())
    {
        for (AZStd::string& extension : extensions)
        {
            extension = extension.substr(1);
            AZStd::to_upper(extension.begin(), extension.end());
        }

        AZStd::string joinedExtensions;
        AzFramework::StringFunc::Join(joinedExtensions, extensions.begin(), extensions.end(), " or ");

        AZStd::string firstLineText =
            AZStd::string::format(
                "%s files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>",
                joinedExtensions.c_str(), s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText =
            AZStd::string::format("To adjust the %s settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.", joinedExtensions.c_str());
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());
    }
    else
    {
        AZStd::string firstLineText =
            AZStd::string::format(
                "Files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>", s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText = "To adjust the settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.";
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());

        // Hide the initial browse container so we can show the error (it will be shown again when the overlay pops)
        ui->m_initialBrowseContainer->hide();

        QMessageBox::critical(this, "No Extensions Detected",
            "No importable file types were detected. This likely means an internal error has taken place which has broken the "
            "registration of valid import types (e.g. FBX). This type of issue requires engineering support.");
    }
    
    QObject::connect(&m_qtFileWatcher, &QFileSystemWatcher::fileChanged, this, &AssetImporterWindow::FileChanged);
}

void AssetImporterWindow::OpenFileInternal(const AZStd::string& filePath)
{
    ++m_identityOwnerEpoch;
    using namespace AZ::SceneAPI::SceneUI;

    // Clear all previously watched files
    m_qtFileWatcher.removePaths(m_qtFileWatcher.files());

    auto asyncLoadHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::AsyncOperationProcessingHandler>(
        s_browseTag,
        [this, filePath]()
        {
            // this is invoked across threads, so ensure that nothing touches the main thread that isn't thread safe.
            // Qt objects, in particular, should talk via timers or queued connections.
            m_assetImporterDocument->LoadScene(filePath);
            QMetaObject::invokeMethod(this, &AssetImporterWindow::UpdateDefaultSceneDisplay, Qt::QueuedConnection);
        },
        [this]()
        {
            QMetaObject::invokeMethod(this, &AssetImporterWindow::HandleAssetLoadingCompleted, Qt::QueuedConnection);
        }, this);
        
    QFileInfo fileInfo(filePath.c_str());
    SceneSettingsCard* card = CreateSceneSettingsCard(fileInfo.fileName(), SceneSettingsCard::Layout::Loading, SceneSettingsCard::State::Loading);
    card->SetAndStartProcessingHandler(asyncLoadHandler);

}

SceneSettingsCard* AssetImporterWindow::CreateSceneSettingsCard(
    QString fileName,
    SceneSettingsCard::Layout layout,
    SceneSettingsCard::State state)
{
    while (QLayoutItem* cardToDelete = ui->m_cardAreaLayout->takeAt(0))
    {
        delete cardToDelete->widget();
        delete cardToDelete;
    }

    SceneSettingsCard* card = new SceneSettingsCard(s_browseTag, fileName, layout, ui->m_cardAreaLayoutWidget);
    card->setExpanded(false);
    ui->m_notificationAreaLayoutWidget->show();
    card->SetState(state);
    ui->m_cardAreaLayout->addWidget(card);
    ui->m_cardAreaLayoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    ++m_openSceneSettingsCards;
    connect(card, &QObject::destroyed, this, &AssetImporterWindow::SceneSettingsCardDestroyed);

    connect(card, &SceneSettingsCard::ProcessingCompleted, this, &AssetImporterWindow::SceneSettingsCardProcessingCompleted);

    // Not passing in a QLabel to display because with a QLabel it won't darken the rest of the interface, which is preferred.
    m_sceneSettingsCardOverlay = m_overlay->PushLayer(nullptr, nullptr, "Waiting for file to finish processing", AzQtComponents::OverlayWidgetButtonList());
    return card;
}

void AssetImporterWindow::SceneSettingsCardDestroyed()
{
    if (m_isClosed)
    {
        return;
    }
    if (m_openSceneSettingsCards > 0)
    {
        --m_openSceneSettingsCards;
    }
    if (m_openSceneSettingsCards <= 0)
    {
        ui->m_notificationAreaLayoutWidget->hide();
    }
}

void AssetImporterWindow::SceneSettingsCardProcessingCompleted()
{
    if (m_isClosed)
    {
        return;
    }
    
    m_isSaving = false;
    m_overlay->PopLayer(m_sceneSettingsCardOverlay);
    m_sceneSettingsCardOverlay = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
    RefreshRegionEditor(); // PopLayer emits LayerRemoved before this overlay ID is invalidated.
}

bool AssetImporterWindow::IsAllowedToChangeSourceFile()
{
    if (!m_rootDisplay->HasUnsavedChanges())
    {
        return true;
    }

    const int result = QMessageBox::question(
        this,
        tr("Save Asset Changes?"),
        tr("Changes have been made to the asset in the Inspector Scene Settings. Would you like to save these changes prior to switching assets?"),
        QMessageBox::Yes,
        QMessageBox::No);

    if (result == QMessageBox::No)
    {
        return true;
    }
    m_isSaving = true;
    RefreshRegionEditor();
    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_assetImporterDocument->SaveScene(
        output,
        [](bool wasSuccessful)
        {
            if (!wasSuccessful)
            {
                QMessageBox messageBox(
                    QMessageBox::Icon::Warning,
                    tr("Failed to save"),
                    tr("An error has been encountered saving this file. See the logs for details."));
                messageBox.exec();
            }
        });

    return true;
}

bool AssetImporterWindow::ShouldSaveBeforeClose()
{
    if (!m_rootDisplay->HasUnsavedChanges())
    {
        return false;
    }

    const int result = QMessageBox::question(
        this,
        tr("Save Asset Changes?"),
        tr("Changes have been made to the asset in the Inspector Scene Settings. Would you like to save these changes prior to closing the window?"),
        QMessageBox::Yes,
        QMessageBox::No);

    if (result == QMessageBox::No)
    {
        return false;
    }
    m_isSaving = true;
    RefreshRegionEditor();
    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_assetImporterDocument->SaveScene(
        output,
        [this](bool wasSuccessful)
        {
            if (!wasSuccessful)
            {
                QMessageBox messageBox(
                    QMessageBox::Icon::Warning,
                    tr("Failed to save"),
                    tr("An error has been encountered saving this file. See the logs for details."));
                messageBox.exec();
            }

            QWidget* dock = parentWidget();
            while (dock)
            {
                QDockWidget* converted = qobject_cast<QDockWidget*>(dock);
                if (converted)
                {
                    m_isClosed = true;
                    converted->deleteLater();
                    break;
                }
                else
                {
                    dock = dock->parentWidget();
                }
            }
        });

    return true;
}

void AssetImporterWindow::SaveClicked()
{
    using namespace AZ::SceneAPI::SceneUI;

    // There are specific measures in place to block re-entry, applying asserts to be safe
    if (m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex)
    {
        return;
    }
    else if (!m_scriptProcessorRuleFilename.empty())
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "A script updates the manifest; will not save.");
        QMessageBox messageBox(
            QMessageBox::Icon::Warning,
            tr("Failed to save"),
            tr("A script updates this file; will not save."));
        messageBox.exec();
        return;
    }

    SceneSettingsCard* card = CreateSceneSettingsCard(m_rootDisplay->GetHeaderFileName(), SceneSettingsCard::Layout::Exporting, SceneSettingsCard::State::Processing);

    bool isSourceControlActive = false;
    {
        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::BroadcastResult(isSourceControlActive, &SCRequestBus::Events::IsActive);
    }

    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_isSaving = true;
    RefreshRegionEditor();
    m_assetImporterDocument->SaveScene(output,
        [output, this, isSourceControlActive, card](bool wasSuccessful)
        {
            m_rootDisplay->UpdateTimeStamp(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str(), gSettings.enableSceneInspector);
            if (output->HasAnyWarnings())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "%s", output->BuildWarningMessage().c_str());
            }
            if (output->HasAnyErrors())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "%s", output->BuildErrorMessage().c_str());
            }

            if (wasSuccessful)
            {
                if (!isSourceControlActive)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::SuccessWindow, "Saving complete");
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::SuccessWindow, "Saving & source control operations complete");
                }

                m_rootDisplay->HandleSaveWasSuccessful();

                // Don't attach the job processor until all files are saved.
                card->SetAndStartProcessingHandler(AZStd::make_shared<ExportJobProcessingHandler>(s_browseTag, m_fullSourcePath));
            }
        }
    );
}

void AssetImporterWindow::OnClearUnsavedChangesRequested()
{
    ReloadCurrentScene(false);
}

void AssetImporterWindow::OnSceneResetRequested()
{
    using namespace AZ::SceneAPI::Events;
    using namespace AZ::SceneAPI::SceneUI;
    using namespace AZ::SceneAPI::Utilities;

    auto asyncLoadHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::AsyncOperationProcessingHandler>(
        s_browseTag,
        [this]()
        {
            m_assetImporterDocument->GetScene()->GetManifest().Clear();

            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            AssetImportRequestBus::BroadcastResult(
                result,
                &AssetImportRequestBus::Events::UpdateManifest,
                *m_assetImporterDocument->GetScene(),
                AssetImportRequest::ManifestAction::ConstructDefault,
                AssetImportRequest::RequestingApplication::Editor);

            // Specifically using success, because ignore would be an invalid case.
            // Whenever we do construct default, it should always be done
            if (result.GetResult() == ProcessingResult::Success)
            {
                AZ_TracePrintf(SuccessWindow, "Successfully reset the manifest.");
            }
            else
            {
                m_assetImporterDocument->ClearScene();
                AZ_TracePrintf(ErrorWindow, "Manifest reset returned in '%s'",
                    result.GetResult() == ProcessingResult::Failure ? "Failure" : "Ignored");
            }
        },
        [this]()
        {
            m_rootDisplay->HandleSceneWasReset(m_assetImporterDocument->GetScene());
        }, this);

    // reset the script rule from the .assetinfo file if it exists
    if (!m_scriptProcessorRuleFilename.empty())
    {
        m_scriptProcessorRuleFilename.clear();
        if (QFile::exists(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str()))
        {
            QFile file(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str());
            file.remove();
        }
    }

    SceneSettingsCard* card = CreateSceneSettingsCard(m_rootDisplay->GetHeaderFileName(), SceneSettingsCard::Layout::Resetting, SceneSettingsCard::State::Loading);
    card->SetAndStartProcessingHandler(asyncLoadHandler);
}

void AssetImporterWindow::OnAssignScript()
{
    using namespace AZ::SceneAPI;
    using namespace AZ::SceneAPI::Events;
    using namespace AZ::SceneAPI::SceneUI;
    using namespace AZ::SceneAPI::Utilities;

    // use QFileDialog to select a Python script to embed into a scene manifest file
    QString pyFilename = QFileDialog::getOpenFileName(this,
        tr("Select scene builder Python script"),
        Path::GetEditingGameDataFolder().c_str(),
        tr("Python (*.py)"));

    if (pyFilename.isNull())
    {
        return;
    }

    // reset the script rule from the .assetinfo file if it exists
    if (!m_scriptProcessorRuleFilename.empty())
    {
        m_scriptProcessorRuleFilename.clear();
        if (QFile::exists(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str()))
        {
            QFile file(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str());
            file.remove();
        }
    }

    // find the path relative to the project folder
    pyFilename = Path::GetRelativePath(pyFilename, true);

    // create a script rule
    auto scriptProcessorRule = AZStd::make_shared<SceneData::ScriptProcessorRule>();
    scriptProcessorRule->SetScriptFilename(pyFilename.toUtf8().toStdString().c_str());

    // add the script rule to the manifest & save off the scene manifest
    Containers::SceneManifest sceneManifest;
    sceneManifest.AddEntry(scriptProcessorRule);
    if (sceneManifest.SaveToFile(m_assetImporterDocument->GetScene()->GetManifestFilename()))
    {
        OpenFile(m_assetImporterDocument->GetScene()->GetSourceFilename());
    }
}

void AssetImporterWindow::OnInspect()
{
    AZ::SceneAPI::UI::OverlayWidgetButtonList buttons;
    AZ::SceneAPI::UI::OverlayWidgetButton closeButton;
    closeButton.m_text = "Close";
    closeButton.m_triggersPop = true;
    buttons.push_back(&closeButton);

    QLabel* label = new QLabel("Please close the inspector to continue editing the settings.");
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    // make sure the inspector doesn't outlive the AssetImporterWindow, since we own the data it will be inspecting.
    auto* theInspectWidget = aznew AZ::SceneAPI::UI::SceneGraphInspectWidget(*m_assetImporterDocument->GetScene());
    QObject::connect(this, &QObject::destroyed, theInspectWidget, [theInspectWidget]() { theInspectWidget->window()->close(); } );

    m_overlay->PushLayer(label, theInspectWidget, "Scene Inspector", buttons);
}

void AssetImporterWindow::OverlayLayerAdded()
{
    setCursor(Qt::WaitCursor);
    if (m_regionPanel) { m_regionPanel->setEnabled(false); }
}

void AssetImporterWindow::OverlayLayerRemoved()
{
    if (m_isClosed && !m_overlay->IsAtRoot())
    {
        return;
    }

    setCursor(Qt::ArrowCursor);

    if (!m_assetImporterDocument->GetScene())
    {
        ui->m_initialBrowseContainer->show();
        m_rootDisplay->hide();
    }
    RefreshRegionEditor();
}

void AssetImporterWindow::UpdateDefaultSceneDisplay()
{
    UpdateSceneDisplay({});
}

void AssetImporterWindow::UpdateSceneDisplay(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene) const
{
    QString sceneHeaderText;
    if(scene.get())
    {
        sceneHeaderText = QString::fromUtf8(scene->GetManifestFilename().c_str(), static_cast<int>(scene->GetManifestFilename().size()));
    }

    if (scene)
    {
        m_rootDisplay->SetSceneDisplay(sceneHeaderText, scene);
    }
    else
    {
        m_rootDisplay->SetSceneHeaderText(sceneHeaderText);
    }
    
    m_rootDisplay->SetPythonBuilderText(m_scriptProcessorRuleFilename.c_str());
    RefreshRegionEditor();

    // UpdateSceneDisplay gets called both when the file is saved from this tool, as well as when it's modified externally.
    m_rootDisplay->UpdateTimeStamp(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str(), gSettings.enableSceneInspector);

}

void AssetImporterWindow::HandleAssetLoadingCompleted()
{
    if (!m_assetImporterDocument->GetScene())
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to load scene.");
        return;
    }

    m_fullSourcePath = m_assetImporterDocument->GetScene()->GetSourceFilename();

    using namespace AZ::SceneAPI;
    m_scriptProcessorRuleFilename.clear();

    // load up the source scene manifest file
    Containers::SceneManifest sceneManifest;
    if (sceneManifest.LoadFromFile(m_assetImporterDocument->GetScene()->GetManifestFilename()))
    {
        // check a Python script rule is in that source manifest
        auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(sceneManifest.GetValueStorage());
        if (!view.empty())
        {
            // record the info about the rule in the class
            const auto scriptProcessorRule = &*view.begin();
            m_scriptProcessorRuleFilename = scriptProcessorRule->GetScriptFilename();
        }
    }

    UpdateSceneDisplay(m_assetImporterDocument->GetScene());

    // Once we've browsed to something successfully, we need to hide the initial browse button layer and
    //  show the main area where all the actual work takes place
    ui->m_initialBrowseContainer->hide();
    m_rootDisplay->show();

   
    m_qtFileWatcher.addPath(m_fullSourcePath.c_str());
    m_qtFileWatcher.addPath(m_assetImporterDocument->GetScene()->GetManifestFilename().c_str());
}

void AssetImporterWindow::ReloadCurrentScene(bool warnUser)
{
    if (m_isSaving)
    {
        return;
    }

    QString promptMessage(
        [this]()
        {
            if (m_rootDisplay->HasUnsavedChanges())
            {
                return tr("The file %1 has been changed outside of the scene settings tool. This tool will be reloaded and any unsaved "
                          "changes will be lost. \n\n"
                          "To prevent this from occuring in the future, do not modify the scene file or scene manifest outside of this "
                          "tool while this tool has unsaved work.");
            }
            return tr("The file %1 has been changed outside of the scene settings tool. This tool will be reloaded.");
        }());

    // The scene system holds weak pointers to any previously loaded scenes,
    // and will return a previously cached scene on a requested load.
    // In this case, it's known the scene file is different than what's in memory, so make sure to flush
    // any cached scene info, so it is freshly reloaded from disk.
    m_assetImporterDocument->ClearScene();
    m_rootDisplay->GetManifestWidget()->ResetScene();

    // Verify nothing is left holding a shared pointer to the scene.
    namespace SceneEvents = AZ::SceneAPI::Events;
    bool foundSharedScene = true; // If the ebus fails, default to true to assume there's something sharing the scene still.
    SceneEvents::SceneSerializationBus::BroadcastResult(
        foundSharedScene, &SceneEvents::SceneSerializationBus::Events::IsSceneCached, m_fullSourcePath);

    // The scene is still cached, somewhere. Warn the user.
    if (foundSharedScene)
    {
        QString sharedSceneWarningMessage(tr("This scene file is still cached and will not reload correctly. The Editor should be shut down and "
                            "re-launched to properly load the modified external data."));
        if (warnUser)
        {
            promptMessage = QString("%1\n\n%2").arg(promptMessage).arg(sharedSceneWarningMessage);
        }
        else
        {
            promptMessage = sharedSceneWarningMessage;
        }
    }

    if (warnUser || foundSharedScene)
    {
        QMessageBox::question(
            this,
            tr("Reloading Scene Settings"),
            promptMessage.arg(m_fullSourcePath.c_str()),
            QMessageBox::StandardButtons(QMessageBox::StandardButton::Ok));
    }

    OpenFileInternal(m_fullSourcePath);
}

void AssetImporterWindow::FileChanged([[maybe_unused]] QString path)
{
    ReloadCurrentScene(true);
}



void AssetImporterWindow::GetAvailableModifiers(AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
    const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject& target)
{
    using Rule = AssetImporterDocument::SurfaceRule;
    if (QThread::currentThread() != thread() || m_isClosed || m_isSaving ||
        m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex || !m_assetImporterDocument ||
        m_assetImporterDocument->GetScene().get() != &scene) { return; }
    const auto* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(&target);
    if (group && !group->GetRuleContainerConst().ContainsRuleOfType<Rule>())
    { modifiers.push_back(azrtti_typeid<Rule>()); }
}

void AssetImporterWindow::ChangeSurfaceIdentity(AZ::SceneAPI::DataTypes::IManifestObject* owner,
    AZ::SceneAPI::DataTypes::IManifestObject* modifier, bool adding, bool& handled, bool& accepted)
{
    using Rule = AssetImporterDocument::SurfaceRule;
    if (!azrtti_istypeof<Rule>(modifier)) { return; }
    handled = true; accepted = false; // Never fall back to inserting an empty identity rule.
    if (QThread::currentThread() != thread() || m_isClosed || m_isSaving ||
        m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex || !m_assetImporterDocument) { return; }
    const auto scene = m_assetImporterDocument->GetScene();
    const auto* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(owner);
    if (!scene || !group || group->GetId().IsNull()) { return; }
    size_t matchingId = 0; bool exactOwner = false;
    for (const auto& object : scene->GetManifest().GetValueStorage())
    {
        if (const auto* candidate = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(object.get());
            candidate && candidate->GetId() == group->GetId())
        { ++matchingId; exactOwner = exactOwner || candidate == group; }
    }
    if (!exactOwner || matchingId != 1) { return; }
    AZStd::optional<Rule> proposed;
    if (adding)
    {
        const auto prepared = Rule::PrepareForMeshGroup(*scene, *group);
        if (!prepared.IsSuccess()) { QMessageBox::warning(this, tr("Surface Identity"), QString::fromUtf8(prepared.GetError().c_str())); return; }
        proposed = prepared.GetValue();
    }
    else
    {
        bool exactRule = false;
        for (size_t i = 0; i != group->GetRuleContainerConst().GetRuleCount(); ++i)
        { exactRule = exactRule || group->GetRuleContainerConst().GetRule(i).get() == modifier; }
        if (!exactRule) { return; }
    }
    const QPointer<AssetImporterWindow> guard(this);
    const auto ownerEpoch = m_identityOwnerEpoch; const auto documentEpoch = m_assetImporterDocument->GetEpoch();
    const auto isCurrent = [guard, ownerEpoch, documentEpoch]()
    {
        return guard && !guard->m_isClosed && !guard->m_isSaving && guard->m_identityOwnerEpoch == ownerEpoch &&
            guard->m_sceneSettingsCardOverlay == AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex &&
            guard->m_assetImporterDocument && guard->m_assetImporterDocument->GetEpoch() == documentEpoch;
    };
    const auto changed = [guard, isCurrent](const AZ::SceneAPI::Containers::Scene& current,
        const AZ::SceneAPI::DataTypes::IMeshGroup& changedGroup)
    {
        if (!isCurrent()) { return; }
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Broadcast(
            &AZ::SceneAPI::Events::ManifestMetaInfoBus::Events::ObjectUpdated, current, &changedGroup, guard.data());
        if (!isCurrent()) { return; } // ObjectUpdated listeners may have closed or reloaded this document.
        // Do not destroy the row/property control while its Add/Remove callback is on the stack.
        QTimer::singleShot(0, guard.data(), [guard, isCurrent]()
        {
            if (isCurrent())
            {
                guard->m_rootDisplay->GetManifestWidget()->BuildFromScene(guard->m_assetImporterDocument->GetScene());
                if (isCurrent()) { guard->RefreshRegionEditor(); }
            }
        });
    };
    const auto result = m_assetImporterDocument->ApplySurfaceIdentityChange(group->GetId(), proposed, this, isCurrent, changed);
    if (!result.IsSuccess()) { QMessageBox::warning(this, tr("Surface Identity"), QString::fromUtf8(result.GetError().c_str())); return; }
    accepted = true;
}

namespace
{
    const AZ::SceneAPI::DataTypes::IMeshGroup* RegionEditingGroup(const AZ::SceneAPI::Containers::Scene& scene, const QString& id)
    {
        const auto text = id.toUtf8();
        const auto uuid = AZ::Uuid::CreateString(AZStd::string_view(text.constData(), text.size()));
        if (uuid.IsNull()) { return nullptr; }
        const AZ::SceneAPI::DataTypes::IMeshGroup* result = nullptr;
        for (const auto& value : scene.GetManifest().GetValueStorage())
        {
            const auto* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(value.get());
            if (group && group->GetId() == uuid) { if (result) { return nullptr; } result = group; }
        }
        return result;
    }
}
void AssetImporterWindow::RefreshRegionEditor() const
{
    if (!m_regionPanel) { return; }
    using Rule = AssetImporterDocument::SurfaceRule;
    m_regionPanel->setEnabled(false);
    // Keep the selection while busy; do not read a scene that the loading worker is replacing.
    if (m_isClosed || m_isSaving || !m_overlay || !m_overlay->IsAtRoot() ||
        m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex || !m_assetImporterDocument) { return; }
    const auto scene = m_assetImporterDocument->GetScene();
    if (!scene) { return; }
    const QSignalBlocker groups(m_regionGroup), surfaces(m_regionSurface), regions(m_regionChoice);
    const auto groupId = m_regionGroup->currentData().toString(); const auto surfaceId = m_regionSurface->currentData().toString();
    m_regionGroup->clear(); m_regionSurface->clear(); m_regionChoice->clear(); m_regionName->clear();
    for (const auto& value : scene->GetManifest().GetValueStorage())
    {
        const auto* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshGroup*>(value.get());
        if (!group) { continue; }
        const auto rule = group->GetRuleContainerConst().FindFirstByType<Rule>();
        if (rule && Rule::MatchesMeshGroup(*scene, *group, *rule))
        {
            m_regionGroup->addItem(QString::fromUtf8(group->GetName().c_str()), QString::fromUtf8(group->GetId().ToString<AZStd::string>().c_str()));
        }
    }
    if (const int selected = m_regionGroup->findData(groupId); selected >= 0) { m_regionGroup->setCurrentIndex(selected); }
    const auto* group = RegionEditingGroup(*scene, m_regionGroup->currentData().toString());
    if (!group) { return; }
    const auto rule = group->GetRuleContainerConst().FindFirstByType<Rule>();
    if (!rule || !Rule::MatchesMeshGroup(*scene, *group, *rule)) { return; }
    AZStd::unordered_set<AZ::Uuid> seen;
    for (const auto& node : rule->m_nodes)
    {
        for (const auto& face : node.m_faces)
        {
            if (seen.insert(face.m_surfaceId).second)
            {
                // Source paths are authored labels. The UUID stays internal in item data.
                QString label = QString::fromUtf8(node.m_nodePath.c_str());
                if (m_regionSurface->findText(label) >= 0) { label += tr(" (surface %1)").arg(m_regionSurface->count() + 1); }
                m_regionSurface->addItem(label, QString::fromUtf8(face.m_surfaceId.ToString<AZStd::string>().c_str()));
            }
        }
    }
    if (const int selected = m_regionSurface->findData(surfaceId); selected >= 0) { m_regionSurface->setCurrentIndex(selected); }
    const auto surfaceText = m_regionSurface->currentData().toString().toUtf8();
    const auto selectedSurface = AZ::Uuid::CreateString(AZStd::string_view(surfaceText.constData(), surfaceText.size()));
    AZStd::optional<AZ::Uuid> membership; size_t count = 0; bool mixed = false;
    m_regionChoice->addItem(tr("No region"), QString());
    AZStd::unordered_set<AZ::Uuid> regionIds;
    for (const auto& region : rule->m_regions)
    {
        regionIds.insert(region.m_id);
        m_regionChoice->addItem(QString::fromUtf8(region.m_name.c_str()), QString::fromUtf8(region.m_id.ToString<AZStd::string>().c_str()));
        m_regionChoice->setItemData(m_regionChoice->count() - 1, QString::fromUtf8(region.m_name.c_str()), Qt::UserRole + 1);
    }
    for (const auto& node : rule->m_nodes)
    {
        for (const auto& face : node.m_faces)
        {
            if (face.m_regionId)
            {
                const QString id = QString::fromUtf8(face.m_regionId->ToString<AZStd::string>().c_str());
                if (regionIds.insert(*face.m_regionId).second)
                {
                    // Explicit missing-label status, not an invented persistent region name.
                    m_regionChoice->addItem(tr("Unnamed region — %1").arg(QString::fromUtf8(node.m_nodePath.c_str())), id);
                }
            }
            if (face.m_surfaceId == selectedSurface)
            {
                if (count++ == 0) { membership = face.m_regionId; } else { mixed = mixed || membership != face.m_regionId; }
            }
        }
    }
    m_regionChoice->addItem(tr("New region…"), QStringLiteral("new"));
    const QString selectedRegion = membership ? QString::fromUtf8(membership->ToString<AZStd::string>().c_str()) : QString();
    m_regionChoice->setCurrentIndex(mixed ? -1 : m_regionChoice->findData(selectedRegion));
    m_regionChoice->setPlaceholderText(tr("Multiple regions"));
    m_regionPanel->setEnabled(count != 0);
    SelectRegionEditorEntry();
}
void AssetImporterWindow::SelectRegionEditorEntry() const
{
    const auto id = m_regionChoice->currentData().toString();
    m_regionName->setEnabled(!id.isEmpty());
    m_regionName->setText(m_regionChoice->currentData(Qt::UserRole + 1).toString());
    m_regionName->setPlaceholderText(tr("Region name"));
    m_regionApply->setEnabled(m_regionChoice->currentIndex() >= 0);
    m_regionApply->setText(id == "new" ? tr("Create and assign") : id.isEmpty() ? tr("Clear region") : tr("Apply"));
}
void AssetImporterWindow::ApplyRegionEditor()
{
    using Rule = AssetImporterDocument::SurfaceRule;
    if (QThread::currentThread() != thread() || m_isClosed || m_isSaving || !m_assetImporterDocument ||
        m_sceneSettingsCardOverlay != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex || !m_overlay || !m_overlay->IsAtRoot() ||
        m_regionChoice->currentIndex() < 0) { return; }
    const auto scene = m_assetImporterDocument->GetScene(); if (!scene) { return; }
    const auto* group = RegionEditingGroup(*scene, m_regionGroup->currentData().toString()); if (!group) { return; }
    const auto original = group->GetRuleContainerConst().FindFirstByType<Rule>();
    if (!original || !Rule::MatchesMeshGroup(*scene, *group, *original)) { return; }
    const auto encoded = m_regionSurface->currentData().toString().toUtf8();
    const auto surface = AZ::Uuid::CreateString(AZStd::string_view(encoded.constData(), encoded.size()));
    bool found = false;
    for (const auto& node : original->m_nodes) { for (const auto& face : node.m_faces) { found = found || face.m_surfaceId == surface; } }
    if (!found) { return; }
    const auto id = m_regionChoice->currentData().toString();
    Rule proposed = *original; AZStd::optional<AZ::Uuid> region;
    if (!id.isEmpty())
    {
        const auto name = m_regionName->text().trimmed().toUtf8();
        const AZStd::string_view label(name.constData(), name.size());
        if (id == "new")
        {
            const auto created = proposed.CreateRegion(label);
            if (!created.IsSuccess()) { QMessageBox::warning(this, tr("Surface regions"), QString::fromUtf8(created.GetError().c_str())); return; }
            region = created.GetValue();
        }
        else
        {
            const auto regionText = id.toUtf8(); region = AZ::Uuid::CreateString(AZStd::string_view(regionText.constData(), regionText.size()));
            const auto named = proposed.SetRegionName(*region, label);
            if (!named.IsSuccess()) { QMessageBox::warning(this, tr("Surface regions"), QString::fromUtf8(named.GetError().c_str())); return; }
        }
    }
    const auto edited = proposed.SetRegionForSurface(surface, region);
    if (!edited.IsSuccess()) { QMessageBox::warning(this, tr("Surface regions"), QString::fromUtf8(edited.GetError().c_str())); return; }
    const QPointer<AssetImporterWindow> guard(this);
    const auto ownerEpoch = m_identityOwnerEpoch; const auto documentEpoch = m_assetImporterDocument->GetEpoch();
    const AZStd::weak_ptr<AZ::SceneAPI::Containers::Scene> weakScene = scene;
    const auto isCurrent = [guard, ownerEpoch, documentEpoch, weakScene]()
    {
        const auto expectedScene = weakScene.lock();
        return expectedScene && guard && !guard->m_isClosed && !guard->m_isSaving && guard->m_identityOwnerEpoch == ownerEpoch &&
            guard->m_sceneSettingsCardOverlay == AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex &&
            guard->m_overlay && guard->m_overlay->IsAtRoot() && guard->m_assetImporterDocument &&
            guard->m_assetImporterDocument->GetEpoch() == documentEpoch && guard->m_assetImporterDocument->GetScene() == expectedScene;
    };
    const auto changed = [guard, isCurrent](const AZ::SceneAPI::Containers::Scene& current, const AZ::SceneAPI::DataTypes::IMeshGroup& changedGroup)
    {
        if (!isCurrent()) { return; }
        AZ::SceneAPI::Events::ManifestMetaInfoBus::Broadcast(&AZ::SceneAPI::Events::ManifestMetaInfoBus::Events::ObjectUpdated, current, &changedGroup, guard.data());
        if (!isCurrent()) { return; }
        QTimer::singleShot(0, guard.data(), [guard, isCurrent]()
        {
            if (!isCurrent()) { return; }
            guard->m_rootDisplay->GetManifestWidget()->BuildFromScene(guard->m_assetImporterDocument->GetScene());
            if (isCurrent()) { guard->RefreshRegionEditor(); }
        });
    };
    const auto result = m_assetImporterDocument->ApplySurfaceIdentityChange(group->GetId(), proposed, this, isCurrent, changed);
    if (!result.IsSuccess() && guard) { QMessageBox::warning(guard.data(), tr("Surface regions"), QString::fromUtf8(result.GetError().c_str())); }
    else if (result.IsSuccess() && !result.GetValue() && isCurrent()) { guard->RefreshRegionEditor(); }
}
