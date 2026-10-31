/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentNotificationBus.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <JE/Editor/Tools/Graph/GraphDocumentNotificationBus.h>

namespace JE::Editor
{
    class JE_EDITOR_MATERIALS_API MaterialPreviewViewportContent final
        : public JE::Editor::Tools::EntityPreviewViewportContent
        , public JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        , public JE::Editor::Tools::GraphDocumentNotificationBus::Handler
    {
    public:
        MaterialPreviewViewportContent(
            const AZ::Crc32& toolId,
            JE::Editor::Tools::RenderViewportWidget* widget,
            AZStd::shared_ptr<AzFramework::EntityContext> entityContext, bool followOpenedDocuments = true);
        ~MaterialPreviewViewportContent();
        void SetDocumentId(const AZ::Uuid& documentId);
        void SetGraphDocumentId(const AZ::Uuid& documentId);

        AZ::EntityId GetObjectEntityId() const override;
        AZ::EntityId GetEnvironmentEntityId() const override;
        AZ::EntityId GetPostFxEntityId() const override;
        AZ::EntityId GetShadowCatcherEntityId() const;
        AZ::EntityId GetGridEntityId() const;

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        // GraphDocumentNotificationBus::Handler overrides...
        void OnCompileGraphStarted(const AZ::Uuid& documentId) override;
        void OnCompileGraphCompleted(const AZ::Uuid& documentId) override;
        void OnCompileGraphFailed(const AZ::Uuid& documentId) override;
        void ApplyGeneratedMaterial(const AZ::Uuid& documentId);

        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        const bool m_followOpenedDocuments = true;
        AZ::Uuid m_graphDocumentId = AZ::Uuid::CreateNull();
        AZ::Entity* m_environmentEntity = {};
        AZ::Entity* m_gridEntity = {};
        AZ::Entity* m_objectEntity = {};
        AZ::Entity* m_postFxEntity = {};
        AZ::Entity* m_shadowCatcherEntity = {};
    };
} // namespace JE::Editor
