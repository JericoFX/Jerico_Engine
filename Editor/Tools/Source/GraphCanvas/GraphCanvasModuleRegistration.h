/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/list.h>

namespace JE::Editor::Tools::GraphCanvas
{
    void AppendGraphCanvasDescriptors(AZStd::list<AZ::ComponentDescriptor*>& descriptors);
}
