/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/FeatureProcessor.h>

namespace JE::Render::Features
{
    class SplashScreenFeatureProcessor
        : public JE::Render::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI_WITH_NAME(JE::Render::Features::SplashScreenFeatureProcessor, "AZ::Render::SplashScreenFeatureProcessor", "{B89EDE58-2C59-4E17-A691-019F80227F8A}", JE::Render::RPI::FeatureProcessor);
        AZ_CLASS_ALLOCATOR(JE::Render::Features::SplashScreenFeatureProcessor, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        SplashScreenFeatureProcessor() = default;
        virtual ~SplashScreenFeatureProcessor() = default;

    private:
        // FeatureProcessor overrides
        void AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline) override;
    };
}
