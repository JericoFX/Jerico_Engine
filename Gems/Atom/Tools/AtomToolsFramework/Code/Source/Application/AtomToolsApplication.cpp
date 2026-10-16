/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <JE/Debug/Profiler/ProfilerModule.h>
#include <JE/Render/Debug/RenderDebugModule.h>
#include <JE/World/Camera/Debug/DebugCameraModule.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AtomToolsFramework/Application/AtomToolsApplication.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/AzFrameworkNativeUIModule.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AzToolsFrameworkModule.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include "AtomToolsFramework_Traits_Platform.h"

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QClipboard>
#include <QMessageBox>
#include <QObject>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsApplication* AtomToolsApplication::m_instance = {};

    AtomToolsApplication::AtomToolsApplication(const char* targetName, int* argc, char*** argv)
        : AtomToolsApplication(targetName, argc, argv, {})
    {
    }

    AtomToolsApplication::AtomToolsApplication(const char* targetName, int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
        : AzQtApplication(*argc, *argv)
        , Application(argc, argv, AZStd::move(componentAppSettings))
        , m_targetName(targetName)
        , m_toolId(targetName)
    {
        m_instance = this;

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), m_targetName);

        // Suppress spam from the Source Control system
        m_traceLogger.AddWindowFilter(AzToolsFramework::SCC_WINDOW);

        const AZ::IO::FixedMaxPath engineRootPath(
            GetSettingsValue(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, AZStd::string()));
        m_styleManager.reset(new AzQtComponents::StyleManager(this));
        m_styleManager->initialize(this, engineRootPath);
    }

    AtomToolsApplication ::~AtomToolsApplication()
    {
        m_instance = {};
        m_styleManager.reset();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
    }

    void AtomToolsApplication::CreateReflectionManager()
    {
        Base::CreateReflectionManager();
        GetSerializeContext()->CreateEditContext();
    }

    const char* AtomToolsApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseAtomTools";
#elif defined(_DEBUG)
        return "DebugAtomTools";
#else
        return "ProfileAtomTools";
#endif
    }

    void AtomToolsApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);

        AzToolsFramework::QTreeViewWithStateSaving::Reflect(context);
        AzToolsFramework::QWidgetSavedState::Reflect(context);
        ReflectUtilFunctions(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Group native automation methods in the general reflection module
            auto addGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "atomtools.general");
            };

            addGeneral(behaviorContext->Method(
                "idle_wait_frames", &AtomToolsApplication::IdleWaitFrames, nullptr,
                "Waits idling for a frames. Primarily used for auto-testing."));
            addGeneral(behaviorContext->Method(
                "exit", &AtomToolsApplication::ExitApplication, nullptr,
                "Exit application. Primarily used for auto-testing."));
            addGeneral(behaviorContext->Method(
                "crash", &AtomToolsApplication::CrashApplication, nullptr,
                "Crashes the application, useful for testing crash reporting and other automation tools."));
            addGeneral(behaviorContext->Method(
                "test_output", &AtomToolsApplication::TestOutput, nullptr,
                "Report test information."));
        }
    }

    void AtomToolsApplication::RegisterCoreComponents()
    {
        Base::RegisterCoreComponents();
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PerforceComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList AtomToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Base::GetRequiredSystemComponents();

        components.insert(
            components.end(),
            {
                azrtti_typeid<AzToolsFramework::ActionManagerSystemComponent>(),
                azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>(),
                azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
                azrtti_typeid<AzToolsFramework::Components::PropertyManagerComponent>(),
                azrtti_typeid<AzToolsFramework::PerforceComponent>(),
                azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>(),
                azrtti_typeid<AzFramework::NativeUISystemComponent>(),
            });

        return components;
    }

    void AtomToolsApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew AzToolsFramework::AzToolsFrameworkModule);
        outModules.push_back(aznew AzFramework::AzFrameworkNativeUIModule());
        outModules.emplace_back(JE::Debug::Profiler::CreateVisualModule());
        outModules.emplace_back(JE::Render::Debug::CreateViewportModule());
        outModules.emplace_back(JE::Render::Debug::CreateToolsModule());
        outModules.emplace_back(JE::World::Camera::Debug::CreateDebugModule());
        outModules.emplace_back(JE::Editor::RenderIntegration::CreateAuthoringModule());
        outModules.emplace_back(JE::Assets::RenderBuilders::CreateToolsModule());
        outModules.emplace_back(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
    }

    void AtomToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        // Before serializing data to the log file, determine if it should be cleared first.
        const bool clearLogFile = GetSettingsValue("/O3DE/AtomToolsFramework/Application/ClearLogOnStart", false);

        // Now that the base application is initialized, open the file to record any log messages and dump any pending content into it.
        if (m_commandLine.HasSwitch("logfile"))
        {
            // If a custom log file name was supplied via command line, redirect output to it.
            m_traceLogger.OpenLogFile(m_commandLine.GetSwitchValue("logfile", 0), clearLogFile);
        }
        else
        {
            m_traceLogger.OpenLogFile(m_targetName + ".log", clearLogFile);
        }

        ConnectToAssetProcessor();

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);

        // Disabling source control integration by default to disable messages and menus if no supported source control system is active
        const bool enableSourceControl = GetSettingsValue("/O3DE/AtomToolsFramework/Application/EnableSourceControl", false);
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(
            &AzToolsFramework::SourceControlConnectionRequests::EnableSourceControl, enableSourceControl);

        auto rpiInterface = AZ::RPI::RPISystemInterface::Get();
        if (rpiInterface && !rpiInterface->IsInitialized())
        {
            AZ::RPI::RPISystemInterface::Get()->InitializeSystemAssets();
        }

        LoadSettings();

        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);

        // Handle command line options for setting up a test environment that should not be affected forwarding commands from other
        // instances of an application
        if (m_commandLine.HasSwitch("autotest_mode"))
        {
            // Nullroute all stdout to null for automated tests, this way we make sure
            // that the test result output is not polluted with unrelated output data.
            RedirectStdoutToNull();
        }
        else
        {
            // Enable native UI for some low level system popup message when it's not in automated test mode
            if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
            {
                nativeUI->SetMode(AZ::NativeUI::Mode::ENABLED);
            }
        }

        // Per Qt documentation, forcing Stop to be called when the application is about to quit in case exit bypasses Stop or destructor
        connect(this, &QApplication::aboutToQuit, this, [this] {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
            Stop();
        });
    }

    void AtomToolsApplication::Destroy()
    {
        // Clearing graph canvas clipboard mime data for copied nodes before exiting the application to prevent a crash in qt_call_post_routines
        QApplication::clipboard()->clear();

        m_assetBrowserInteractions.reset();
        m_styleManager.reset();

        // Save application registry settings to a target specific settings file. The file must be named so that it is only loaded for an
        // application with the corresponding target name.
        AZStd::string settingsFileName(AZStd::string::format("usersettings.%s.setreg", m_targetName.c_str()));
        AZStd::to_lower(settingsFileName.begin(), settingsFileName.end());

        const AZ::IO::FixedMaxPath settingsFilePath(
            AZStd::string::format("%s/user/Registry/%s", AZ::Utils::GetProjectPath().c_str(), settingsFileName.c_str()));

        // This will only save modified registry settings that match the following filters
        const AZStd::vector<AZStd::string> filters = {
            "/O3DE/AtomToolsFramework", "/O3DE/Atom/Tools", AZStd::string::format("/O3DE/Atom/%s", m_targetName.c_str()) }; 

        SaveSettingsToFile(settingsFilePath, filters);

        // Handler for serializing legacy user settings
        UnloadSettings();

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

#if AZ_TRAIT_ATOMTOOLSFRAMEWORK_SKIP_APP_DESTROY
        ::_exit(0);
#else
        Base::Destroy();
#endif
    }

    void AtomToolsApplication::RunMainLoop()
    {
        // Start initial command line processing and application update as part of the Qt event loop
        QTimer::singleShot(0, this, [this]() { OnIdle(); ProcessCommandLine(m_commandLine); });
        exec();
    }

    void AtomToolsApplication::OnIdle()
    {
        // Process a single application tick unless exit was requested
        if (!WasExitMainLoopRequested())
        {
            PumpSystemEventLoopUntilEmpty();
            TickSystem();
            Tick();

            // Rescheduling the update every frame with an interval based on the state of the application.
            // This allows the tool to free up resources for other processes when it's not in focus.
            const int updateInterval = (applicationState() & Qt::ApplicationActive)
                ? aznumeric_cast<int>(GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/Application/UpdateIntervalWhenActive", 1))
                : aznumeric_cast<int>(GetSettingsValue<AZ::u64>("/O3DE/AtomToolsFramework/Application/UpdateIntervalWhenNotActive", 250));
            QTimer::singleShot(updateInterval, this, &AtomToolsApplication::OnIdle);
            return;
        }

        quit();
    }

    AZStd::vector<AZStd::string> AtomToolsApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({});
    }

    void AtomToolsApplication::ConnectToAssetProcessor()
    {
        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection =
            AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = m_targetName;
        connectionSettings.m_loggingCallback = [targetName = m_targetName]([[maybe_unused]] AZStd::string_view logData)
        {
            AZ_UNUSED(targetName);  // Prevent unused warning in release builds
            AZ_TracePrintf(targetName.c_str(), "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
        };

        AzFramework::AssetSystemRequestBus::BroadcastResult(
            connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

        if (connectedToAssetProcessor)
        {
            CompileCriticalAssets();
        }
    }

    void AtomToolsApplication::CompileCriticalAssets()
    {
        AZ_TracePrintf(m_targetName.c_str(), "Compiling critical assets.\n");

        QStringList failedAssets;

        // Forced asset processor to synchronously process all critical assets
        // Note: with AssetManager's current implementation, a compiled asset won't be added in asset registry until next system tick.
        // So the asset id won't be found right after CompileAssetSync call.
        for (const AZStd::string& assetFilters : GetCriticalAssetFilters())
        {
            AZ_TracePrintf(m_targetName.c_str(), "Compiling critical asset matching: %s.\n", assetFilters.c_str());

            // Wait for the asset be compiled
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, assetFilters);
            if (status != AzFramework::AssetSystem::AssetStatus_Compiled && status != AzFramework::AssetSystem::AssetStatus_Unknown)
            {
                failedAssets.append(assetFilters.c_str());
            }
        }

        if (!failedAssets.empty())
        {
            QMessageBox::critical(
                GetToolMainWindow(),
                QString("Failed to compile critical assets"),
                QString("Failed to compile the following critical assets:\n%1\n%2")
                .arg(failedAssets.join(",\n"))
                .arg("Make sure this is an Atom project."));
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
        }

        // Reload the assetcatalog.xml at this point again
        // Start monitoring asset changes over the network and load the AssetCatalog
        auto LoadCatalog = [settingsRegistry = m_settingsRegistry.get()](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
        {
            if (AZ::IO::FixedMaxPath assetCatalogPath;
                settingsRegistry->Get(assetCatalogPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
            {
                assetCatalogPath /= "assetcatalog.xml";
                assetCatalogRequests->LoadCatalog(assetCatalogPath.c_str());
            }
        };
        AZ::Data::AssetCatalogRequestBus::Broadcast(AZStd::move(LoadCatalog));

        // Only signal the event *after* the asset catalog has been loaded.
        AZ::ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "CriticalAssetsCompiled", R"({})");
    }

    void AtomToolsApplication::SaveSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            char resolvedPath[AZ_MAX_PATH_LEN] = "";
            AZStd::string fileName = "@user@/" + m_targetName + "UserSettings.xml";

            AZ::IO::FileIOBase::GetInstance()->ResolvePath(
                fileName.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            m_localUserSettings.Save(resolvedPath, context);
        }
    }

    void AtomToolsApplication::LoadSettings()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(context, "No serialize context");

        char resolvedPath[AZ_MAX_PATH_LEN] = "";
        AZStd::string fileName = "@user@/" + m_targetName + "UserSettings.xml";

        AZ::IO::FileIOBase::GetInstance()->ResolvePath(fileName.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

        m_localUserSettings.Load(resolvedPath, context);
        m_localUserSettings.Activate(AZ::UserSettings::CT_LOCAL);
        AZ::UserSettingsOwnerRequestBus::Handler::BusConnect(AZ::UserSettings::CT_LOCAL);
        m_activatedLocalUserSettings = true;
    }

    void AtomToolsApplication::UnloadSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            SaveSettings();
            m_localUserSettings.Deactivate();
            AZ::UserSettingsOwnerRequestBus::Handler::BusDisconnect();
            m_activatedLocalUserSettings = false;
        }
    }

    void AtomToolsApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        if (commandLine.HasSwitch("activatewindow"))
        {
            AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Handler::ActivateWindow);
        }

        const AZStd::string timeoutSwitchName  = "timeout";
        if (commandLine.HasSwitch(timeoutSwitchName ))
        {
            const AZStd::string& timeoutValue = commandLine.GetSwitchValue(timeoutSwitchName , 0);
            const uint32_t timeoutInMs = atoi(timeoutValue.c_str());
            AZ_Printf(m_targetName.c_str(), "Timeout scheduled, shutting down in %u ms", timeoutInMs);
            QTimer::singleShot(timeoutInMs, this, [targetName = m_targetName]{
                AZ_Printf(targetName.c_str(), "Timeout reached, shutting down");
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
            });
        }

        if (commandLine.HasSwitch("autotest_mode") ||
            commandLine.HasSwitch("exitaftercommands"))
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
        }
    }

    void AtomToolsApplication::PrintAlways(const AZStd::string& output)
    {
        m_stdoutRedirection.WriteBypassingRedirect(output.c_str(), static_cast<unsigned int>(output.size()));
    }

    void AtomToolsApplication::RedirectStdoutToNull()
    {
        m_stdoutRedirection.RedirectTo(AZ::IO::SystemFile::GetNullFilename());
    }

    bool AtomToolsApplication::LaunchLocalServer()
    {
        // The socket and server are currently used to forward all requests to an existing application process if one is already running.
        // These additional settings will allow multiple instances to be launched in automated testing batch mode and other scenarios.
        const bool allowMultipleInstances = GetSettingsValue("/O3DE/AtomToolsFramework/Application/AllowMultipleInstances", false);
        if (allowMultipleInstances || m_commandLine.HasSwitch("allowMultipleInstances") || m_commandLine.HasSwitch("batchmode"))
        {
            return true;
        }

        // Determine if this is the first launch of the tool by attempting to connect to a running server
        if (m_socket.Connect(QApplication::applicationName()))
        {
            // If the server was located, the application is already running.
            // Forward commandline options to other application instance.
            QByteArray buffer;
            buffer.append("ProcessCommandLine:");

            // Add the command line options from this process to the message, skipping the executable path
            for (int argi = 1; argi < m_argC; ++argi)
            {
                buffer.append(QString(m_argV[argi]).append("\n").toUtf8());
            }

            // Inject command line option to always bring the main window to the foreground
            buffer.append("--activatewindow\n");

            m_socket.Send(buffer);
            m_socket.Disconnect();
            return false;
        }

        // Setup server to handle basic commands
        m_server.SetReadHandler(
            [this](const QByteArray& buffer)
            {
                // Handle commmand line params from connected socket
                if (buffer.startsWith("ProcessCommandLine:"))
                {
                    // Remove header and parse commands
                    AZStd::string params(buffer.data(), buffer.size());
                    params = params.substr(strlen("ProcessCommandLine:"));

                    AZStd::vector<AZStd::string> tokens;
                    AZ::StringFunc::Tokenize(params, tokens, "\n");

                    if (!tokens.empty())
                    {
                        AZ::CommandLine commandLine;
                        commandLine.Parse(tokens);
                        QTimer::singleShot(0, this, [this, commandLine]() { ProcessCommandLine(commandLine); });
                    }
                }
            });

        // Launch local server
        if (!m_server.Connect(QApplication::applicationName()))
        {
            return false;
        }

        return true;
    }

    bool AtomToolsApplication::GetAssetDatabaseLocation(AZStd::string& result)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath assetDatabaseSqlitePath;
        if (settingsRegistry &&
            settingsRegistry->Get(assetDatabaseSqlitePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            assetDatabaseSqlitePath /= "assetdb.sqlite";
            result = AZStd::string_view(assetDatabaseSqlitePath.Native());
            return true;
        }

        return false;
    }

    void AtomToolsApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Tool;
    }

    void AtomToolsApplication::IdleWaitFrames(uint32_t frames)
    {
        // Create a child event loop that takes control of updating the application for a set number of frames.
        // When executed from a script, this continues to update the application but allows the script to pause until the number of frames
        // have passed.
        QEventLoop loop;
        QTimer timer;

        uint32_t frame = 0;
        QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
            auto app = AtomToolsApplication::GetInstance();
            if (app && !app->WasExitMainLoopRequested() && frame++ < frames)
            {
                app->PumpSystemEventLoopUntilEmpty();
                app->TickSystem();
                app->Tick();
                return;
            }

            timer.stop();
            loop.quit();
        });

        timer.setInterval(0);
        timer.start();
        loop.exec();
    }

    void AtomToolsApplication::ExitApplication()
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
    }

    void AtomToolsApplication::CrashApplication()
    {
        AZ_Crash();
    }

    void AtomToolsApplication::TestOutput(const AZStd::string& output)
    {
        AtomToolsApplication::GetInstance()->PrintAlways(output);
    }

    AtomToolsApplication* AtomToolsApplication::GetInstance()
    {
        return m_instance;
    }
} // namespace AtomToolsFramework
