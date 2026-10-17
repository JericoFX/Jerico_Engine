/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}FeatureProcessor.h    (variant: ScreenSpaceConstant)
// -----------------------------------------------------------------------------
// FeatureProcessor that injects ${Name}Pass into every render pipeline at the
// canonical post-process insertion point (after "PostProcessPass").
//
// PATTERN ORIGIN:
//   Mirrors AZ::Render::SilhouetteFeatureProcessor. Atom calls AddRenderPasses
//   once per pipeline, before passes are finalized. Inside that hook we build
//   a PassRequest referencing the disk-loaded "${Name}Template" and splice it
//   in via RenderPipeline::AddPassAfter.
//
// WHY A FEATURE PROCESSOR INSTEAD OF A SETREG OR MANUAL PASTE?
//   * Setreg-based pass-template mappings replace, not insert -- no good for
//     adding a single pass to an existing pipeline.
//   * Manual paste in the project's pipeline asset is brittle, project-local,
//     and forgettable. The FeatureProcessor approach makes the pass active
//     anywhere this gem is loaded, with zero per-project setup.
//
// CVar TOGGLE:
//   r_${Name}Enabled (bool, default true) flips the pass on/off at runtime
//   without rebuilding. Useful for A/B testing and shipping kill-switches.
// =============================================================================

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>

namespace ${GemName}
{
    class ${SanitizedCppName}FeatureProcessor final
        : public JE::Render::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}FeatureProcessor, "{${Random_Uuid}}", JE::Render::RPI::FeatureProcessor);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}FeatureProcessor, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        ${SanitizedCppName}FeatureProcessor() = default;
        ~${SanitizedCppName}FeatureProcessor() override = default;

    protected:
        // FeatureProcessor overrides...
        void Activate() override;
        void Deactivate() override;

        //! Called by Atom once per pipeline before pass finalization.
        //! This is where the pass gets spliced into the pipeline graph.
        void AddRenderPasses(JE::Render::RPI::RenderPipeline* renderPipeline) override;

        //! Per-frame hook -- used here only to honour the runtime CVar toggle.
        void OnRenderEnd() override;

    private:
        void SetPassEnabled(bool enabled);

        JE::Render::RPI::Pass*           m_pass = nullptr;
        JE::Render::RPI::RenderPipeline* m_renderPipeline = nullptr;
    };

} // namespace ${GemName}
