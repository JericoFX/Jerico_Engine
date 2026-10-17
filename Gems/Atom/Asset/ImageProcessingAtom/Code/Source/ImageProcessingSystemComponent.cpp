/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/string_view.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <BuilderSettings/TextureSettings.h>
#include <Editor/TexturePropertyEditor.h>

#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImagePreview.h>
#include <Processing/ImageToProcess.h>
#include <Processing/Utils.h>

#include "ImageProcessingSystemComponent.h"
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QThread>

namespace ImageProcessingAtom
{
    void ImageProcessingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ImageProcessingSystemComponent, AZ::Component>()
                ->Version(0)
            ;
        }
    }

    void ImageProcessingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomImageBuilderService"));
    }

    void ImageProcessingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomImageBuilderService"));
    }

    void ImageProcessingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ImageProcessingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ImageProcessingSystemComponent::Init()
    {
        m_previewerFactory.reset(new ImagePreviewerFactory);
    }

    void ImageProcessingSystemComponent::Activate()
    {
        // Note: the editor initialization will only report incompatible components if we have two system components are incompatible.
        // It won't interrupt the initialization. And the CSystem::Init would continue and report totally irrelevant message 
        // due to cinematic system pointer was empty since the ActivateEntities returned early
        // This is a temporary solution until we have the proper system for gem incompatible mechanism in place ( LY-105408)
        // Here it would pop out a message box if we found the LY ImageProcessingSystemComponent was reflected. 
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext->FindClassData(AZ::Uuid("{13B1EB88-316F-4D44-B59C-886F023A5A58}")))
        {
            AZ::OSString errorMsg = "Incompatible gem detected. Please disable ImageProcessing Gem for Atom project";
            AZ_Error("ImageProcessingAtom", false, errorMsg.data());
            AZ::NativeUI::NativeUIRequestBus::Broadcast(&AZ::NativeUI::NativeUIRequestBus::Events::DisplayOkDialog, "", errorMsg.c_str(), false);
            return;
        }

        // Call to allocate BuilderSettingManager
        BuilderSettingManager::CreateInstance();

        ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusConnect();

        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusConnect();
        ImageProcessingRequestBus::Handler::BusConnect();
    }

    void ImageProcessingSystemComponent::Deactivate()
    {
        ImageProcessingRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusDisconnect();
        ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();

        // Deallocate BuilderSettingManager
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
    }

    void ImageProcessingSystemComponent::OpenSourceTextureFile(const AZ::Uuid& textureSourceID)
    {
        ImageProcessingAtomEditor::TexturePropertyEditor editor(textureSourceID, QApplication::activeWindow());
        editor.exec();
    }

    AZ::Outcome<bool, AZStd::string> ImageProcessingSystemComponent::EnsureTexturePresetIfUnset(
        const AZStd::string& sourcePath, const AZStd::string& presetName)
    {
        const bool color = presetName == "Albedo" || presetName == "Emissive";
        const bool scalar = presetName == "Reflectance" || presetName == "AmbientOcclusion" || presetName == "Displacement";
        const bool normal = presetName == "Normals";
        const bool packed = presetName == "LayerMask";
        if (!color && !scalar && !normal && !packed)
        {
            return AZ::Failure(AZStd::string("The requested preset is not supported by texture-set authoring."));
        }
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        const char* projectAlias = fileIO ? fileIO->GetAlias("@projectroot@") : nullptr;
        const QString project = projectAlias ? QFileInfo(QString::fromUtf8(projectAlias)).canonicalFilePath() : QString{};
        const QFileInfo image(QString::fromUtf8(sourcePath.c_str()));
        const QString source = image.canonicalFilePath();
        const auto inside = [](const QString& root, const QString& path)
        {
            const QString relative = QDir(root).relativeFilePath(path);
            return !root.isEmpty() && !path.isEmpty() && !QDir::isAbsolutePath(relative) &&
                relative != ".." && !relative.startsWith("../");
        };
        if (!image.isFile() || !image.isAbsolute() || !inside(project, source) ||
            !IsExtensionSupported(image.suffix().toUtf8().constData()))
        {
            return AZ::Failure(AZStd::string("Select an existing project texture source, outside engine and third-party folders."));
        }
        const QString firstFolder = QDir(project).relativeFilePath(source).section('/', 0, 0).toLower();
        if (firstFolder == "cache" || firstFolder == "user" || firstFolder == "build" ||
            firstFolder == "gems" || firstFolder == "3rdparty" || firstFolder == "external")
        {
            return AZ::Failure(AZStd::string("Generated, gem and third-party folders are not texture-set authoring destinations."));
        }
        const AZStd::string canonicalSource(source.toUtf8().constData());
        bool sourceFolderFound = false;
        AZStd::string relativeSource;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceFolderFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GenerateRelativeSourcePath,
            canonicalSource, relativeSource, watchFolder);
        const QString canonicalWatch = QFileInfo(QString::fromUtf8(watchFolder.c_str())).canonicalFilePath();
        if (!sourceFolderFound || !inside(project, canonicalWatch) || !inside(canonicalWatch, source))
        {
            return AZ::Failure(AZStd::string("The texture is not in an authoring scan folder of the active project."));
        }
        const QString destination = source + QString::fromUtf8(TextureSettings::ExtensionName);
        auto* manager = BuilderSettingManager::Instance(); // Owned and configured by the existing Image Processing component.
        const PresetName requested(presetName);
        const PresetSettings* preset = manager->GetPreset(requested);
        if (!preset)
        {
            return AZ::Failure(AZStd::string("The requested built-in image preset is not loaded. Review Image Processing settings."));
        }
        const auto compatible = [color, scalar, normal, packed](const PresetSettings* value)
        {
            if (!value || value->m_generateIBLOnly || value->m_cubemapSetting ||
                value->m_outputTypeHandling != PresetSettings::UseSpecifiedOutputType)
            {
                return false;
            }
            if (color)
            {
                return value->m_srcColorSpace == ColorSpace::sRGB &&
                    (value->m_destColorSpace == ColorSpace::sRGB || value->m_destColorSpace == ColorSpace::autoSelect) &&
                    value->m_swizzle.empty() && !value->m_isMipRenormalize &&
                    (value->m_pixelFormat == ePixelFormat_BC1 || value->m_pixelFormat == ePixelFormat_BC3 || value->m_pixelFormat == ePixelFormat_BC7 ||
                        value->m_pixelFormat == ePixelFormat_ASTC_4x4 || value->m_pixelFormat == ePixelFormat_ASTC_6x6);
            }
            if (value->m_srcColorSpace != ColorSpace::linear || value->m_destColorSpace != ColorSpace::linear)
            {
                return false;
            }
            if (packed)
            {
                // Preserve every data channel and the authored extent. No scalar compression, swizzle or normal-vector filtering.
                return value->m_pixelFormat == ePixelFormat_R8G8B8X8 && value->m_swizzle.empty() &&
                    !value->m_isMipRenormalize && !value->m_discardAlpha && !value->m_isPowerOf2 &&
                    value->m_sizeReduceLevel == 0 && value->m_maxTextureSize == 0 && value->m_minTextureSize == 0 &&
                    value->m_glossFromNormals == 0 && !value->m_mipmapSetting;
            }
            if (normal)
            {
                return value->m_isMipRenormalize && value->m_swizzle.empty() &&
                    (value->m_pixelFormat == ePixelFormat_BC5s || value->m_pixelFormat == ePixelFormat_ASTC_4x4);
            }
            return scalar && !value->m_isMipRenormalize &&
                (value->m_swizzle.empty() || value->m_swizzle == "rrr1") &&
                (value->m_pixelFormat == ePixelFormat_BC4 || value->m_pixelFormat == ePixelFormat_ASTC_4x4 ||
                    value->m_pixelFormat == ePixelFormat_ASTC_6x6);
        };
        if (!compatible(preset))
        {
            return AZ::Failure(AZStd::string("The effective preset no longer preserves the requested texture data. Review its settings."));
        }
        for (const auto& platform : manager->GetPlatformList())
        {
            if (!compatible(manager->GetPreset(requested, platform)))
            {
                return AZ::Failure(AZStd::string::format("The image preset requires review for platform %s.", platform.c_str()));
            }
        }
        if (QFileInfo::exists(destination))
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (!context || !context->FindClassData(azrtti_typeid<TextureSettings>()))
            {
                return AZ::Failure(AZStd::string("Image Processing texture settings are not reflected."));
            }
            QFile file(destination);
            if (!file.open(QIODevice::ReadOnly))
            {
                return AZ::Failure(AZStd::string("Existing texture settings could not be read; they were preserved."));
            }
            const QByteArray bytes = file.readAll();
            if (file.error() != QFileDevice::NoError)
            {
                return AZ::Failure(AZStd::string("Existing texture settings could not be read; they were preserved."));
            }
            // Convert only synchronous serializer diagnostics from this operation into an inline failure.
            // Pre-event observers still see these messages; asserts and other threads/windows are not intercepted.
            class SettingsDiagnostics final : public AZ::Debug::TraceMessageBus::Handler
            {
            public:
                SettingsDiagnostics() { BusConnect(); }
                ~SettingsDiagnostics() override { BusDisconnect(); }
                bool OnPreError(const char* window, const char*, int, const char*, const char* message) override
                {
                    return Capture(window, message);
                }
                bool OnPreWarning(const char* window, const char*, int, const char*, const char* message) override
                {
                    return Capture(window, message);
                }
                AZStd::string m_message;
            private:
                bool Capture(const char* window, const char* message)
                {
                    if (QThread::currentThreadId() != m_thread || !window ||
                        (AZStd::string_view(window) != "Serialize" && AZStd::string_view(window) != "Serialization"))
                    {
                        return false;
                    }
                    if (!m_message.empty())
                    {
                        m_message += '\n';
                    }
                    m_message += message;
                    return true;
                }
                const Qt::HANDLE m_thread = QThread::currentThreadId();
            } diagnostics;
            TextureSettings settings;
            AZ::IO::MemoryStream stream(bytes.constData(), static_cast<size_t>(bytes.size()));
            const AZ::ObjectStream::FilterDescriptor filter(nullptr, AZ::ObjectStream::FILTERFLAG_STRICT);
            if (!AZ::Utils::LoadObjectFromStreamInPlace(stream, context, azrtti_typeid<TextureSettings>(), &settings, filter) ||
                !diagnostics.m_message.empty())
            {
                return AZ::Failure(AZStd::string::format("Existing texture settings could not be loaded: %s", diagnostics.m_message.c_str()));
            }
            const auto matchesRequested = [manager, &requested, packed](TextureSettings& value)
            {
                // Preserve the native legacy UUID-to-name interpretation without applying or changing a preset.
                if (value.m_preset.IsEmpty())
                {
                    value.m_preset = manager->GetPresetNameFromId(value.m_presetId);
                }
                return value.m_preset == requested && value.m_mipAlphaAdjust.size() == TextureSettings::s_MaxMipMaps &&
                    (!packed || value.m_sizeReduceLevel == 0);
            };
            if (!matchesRequested(settings))
            {
                return AZ::Failure(AZStd::string("Existing texture settings use a different or invalid preset. Review them in Edit Texture Settings."));
            }
            for (const auto& platform : manager->GetPlatformList())
            {
                TextureSettings effective = settings;
                if (settings.HasPlatformOverride(platform) &&
                    !TextureSettings::GetPlatformSpecificTextureSetting(platform, settings, effective, context).IsSuccess())
                {
                    return AZ::Failure(AZStd::string::format("Existing texture settings have an invalid override for %s.", platform.c_str()));
                }
                if (!matchesRequested(effective) || !diagnostics.m_message.empty())
                {
                    return AZ::Failure(AZStd::string::format("Existing texture settings require review for platform %s.", platform.c_str()));
                }
            }
            QFile current(destination);
            if (!current.open(QIODevice::ReadOnly) || current.readAll() != bytes || current.error() != QFileDevice::NoError)
            {
                return AZ::Failure(AZStd::string("Texture settings changed while being read; review them before generating the material."));
            }
            return AZ::Success(false);
        }
        const auto inferred = manager->GetPresetsForFileMask(manager->GetFileMask(canonicalSource));
        if ((inferred.size() == 1 && inferred.front() == requested) ||
            (inferred.empty() && manager->GetDefaultPreset() == requested && manager->GetDefaultAlphaPreset() == requested) ||
            (inferred.empty() && presetName == "Albedo" && manager->GetDefaultPreset() == PresetName("Albedo") &&
                manager->GetDefaultAlphaPreset() == PresetName("AlbedoWithGenericAlpha") &&
                compatible(manager->GetPreset(PresetName("AlbedoWithGenericAlpha")))))
        {
            // Do not decode large images in the editor just to inspect alpha; both default color presets preserve base-color RGB.
            return AZ::Success(false);
        }
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!context || !context->FindClassData(azrtti_typeid<TextureSettings>()))
        {
            return AZ::Failure(AZStd::string("Image Processing texture settings are not reflected."));
        }
        TextureSettings settings;
        settings.ApplyPreset(requested);
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        if (!AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &settings, context))
        {
            return AZ::Failure(AZStd::string("Failed to serialize image settings."));
        }
        QTemporaryFile temporary(destination + ".je-XXXXXX.tmp");
        if (!temporary.open() || temporary.write(reinterpret_cast<const char*>(bytes.data()), static_cast<qint64>(bytes.size())) !=
            static_cast<qint64>(bytes.size()) || !temporary.flush())
        {
            return AZ::Failure(AZStd::string("Failed to write the temporary image settings."));
        }
        const AZStd::string temporaryPath(temporary.fileName().toUtf8().constData());
        temporary.close();
        TextureSettings verified;
        if (!TextureSettings::LoadTextureSetting(temporaryPath, verified, context).IsSuccess() ||
            !verified.Equals(settings, context) || verified.m_enableMipmap != settings.m_enableMipmap)
        {
            return AZ::Failure(AZStd::string("Failed to verify the serialized image settings."));
        }
        // QTemporaryFile only supports atomic renames and does not replace an existing destination, including a raced-in manual sidecar.
        if (!temporary.rename(destination))
        {
            return AZ::Failure(AZStd::string("Image settings could not be published exclusively. Existing settings were preserved."));
        }
        temporary.setAutoRemove(false);
        return AZ::Success(true);
    }
    IImageObjectPtr ImageProcessingSystemComponent::LoadImage(const AZStd::string& filePath)
    {
        return IImageObjectPtr(LoadImageFromFile(filePath));
    }

    IImageObjectPtr ImageProcessingSystemComponent::LoadImagePreview(const AZStd::string& filePath)
    {
        IImageObjectPtr image(LoadImageFromFile(filePath));
        if (image)
        {
            ImageToProcess imageToProcess(image);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
            return imageToProcess.Get();
        }
        return image;
    }

    void ImageProcessingSystemComponent::AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        // Load Texture Settings
        static bool isSettingLoaded = false;

        if (!isSettingLoaded)
        {
            // Load the preset settings before editor open
            auto outcome = ImageProcessingAtom::BuilderSettingManager::Instance()->LoadConfig();

            if (outcome.IsSuccess())
            {
                isSettingLoaded = true;
            }
            else
            {
                AZ_Error("Image Processing", false, "Failed to load default preset settings!");
                return;
            }
        }

        // Register right click menu
        using namespace AzToolsFramework::AssetBrowser;
        auto entryIt = AZStd::find_if
            (
            entries.begin(),
            entries.end(),
            [](const AssetBrowserEntry* entry) -> bool
            {
                return entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source
                    || entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product;
            }
            );

        if (entryIt == entries.end())
        {
            return;
        }

        if ((*entryIt)->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            // For supported source image files, add menu item to open texture setting editor
            const SourceAssetBrowserEntry* source = azrtti_cast<const SourceAssetBrowserEntry*>(*entryIt);

            if (!HandlesSource(source))
            {
                return;
            }

            AZ::Uuid sourceId = source->GetSourceUuid();
            if (!sourceId.IsNull())
            {
                menu->addAction("Edit Texture Settings...", [sourceId, this]()
                    {
                        OpenSourceTextureFile(sourceId);
                    });
            }
        }
        else if ((*entryIt)->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            // For product which is streaming image asset, add menu item to save it to a dds file
            const ProductAssetBrowserEntry* product = azrtti_cast<const ProductAssetBrowserEntry*>(*entryIt);
            if (product->GetAssetType() == azrtti_typeid<JE::Render::RPI::StreamingImageAsset>())
            {
                AZ::Data::AssetId assetId = product->GetAssetId();
                menu->addAction("Save as DDS...", [assetId, this]()
                    {
                        QString filePath = AzQtComponents::FileDialog::GetSaveFileName(nullptr, QString("Save to file"), m_lastSavedPath, QString("DDS file (*.dds)"));
                        if (filePath.isEmpty())
                        {
                            return;
                        }
                        if (SaveStreamingImageAssetToDDS(assetId, filePath.toUtf8().data()))
                        {
                            AZ_Printf("Image Processing", "Image was saved to a dds file %s", filePath.toUtf8().data());
                            m_lastSavedPath = filePath;
                        }
                    });
            }
        }
    }

    bool ImageProcessingSystemComponent::HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const
    {
        AZStd::string targetExtension = entry->GetExtension();

        for (int i = 0; i < s_TotalSupportedImageExtensions; i++)
        {
            if (AZStd::wildcard_match(s_SupportedImageExtensions[i], targetExtension.c_str()))
            {
                return true;
            }
        }

        return false;
    }

    const AzToolsFramework::AssetBrowser::PreviewerFactory* ImageProcessingSystemComponent::GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        return m_previewerFactory->IsEntrySupported(entry) ? m_previewerFactory.get() : nullptr;
    }

    bool ImageProcessingSystemComponent::SaveStreamingImageAssetToDDS(const AZ::Data::AssetId& assetId, AZStd::string_view filePath)
    {
        IImageObjectPtr loadedImage = Utils::LoadImageFromImageAsset(assetId);
        if (!loadedImage)
        {
            AZ_Warning("ImageProcessingSystemComponent", false, "Failed to load product asset");
            return false;
        }

        if (!Utils::SaveImageToDdsFile(loadedImage, filePath))
        {
            AZ_Warning("ImageProcessingSystemComponent", false, "Failed to save image to dds file %s", filePath.data());
            return false;
        }
        return true;
    }
} // namespace ImageProcessingAtom
