/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Inspector/InspectorGroupWidget.h>

namespace JE::Editor::Tools
{
    InspectorGroupWidget::InspectorGroupWidget(QWidget* parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    void InspectorGroupWidget::Refresh()
    {
    }

    void InspectorGroupWidget::Rebuild()
    {
    }
} // namespace JE::Editor::Tools

