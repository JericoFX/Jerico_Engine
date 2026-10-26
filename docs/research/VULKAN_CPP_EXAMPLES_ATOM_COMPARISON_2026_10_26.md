# VulkanCppExamples vs Atom — research triage for JericoEngine (2026-10-26)

**Status: RESEARCH ONLY / NOT INTEGRATED / NOT BENCHMARKED.** This document records a candidate source for bounded code-level audits. It does not authorize a renderer replacement, new subsystem, changed active slice, or GPU feature acceptance.

- External source: [myemural/VulkanCppExamples](https://github.com/myemural/VulkanCppExamples), README on `master` inspected 2026-10-26, README blob SHA `820897cfb387cd3c52c486adf88c5d0bdcacfcf8`.
- Native baseline: O3DE Atom RHI/RPI/Feature Processors/Pass System, inherited and being adapted to `JE::Render`.
- Owners and constraints: [GOAL](../../GOAL.md), [AGENTS](../../AGENTS.md), [render master](../RENDERING_RASTER_SIGGRAPH_MASTER.md), [active slice](../work/ACTIVE_SLICE.md).
- Active graphics backend **Vulkan** on the currently authorized paths. Retain DX12 source, but **do not activate DX12 builds/cooks/distribution/tests** without a later owner order. Preserve Vulkan Atom DDGI and ray tracing. No additional RHI, RenderGraph, device scheduler, standalone engine, Gem activation, AWS or multiplayer.
- Source repository reports MIT licensing. Verify actual LICENSE and third-party shader/assets/licenses before copying; preserve notices and update provenance. External public code is not automatically production-ready.

## What the external repository actually offers

Modern C++20 learning examples, one CMake target per example, with Vulkan SDK/GLFW/glm/tinygltf dependencies and Windows/Linux targets reported in its README. Examples are isolated teaching implementations, not a full engine, not Atom replacements, and not evidence of FPS improvement. They do not establish Jerico capability or Atom absence.

| Priority | Examples (path relative to external repository `Examples/`) | Atom/JE overlap and hypothesis | Required decision |
|---|---|---|---|
| P1 investigation | `AdvancedShaderProgramming/AdvancedComputeShaders/OcclusionCullingCompute`; `Fundamentals/QueriesAndPerformance/IndirectDrawing` | Compare GPU hierarchical depth culling against existing Atom visibility/software hierarchical-Z and JE BSP/room visibility. Avoid duplicate GPU Scene/registries. | EXISTING / IMPROVE / REJECT after real BSP + cave measurement |
| P1 investigation | `AdvancedShaderProgramming/MeshAndTaskShaders/{MeshletRendering,FrustumCullingTaskShader,LodTaskShader}` | Compare geometry submission and LOD against current Atom draw/instancing paths; extension and hardware support must be queried. | Optional feature path with fallback; do not make first BSP dependent on it |
| P1 investigation | `PostProcessingEffects/AmbientOcclusion/{ScreenSpaceAo,HorizonBasedAo,GroundTruthAo}` | Atom includes SSAO and other GI/reflection techniques; audit the exact current AO path, temporal stability, contact detail, interaction with DDGI. | A/B image and cost, no second AO stack |
| P1 investigation | `RealTimeShadows/ShadowFilteringAndSoftening/{PercentageCloserSoftShadows,ExponentialVarianceShadow,VarianceShadowMapping}` | Atom already documents cascaded and punctual shadows and PCF/ESM. PCSS/EVSM/VSM are candidates, not proven missing features. | Compare penumbra, leaks, acne, temporal stability and atlas bandwidth |
| P1 investigation | `PhysicallyBasedRendering/AreaLights/RectangularAreaLights` | Atom has area lights; compare LTC implementation/energy and authoring with native feature. | Improve existing light type only if measured |
| P2 | `RealTimeLighting/TransparencyTechniques/{WeightedBlendedTransparency,DepthPeelingTransparency,PixelLinkedListsTransparency}` | Candidate transparency modes for glass, VFX, webs. Per-pixel linked lists may have severe memory/overdraw costs. | Choose one bounded use-case and retain fallback |
| P2 | `PhysicallyBasedRendering/DisneyPrincipledBrdf/*`; `PhysicallyBasedRendering/DisneyBsdfTransmission/*` | Compare clear coat, sheen, anisotropy, IOR/transmission, thin surfaces/SSS against actual Atom materials. Maintain arbitrary texture names, explicit slots, sRGB/linear, normal orientation and KTX/KTX2 contracts. | Shader/material/cook/reimport gates before adoption |
| P2 | `PostProcessingEffects/ColorProcessingToneMapping/HdrToneMapping`; `PostProcessingEffects/ExposureAndAdaptation/{HistogramBasedAutoExposure,EyeAdaptationAutoExposure,LocalExposure}` | Atom already has tone/color/exposure; candidate AgX, ACES, local exposure comparisons. | One environment/exposure pipeline; artifact and flicker checks |
| P2 | `AdvancedShaderProgramming/AdvancedComputeShaders/{FftOceanCompute,FluidSimulationLbm,FluidSimulationNavierStokes,ClothSimulationCompute,WindSimulationFoliage}` | Research only for water, FX and environmental motion. FFT ocean ≠ buoyancy; fluids ≠ Jolt. Buoyancy remains DESIGN ONLY. | Isolated prototype only after consumer and budget are defined |
| P2 | `Fundamentals/ComputeShaders/BasicParticles`; `Fundamentals/QueriesAndPerformance/{TimestampQuery,PipelineStatisticsQuery,DescriptorIndexing,BufferSuballocation}` | Existing Atom RHI/profiler and VFX paths come first. These are diagnostics/experiments to check actual assumptions. | Reuse existing profiling and resource ownership |
| R / defer | `AdvancedShaderProgramming/TessellationShaders/*`; geometry-shader grass; multiple competing clustered/deferred paths | Atom already uses Forward+ and modular passes; geometry/tessellation paths should not multiply rendering architectures. | Only revisit against measured real consumer bottleneck |

## Atom facts versus unknowns

O3DE's published Atom pipeline describes RHI, RPI, FrameGraph, Forward+ as the default main path, area lights, shadows, probe reflections, skinning, color grading, eye adaptation, and terrain. Documentation is evidence of upstream design, **not** proof that a JE production path is passing. The inherited code and adapted JE code must be inspected at the actual `development` HEAD. The status labels in the table are *investigation candidates*, not a claim that Atom lacks their features.

Official references:
- https://docs.o3de.org/docs/atom-guide/
- https://docs.o3de.org/docs/atom-guide/dev-guide/render-pipelines/
- https://docs.o3de.org/docs/atom-guide/dev-guide/rhi/
- https://docs.o3de.org/docs/atom-guide/dev-guide/passes/pass-system/

## Codex follow-up: exact audit gates (NO IMPLEMENTATION implied)

1. Freeze the inspected JE `development` HEAD and upstream sample commit; list exact source files, shader sources, dependencies and licenses for each selected example. Check relevant open/merged O3DE PRs before proposing a subsystem change, and record the result.
2. Trace Atom producers, Feature Processors, pass templates, shaders, RPI/RHI consumers and actual JE native ownership. Classify each technique **EXISTS / IMPROVE / MISSING / REJECT / BLOCKED**, with source-path and commit evidence. Do not classify solely from filenames or README.
3. Select **one** candidate per authorized vertical: first BSP/corridor with occluders and doors; then cliffs/rock assets; material AO/shadow visuals as a separate bounded comparison. The current MaterialCanvas active slice retains precedence.
4. For a proposed improvement, capture real Vulkan editor/runtime image BEFORE and AFTER, same camera, scene, lights, resolution, driver, GPU and quality tier; report CPU/GPU p50/p95/p99, VRAM/RAM, draws/dispatches, image differences and artifacts. Use warm/cold measurements as applicable. ON/OFF/ON or A/B/A preferred. GPU timestamp alone is not FPS.
5. Validate bounds/invalidation with open/closed doors, movable lights, cameras grazing tunnels, transparent materials, animated content, editor Undo/Redo, save/reopen/cook and fallback/unsupported capabilities. Include Linux Vulkan and OpenXR evidence only when actually run.
6. Keep/extend current Atom pass and resource ownership; no parallel rendering engine, shadow registry, exposure system or scheduler. Any retained external source must include per-file provenance/license notices, no hidden downloads and no AWS dependency.
7. Record verdict and measured evidence in the existing work/evidence mechanism; proposals are not PASS. No GPU/build/runtime measurements were performed for this document.

**Suggested first concrete question:** does the external hierarchical-Z compute example beat or complement the existing Atom visibility path in a representative JE BSP mine corridor with doors and intersecting geometry, without adding a second visibility authority?
