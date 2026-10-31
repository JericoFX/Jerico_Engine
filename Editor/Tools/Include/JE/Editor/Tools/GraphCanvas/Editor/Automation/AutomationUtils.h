/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QObject>

#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class AutomationUtils
    {
    public:
        // These have some issues with custom types due to dll boundaries and how the dynamic typing system in qt works.
        // So for the most part these will only return native qt types cleanly.
        template<typename T = QObject>
        static T* FindObjectById(JE::Editor::Tools::GraphCanvas::EditorId editorId, AZ::Crc32 id)
        {
            QObject* object = nullptr;
            JE::Editor::Tools::GraphCanvas::AssetEditorAutomationRequestBus::EventResult(object, editorId, &JE::Editor::Tools::GraphCanvas::AssetEditorAutomationRequests::FindObject, id);

            return qobject_cast<T*>(object);
        }

        template<typename T = QObject>
        static T* FindObjectByName(JE::Editor::Tools::GraphCanvas::EditorId editorId, QString name)
        {
            T* object = nullptr;
            JE::Editor::Tools::GraphCanvas::AssetEditorAutomationRequestBus::EventResult(object, editorId, &JE::Editor::Tools::GraphCanvas::AssetEditorAutomationRequests::FindElementByName, name);
            return qobject_cast<T*>(object);
        }
    };
}
