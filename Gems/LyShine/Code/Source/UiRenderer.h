/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/DynamicDraw/DynamicDrawContext.h>
#include <JE/Render/RPI/Public/WindowContext.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <JE/Render/Bootstrap/BootstrapNotificationBus.h>

#ifndef _RELEASE
#include <AzCore/std/containers/unordered_set.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UI render interface
//
class UiRenderer
    : public JE::Render::Bootstrap::NotificationBus::Handler
{
public: // types

    // Cached shader data
    struct UiShaderData
    {
        AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_isClampInputIndex;

        JE::Render::RPI::ShaderVariantId m_shaderVariantTextureLinear;
        JE::Render::RPI::ShaderVariantId m_shaderVariantTextureSrgb;
        JE::Render::RPI::ShaderVariantId m_shaderVariantAlphaTestMask;
        JE::Render::RPI::ShaderVariantId m_shaderVariantGradientMask;
    };

    // Base state
    struct BaseState
    {
        BaseState()
        {
            ResetToDefault();
        }

        void ResetToDefault()
        {
            // Enable blend/color write
            m_blendStateEnabled = true;
            m_blendStateWriteMask = 0xF;

            // Disable stencil
            m_stencilState = AZ::RHI::StencilState();
            m_stencilState.m_enable = 0;

            m_useAlphaTest = false;
            m_modulateAlpha = false;
        }

        uint32_t m_blendStateEnabled = true;
        uint32_t m_blendStateWriteMask = 0xF;
        AZ::RHI::StencilState m_stencilState;
        bool m_useAlphaTest = false;
        bool m_modulateAlpha = false;
        bool m_srgbWrite = true;
    };

public: // member functions

    //! Constructor, constructed by the LyShine class
    UiRenderer(JE::Render::RPI::ViewportContextPtr viewportContext = nullptr);
    ~UiRenderer();

    //! Returns whether RPI has loaded all its assets and is ready to render
    bool IsReady();

    //! Start the rendering of the frame for LyShine
    void BeginUiFrameRender();

    //! End the rendering of the frame for LyShine
    void EndUiFrameRender();

    //! Start the rendering of a UI canvas
    void BeginCanvasRender();

    //! End the rendering of a UI canvas
    void EndCanvasRender();

    //! Return the dynamic draw context associated with this UI renderer
    AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext> GetDynamicDrawContext();

    AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext> CreateDynamicDrawContextForRTT(const AZStd::string& rttName);

    //! Return the shader data for the ui shader
    const UiShaderData& GetUiShaderData();

    //! Return the current orthographic view matrix
    AZ::Matrix4x4 GetModelViewProjectionMatrix();

    //! Return the curent viewport size
    AZ::Vector2 GetViewportSize();

    //! Get the current base state
    BaseState GetBaseState();

    //! Set the base state
    void SetBaseState(BaseState state);

    //! Get the shader variant based on current render properties
    JE::Render::RPI::ShaderVariantId GetCurrentShaderVariant();

    //! Get the current stencil test reference value
    uint32_t GetStencilRef();

    //! Set the stencil test reference value
    void SetStencilRef(uint32_t);

    //! Increment the current stencil reference value
    void IncrementStencilRef();

    //! Decrement the current stencil reference value
    void DecrementStencilRef();

    //! Return the viewport context set by the user, or the default if not set
    AZStd::shared_ptr<JE::Render::RPI::ViewportContext> GetViewportContext();

#ifndef _RELEASE
    //! Setup to record debug texture data before rendering
    void DebugSetRecordingOptionForTextureData(int recordingOption);

    //! Display debug texture data after rendering
    void DebugDisplayTextureData(int recordingOption);

    //! Track textures being used in the current frame
    void DebugUseTexture(AZ::Data::Instance<JE::Render::RPI::Image> image);
#endif

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiRenderer);

    // JE::Render::Bootstrap::Notification
    void OnBootstrapSceneReady(JE::Render::RPI::Scene* bootstrapScene) override;
    // ~JE::Render::Bootstrap::Notification

    //! Create a scene for the user defined viewportContext
    JE::Render::RPI::ScenePtr CreateScene(AZStd::shared_ptr<JE::Render::RPI::ViewportContext> viewportContext);

    //! Create a dynamic draw context for this renderer
    AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext> CreateDynamicDrawContext(
        AZ::Data::Instance<JE::Render::RPI::Shader> uiShader);

    //! Bind the global white texture for all the texture units we use
    void BindNullTexture();

    //! Store shader data for later use
    void CacheShaderData(const AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext>& dynamicDraw);

protected: // attributes

    static constexpr char LogName[] = "UiRenderer";

    BaseState m_baseState;
    uint32_t m_stencilRef = 0;

    UiShaderData m_uiShaderData;
    AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext> m_dynamicDraw;
    bool m_isRPIReady = false;

    // Set by user when viewport context is not the main/default viewport
    AZStd::shared_ptr<JE::Render::RPI::ViewportContext> m_viewportContext;

    JE::Render::RPI::ScenePtr m_ownedScene;
    JE::Render::RPI::Scene* m_scene = nullptr;

#ifndef _RELEASE
    int m_debugTextureDataRecordLevel = 0;
    AZStd::unordered_set<AZ::Data::Instance<JE::Render::RPI::Image>> m_texturesUsedInFrame;
#endif
};
