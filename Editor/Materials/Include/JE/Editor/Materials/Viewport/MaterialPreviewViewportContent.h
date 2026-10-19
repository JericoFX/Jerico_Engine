/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>

namespace JE::Editor
{
    class JE_EDITOR_MATERIALS_API MaterialPreviewViewportContent final
        : public AtomToolsFramework::EntityPreviewViewportContent
        , public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        MaterialPreviewViewportContent(
            const AZ::Crc32& toolId,
            AtomToolsFramework::RenderViewportWidget* widget,
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext, bool followOpenedDocuments = true);
        ~MaterialPreviewViewportContent();
        void SetDocumentId(const AZ::Uuid& documentId);

        AZ::EntityId GetObjectEntityId() const override;
        AZ::EntityId GetEnvironmentEntityId() const override;
        AZ::EntityId GetPostFxEntityId() const override;
        AZ::EntityId GetShadowCatcherEntityId() const;
        AZ::EntityId GetGridEntityId() const;

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        const bool m_followOpenedDocuments = true;
        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
    };
} // namespace JE::Editor
