/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>
#include <RPI/Module.h>
#include <RPI/RPISystemComponent.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>

namespace JE::Render::RPI
{
    class EditorModule final
        : public Module
    {
    public:
        AZ_RTTI(EditorModule, "{F2DF5DD9-1323-436C-B0E5-B200B8709CE5}", Module);

        EditorModule()
            : Module(Editor::RPISystemComponent::CreateDescriptor())
        {
        }
    };
} // namespace JE::Render::RPI

namespace JE::Editor::RenderIntegration
{
    AZ::Module* CreateRpiEditorModule()
    {
        return aznew JE::Render::RPI::EditorModule;
    }
} // namespace JE::Editor::RenderIntegration
