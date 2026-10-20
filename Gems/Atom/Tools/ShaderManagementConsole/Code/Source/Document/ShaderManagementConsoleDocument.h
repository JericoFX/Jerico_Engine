/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/RPI/Edit/Shader/ShaderVariantListSourceData.h>
#include <JE/Render/RPI/Reflect/Material/ShaderCollection.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <JE/Editor/Tools/Document/AtomToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleDocument provides an API for modifying and saving document properties.
    class ShaderManagementConsoleDocument
        : public JE::Editor::Tools::AtomToolsDocument
        , public ShaderManagementConsoleDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ShaderManagementConsoleDocument, "{C8FAF1C7-8665-423C-B1DD-82016231B17B}", JE::Editor::Tools::AtomToolsDocument);
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleDocument, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(ShaderManagementConsoleDocument);

        static void Reflect(AZ::ReflectContext* context);

        ShaderManagementConsoleDocument() = default;
        ShaderManagementConsoleDocument(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& documentTypeInfo);
        ~ShaderManagementConsoleDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        static JE::Editor::Tools::DocumentTypeInfo BuildDocumentTypeInfo();
        JE::Editor::Tools::DocumentObjectInfoVector GetObjectInfo() const override;
        bool Open(const AZStd::string& loadPath) override;
        bool Save() override;
        bool SaveAsCopy(const AZStd::string& savePath) override;
        bool SaveAsChild(const AZStd::string& savePath) override;
        bool IsModified() const override;
        bool BeginEdit() override;
        bool EndEdit() override;

        // ShaderManagementConsoleDocumentRequestBus::Handler overrides...
        void AddOneVariantRow() override;
        void AppendSparseVariantSet(
            AZStd::vector<AZ::Name> optionHeaders,
            AZStd::vector<AZ::Name> matrixOfValues) override;
        void MultiplySparseVariantSet(
            AZStd::vector<AZ::Name> optionHeaders,
            AZStd::vector<AZ::Name> matrixOfValues) override;
        void DefragmentVariantList() override;
        void SetShaderVariantListSourceData(const JE::Render::RPI::ShaderVariantListSourceData& shaderVariantListSourceData) override;
        const JE::Render::RPI::ShaderVariantListSourceData& GetShaderVariantListSourceData() const override;
        size_t GetShaderOptionDescriptorCount() const override;
        const JE::Render::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const override;
        DocumentVerificationResult Verify() const override;

    private:
        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;

        // Write shader variant list source data to JSON
        bool SaveSourceData();

        // Read shader variant list source data from JSON and initialize the document
        bool LoadShaderSourceData();

        // Read shader source data from JSON then find all references to populate the shader variant list and initialize the document
        bool LoadShaderVariantListSourceData();

        // Source data for shader variant list
        JE::Render::RPI::ShaderVariantListSourceData m_shaderVariantListSourceData;

        // Backup copy of the shader variant list source data that will be saved for restoration during undo.
        JE::Render::RPI::ShaderVariantListSourceData m_shaderVariantListSourceDataBeforeEdit;

        // Shader asset for the corresponding shader variant list
        AZ::Data::Asset<JE::Render::RPI::ShaderAsset> m_shaderAsset;

        JE::Render::RPI::ShaderOptionDescriptor m_invalidDescriptor;

        // Flag tracking the modified state of the document.
        // Will be set to true anytime data is changed and cleared anytime the document is saved.
        bool m_modified = {};
    };
} // namespace ShaderManagementConsole
