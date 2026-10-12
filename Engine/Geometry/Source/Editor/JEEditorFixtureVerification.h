/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

class QImage;

namespace JE::BSP::Editor
{
    // Observations from the original Root.spawnable prototypes or its ticket's actual indexed entities; not a second entity store.
    struct FixtureEntityObservation
    {
        AZ::EntityId m_id;
        uint32_t m_sourceIndex = 0;
        AZStd::string m_name;
        AZ::Uuid m_componentType;
        bool m_active = false;
        AZ::Uuid m_context;
        uint32_t m_spawnTicket = 0;
    };
    struct FixtureSourceIndices
    {
        uint32_t m_room = 0;
        uint32_t m_camera = 0;
    };
    struct FixtureRuntimeIdentity
    {
        AZ::EntityId m_room;
        AZ::EntityId m_camera;
    };
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<FixtureSourceIndices, AZStd::string> ResolveFixtureSourceIndices(
        AZStd::span<const FixtureEntityObservation> entities, AZStd::string_view roomName, AZStd::string_view cameraName);
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<FixtureRuntimeIdentity, AZStd::string> ResolveFixtureRuntimeIdentity(
        AZStd::span<const FixtureEntityObservation> entities, FixtureSourceIndices indices, AZStd::string_view roomName,
        AZStd::string_view cameraName, const AZ::Uuid& runtimeContext, uint32_t spawnTicket);

    //! Requires an absent child of this checkout's existing build/n0-validation tree; never creates it.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<void, AZStd::string> ValidateFixtureVerificationOutputPath(
        AZStd::string_view engine, AZStd::string_view output);
    //! Rejects empty/uniform readbacks; spatial variation alone is not visual acceptance.
    JE_GEOMETRY_AUTHORING_API bool HasFixtureCaptureContent(const QImage& image);
    //! EntityContext keeps requesting=true during normal play. Exit requires both flags cleared.
    JE_GEOMETRY_AUTHORING_API bool IsFixtureGameModeReady(bool runtime, bool running, bool requesting);

    //! Explicit native Editor command only. Owns the bounded live-Editor run and its bus connections.
    class FixtureVerification final
    {
    public:
        FixtureVerification();
        ~FixtureVerification();
        FixtureVerification(const FixtureVerification&) = delete;
        FixtureVerification& operator=(const FixtureVerification&) = delete;
        void RegisterCommands();
        void Deactivate();

    private:
        class Implementation;
        AZStd::unique_ptr<Implementation> m_impl;
    };
}
