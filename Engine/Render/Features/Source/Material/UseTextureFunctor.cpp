/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./UseTextureFunctor.h"
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Reflect/Image/Image.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>

namespace JE::Render
{
    namespace Features
    {
        void UseTextureFunctor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<UseTextureFunctor, JE::Render::RPI::MaterialFunctor>()
                    ->Version(5)
                    ->Field("texturePropertyIndex", &UseTextureFunctor::m_texturePropertyIndex)
                    ->Field("useTexturePropertyIndex", &UseTextureFunctor::m_useTexturePropertyIndex)
                    ->Field("dependentPropertyIndexes", &UseTextureFunctor::m_dependentPropertyIndexes)
                    ->Field("useTextureOptionName", &UseTextureFunctor::m_useTextureOptionName)
                    ;
            }
        }

        void UseTextureFunctor::Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            using namespace JE::Render::RPI;

            auto texture = context.GetMaterialPropertyValue<AZ::Data::Instance<Image>>(m_texturePropertyIndex);
            bool useTextureFlag = context.GetMaterialPropertyValue<bool>(m_useTexturePropertyIndex);

            ShaderOptionValue useTexture{useTextureFlag && nullptr != texture};

            context.SetShaderOptionValue(m_useTextureOptionName, useTexture);
        }

        void UseTextureFunctor::Process(JE::Render::RPI::MaterialFunctorAPI::EditorContext& context)
        {
            const bool useTextureFlag = context.GetMaterialPropertyValue<bool>(m_useTexturePropertyIndex);
            AZ::Data::Instance<JE::Render::RPI::Image> image = context.GetMaterialPropertyValue<AZ::Data::Instance<JE::Render::RPI::Image>>(m_texturePropertyIndex);

            context.SetMaterialPropertyVisibility(
                m_useTexturePropertyIndex,
                nullptr != image ? JE::Render::RPI::MaterialPropertyVisibility::Enabled : JE::Render::RPI::MaterialPropertyVisibility::Hidden
            );

            JE::Render::RPI::MaterialPropertyVisibility dependentVisibility = JE::Render::RPI::MaterialPropertyVisibility::Enabled;
            if (nullptr == image)
            {
                dependentVisibility = JE::Render::RPI::MaterialPropertyVisibility::Hidden;
            }
            else if (!useTextureFlag)
            {
                dependentVisibility = JE::Render::RPI::MaterialPropertyVisibility::Disabled;
            }

            // These properties are enabled only when the texture is actually going to be sampled by the shader.
            for (JE::Render::RPI::MaterialPropertyIndex index : m_dependentPropertyIndexes)
            {

                context.SetMaterialPropertyVisibility(index, dependentVisibility);
            }
        }

    } // namespace Features
} // namespace JE::Render
