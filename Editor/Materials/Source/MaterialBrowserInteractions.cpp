/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Common/JsonUtils.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <QMessageBox>
#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <JE/Editor/Materials/EditorMaterialComponentUtil.h>
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>

namespace JE
{
    namespace Editor
    {
        MaterialBrowserInteractions::MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }

        MaterialBrowserInteractions::~MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void MaterialBrowserInteractions::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            const AZStd::string_view path(fullSourceFileName);
            if (path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialExtensionWithDot) ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialTypeExtensionWithDot))
            {
                openers.push_back({ "Material_Editor", "Open in Material Editor...", QIcon(":/Menu/material_editor.svg"),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                            fullSourceFileNameInCallback);
                    } });
                return;
            }

            if (path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot) ||
                path.ends_with(".passgraph") || path.ends_with(".passgraphtemplate") || path.ends_with(".passgraphnode") ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::ShaderExtensionWithDot))
            {
                openers.push_back({ "Material_Canvas", "Open in Graph Workspace...", QIcon(":/Menu/material_canvas.svg"),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialCanvas,
                            fullSourceFileNameInCallback);
                    } });
                return;
            }
        }

        void MaterialBrowserInteractions::AddSourceFileCreators(
            [[maybe_unused]] const char* fullSourceFolderName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            using namespace AtomToolsFramework;

            creators.push_back(
                { "Material_Creator", "Material", QIcon(),
                  [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                      CreateMaterial(fullSourceFolderNameInCallback);
                  } });

            creators.push_back(
                { "Material_Graph_Creator", "Material Graph", QIcon(),
                  [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                      const AZStd::string defaultMaterialGraphTemplate =
                          AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                              "/O3DE/Atom/MaterialCanvas/DefaultMaterialGraphTemplate",
                              "@engroot@/Editor/Materials/Assets/MaterialCanvas/GraphData/blank_graph.materialgraphtemplate"));

                      QWidget* mainWindow = nullptr;
                      AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                          mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                      CreateDocumentDialog dialog(
                          QObject::tr("Create Material Graph"),
                          QObject::tr("Select Material Graph Template"),
                          fullSourceFolderNameInCallback.empty() ? QObject::tr("Select Material Path") : QString(),
                          fullSourceFolderNameInCallback.c_str(),
                          { "materialgraph" },
                          defaultMaterialGraphTemplate.c_str(),
                          [](const AZStd::string& path)
                          {
                              return IsDocumentPathEditable(path) &&
                                  path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot);
                          },
                          mainWindow);
                      dialog.adjustSize();

                      if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                      {
                          AZ::IO::FileIOBase::GetInstance()->Copy(
                              dialog.m_sourcePath.toUtf8().constData(), dialog.m_targetPath.toUtf8().constData());
                          AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                              AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                              &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                              dialog.m_targetPath.toUtf8().constData(),
                              AZ::Crc32(),
                              false);
                      }
                  } });
        }

        AZStd::string MaterialBrowserInteractions::CreateMaterial(
            const AZStd::string& initialFolder, const AZStd::string& initialMaterialType)
        {
            const AZStd::string materialType = initialMaterialType.empty()
                ? AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                      "/O3DE/Atom/MaterialEditor/DefaultMaterialType",
                      "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype"))
                : initialMaterialType;
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);
            AtomToolsFramework::CreateDocumentDialog dialog(
                QObject::tr("Create Material"), QObject::tr("Select Material Type"),
                initialFolder.empty() ? QObject::tr("Select Material Path") : QString(),
                initialFolder.c_str(), {"material"}, materialType.c_str(),
                [](const AZStd::string& path)
                {
                    return AtomToolsFramework::IsDocumentPathEditable(path) &&
                        path.ends_with(EditorMaterialComponentUtil::MaterialTypeExtensionWithDot);
                }, mainWindow);
            dialog.adjustSize();
            if (dialog.exec() != QDialog::Accepted)
            {
                return {};
            }

            // Keep both strings alive through loading, serialization and the asset creation notification.
            const AZStd::string sourcePath = dialog.m_sourcePath.toUtf8().constData();
            AZStd::string targetPath = dialog.m_targetPath.toUtf8().constData();
            if (!AtomToolsFramework::ValidateDocumentPath(targetPath))
            {
                QMessageBox::critical(mainWindow, QObject::tr("Create Material"), QObject::tr("The material destination is not editable."));
                return {};
            }
            auto result = CreateMaterialSourceFile(sourcePath, targetPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(mainWindow, QObject::tr("Create Material"), QString::fromUtf8(result.GetError().c_str()));
                return {};
            }
            AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                targetPath.c_str(), AZ::Crc32(), false);
            return targetPath;
        }

        AZ::Outcome<void, AZStd::string> MaterialBrowserInteractions::CreateMaterialSourceFile(
            const AZStd::string& materialTypePath, const AZStd::string& materialPath)
        {
            const AZStd::string sourcePath = AtomToolsFramework::GetPathWithoutAlias(materialTypePath);
            if (!AZ::IO::PathView(sourcePath).IsAbsolute() ||
                !sourcePath.ends_with(EditorMaterialComponentUtil::MaterialTypeExtensionWithDot) ||
                !AZ::IO::PathView(materialPath).IsAbsolute() ||
                !materialPath.ends_with(EditorMaterialComponentUtil::MaterialExtensionWithDot))
            {
                return AZ::Failure(AZStd::string("Select an absolute .materialtype source and a .material destination."));
            }
            if (AZ::IO::SystemFile::Exists(materialPath.c_str()))
            {
                return AZ::Failure(AZStd::string("The material destination already exists."));
            }
            auto materialType = JE::Render::RPI::MaterialUtils::LoadMaterialTypeSourceData(sourcePath);
            if (!materialType.IsSuccess())
            {
                return AZ::Failure(AZStd::string("Failed to load the selected material type."));
            }
            JE::Render::RPI::MaterialSourceData materialData;
            materialData.m_materialTypeVersion = materialType.GetValue().m_version;
            materialData.m_materialType = AtomToolsFramework::GetPathToExteralReference(materialPath, sourcePath);
            materialData.m_parentMaterial.clear();

            // Preserve the unwrapped .material schema and check the actual write, not only serialization.
            rapidjson::Document document;
            document.SetObject();
            AZ::JsonSerializerSettings settings;
            JE::Render::RPI::JsonReportingHelper reporting;
            reporting.Attach(settings);
            AZ::JsonSerialization::Store(document, document.GetAllocator(), materialData, settings);
            if (reporting.ErrorsReported())
            {
                return AZ::Failure(AZStd::string("Failed to serialize the material source."));
            }
            AZStd::string contents;
            auto serialized = AZ::JsonSerializationUtils::WriteJsonString(document, contents);
            if (!serialized.IsSuccess())
            {
                return serialized;
            }
            AZ::IO::SystemFile output;
            if (!output.Open(materialPath.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_NEW))
            {
                return AZ::Failure(AZStd::string("Failed to create the material destination. Existing files are not overwritten."));
            }
            const auto written = output.Write(contents.data(), contents.size());
            output.Close();
            if (written != contents.size())
            {
                // This operation exclusively created the file; never remove a pre-existing destination.
                AZ::IO::SystemFile::Delete(materialPath.c_str());
                return AZ::Failure(AZStd::string("Failed to write the complete material source."));
            }
            return AZ::Success();
        }

        bool MaterialBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
        {
            return fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialExtensionWithDot) ||
                fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialTypeExtensionWithDot) ||
                fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot) ||
                fileName.ends_with(".passgraph") || fileName.ends_with(".passgraphtemplate") || fileName.ends_with(".passgraphnode") ||
                fileName.ends_with(JE::Editor::EditorMaterialComponentUtil::ShaderExtensionWithDot);
        }
    } // namespace Editor
} // namespace JE
