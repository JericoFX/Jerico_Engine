/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <QMessageBox>
#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <JE/Editor/Materials/EditorMaterialComponentUtil.h>
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>
#include "MaterialTextureSet.h"
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <QAction>
#include <QMenu>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFileInfo>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScopedValueRollback>
#include <QPointer>
#include <QTimer>
#include <Atom/ImageProcessing/ImageProcessingEditorBus.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#if defined(AZ_PLATFORM_LINUX)
#include <sys/stat.h>
#endif

namespace JE
{
    namespace Editor
    {
        namespace
        {
            bool IsTexturePathInCurrentProject(const AZStd::string& path)
            {
                const auto project = AZ::Utils::GetProjectPath();
                const QString root = QFileInfo(QString::fromUtf8(project.c_str())).canonicalFilePath();
                const QFileInfo target(QString::fromUtf8(path.c_str()));
                const QString parent = target.exists()
                    ? QFileInfo(target.canonicalFilePath()).dir().canonicalPath()
                    : target.dir().canonicalPath();
                if (project.empty() || root.isEmpty() || parent.isEmpty())
                {
                    return false;
                }
    #if defined(AZ_PLATFORM_WINDOWS)
                constexpr auto sensitivity = Qt::CaseInsensitive;
    #else
                constexpr auto sensitivity = Qt::CaseSensitive;
    #endif
                if (parent.compare(root, sensitivity) != 0 && !parent.startsWith(root + '/', sensitivity))
                {
                    return false;
                }
                const QString first = QDir(root).relativeFilePath(parent).section('/', 0, 0).toLower();
                return first != "cache" && first != "user" && first != "build" &&
                    first != "gems" && first != "3rdparty" && first != "external";
            }

            using TexturePresetPlan = AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>;

            AZ::Outcome<TexturePresetPlan, AZStd::string> PlanTexturePresets(
                const MaterialTextureSet& set, const AZStd::vector<size_t>& selected)
            {
                TexturePresetPlan plan;
                for (const auto index : selected)
                {
                    if (index >= set.candidates.size())
                    {
                        return AZ::Failure(AZStd::string("A selected texture candidate no longer exists."));
                    }
                    const auto& candidate = set.candidates[index];
                    if (!IsTexturePathInCurrentProject(candidate.path))
                    {
                        return AZ::Failure(AZStd::string("Selected textures must be project sources outside Cache, user, build, Gems, 3rdParty and External. No settings have been written."));
                    }
                    const QString token = QString::fromUtf8(candidate.token.c_str()).toLower();
                    const bool packed = token == "arm" || token == "orm" || token == "ordp";
                    const char* preset = nullptr;
                    if (packed)
                    {
                        preset = "LayerMask";
                    }
                    else
                    {
                        switch (candidate.role)
                        {
                        case MaterialTextureRole::BaseColor: preset = "Albedo"; break;
                        case MaterialTextureRole::Normal: preset = "Normals"; break;
                        case MaterialTextureRole::Roughness:
                        case MaterialTextureRole::Metallic: preset = "Reflectance"; break;
                        case MaterialTextureRole::Occlusion: preset = "AmbientOcclusion"; break;
                        case MaterialTextureRole::Emissive: preset = "Emissive"; break;
                        case MaterialTextureRole::Height: preset = "Displacement"; break;
                        default: break;
                        }
                    }
                    if (!candidate.supported || !preset || !IsMaterialTextureSource(candidate.path))
                    {
                        return AZ::Failure(AZStd::string("A selected texture has no supported processing preset. Review it before creating the material."));
                    }
                    bool found = false;
                    for (const auto& entry : plan)
                    {
                        if (entry.first == candidate.path)
                        {
                            if (entry.second != preset)
                            {
                                return AZ::Failure(AZStd::string("One texture requires conflicting processing presets. Resolve the assignments before creating the material."));
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        plan.emplace_back(candidate.path, preset);
                    }
                }
                return AZ::Success(AZStd::move(plan));
            }

            struct TextureSidecarSnapshot
            {
                AZStd::string path;
                AZStd::string bytes;
                AZStd::array<AZ::u64, 6> identity{};
                AZStd::array<AZ::u32, 5> digest{};
            };

            bool CaptureTextureSidecar(const AZStd::string& path, TextureSidecarSnapshot& snapshot)
            {
                AZ::IO::SystemFile file;
                if (!file.Open(path.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
                {
                    return false;
                }
                snapshot.path = path;
    #if defined(AZ_PLATFORM_WINDOWS)
                BY_HANDLE_FILE_INFORMATION info{};
                if (!GetFileInformationByHandle(file.NativeHandle(), &info))
                {
                    return false;
                }
                const auto value64 = [](DWORD low, DWORD high) { return (AZ::u64(high) << 32) | low; };
                if (value64(info.nFileIndexLow, info.nFileIndexHigh) == 0)
                {
                    return false;
                }
                snapshot.identity = {info.dwVolumeSerialNumber, value64(info.nFileIndexLow, info.nFileIndexHigh),
                    value64(info.ftCreationTime.dwLowDateTime, info.ftCreationTime.dwHighDateTime),
                    value64(info.ftLastWriteTime.dwLowDateTime, info.ftLastWriteTime.dwHighDateTime),
                    info.dwFileAttributes, value64(info.nFileSizeLow, info.nFileSizeHigh)};
    #elif defined(AZ_PLATFORM_LINUX)
                struct stat info{};
                if (fstat(file.NativeHandle(), &info) != 0 || info.st_ino == 0)
                {
                    return false;
                }
                snapshot.identity = {static_cast<AZ::u64>(info.st_dev), static_cast<AZ::u64>(info.st_ino),
                    static_cast<AZ::u64>(info.st_ctim.tv_sec), static_cast<AZ::u64>(info.st_ctim.tv_nsec),
                    static_cast<AZ::u64>(info.st_mtim.tv_sec), static_cast<AZ::u64>(info.st_mtim.tv_nsec)};
    #else
                return false;
    #endif
                // These are newly written XML settings, not texture payloads. Retain an unexpected large file for review.
                const auto length = file.Length();
                if (length > 1024 * 1024)
                {
                    return false;
                }
                snapshot.bytes.resize(static_cast<size_t>(length));
                if (file.Read(length, snapshot.bytes.data()) != length)
                {
                    return false;
                }
                AZ::Sha1 hash;
                hash.ProcessBytes(reinterpret_cast<const AZStd::byte*>(snapshot.bytes.data()), snapshot.bytes.size());
                AZ::u32 digest[5]{};
                hash.GetDigest(digest);
                for (size_t index = 0; index < snapshot.digest.size(); ++index)
                {
                    snapshot.digest[index] = digest[index];
                }
                return true;
            }

            QStringList RollbackTextureSidecars(const AZStd::vector<TextureSidecarSnapshot>& created)
            {
                QStringList retained;
                for (const auto& owned : created)
                {
                    if (!AZ::IO::SystemFile::Exists(owned.path.c_str()))
                    {
                        continue;
                    }
                    TextureSidecarSnapshot current;
                    if (!CaptureTextureSidecar(owned.path, current) || owned.identity != current.identity ||
                        owned.digest != current.digest || owned.bytes != current.bytes ||
                        !AZ::IO::SystemFile::Delete(owned.path.c_str()))
                    {
                        retained << QString::fromUtf8(owned.path.c_str());
                    }
                    // This is a revalidated best-effort rollback, not an atomic check-and-delete transaction.
                }
                return retained;
            }
            // File discovery precedes source metadata; wait for the model to publish the complete source.
            class PendingMaterialSelection final
                : public QObject
                , private AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
            {
            public:
                PendingMaterialSelection(QWidget* caller, const AZStd::string& path)
                    : QObject(caller), m_caller(caller), m_path(path)
                {
                    setObjectName("PendingGeneratedMaterialSelection");
                    BusConnect();
                    TryQueueSelection();
                }
                ~PendingMaterialSelection() override { BusDisconnect(); }

            private:
                bool IsSourceReady() const
                {
                    if (!m_model)
                    {
                        return false;
                    }
                    const auto index = m_model->findIndex(QDir::fromNativeSeparators(QString::fromUtf8(m_path.c_str())));
                    const auto* entry = index.isValid()
                        ? index.data(AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
                            .value<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>() : nullptr;
                    const auto* source = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry);
                    if (!source || source->GetSourceID() < 0 || source->GetSourceUuid().IsNull()
                        || AZ::IO::PathView(source->GetFullPath()).LexicallyNormal() != AZ::IO::PathView(m_path).LexicallyNormal())
                    {
                        return false;
                    }
                    const auto cache = AzToolsFramework::AssetBrowser::EntryCache::GetInstance();
                    if (!cache)
                    {
                        return false;
                    }
                    const auto file = cache->m_fileIdMap.find(source->GetFileID());
                    const auto id = cache->m_sourceIdMap.find(source->GetSourceID());
                    const auto guid = cache->m_sourceUuidMap.find(source->GetSourceUuid());
                    return file != cache->m_fileIdMap.end() && file->second == source
                        && id != cache->m_sourceIdMap.end() && id->second == source
                        && guid != cache->m_sourceUuidMap.end() && guid->second == source;
                }
                void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override
                {
                    const auto* source = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry);
                    if (source && AZ::IO::PathView(source->GetFullPath()).LexicallyNormal() == AZ::IO::PathView(m_path).LexicallyNormal())
                    {
                        TryQueueSelection();
                    }
                }
                void TryQueueSelection()
                {
                    if (m_selectionQueued)
                    {
                        return;
                    }
                    if (!m_model)
                    {
                        AzToolsFramework::AssetBrowser::AssetBrowserModel* model = nullptr;
                        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(model,
                            &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);
                        m_model = model;
                        if (m_model)
                        {
                            connect(m_model.data(), &QAbstractItemModel::dataChanged, this, [this]() { TryQueueSelection(); });
                        }
                    }
                    if (IsSourceReady())
                    {
                        QueueSelection();
                    }
                }
                void QueueSelection()
                {
                    m_selectionQueued = true;
                    BusDisconnect();
                    disconnect(m_model.data(), nullptr, this, nullptr);
                    // Retain one queued UI action with QObject cancellation, without retaining a source entry.
                    QTimer::singleShot(0, this, [this]()
                    {
                        if (m_caller && IsSourceReady())
                        {
                            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                                &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::SelectAsset,
                                m_caller.data(), m_path);
                        }
                        deleteLater();
                    });
                }
                QPointer<QWidget> m_caller;
                QPointer<AzToolsFramework::AssetBrowser::AssetBrowserModel> m_model;
                AZStd::string m_path;
                bool m_selectionQueued = false;
            };

            void SelectGeneratedMaterial(QWidget* caller, const AZStd::string& path)
            {
                if (!caller)
                {
                    return;
                }
                // A later user action replaces an unfinished selection owned by this same browser widget.
                delete caller->findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly);
                new PendingMaterialSelection(caller, path);
            }

            class TextureMaterialCreationDialog final : public QDialog
            {
            public:
                explicit TextureMaterialCreationDialog(QWidget* caller)
                    : QDialog(caller)
                    , m_browserCaller(caller)
                {
                    setObjectName("MaterialTextureReview");
                    setWindowTitle(tr("Generar material"));
                    setAttribute(Qt::WA_DeleteOnClose);
                    new QVBoxLayout(this);
                }
                void GenerateMaterialFromTexture(const AZStd::string& sourcePath);

            private:
                void CreateTextureMaterial();
                QPointer<QWidget> m_browserCaller;
                AZStd::optional<MaterialTextureSet> m_textureSet;
                QWidget* m_texturePanel = nullptr;
                QLabel* m_textureStatus = nullptr;
                QPushButton* m_textureCreate = nullptr;
                AZStd::vector<QComboBox*> m_textureChoices;
                bool m_creatingTextureMaterial = false;
            };
        }
        void TextureMaterialCreationDialog::GenerateMaterialFromTexture(const AZStd::string& sourcePath)
        {
            m_texturePanel = new QGroupBox(tr("Generar material desde texturas"), this);
            m_texturePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            auto* panelLayout = new QVBoxLayout(m_texturePanel);
            m_textureStatus = new QLabel(m_texturePanel);
            m_textureStatus->setObjectName("MaterialTextureStatus");
            m_textureStatus->setWordWrap(true);
            m_textureStatus->setTextFormat(Qt::PlainText);
            m_textureStatus->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            panelLayout->addWidget(m_textureStatus);
            static_cast<QVBoxLayout*>(layout())->addWidget(m_texturePanel);
            auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
            connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
            static_cast<QVBoxLayout*>(layout())->addWidget(buttons);

            auto analyzed = AnalyzeMaterialTextureSet(sourcePath);
            if (!analyzed.IsSuccess())
            {
                m_textureStatus->setText(QString::fromUtf8(analyzed.GetError().c_str()));
                show();
                return;
            }
            m_textureSet = AZStd::move(analyzed.GetValue());
            if (AZ::IO::SystemFile::Exists(m_textureSet->suggestedMaterialPath.c_str()))
            {
                // A generated source already records its assignments; retain subsequent manual edits.
                CreateTextureMaterial();
                return;
            }
            QStringList information;
            information << tr("Destino: %1").arg(QString::fromUtf8(m_textureSet->suggestedMaterialPath.c_str()));
            if (!m_textureSet->missing.empty())
            {
                information << tr("Conjunto incompleto: se conservarán los orígenes; los mapas ausentes no se inventan.");
            }
            for (const auto& warning : m_textureSet->warnings)
            {
                information << QString::fromUtf8(warning.c_str());
            }
            auto* choices = new QFormLayout();
            panelLayout->addLayout(choices);
            const char* labels[] = {"Color base", "Normal", "Rugosidad", "Metallic", "Oclusión", "Emisión", "Altura"};
            for (size_t roleIndex = 0; roleIndex < AZ_ARRAY_SIZE(labels); ++roleIndex)
            {
                const auto role = static_cast<MaterialTextureRole>(roleIndex);
                size_t count = 0;
                for (const auto& candidate : m_textureSet->candidates)
                {
                    count += candidate.supported && candidate.role == role ? 1 : 0;
                }
                if (count < 2)
                {
                    continue;
                }
                auto* choice = new QComboBox(m_texturePanel);
                choice->setObjectName(QString("MaterialTextureChoice%1").arg(roleIndex));
                choice->addItem(tr("Sin asignar"), -1);
                for (size_t index = 0; index < m_textureSet->candidates.size(); ++index)
                {
                    const auto& candidate = m_textureSet->candidates[index];
                    if (candidate.supported && candidate.role == role)
                    {
                        choice->addItem(QString::fromUtf8(candidate.path.c_str()), static_cast<int>(index));
                        choice->setItemData(choice->count() - 1, QString::fromUtf8(candidate.token.c_str()), Qt::ToolTipRole);
                    }
                }
                choices->addRow(tr(labels[roleIndex]), choice);
                m_textureChoices.push_back(choice);
            }
            if (!m_textureChoices.empty())
            {
                information << tr("Hay mapas en conflicto. Elija un candidato por función o deje Sin asignar para crear un material parcial.");
                m_textureCreate = new QPushButton(tr("Crear material"), m_texturePanel);
                m_textureCreate->setObjectName("CreateMaterialFromTextures");
                m_textureCreate->setDefault(true);
                panelLayout->addWidget(m_textureCreate);
                connect(m_textureCreate, &QPushButton::clicked, this, &TextureMaterialCreationDialog::CreateTextureMaterial);
            }
            m_textureStatus->setText(information.join('\n'));
            if (m_textureChoices.empty())
            {
                CreateTextureMaterial();
            }
            else
            {
                show();
            }
        }

        void TextureMaterialCreationDialog::CreateTextureMaterial()
        {
            if (m_creatingTextureMaterial || !m_textureSet || !m_textureStatus)
            {
                return;
            }
            const QScopedValueRollback<bool> generating(m_creatingTextureMaterial, true);
            const auto report = [this](const QString& message)
            {
                m_textureStatus->setText(m_textureStatus->text() + "\n" + message);
                show();
            };
            AZStd::string path = m_textureSet->suggestedMaterialPath;
            if (!AtomToolsFramework::ValidateDocumentPath(path))
            {
                report(tr("El destino del material no es editable."));
                return;
            }
            if (!IsTexturePathInCurrentProject(path))
            {
                report(tr("El destino debe estar en las fuentes del proyecto actual, fuera de Cache/user/build/Gems/3rdParty/External. No se ha escrito ningún archivo."));
                return;
            }
            if (AZ::IO::SystemFile::Exists(path.c_str()))
            {
                if (!IsMaterialForTextureSet(*m_textureSet, path))
                {
                    report(tr("El nombre está ocupado por otro material. No se ha modificado ni adoptado ese archivo."));
                    return;
                }
                // Existing generated materials keep all subsequent manual property edits.
                SelectGeneratedMaterial(m_browserCaller.data(), path);
            }
            else
            {
                auto selected = m_textureSet->selected;
                for (const auto* choice : m_textureChoices)
                {
                    const int index = choice->currentData().toInt();
                    if (index >= 0)
                    {
                        selected.push_back(static_cast<size_t>(index));
                    }
                }
                auto source = BuildTextureMaterialSource(*m_textureSet, selected, path);
                if (!source.IsSuccess())
                {
                    report(QString::fromUtf8(source.GetError().c_str()));
                    return;
                }
                auto plan = PlanTexturePresets(*m_textureSet, selected);
                if (!plan.IsSuccess())
                {
                    report(QString::fromUtf8(plan.GetError().c_str()));
                    return;
                }
                AZStd::vector<TextureSidecarSnapshot> created;
                const auto rollback = [&created, &report](const QString& error)
                {
                    const auto retained = RollbackTextureSidecars(created);
                    report(error);
                    if (!retained.empty())
                    {
                        report(tr("Se conservaron archivos de configuración que no se pudieron retirar con seguridad:\n%1")
                            .arg(retained.join('\n')));
                    }
                };
                // Validate the complete plan before asking the existing image-processing owner to write anything.
                for (const auto& entry : plan.GetValue())
                {
                    AZ::Outcome<bool, AZStd::string> prepared = AZ::Failure(AZStd::string(
                        "Image processing is unavailable. Review the texture presets before creating the material."));
                    ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::BroadcastResult(prepared,
                        &ImageProcessingAtomEditor::ImageProcessingEditorRequests::EnsureTexturePresetIfUnset,
                        entry.first, entry.second);
                    if (!prepared.IsSuccess())
                    {
                        rollback(QString::fromUtf8(prepared.GetError().c_str()));
                        return;
                    }
                    if (prepared.GetValue())
                    {
                        TextureSidecarSnapshot snapshot;
                        const auto sidecarPath = entry.first + ".assetinfo";
                        if (!CaptureTextureSidecar(sidecarPath, snapshot))
                        {
                            rollback(tr("Se creó una configuración, pero no se pudo verificar su identidad. Se conserva para revisión: %1")
                                .arg(QString::fromUtf8(sidecarPath.c_str())));
                            return;
                        }
                        created.push_back(AZStd::move(snapshot));
                    }
                }
                const auto written = WriteMaterialSourceFileNew(source.GetValue(), path);
                if (!written.IsSuccess())
                {
                    rollback(QString::fromUtf8(written.GetError().c_str()));
                    return;
                }
                AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                    &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                    path.c_str(), AZ::Crc32(), false);
                SelectGeneratedMaterial(m_browserCaller.data(), path);
            }
            for (auto* choice : m_textureChoices)
            {
                choice->setEnabled(false);
            }
            if (m_textureCreate)
            {
                m_textureCreate->setEnabled(false);
            }
            hide();
            deleteLater();
        }

        MaterialBrowserInteractions::MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }

        MaterialBrowserInteractions::~MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void MaterialBrowserInteractions::AddContextMenuActions(
            QWidget* caller, QMenu* menu,
            const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
        {
            if (!menu || entries.size() != 1)
            {
                return;
            }
            const auto* source = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entries.front());
            if (!source || !IsMaterialTextureSource(source->GetFullPath()))
            {
                return;
            }
            const AZStd::string path = source->GetFullPath();
            auto* action = menu->addAction(QObject::tr("Generar material"));
            QWidget* owner = caller;
            if (!owner)
            {
                AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                    owner, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);
            }
            const QPointer<QWidget> context(owner);
            QObject::connect(action, &QAction::triggered, action, [path, context]()
            {
                if (!context)
                {
                    return;
                }
                delete context->findChild<QDialog*>("MaterialTextureReview", Qt::FindDirectChildrenOnly);
                auto* review = new TextureMaterialCreationDialog(context.data());
                review->GenerateMaterialFromTexture(path);
            });
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
            return JE::Editor::CreateMaterialSourceFile(materialTypePath, materialPath);
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
