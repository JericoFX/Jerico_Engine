/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <JE/Editor/Render/Features/Utils/EditorModelPreset.h>
#include <JE/Render/Features/Utils/ModelPreset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Editor::Render
{
    namespace Features
    {
        void EditorModelPreset::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<JE::Render::Features::ModelPreset>(
                        "ModelPreset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &JE::Render::Features::ModelPreset::m_modelAsset, QT_TRANSLATE_NOOP("Atom::Feature", "Model Asset"), QT_TRANSLATE_NOOP("Atom::Feature", "Model asset reference"))
                        ;
                }
            }
        }
    }
}
