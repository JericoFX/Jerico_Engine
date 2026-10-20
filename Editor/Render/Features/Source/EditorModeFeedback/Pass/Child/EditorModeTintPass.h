/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/Child/EditorModeFeedbackChildPassBase.h>

namespace JE::Editor
{
    namespace Render::Features
    {
        //! Pass for editor mode feedback color tint effect.
        class EditorModeTintPass
            : public EditorModeFeedbackChildPassBase
        {
        public:
            AZ_RTTI(EditorModeTintPass, "{3E4FEFCB-9416-4CAE-8918-72D31AA482C5}", EditorModeFeedbackChildPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeTintPass, AZ::SystemAllocator);

            virtual ~EditorModeTintPass() = default;

            //! Creates a EditorModeTintPass
            static JE::Render::RPI::Ptr<EditorModeTintPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            //! Sets the amount of tint to apply.
            void SetTintAmount(float amount);

            //! Sets the color of tint to apply.
            void SetTintColor(AZ::Color color);

        protected:
            EditorModeTintPass(const JE::Render::RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //! Sets the shader constant values for the color tint effect.
            void SetSrgConstants();

            AZ::RHI::ShaderInputNameIndex m_tintAmountIndex = "m_tintAmount";
            AZ::RHI::ShaderInputNameIndex m_tintColorIndex = "m_tintColor";
            float m_tintAmount = 0.5f; //!< Default tint amount for the color tint effect.
            AZ::Color m_tintColor = AZ::Color::CreateZero(); //!< Default tint color for the color tint effect.
        };
    }   // namespace Render::Features
}   // namespace JE::Editor
