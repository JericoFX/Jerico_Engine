/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/State/EditorStateBase.h>

#include <JE/Render/RPI/Reflect/Pass/RenderPassData.h>
#include <JE/Render/RPI/Reflect/Pass/FullscreenTrianglePassData.h>

namespace JE::Editor::Render::Features
{
    //! Custom data for the EditorStateParentPass. Should be specified in the PassRequest.
    struct EditorStateParentPassData
        : public JE::Render::RPI::RenderPassData
    {
        AZ_RTTI(EditorStateParentPassData, "{0E0FD1EE-906C-45B5-B65E-463719A90712}", JE::Render::RPI::RenderPassData);
        AZ_CLASS_ALLOCATOR(EditorStateParentPassData, AZ::SystemAllocator);

        EditorStateParentPassData() = default;
        virtual ~EditorStateParentPassData() = default;

        //! Pointer to the owning editor state effect parent pass instance.
        const JE::Editor::Render::Features::EditorStateBase* editorStatePass = nullptr;
    };
} // namespace JE::Editor::Render::Features
