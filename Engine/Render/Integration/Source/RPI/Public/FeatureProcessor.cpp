/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Public/Scene.h>

namespace JE::Render
{
    namespace RPI
    {
        void FeatureProcessor::EnableSceneNotification()
        {
            if (m_parentScene && !m_parentScene->GetId().IsNull())
            {
                SceneNotificationBus::Handler::BusConnect(m_parentScene->GetId());
            }
        }

        void FeatureProcessor::DisableSceneNotification()
        {
            SceneNotificationBus::Handler::BusDisconnect();
        }
    }
}
