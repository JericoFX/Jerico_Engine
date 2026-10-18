/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Scene/CubeMapCapture/CubeMapCaptureTypes.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/SystemFile.h>
#include <Atom/Utils/DdsFile.h>
#include <JE/Render/Features/CubeMapCapture/CubeMapCaptureFeatureProcessorInterface.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        static const char* CubeMapSpecularFileSuffixes[] =
        {
            "_iblspecularcm64.dds",
            "_iblspecularcm128.dds",
            "_iblspecularcm256.dds",
            "_iblspecularcm512.dds",
            "_iblspecularcm1024.dds"
        };

        static_assert(AZ_ARRAY_SIZE(CubeMapSpecularFileSuffixes) == aznumeric_cast<uint32_t>(JE::Render::Scene::CubeMapSpecularQualityLevel::Count),
            "CubeMapSpecularFileSuffixes must have the same number of entries as CubeMapSpecularQualityLevel");

        static constexpr const char* CubeMapDiffuseFileSuffix = "_ibldiffusecm.dds";

        // Mixin class that provides cubemap capture capability for editor components
        class EditorCubeMapRenderer
        {
        protected:
            EditorCubeMapRenderer() = default;

            // initiate the cubemap render and update the relative path if necessary
            AZ::u32 RenderCubeMap(
                AZStd::function<void(JE::Render::Features::RenderCubeMapCallback, AZStd::string&)> renderCubeMapFn,
                const AZStd::string dialogText,
                const AZ::Entity* entityRequestingRender,
                const AZStd::string& folderName,
                AZStd::string& relativePath,
                JE::Render::Scene::CubeMapCaptureType captureType,
                JE::Render::Scene::CubeMapSpecularQualityLevel specularQualityLevel = JE::Render::Scene::CubeMapSpecularQualityLevel::Medium);

        private:
            // save the cubemap data to the output file
            void WriteOutputFile(AZStd::string filePath, uint8_t* const* cubeMapTextureData, AZ::RHI::Format cubeMapTextureFormat);

            // flag indicating if a cubemap render is currently in progress
            AZStd::atomic_bool m_renderInProgress = false;
        };
    } // namespace Render
} // namespace AZ
