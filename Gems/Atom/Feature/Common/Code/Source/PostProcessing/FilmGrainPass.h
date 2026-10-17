/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>

namespace AZ
{
    namespace Render
    {
        // Class for controlling Film Grain effect
        class FilmGrainPass final : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(FilmGrainPass);

        public:
            AZ_RTTI(FilmGrainPass, "{786F1310-1AA1-42EB-90BF-14DF4A60BA9C}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(FilmGrainPass, SystemAllocator, 0);

            ~FilmGrainPass() = default;
            static JE::Render::RPI::Ptr<FilmGrainPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            FilmGrainPass(const JE::Render::RPI::PassDescriptor& descriptor);

            Data::Instance<JE::Render::RPI::Image> m_grainImage;

            AZStd::string m_currentGrainPath = "";

            RHI::ShaderInputNameIndex m_grainIndex = "m_grain";
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
