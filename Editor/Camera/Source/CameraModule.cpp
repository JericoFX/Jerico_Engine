/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/Camera/EditorCameraComponent.h>
#include <JE/Editor/Camera/CameraEditorSystemComponent.h>
#include <JE/World/Camera/CameraSystemComponent.h>

namespace JE::Editor::Camera
{
    CameraAuthoringModule::CameraAuthoringModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CameraEditorSystemComponent::CreateDescriptor(),
            EditorCameraComponent::CreateDescriptor() });
    }
    CameraAuthoringModule::~CameraAuthoringModule() = default;
    AZ::ComponentTypeList CameraAuthoringModule::GetRequiredSystemComponents() const
    {
        return { azrtti_typeid<JE::World::Camera::CameraSystemComponent>(),
            azrtti_typeid<CameraEditorSystemComponent>() };
    }
    AZ::Module* CreateAuthoringModule()
    {
        return aznew CameraAuthoringModule;
    }
}
