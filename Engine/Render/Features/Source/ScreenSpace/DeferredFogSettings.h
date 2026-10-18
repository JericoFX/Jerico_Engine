/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>

#include <JE/Render/Features/ParamMacros/ParamMacrosHowTo.inl>    // for education purposes
#include <JE/Render/Features/ScreenSpace/DeferredFogSettingsInterface.h>
#include <PostProcess/PostProcessBase.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        // The post process sub-settings class for the exposure control feature over
        // full screen deferred fog.
        // The fog is calculated using the linear depth and turbulence texture with two blended
        // octaves that emulate the fog thickness and motion along the view ray direction.
        //
        // [GFX TODO] most methods in this class can be auto generated using a reflection mechanism
        //  based on the PerPass Srg.  Currently we don't have such a reflection system in place
        //  and so, a temporary approach demonstrating partial reflection through preprocessor
        //  macros is used.
        class DeferredFogSettings final :
            public DeferredFogSettingsInterface,
            public PostProcessBase
        {
            friend class DeferredFogPass;
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(DeferredFogSettings, "{FD822CC5-4E7B-4471-AA7D-1FCDF6CAC979}",
                JE::Render::Features::DeferredFogSettingsInterface, JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(DeferredFogSettings, AZ::SystemAllocator);

            DeferredFogSettings(PostProcessFeatureProcessor* featureProcessor);
            DeferredFogSettings();
            ~DeferredFogSettings() = default;

            // DeferredFogSettingsInterface overrides...
            void OnSettingsChanged() override;
            bool GetSettingsNeedUpdate()
            {
                return m_needUpdate;
            }
            void SetSettingsNeedUpdate(bool needUpdate)
            {
                m_needUpdate = needUpdate;
            }

            void SetInitialized(bool isInitialized) override
            {
                m_isInitialized = isInitialized;
            }
            bool IsInitialized() override
            {
                return m_isInitialized;
            }

            // Called by the post effects feature processor for doing per frame prep if required
            void Simulate([[maybe_unused]]float deltaTime) {};

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(DeferredFogSettings* target, float alpha) const;

            AZ::Data::Instance<JE::Render::RPI::StreamingImage> LoadStreamingImage(const char* textureFilePath, const char* sampleName);

            //---------------------------------------------------------
            // Generate Get / Set override methods
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override;                                                           \
        void Set##Name(ValueType val) override;                                                         \

#include <JE/Render/Features/ParamMacros/MapParamCommon.inl>
#include <JE/Render/Features/ScreenSpace/DeferredFogParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            //---------------------------------------------------------
            
            // SRG constants
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType MemberName = DefaultValue;                                                            \

#include <JE/Render/Features/ParamMacros/MapParamCommon.inl>
#include <JE/Render/Features/ScreenSpace/DeferredFogParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            //---------------------------------------------------------
            // SRG constants binding indices
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)      \
            AZ::RHI::ShaderInputConstantIndex MemberName##SrgIndex;             \

#include <JE/Render/Features/ParamMacros/MapParamCommon.inl>

            // Change the handling for texture handles
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)       \
            AZ::RHI::ShaderInputImageIndex MemberName##SrgIndex;         \

#include <JE/Render/Features/ScreenSpace/DeferredFogParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            //---------------------------------------------------------
            // Generate all Image members instances
            // Set all macros to be empty, but override the texture for defining image members
#include <JE/Render/Features/ParamMacros/MapParamEmpty.inl>

            // Macro to replace only the image macro for defining only image resources
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)            \
            AZ::Data::Instance<JE::Render::RPI::Image> MemberName##Image;             \
            AZStd::string MemberName##CurrentlyLoaded;

#include <JE/Render/Features/ScreenSpace/DeferredFogParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            PostProcessSettings* m_parentSettings = nullptr;

            // The 'm_enabled' member indicates if the pass should be enabled. By default this is set to true since if the
            // pass is data driven enabled, the default settings should not disable it.  If it is driven by
            // an engine component, these settings will be replaced by the component's settings controlled by
            // the user. Defined using DeferredFogParams.inl, along with others.

            // Indication if the SRG indices were set
            bool m_isInitialized = false;

            // Indicates that the srg indices were set
            bool m_needUpdate = true;

        };

    }
}
