/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>

namespace JE::Editor
{
    //! Inject the same policy into all graph/node-config/shader documents retained by one Editor session.
    //! Empty policy remains the standalone default; no document pointer is retained by global commands.
    JE_EDITOR_MATERIALS_API AtomToolsFramework::DocumentHistoryPolicy CreateMaterialCanvasDocumentHistoryPolicy();
}
