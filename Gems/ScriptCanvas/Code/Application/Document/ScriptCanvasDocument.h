/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Document/AtomToolsDocument.h>
#include <AzCore/RTTI/RTTI.h>

#include "ScriptCanvasDocumentRequestBus.h"

namespace ScriptCanvas
{
    class ScriptCanvasDocument
        : public JE::Editor::Tools::AtomToolsDocument
        , public ScriptCanvasDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ScriptCanvasDocument, "{1030D380-F43F-4C84-9041-8B85B0EF75A3}", JE::Editor::Tools::AtomToolsDocument);
        AZ_CLASS_ALLOCATOR(ScriptCanvasDocument, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(ScriptCanvasDocument);

        static void Reflect(AZ::ReflectContext* context);

        ScriptCanvasDocument() = default;
        ScriptCanvasDocument(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& documentTypeInfo);
        virtual ~ScriptCanvasDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        static JE::Editor::Tools::DocumentTypeInfo BuildDocumentTypeInfo();
        // ~AtomToolsFramework::AtomToolsDocument
    };

} // namespace ScriptCanvas
