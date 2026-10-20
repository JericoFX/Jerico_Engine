#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/JE/Editor/Tools/Module.h
    Source/Module.cpp
    Source/AtomToolsFrameworkSystemComponent.cpp
    Source/AtomToolsFrameworkSystemComponent.h

    Include/JE/Editor/Tools/Application/AtomToolsApplication.h
    Source/Application/AtomToolsApplication.cpp

    Include/JE/Editor/Tools/AssetBrowser/AtomToolsAssetBrowser.h
    Include/JE/Editor/Tools/AssetBrowser/AtomToolsAssetBrowserInteractions.h
    Source/AssetBrowser/AtomToolsAssetBrowser.cpp
    Source/AssetBrowser/AtomToolsAssetBrowser.qrc
    Source/AssetBrowser/AtomToolsAssetBrowser.ui
    Source/AssetBrowser/AtomToolsAssetBrowserInteractions.cpp

    Include/JE/Editor/Tools/AtomToolsFrameworkSystemRequestBus.h

    Include/JE/Editor/Tools/AssetSelection/AssetSelectionComboBox.h
    Include/JE/Editor/Tools/AssetSelection/AssetSelectionGrid.h
    Source/AssetSelection/AssetSelectionComboBox.cpp
    Source/AssetSelection/AssetSelectionGrid.cpp
    Source/AssetSelection/AssetSelectionGrid.ui

    Include/JE/Editor/Tools/Communication/LocalServer.h
    Include/JE/Editor/Tools/Communication/LocalSocket.h
    Source/Communication/LocalServer.cpp
    Source/Communication/LocalSocket.cpp

    Include/JE/Editor/Tools/Debug/TraceRecorder.h
    Source/Debug/TraceRecorder.cpp

    Include/JE/Editor/Tools/Document/AtomToolsAnyDocument.h
    Include/JE/Editor/Tools/Document/AtomToolsAnyDocumentNotificationBus.h
    Include/JE/Editor/Tools/Document/AtomToolsAnyDocumentRequestBus.h
    Include/JE/Editor/Tools/Document/AtomToolsDocument.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentApplication.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentInspector.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentMainWindow.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentNotificationBus.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentObjectInfo.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentSystem.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentSystemRequestBus.h
    Include/JE/Editor/Tools/Document/AtomToolsDocumentTypeInfo.h
    Include/JE/Editor/Tools/Document/CreateDocumentDialog.h
    Source/Document/AtomToolsAnyDocument.cpp
    Source/Document/AtomToolsDocument.cpp
    Source/Document/AtomToolsDocumentApplication.cpp
    Source/Document/AtomToolsDocumentInspector.cpp
    Source/Document/AtomToolsDocumentMainWindow.cpp
    Source/Document/AtomToolsDocumentSystem.cpp
    Source/Document/AtomToolsDocumentTypeInfo.cpp
    Source/Document/CreateDocumentDialog.cpp

    Include/JE/Editor/Tools/DynamicProperty/DynamicProperty.h
    Include/JE/Editor/Tools/DynamicProperty/DynamicPropertyGroup.h
    Source/DynamicProperty/DynamicProperty.cpp
    Source/DynamicProperty/DynamicPropertyGroup.cpp

    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportContent.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportInputController.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportScene.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettings.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportToolBar.h
    Include/JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportWidget.h
    Source/EntityPreviewViewport/EntityPreviewViewportContent.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportInputController.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportScene.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettings.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportToolBar.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportWidget.cpp

    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNode.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodeConfig.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManager.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManagerRequestBus.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodePaletteItem.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodeSlotConfig.h
    Include/JE/Editor/Tools/Graph/DynamicNode/DynamicNodeUtil.h
    Source/Graph/DynamicNode/DynamicNode.cpp
    Source/Graph/DynamicNode/DynamicNodeConfig.cpp
    Source/Graph/DynamicNode/DynamicNodeManager.cpp
    Source/Graph/DynamicNode/DynamicNodePaletteItem.cpp
    Source/Graph/DynamicNode/DynamicNodeSlotConfig.cpp
    Source/Graph/DynamicNode/DynamicNodeUtil.cpp

    Include/JE/Editor/Tools/Graph/AssetStatusReporter.h
    Include/JE/Editor/Tools/Graph/AssetStatusReporterState.h
    Include/JE/Editor/Tools/Graph/AssetStatusReporterSystem.h
    Include/JE/Editor/Tools/Graph/AssetStatusReporterSystemRequestBus.h
    Include/JE/Editor/Tools/Graph/GraphCompiler.h
    Include/JE/Editor/Tools/Graph/GraphDocument.h
    Include/JE/Editor/Tools/Graph/GraphDocumentNotificationBus.h
    Include/JE/Editor/Tools/Graph/GraphDocumentRequestBus.h
    Include/JE/Editor/Tools/Graph/GraphDocumentView.h
    Include/JE/Editor/Tools/Graph/GraphTemplateFileData.h
    Include/JE/Editor/Tools/Graph/GraphTemplateFileDataCache.h
    Include/JE/Editor/Tools/Graph/GraphTemplateFileDataCacheRequestBus.h
    Include/JE/Editor/Tools/Graph/GraphUtil.h
    Include/JE/Editor/Tools/Graph/GraphView.h
    Include/JE/Editor/Tools/Graph/GraphViewConstructPresets.h
    Include/JE/Editor/Tools/Graph/GraphViewSettings.h
    Source/Graph/AssetStatusReporter.cpp
    Source/Graph/AssetStatusReporterSystem.cpp
    Source/Graph/GraphCompiler.cpp
    Source/Graph/GraphDocument.cpp
    Source/Graph/GraphDocumentView.cpp
    Source/Graph/GraphTemplateFileData.cpp
    Source/Graph/GraphTemplateFileDataCache.cpp
    Source/Graph/GraphUtil.cpp
    Source/Graph/GraphView.cpp
    Source/Graph/GraphView.qrc
    Source/Graph/GraphView.qss
    Source/Graph/GraphViewConstructPresets.cpp
    Source/Graph/GraphViewSettings.cpp

    Include/JE/Editor/Tools/Inspector/InspectorGroupHeaderWidget.h
    Include/JE/Editor/Tools/Inspector/InspectorGroupWidget.h
    Include/JE/Editor/Tools/Inspector/InspectorNotificationBus.h
    Include/JE/Editor/Tools/Inspector/InspectorPropertyGroupWidget.h
    Include/JE/Editor/Tools/Inspector/InspectorRequestBus.h
    Include/JE/Editor/Tools/Inspector/InspectorWidget.h
    Source/Inspector/InspectorGroupHeaderWidget.cpp
    Source/Inspector/InspectorGroupWidget.cpp
    Source/Inspector/InspectorPropertyGroupWidget.cpp
    Source/Inspector/InspectorWidget.cpp
    Source/Inspector/InspectorWidget.qrc
    Source/Inspector/InspectorWidget.ui
    Source/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.cpp
    Source/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h

    Include/JE/Editor/Tools/PerformanceMonitor/PerformanceMetrics.h
    Include/JE/Editor/Tools/PerformanceMonitor/PerformanceMonitorRequestBus.h
    Source/PerformanceMonitor/PerformanceMonitorSystemComponent.cpp
    Source/PerformanceMonitor/PerformanceMonitorSystemComponent.h

    Include/JE/Editor/Tools/PreviewRenderer/PreviewContent.h
    Include/JE/Editor/Tools/PreviewRenderer/PreviewerFeatureProcessorProviderBus.h
    Include/JE/Editor/Tools/PreviewRenderer/PreviewRendererCaptureRequest.h
    Include/JE/Editor/Tools/PreviewRenderer/PreviewRendererInterface.h
    Include/JE/Editor/Tools/PreviewRenderer/PreviewRendererSystemRequestBus.h
    Source/PreviewRenderer/PreviewRenderer.cpp
    Source/PreviewRenderer/PreviewRenderer.h
    Source/PreviewRenderer/PreviewRendererCaptureState.cpp
    Source/PreviewRenderer/PreviewRendererCaptureState.h
    Source/PreviewRenderer/PreviewRendererIdleState.cpp
    Source/PreviewRenderer/PreviewRendererIdleState.h
    Source/PreviewRenderer/PreviewRendererLoadState.cpp
    Source/PreviewRenderer/PreviewRendererLoadState.h
    Source/PreviewRenderer/PreviewRendererState.h
    Source/PreviewRenderer/PreviewRendererSystemComponent.cpp
    Source/PreviewRenderer/PreviewRendererSystemComponent.h

    Include/JE/Editor/Tools/SettingsDialog/SettingsDialog.h
    Source/SettingsDialog/SettingsDialog.cpp

    Include/JE/Editor/Tools/Util/MaterialPropertyUtil.h
    Include/JE/Editor/Tools/Util/Util.h
    Source/Util/MaterialPropertyUtil.cpp
    Source/Util/Util.cpp

    Include/JE/Editor/Tools/Viewport/ModularViewportCameraController.h
    Include/JE/Editor/Tools/Viewport/ModularViewportCameraControllerRequestBus.h
    Include/JE/Editor/Tools/Viewport/RenderViewportWidget.h

    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/IdleBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateCameraBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h
    Include/JE/Editor/Tools/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h
    Source/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/IdleBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/PanCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.cpp

    Include/JE/Editor/Tools/Viewport/ViewportInteractionImpl.h
    Source/Viewport/ModularViewportCameraController.cpp
    Source/Viewport/RenderViewportWidget.cpp
    Source/Viewport/ViewportInteractionImpl.cpp

    Include/JE/Editor/Tools/Window/AtomToolsMainWindow.h
    Include/JE/Editor/Tools/Window/AtomToolsMainWindowRequestBus.h
    Include/JE/Editor/Tools/Window/AtomToolsMainWindowNotificationBus.h
    Source/Window/AtomToolsMainWindow.cpp
    Source/Window/AtomToolsMainWindowSystemComponent.cpp
    Source/Window/AtomToolsMainWindowSystemComponent.h
)