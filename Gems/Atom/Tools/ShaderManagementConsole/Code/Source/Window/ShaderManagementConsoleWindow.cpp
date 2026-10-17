/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Shader/ShaderSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderVariantListSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Common/JsonUtils.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <AtomToolsFramework/Debug/TraceRecorder.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/Utils/Utils.h>

#include <Data/ShaderVariantStatisticData.h>
#include <Data/ShaderVariantListGenerator.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Window/ShaderManagementConsoleWindow.h>
#include <Window/ShaderManagementConsoleStatisticView.h>
#include <ShaderManagementConsoleRequestBus.h>

#include <QFileDialog>
#include <QMenu>
#include <QProgressDialog>
#include <QUrl>
#include <QWindow>
#include <QMessageBox>
#include <QMenuBar>
#include <QCoreApplication>
#include <QFileInfo>
#include <QScopedValueRollback>
#include <AzCore/std/algorithm.h>
#include <limits>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, "ShaderManagementConsoleWindow", parent)
    {
        m_assetBrowser->GetSearchWidget()->ClearTypeFilter();
        m_assetBrowser->GetSearchWidget()->SetTypeFilterVisible(false);
        m_assetBrowser->SetFileTypeFilters({
            { tr("Material").toUtf8().constData(), { "material" }, true },
            { tr("Material Type").toUtf8().constData(), { "materialtype" }, true },
            { tr("Shader").toUtf8().constData(), { "shader" }, true },
            { tr("Shader Template").toUtf8().constData(), { "shader.template" }, true },
            { tr("Shader Variant List").toUtf8().constData(), { "shadervariantlist" }, true },
            { "AZSL", { "azsl", "azsli", "srgi" }, true },
        });

        m_documentInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/ShaderManagementConsole/DocumentInspector");
        AddDockWidget("Inspector", m_documentInspector, Qt::RightDockWidgetArea);
        SetDockWidgetVisible("Inspector", false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
        this->setContextMenuPolicy(Qt::CustomContextMenu);
        this->connect(this, &QTableWidget::customContextMenuRequested, this, &ShaderManagementConsoleWindow::ShowContextMenu);
    }

    void ShaderManagementConsoleWindow::ShowContextMenu(const QPoint& pos)
    {
        QMenu contextMenu(tr("Context menu"), this);
        UpdateRecentFileMenu();
        contextMenu.insertMenu(nullptr, m_menuOpenRecent);
        contextMenu.exec(mapToGlobal(pos));
    }

    void ShaderManagementConsoleWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_documentInspector->SetDocumentId(documentId);
    }

    AZStd::string ShaderManagementConsoleWindow::GetSaveDocumentParams(const AZStd::string& initialPath, const AZ::Uuid& documentId) const
    {
        // Get shader file path
        AZ::IO::Path shaderFullPath;
        JE::Render::RPI::ShaderVariantListSourceData shaderVariantList{};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantList,
            documentId,
            &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
        shaderFullPath = JE::Render::RPI::AssetUtils::ResolvePathReference(initialPath, shaderVariantList.m_shaderFilePath);
        
        QMessageBox msgBox;
        msgBox.setText(tr("Where do you want to save the list?"));
        QPushButton* projectBtn = msgBox.addButton(tr("Save to project"), QMessageBox::ActionRole);
        QPushButton* engineBtn = msgBox.addButton(tr("Save to engine"), QMessageBox::ActionRole);
        msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msgBox.exec();

        AZ::IO::Path result;
        if (msgBox.clickedButton() == projectBtn)
        {
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            AZ::IO::Path relativePath, rootFolder;
            bool pathFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                pathFound,
                &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                shaderFullPath.Native(),
                relativePath.Native(),
                rootFolder.Native());

            if (pathFound)
            {
                relativePath.ReplaceExtension("shadervariantlist");
                projectPath /= AZ::IO::Path ("ShaderVariants") / relativePath;
                result = projectPath.LexicallyNormal();
            }
            else
            {
                AZ_Error("ShaderManagementConsoleWindow", false, "Can not find a relative path from the shader: '%s'.", shaderFullPath.c_str());
            }
        }
        else if(msgBox.clickedButton() == engineBtn)
        {
            shaderFullPath.ReplaceExtension("shadervariantlist");
            result = shaderFullPath;
        }

        return result.Native();
    }

    void ShaderManagementConsoleWindow::ErrorMessageBoxesForDocumentVerification(const DocumentVerificationResult& verification)
    {
        if (verification.m_hasRedundantVariants)
        {
            QMessageBox::critical(
                this, tr("QC fail"), tr("Some variants are redundant. Use the recompaction feature before saving."), QMessageBox::Ok);
        }
        if (verification.m_hasRootLike)
        {
            QMessageBox::critical(
                this,
                tr("QC fail"),
                tr("Variant with id %1 is root-like (all options dynamic). Remove it or use recompaction feature before saving.")
                    .arg(verification.m_rootLikeStableId),
                QMessageBox::Ok);
        }
        if (verification.m_hasStableIdJump)
        {
            QMessageBox::critical(
                this,
                tr("QC fail"),
                tr("Stable id %1 isn't compact. Use the recompaction feature before saving.").arg(verification.faultyId),
                QMessageBox::Ok);
        }
    }

    void ShaderManagementConsoleWindow::CreateMenus(QMenuBar* menuBar)
    {
        Base::CreateMenus(menuBar);

        QAction* generateAction = new QAction(tr("Generate Shader Variant List..."), m_menuFile);
        QObject::connect(generateAction, &QAction::triggered, this, &ShaderManagementConsoleWindow::GenerateShaderVariantList);
        m_menuFile->insertAction(m_menuFile->actions().back(), generateAction);

        QAction* verifyAction = new QAction(tr("Verify Variantlist invariants"), m_menuFile);
        QObject::connect(verifyAction, &QAction::triggered, this, [this](){
            DocumentVerificationResult verification;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                verification, GetCurrentDocumentId(), &ShaderManagementConsoleDocumentRequestBus::Events::Verify);
            ErrorMessageBoxesForDocumentVerification(verification);
            if (verification.AllGood())
            {
                QMessageBox::information(
                    this, tr("QC pass"), tr("All good"), QMessageBox::Ok);
            }
            });
        m_menuFile->insertAction(m_menuFile->actions().back(), verifyAction);

        QAction* compactAction = new QAction(tr("Run Compaction"), m_menuFile);
        QObject::connect(
            verifyAction,
            &QAction::triggered,
            this,
            [this]()
            {
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    GetCurrentDocumentId(), &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);

                ShaderManagementConsoleDocumentRequestBus::Event(
                    GetCurrentDocumentId(), &ShaderManagementConsoleDocumentRequestBus::Events::DefragmentVariantList);

                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                    GetCurrentDocumentId(), &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
            });
        m_menuFile->insertAction(m_menuFile->actions().back(), compactAction);

        // Add statistics button
        QAction* action = new QAction(tr("Generate Shader Variant Statistics..."), m_menuFile);
        QObject::connect(action, &QAction::triggered, this, &ShaderManagementConsoleWindow::GenerateStatisticView);
        m_menuFile->insertAction(m_menuFile->actions().back(), action);
    }

    void ShaderManagementConsoleWindow::GenerateShaderVariantList()
    {
        if (m_generationInProgress)
        {
            return;
        }
        QScopedValueRollback<bool> generationGuard(m_generationInProgress, true);
        const QString selectedPath = QFileDialog::getOpenFileName(this, tr("Select shader"), {}, tr("Shader (*.shader)"));
        if (selectedPath.isEmpty())
        {
            return;
        }
        const QFileInfo selectedFile(selectedPath);
        if (!selectedFile.isFile() || selectedFile.suffix() != JE::Render::RPI::ShaderSourceData::Extension)
        {
            QMessageBox::critical(this, tr("Generation failed"), tr("Select an existing .shader file."));
            return;
        }
        AZStd::string shaderPath = selectedFile.absoluteFilePath().toUtf8().constData();
        const auto fail = [this](const AZStd::string& message)
        {
            QMessageBox::critical(this, tr("Generation failed"), QString::fromUtf8(message.c_str()));
        };
        if (!AtomToolsFramework::ValidateDocumentPath(shaderPath))
        {
            fail("The selected shader path is not a supported editable document path.");
            return;
        }
        if (!ShaderManagementConsoleRequestBus::HasHandlers())
        {
            fail("The shader/material asset service is unavailable.");
            return;
        }
        AtomToolsFramework::TraceRecorder assetTrace;
        AZ::Data::AssetInfo sourceInfo;
        ShaderManagementConsoleRequestBus::BroadcastResult(
            sourceInfo, &ShaderManagementConsoleRequestBus::Events::GetSourceAssetInfo, shaderPath);
        const auto shader = JE::Render::RPI::AssetUtils::LoadAsset<JE::Render::RPI::ShaderAsset>(shaderPath);
        if (!sourceInfo.m_assetId.IsValid() || sourceInfo.m_relativePath.empty() || !shader.IsSuccess() ||
            assetTrace.GetErrorCount(true) != 0)
        {
            fail("The selected shader must have a successfully built shader asset.\n" + assetTrace.GetDump());
            return;
        }
        const auto* layout = shader.GetValue()->GetShaderOptionGroupLayout();
        if (!layout || !layout->IsFinalized())
        {
            fail("The selected shader does not have a finalized option layout.");
            return;
        }

        JE::Render::RPI::ShaderOptionValuesSourceData systemOptions;
        AZ::IO::Path systemPath(shaderPath);
        systemPath.ReplaceExtension("systemoptions");
        if (QFileInfo(QString::fromUtf8(systemPath.c_str())).exists() &&
            !JE::Render::RPI::JsonUtils::LoadObjectFromFile(systemPath.Native(), systemOptions))
        {
            fail("The shader's .systemoptions file could not be read.");
            return;
        }
        QProgressDialog progress(tr("Validate shader options..."), tr("Cancel"), 0, 1, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setAutoClose(false);
        progress.setAutoReset(false);
        progress.setMinimumDuration(0);
        const auto isCancelled = [&progress]
        {
            QCoreApplication::processEvents();
            return progress.wasCanceled();
        };
        // Capture the absolute-path system-only baseline expected when the selected shader is opened.
        const auto baseline = JE::ShaderVariants::Generate(shaderPath, *layout, {}, systemOptions, isCancelled);
        if (!baseline.IsSuccess())
        {
            progress.hide();
            if (baseline.GetError().m_reason != JE::ShaderVariants::FailureReason::Cancelled)
            {
                fail(baseline.GetError().m_message);
            }
            return;
        }

        AZStd::vector<AZ::Data::AssetId> materialIds;
        ShaderManagementConsoleRequestBus::BroadcastResult(
            materialIds, &ShaderManagementConsoleRequestBus::Events::FindMaterialAssetsUsingShader, sourceInfo.m_relativePath);
        if (assetTrace.GetErrorCount(true) != 0 || materialIds.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            progress.hide();
            fail("Material dependencies could not be collected.\n" + assetTrace.GetDump());
            return;
        }
        AZStd::sort(materialIds.begin(), materialIds.end(), [](const auto& lhs, const auto& rhs)
        {
            return lhs.ToFixedString() < rhs.ToFixedString();
        });
        materialIds.erase(AZStd::unique(materialIds.begin(), materialIds.end()), materialIds.end());
        progress.setLabelText(tr("Gather shader variants from materials..."));
        progress.setRange(0, AZStd::max(1, static_cast<int>(materialIds.size())));

        AZStd::vector<JE::ShaderVariants::MaterialVariant> materials;
        for (size_t index = 0; index < materialIds.size(); ++index)
        {
            progress.setValue(static_cast<int>(index));
            if (isCancelled())
            {
                return;
            }
            AtomToolsFramework::TraceRecorder materialTrace;
            AZStd::vector<JE::Render::RPI::ShaderCollection::Item> items;
            ShaderManagementConsoleRequestBus::BroadcastResult(
                items, &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems, materialIds[index]);
            if (materialTrace.GetErrorCount(true) != 0)
            {
                progress.hide();
                fail("A dependent material could not be loaded.\n" + materialTrace.GetDump());
                return;
            }
            for (const auto& item : items)
            {
                if (item.GetShaderAssetId() != shader.GetValue().GetId() || item.GetShaderVariantId().IsEmpty())
                {
                    continue;
                }
                JE::ShaderVariants::MaterialVariant material{item.GetShaderOptionGroup(), {}};
                if (!material.m_options.GetShaderOptionLayout())
                {
                    progress.hide();
                    fail("A dependent material has no shader option layout.");
                    return;
                }
                for (const auto& descriptor : material.m_options.GetShaderOptionDescriptors())
                {
                    if (item.MaterialOwnsShaderOption(descriptor.GetName()))
                    {
                        material.m_materialOwnedOptions.insert(descriptor.GetName());
                    }
                }
                materials.push_back(AZStd::move(material));
            }
        }
        progress.setLabelText(tr("Expand and validate shader variants..."));
        auto generated = JE::ShaderVariants::Generate(sourceInfo.m_relativePath, *layout, materials, systemOptions, isCancelled);
        if (!generated.IsSuccess())
        {
            progress.hide();
            if (generated.GetError().m_reason != JE::ShaderVariants::FailureReason::Cancelled)
            {
                fail(generated.GetError().m_message);
            }
            return;
        }
        if (isCancelled())
        {
            return;
        }
        progress.hide();

        const auto matchesData = [](const JE::Render::RPI::ShaderVariantListSourceData& actual, const JE::Render::RPI::ShaderVariantListSourceData& expected)
        {
            if (actual.m_shaderFilePath != expected.m_shaderFilePath || actual.m_materialOptionsHint != expected.m_materialOptionsHint ||
                actual.m_shaderVariants.size() != expected.m_shaderVariants.size())
            {
                return false;
            }
            for (size_t index = 0; index < expected.m_shaderVariants.size(); ++index)
            {
                const auto& actualVariant = actual.m_shaderVariants[index];
                const auto& expectedVariant = expected.m_shaderVariants[index];
                if (actualVariant.m_stableId != expectedVariant.m_stableId || actualVariant.m_options != expectedVariant.m_options ||
                    actualVariant.m_enableRegisterAnalysis != expectedVariant.m_enableRegisterAnalysis || actualVariant.m_asic != expectedVariant.m_asic)
                {
                    return false;
                }
            }
            return true;
        };

        // Opening the shader keeps the absolute source path for existing Save/SaveAs behavior. No file is written here.
        AtomToolsFramework::TraceRecorder documentTrace;
        AZ::Uuid documentId;
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
            documentId, m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath,
            shaderPath, AZStd::string{});
        const auto destroy = [&]
        {
            if (!documentId.IsNull())
            {
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::DestroyDocument, documentId);
            }
        };
        JE::Render::RPI::ShaderVariantListSourceData openedBaseline;
        if (!documentId.IsNull())
        {
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                openedBaseline, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
        }
        if (documentId.IsNull() || documentTrace.GetErrorCount(true) != 0 || !matchesData(openedBaseline, baseline.GetValue()))
        {
            destroy();
            fail("The shader baseline changed or could not be opened. Generate the list again.\n" + documentTrace.GetDump());
            return;
        }
        bool began = false;
        if (!documentId.IsNull() && documentTrace.GetErrorCount(true) == 0)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                began, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::BeginEdit);
        }
        if (!began)
        {
            destroy();
            fail("The variant list document could not be created for editing.\n" + documentTrace.GetDump());
            return;
        }
        ShaderManagementConsoleDocumentRequestBus::Event(
            documentId, &ShaderManagementConsoleDocumentRequestBus::Events::SetShaderVariantListSourceData, generated.GetValue());
        JE::Render::RPI::ShaderVariantListSourceData applied;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            applied, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
        bool ended = false;
        if (matchesData(applied, generated.GetValue()) && documentTrace.GetErrorCount(true) == 0)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                ended, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::EndEdit);
        }
        if (!ended || documentTrace.GetErrorCount(true) != 0)
        {
            destroy();
            fail("The variant list edit could not be completed.\n" + documentTrace.GetDump());
        }
    }

    void ShaderManagementConsoleWindow::GenerateStatisticView()
    {
        AZStd::vector<AZ::Data::AssetId> materialAssetIdList;
        ShaderManagementConsoleRequestBus::BroadcastResult(
            materialAssetIdList, &ShaderManagementConsoleRequestBus::Events::GetAllMaterialAssetIds);

        QProgressDialog progressDialog(AzToolsFramework::GetActiveWindow());
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMaximum(static_cast<int>(materialAssetIdList.size()));
        progressDialog.setMaximumWidth(400);
        progressDialog.setMaximumHeight(100);
        progressDialog.setWindowTitle(tr("Gather information from material assets"));
        progressDialog.setLabelText(tr("Gather shader variant information..."));

        ShaderVariantStatisticData statisticData;
        for (int i = 0; i < materialAssetIdList.size(); ++i)
        {
            AZStd::vector<JE::Render::RPI::ShaderCollection::Item> shaderItemList;
            ShaderManagementConsoleRequestBus::BroadcastResult(
                shaderItemList, &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems, materialAssetIdList[i]);

            for (auto& shaderItem : shaderItemList)
            {
                JE::Render::RPI::ShaderVariantId shaderVariantId = shaderItem.GetShaderVariantId();

                if (!shaderVariantId.IsEmpty())
                {
                    if (statisticData.m_shaderVariantUsage.find(shaderVariantId) == statisticData.m_shaderVariantUsage.end())
                    {
                        // Variant not found
                        statisticData.m_shaderVariantUsage[shaderVariantId].m_count = 1;
                    }
                    else
                    {
                        statisticData.m_shaderVariantUsage[shaderVariantId].m_count += 1;
                    }

                    JE::Render::RPI::ShaderOptionGroup shaderOptionGroup = shaderItem.GetShaderOptionGroup();
                    statisticData.m_shaderVariantUsage[shaderVariantId].m_shaderOptionGroup = shaderOptionGroup;

                    for (auto& shaderOptionDescriptor : shaderOptionGroup.GetShaderOptionDescriptors())
                    {
                        AZ::Name optionName = shaderOptionDescriptor.GetName();
                        JE::Render::RPI::ShaderOptionValue optionValue = shaderOptionGroup.GetValue(optionName);

                        if (!optionValue.IsValid())
                        {
                            continue;
                        }
                        AZ::Name valueName = shaderOptionDescriptor.GetValueName(optionValue);
                        if (statisticData.m_shaderOptionUsage.find(optionName) == statisticData.m_shaderOptionUsage.end())
                        {
                            // Option Not found
                            statisticData.m_shaderOptionUsage[optionName][valueName] = 1;
                        }
                        else
                        {
                            if (statisticData.m_shaderOptionUsage[optionName].find(valueName) ==
                                statisticData.m_shaderOptionUsage[optionName].end())
                            {
                                // Value not found
                                statisticData.m_shaderOptionUsage[optionName][valueName] = 1;
                            }
                            else
                            {
                                statisticData.m_shaderOptionUsage[optionName][valueName] += 1;
                            }
                        }
                    }
                }
            }

            progressDialog.setValue(i);

            if (progressDialog.wasCanceled())
            {
                return;
            }
        }
        progressDialog.close();

        if (m_statisticView)
        {
            delete m_statisticView;
            m_statisticView = nullptr;
        }

        m_statisticView = new ShaderManagementConsoleStatisticView(statisticData, nullptr);
        m_statisticView->setWindowTitle(tr("Shader Variant Statistic View"));
        m_statisticView->show();
    }

    void ShaderManagementConsoleWindow::closeEvent(QCloseEvent* closeEvent)
    {
        if (m_statisticView)
        {
            m_statisticView->close();
            delete m_statisticView;
            m_statisticView = nullptr;
        }

        Base::closeEvent(closeEvent);
    }
} // namespace ShaderManagementConsole

