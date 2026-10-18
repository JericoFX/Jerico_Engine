/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Mesh/MeshInfo.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Buffer/RingBuffer.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Material/PersistentIndexAllocator.h>
#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <AzCore/std/parallel/mutex.h>


namespace JE::Render::Features
{
    class MeshInfoManager
    {
    public:
        MeshInfoManager();
        void Activate(JE::Render::RPI::Scene* scene);
        void Deactivate();

        // integration with the MeshFeatureProcessor
        MeshInfoHandle AcquireMeshInfoEntry();
        void ReleaseMeshInfoEntry(const MeshInfoHandle handle);
        const AZ::RHI::Ptr<MeshInfoEntry> GetMeshInfoEntry(const MeshInfoHandle handle);
        void UpdateMeshInfoEntry(const MeshInfoHandle handle, AZStd::function<bool(MeshInfoEntry*)> updateFunction);
        const AZ::Data::Instance<JE::Render::RPI::Buffer>& GetMeshInfoBuffer() const;

        void UpdateMeshInfoBuffer();

        int32_t GetMaxMeshInfoIndex()
        {
            return m_meshInfoIndices.MaxCount();
        }

        // utility function to initialize the buffer indices from default models
        static void InitMeshInfoGeometryBuffers(
            const JE::Render::RPI::Model* model,
            const size_t lod,
            const size_t meshIndex,
            const JE::Render::RPI::Material* material,
            const JE::Render::RPI::MaterialModelUvOverrideMap& uvMapping,
            MeshInfoEntry* entry);

    private:
        bool m_isEnabled = false;

        JE::Render::RPI::SceneId m_sceneId;

        JE::Render::RPI::PersistentIndexAllocator<int32_t> m_meshInfoIndices;
        AZStd::mutex m_mutex;

        AZStd::vector<AZ::RHI::Ptr<MeshInfoEntry>> m_meshInfoData;
        JE::Render::RPI::RingBuffer m_meshInfoBuffer;

        bool m_meshInfoNeedsUpdate = false;

        AZ::RHI::ShaderInputNameIndex m_meshInfoIndex = "m_meshInfo";
        JE::Render::RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
    };

} // namespace JE::Render::Features