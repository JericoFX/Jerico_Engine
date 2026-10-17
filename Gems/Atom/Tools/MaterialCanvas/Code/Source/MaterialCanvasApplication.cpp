/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/Render/Scene/SceneModule.h>
#include <Atom/RHI/Factory.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentView.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include <MaterialCanvasApplication.h>
#include <JE/Render/Materials/MaterialsRegistration.h>
#include <Window/MaterialCanvasMainWindow.h>

#include <QLabel>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif

namespace MaterialCanvas
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    MaterialCanvasApplication::MaterialCanvasApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
#if defined(EXTERNAL_CRASH_REPORTING)
        CrashHandler::ToolsCrashHandler::InitCrashHandler("MaterialCanvas", {});
#endif

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Material Canvas");

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AZ::RHI::FactoryManagerNotificationBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    MaterialCanvasApplication::~MaterialCanvasApplication()
    {
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        AZ::RHI::FactoryManagerNotificationBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialCanvasApplication::RegisterCoreComponents()
    {
        Base::RegisterCoreComponents();
        RegisterComponentDescriptor(JE::Render::CreateMaterialComponentDescriptor());
    }

    void MaterialCanvasApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.emplace_back(JE::Editor::Render::Scene::CreateModule());
        outModules.emplace_back(JE::Editor::Camera::CreateAuthoringModule());
    }

    void MaterialCanvasApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        JE::Editor::MaterialGraphEnvironment::Reflect(context);
    }

    const char* MaterialCanvasApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialCanvas";
#elif defined(_DEBUG)
        return "DebugMaterialCanvas";
#else
        return "ProfileMaterialCanvas";
#endif
    }

    void MaterialCanvasApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        m_graphEnvironment = AZStd::make_unique<JE::Editor::MaterialGraphEnvironment>(m_toolId);
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));
        InitDocumentTypes();
        InitMainWindow();
        InitDefaultDocument();
    }

    void MaterialCanvasApplication::Destroy()
    {
        m_window.reset();
        m_viewportSettingsSystem.reset();
        // Documents and their views must release the graph services before application teardown.
        m_documentSystem.reset();
        m_graphEnvironment.reset();

        ApplyShaderBuildSettings();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/", "MaterialCanvas/" });
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }

    void MaterialCanvasApplication::FactoryRegistered()
    {
        ApplyShaderBuildSettings();
    }

    void MaterialCanvasApplication::InitDocumentTypes()
    {
        for (auto documentType : m_graphEnvironment->BuildDocumentTypes())
        {
            const bool isGraph = documentType.IsSupportedExtensionToSave("document.materialgraph");
            const auto viewSettings = m_graphEnvironment->GetViewSettings();
            documentType.m_documentViewFactoryCallback =
                [this, isGraph, viewSettings](const AZ::Crc32& toolId, const AZ::Uuid& documentId)
                {
                    if (isGraph)
                    {
                        return m_window->AddDocumentTab(documentId,
                            aznew AtomToolsFramework::GraphDocumentView(toolId, documentId, viewSettings, m_window.get()));
                    }
                    auto* view = new QLabel("Document properties can be edited in the inspector.", m_window.get());
                    view->setAlignment(Qt::AlignCenter);
                    return m_window->AddDocumentTab(documentId, view);
                };
            m_documentSystem->RegisterDocumentType(documentType);
        }
    }

    void MaterialCanvasApplication::InitMainWindow()
    {
        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialCanvasMainWindow(m_toolId, m_graphEnvironment->GetViewSettings()));
        m_window->show();
    }

    void MaterialCanvasApplication::InitDefaultDocument()
    {
        // Create an untitled, empty graph document as soon as the application starts so the user can begin creating immediately.
        if (AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", true))
        {
            AZ::Uuid documentId = AZ::Uuid::CreateNull();
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                documentId,
                m_toolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::CreateDocumentFromTypeName,
                "Material Graph");

            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::OnDocumentOpened, documentId);
        }
    }

    void MaterialCanvasApplication::ApplyShaderBuildSettings()
    {
        // If faster shader build settings are enabled, copy a settings registry file stub into the user settings folder. This will
        // override AP and shader build settings, disabling shaders and shader variant building for inactive platforms and RHI. Copying any
        // of these settings files requires restarting the application and the asset processor for the changes to be picked up.
        if (auto fileIO = AZ::IO::FileIOBase::GetInstance())
        {
            const AZ::IO::FixedMaxPath materialCanvasGemPath = AZ::Utils::GetGemPath("MaterialCanvas");
            const auto settingsPathStub(
                materialCanvasGemPath / AZ::SettingsRegistryConstants::RegistryFolder / "user_minimal_shader_build.setregstub");
            const auto settingsPathDx12Stub(
                materialCanvasGemPath / AZ::SettingsRegistryConstants::RegistryFolder / "user_minimal_shader_build_dx12.setregstub");

            const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            const auto settingsPath(
                projectPath / AZ::SettingsRegistryConstants::DevUserRegistryFolder / "user_minimal_shader_build.setreg");
            const auto settingsPathDx12(
                projectPath / AZ::SettingsRegistryConstants::DevUserRegistryFolder / "user_minimal_shader_build_dx12.setreg");

            const bool enableFasterShaderBuilds =
                AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/EnableFasterShaderBuilds", false);

            if (enableFasterShaderBuilds)
            {
                // Windows is the only platform with multiple, non-null RHI, supporting Vulkan and DX12. If DX12 is the active RHI then it
                // will require copying its own settings file. Settings files for inactive RHI will be deleted from the user folder. 
                if (const AZ::Name apiName = AZ::RHI::Factory::Get().GetName(); apiName == AZ::Name("dx12"))
                {
                    fileIO->Copy(settingsPathDx12Stub.c_str(), settingsPathDx12.c_str());
                    fileIO->Remove(settingsPath.c_str());
                }
                else
                {
                    fileIO->Copy(settingsPathStub.c_str(), settingsPath.c_str());
                    fileIO->Remove(settingsPathDx12.c_str());
                }
            }
            else
            {
                fileIO->Remove(settingsPath.c_str());
                fileIO->Remove(settingsPathDx12.c_str());
            }
        }
    }
} // namespace MaterialCanvas
