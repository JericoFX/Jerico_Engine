/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Model/UvStreamTangentBitmask.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/DrawItem.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Model/ModelLod.h>
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAsset.h>

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector_set.h>

namespace OpenParticle
{
    class ParticleModel
    {
    public:
        ParticleModel() = default;
        ~ParticleModel() = default;

        void SetupModel(AZ::Data::Asset<JE::Render::RPI::ModelAsset>& model, size_t lodIndex);

        size_t GetMeshCount() const;

        AZ::Data::Instance<JE::Render::RPI::ModelLod> GetModelLod() const;

        bool BuildInputStreamLayouts(const JE::Render::RPI::ShaderInputContract& contract);

        bool IsInputStreamLayoutsValid(size_t meshIndex) const;

        const AZ::RHI::InputStreamLayout& GetInputStreamLayout(size_t meshIndex) const;

        const JE::Render::RPI::UvStreamTangentBitmask& GetUvStreamTangentBitmask(size_t meshIndex) const;

        const JE::Render::RPI::ModelLod::StreamBufferViewList& GetStreamBufferViewList(size_t meshIndex) const;

    private:
        bool GetStreamsForMesh(
            AZ::RHI::InputStreamLayoutBuilder& layoutBuilder,
            JE::Render::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
            const JE::Render::RPI::ShaderInputContract& contract,
            size_t meshIndex,
            const JE::Render::RPI::MaterialModelUvOverrideMap& materialModelUvMap = {},
            const JE::Render::RPI::MaterialUvNameMap& materialUvNameMap = {});

        JE::Render::RPI::ModelLod::StreamInfoList::const_iterator FindMatchingStream(
            size_t meshIndex,
            const JE::Render::RPI::MaterialModelUvOverrideMap& materialModelUvMap,
            const JE::Render::RPI::MaterialUvNameMap& materialUvNameMap,
            const JE::Render::RPI::ShaderInputContract::StreamChannelInfo& contractStreamChannel,
            JE::Render::RPI::ModelLod::StreamInfoList::const_iterator defaultUv,
            JE::Render::RPI::ModelLod::StreamInfoList::const_iterator firstUv,
            JE::Render::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut);

        JE::Render::RPI::ModelLod::StreamInfoList::const_iterator FindFirstUvStreamFromMesh(size_t meshIndex);
        JE::Render::RPI::ModelLod::StreamInfoList::const_iterator FindDefaultUvStream(
            size_t meshIndex, const JE::Render::RPI::MaterialUvNameMap& materialUvNameMap);

        AZ::Data::Instance<JE::Render::RPI::Model> m_modelInstance;
        AZ::Data::Instance<JE::Render::RPI::ModelLod> m_modelLod;
        size_t m_lodIndex = 0;
        AZ::HashValue64 m_lastContractHash = AZ::HashValue64{ 0 };
        AZStd::vector<AZ::RHI::InputStreamLayout> m_inputStreamLayouts;
        AZStd::vector<JE::Render::RPI::UvStreamTangentBitmask> m_uvStreamTangentBitmasks;
        AZStd::vector<JE::Render::RPI::ModelLod::StreamBufferViewList> m_streamBufferViews;
    };
} // namespace OpenParticle
