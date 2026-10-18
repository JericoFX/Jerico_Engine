/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>
#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace JE::Editor::Render::Features
{
    class UseTextureFunctorSourceData;
}

namespace JE::Render
{
    namespace Features
    {
        //! Materials can use this functor to control whether a specific texture property will be sampled. 
        //! Sampling will be disabled if no texture is bound or if the useTexture flag is disabled.
        class JE_RENDER_FEATURES_API UseTextureFunctor final
            : public JE::Render::RPI::MaterialFunctor
        {
            friend class JE::Editor::Render::Features::UseTextureFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(UseTextureFunctor , AZ::SystemAllocator)
            AZ_RTTI(UseTextureFunctor, "{CFAC6159-840A-4696-8699-D3850D8A3930}", JE::Render::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context);

            using JE::Render::RPI::MaterialFunctor::Process;
            void Process(JE::Render::RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            void Process(JE::Render::RPI::MaterialFunctorAPI::EditorContext& context) override;

        private:
            // Material property inputs...
            JE::Render::RPI::MaterialPropertyIndex m_texturePropertyIndex;     //!< material property for a texture
            JE::Render::RPI::MaterialPropertyIndex m_useTexturePropertyIndex;  //!< material property for a bool that indicates whether to use the texture

            //! material properties that relate to the texture, which will be enabled only when the texture map is enabled.
            AZStd::vector<JE::Render::RPI::MaterialPropertyIndex> m_dependentPropertyIndexes;

            // Shader option output...
            AZ::Name m_useTextureOptionName;
        };

    } // namespace Features
} // namespace JE::Render
