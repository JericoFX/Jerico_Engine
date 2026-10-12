/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/EditContext.h>
#include <JE/World/Camera/CameraComponentConverters.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include <JE/World/Camera/CameraComponent.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace JE::World::Camera
{
    CameraComponent::CameraComponent(const CameraComponentConfig& properties)
        : CameraComponentBase(properties)
    {
    }

    void CameraComponent::Activate()
    {
        CameraComponentBase::Activate();
    }

    void CameraComponent::Deactivate()
    {
        CameraComponentBase::Deactivate();
    }

    static bool UpdateGameCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (!ClassConverters::UpdateCameraComponentToUseController(context, classElement))
        {
            return false;
        }

        classElement.Convert<CameraComponent>(context);
        return true;
    }

    class CameraNotificationBus_BehaviorHandler : public ::Camera::CameraNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CameraNotificationBus_BehaviorHandler, "{91E442A0-37E7-4E03-AB59-FEC11A06741D}", AZ::SystemAllocator,
            OnCameraAdded, OnCameraRemoved, OnActiveViewChanged);

        void OnCameraAdded(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnCameraAdded, cameraId);
        }

        void OnCameraRemoved(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnCameraRemoved, cameraId);
        }

        void OnActiveViewChanged(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnActiveViewChanged, cameraId);
        };
    };

    void CameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        CameraComponentBase::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("CameraComponent", AZ::Uuid("{A0C21E18-F759-4E72-AF26-7A36FC59E477}"), &ClassConverters::DeprecateCameraComponentWithoutEditor);
            serializeContext->ClassDeprecate("CameraComponent", AZ::Uuid("{E409F5C0-9919-4CA5-9488-1FE8A041768E}"), &UpdateGameCameraComponentToUseController);
            serializeContext->Class<CameraComponent, CameraComponentBase>()
                ->Version(0)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<::Camera::CameraRequestBus>("CameraRequestBus")
                ->Event("GetNearClipDistance", &::Camera::CameraRequestBus::Events::GetNearClipDistance)
                ->Event("GetFarClipDistance", &::Camera::CameraRequestBus::Events::GetFarClipDistance)
                ->Event("GetFovDegrees", &::Camera::CameraRequestBus::Events::GetFovDegrees)
                ->Event("SetFovDegrees", &::Camera::CameraRequestBus::Events::SetFovDegrees)
                ->Event("GetFovRadians", &::Camera::CameraRequestBus::Events::GetFovRadians)
                ->Event("SetFovRadians", &::Camera::CameraRequestBus::Events::SetFovRadians)
                ->Event("GetFov", &::Camera::CameraRequestBus::Events::GetFov) // Deprecated in 1.13
                ->Event("SetFov", &::Camera::CameraRequestBus::Events::SetFov) // Deprecated in 1.13
                ->Event("SetNearClipDistance", &::Camera::CameraRequestBus::Events::SetNearClipDistance)
                ->Event("SetFarClipDistance", &::Camera::CameraRequestBus::Events::SetFarClipDistance)
                ->Event("MakeActiveView", &::Camera::CameraRequestBus::Events::MakeActiveView)
                ->Event("IsActiveView", &::Camera::CameraRequestBus::Events::IsActiveView)
                ->Event("IsOrthographic", &::Camera::CameraRequestBus::Events::IsOrthographic)
                ->Event("SetOrthographic", &::Camera::CameraRequestBus::Events::SetOrthographic)
                ->Event("GetOrthographicHalfWidth", &::Camera::CameraRequestBus::Events::GetOrthographicHalfWidth)
                ->Event("SetOrthographicHalfWidth", &::Camera::CameraRequestBus::Events::SetOrthographicHalfWidth)
                ->Event("SetXRViewQuaternion", &::Camera::CameraRequestBus::Events::SetXRViewQuaternion)
                ->Event("ScreenToWorld", &::Camera::CameraRequestBus::Events::ScreenToWorld)
                ->Event("ScreenNdcToWorld", &::Camera::CameraRequestBus::Events::ScreenNdcToWorld)
                ->Event("WorldToScreen", &::Camera::CameraRequestBus::Events::WorldToScreen)
                ->Event("WorldToScreenNdc", &::Camera::CameraRequestBus::Events::WorldToScreenNdc)
                ->VirtualProperty("FieldOfView","GetFovDegrees","SetFovDegrees")
                ->VirtualProperty("NearClipDistance", "GetNearClipDistance", "SetNearClipDistance")
                ->VirtualProperty("FarClipDistance", "GetFarClipDistance", "SetFarClipDistance")
                ->VirtualProperty("Orthographic", "IsOrthographic", "SetOrthographic")
                ->VirtualProperty("OrthographicHalfWidth", "GetOrthographicHalfWidth", "SetOrthographicHalfWidth")
                ;

            behaviorContext->Class<CameraComponent>()->RequestBus("CameraRequestBus");

            behaviorContext->EBus<::Camera::CameraSystemRequestBus>("CameraSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Camera")
                ->Event("GetActiveCamera", &::Camera::CameraSystemRequestBus::Events::GetActiveCamera)
                ;

            behaviorContext->EBus<::Camera::CameraNotificationBus>("CameraNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Camera")
                ->Handler<CameraNotificationBus_BehaviorHandler>()
                ;
        }
    }
} //namespace Camera

