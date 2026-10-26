# Vulkan 1.4 final target — Atom/JE migration gate (2026-10-26)

**OWNER DECISION / DESIGN TARGET / NOT IMPLEMENTED**

The final integrated **JericoEngine Vulkan renderer targets Vulkan 1.4**. Complete the necessary O3DE Atom/Gem capability absorption into native `JE::` owners **first**, then perform the Vulkan 1.4 runtime/API upgrade as an explicit gated renderer vertical. This changes the final Vulkan target, **not** the current active renderer version or the current MaterialCanvas/Editor/BSP priority.

## Current verified documentary facts

- On `development`, `Gems/Atom/RHI/Vulkan/Code/Source/RHI/Instance.cpp` declares `appApiVersion = VK_API_VERSION_1_3` in instance initialization. Its min/max/requirement negotiation and physical-device feature chains must be traced; this line alone does not prove the runtime negotiated version for every device.
- `docs/evidence/n0/toolchain.json` records a local **Vulkan SDK 1.4.350.0** (inventory captured 2026-10-10); subsequent project evidence records Vulkan Validation Layers 1.4.350 packaged for local use. Headers/tools/layers being 1.4 do **not** mean Atom RHI operates at Vulkan 1.4.
- No Vulkan 1.4 render acceptance, FPS uplift, shader feature parity, Linux validation or headset validation is established by this decision.

## Sequence and precedence

1. **Now — native integration:** continue authorized JE-owned integration of Atom render and the necessary existing Gem capabilities, consumers, passes, assets, editor tools and builders. Do not wait for Vulkan 1.4 to finish BSP/UI/material/Geology work. Do not turn every unrelated Gem into a gate merely because the wording says "all Gems"; the gate means the necessary renderer/Gem ownership graph and interfaces are stable, with no duplicate RHI/RenderGraph. Respect current ACTIVE_SLICE.
2. **Freeze baseline:** once relevant capabilities are integrated, capture the actual Vulkan 1.3 renderer/device/driver capability matrix and a representative Editor/Runtime scene, including DDGI/ray tracing, shadows, PBR, BSP corridors, geology and materials.
3. **Upgrade RHI:** audit loader/glad headers, Vulkan SDK/package pins, instance version negotiation, physical-device enumeration, mandatory/optional feature chains, extensions promoted to 1.4, extension aliases, function loading, shaders/SPIR-V toolchain, memory/barriers, dynamic rendering and validation. Do not merely replace `VK_API_VERSION_1_3` with `VK_API_VERSION_1_4`.
4. **Capability policy:** Vulkan 1.4 is the **final intended renderer target**. Evaluate explicit 1.3 compatibility/fallback separately with hardware/platform data; no guarantee of universal 1.3 fallback or implicit elevation of hardware minimum without product decision. The validation layers must report genuine device support. Keep DX12 code preserved but inactive under the current owner directive; Vulkan remains the sole active graphics backend. OpenXR/VR and Atom DDGI/RT stay required, with their own acceptance gates.
5. **Only then optimize:** compare specific Vulkan 1.4 core capabilities with Atom's present implementations: `VK_KHR_dynamic_rendering_local_read` (check depth/stencil and MSAA property limits), `VK_KHR_push_descriptor`, `VK_EXT_host_image_copy`, `VK_KHR_maintenance5/6`, robustness, subgroup rotate and resource limits. Promotion to core does not itself imply speed improvement or mandate adopting a new algorithm. Cross-reference [VulkanCppExamples research](VULKAN_CPP_EXAMPLES_ATOM_COMPARISON_2026_10_26.md).
6. **Validation:** capture actual Windows/Vulkan and Linux/Vulkan runs separately where available, validation-layer findings, crash/device-lost tests, shader compile/cook/variant regressions, Asset Processor/Editor/Runtime integration, material Save/Reopen, DDGI/RT, and real OpenXR headset tests when hardware exists. Compare CPU/GPU p50/p95/p99, RAM/VRAM, memory bandwidth proxies, draws/dispatches, screenshot diffs and stability before/after on fixed scenes.

## Stop conditions

- No renderer rewrite, extra backend, Vulkan-only experiment presented as integrated JE code, or unreviewed Gem adoption.
- No silent SDK downloads, AWS/CloudFront, multiplayer or new online runtime dependencies.
- No `PASS` status until native builds and GPU execution are recorded. This file is **design-only** and does not supersede older evidence, existing gates or ownership documents.

## Authoritative external references

- Khronos Vulkan 1.4 announcement (2024-12-03): https://www.khronos.org/news/press/khronos-streamlines-development-and-deployment-of-gpu-accelerated-applications-with-vulkan-1.4
- Vulkan version porting guide: https://docs.vulkan.org/guide/latest/versions.html
- Vulkan 1.4 feature requirements and partial dynamic local read guarantees: https://docs.vulkan.org/features/latest/features/proposals/VK_VERSION_1_4.html
