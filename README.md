<p align="center">
  <img src="docs/branding/jerico-j.svg" alt="Jerico modular J logo" width="104">
</p>

<h1 align="center">JericoEngine</h1>

<p align="center"><em>An O3DE-derived engine and integrated editor under active development.</em></p>

> **Project status — October 24, 2026:** Work is ongoing on the `development` branch. Focused Windows/Vulkan editor, rendering, geometry and tooling checks have been recorded. JericoEngine is **not yet a production-ready or fully validated engine**; those checks do not establish a complete build, cross-platform support or a shipping release.

## Direction

JericoEngine is transforming an Open 3D Engine (O3DE) codebase into a coherent, locally operated `JE::` engine and JericoEditor. The priority is a useful world-authoring workflow—BSP/CSG, materials, assets, physics and persistence—with additional systems integrated into the same editor and runtime.

- **Native ownership:** migrate adapted systems, build targets, registrations and public APIs into `JE::` modules. The transition from inherited O3DE structures is ongoing; a namespace alias alone is not considered completion.
- **Rendering:** Vulkan is the active development and validation backend. DirectX 12 remains in the intended Windows architecture but is currently inactive for build/cook/distribution. Windows/Vulkan, Linux/Vulkan and eventual Windows/DX12 acceptance are separate milestones.
- **Lighting and XR:** retain Atom's DDGI and ray tracing paths on Vulkan. OpenXR/VR remains required, but a desktop rendering test is not a headset validation.
- **Offline and single-player by design:** the intended product excludes AWS services/SDKs, multiplayer, matchmaking and online accounts. Cleanup of inherited dependencies and download paths is still being audited.
- **Lean, integrated tooling:** reuse proven O3DE components under engine ownership, remove unnecessary runtime/editor overhead with measurements, and keep authoring work out of source-free Shipping where possible.

## Authoring tools at a glance

These are the preferred, human-readable product and editor names. Technical acronyms remain searchable aliases during the ongoing integration; renaming a label does not rename serialized types or mark an implementation complete.

| Public name | Technical alias | What it does |
| --- | --- | --- |
| **Jerico Assembly** | PWAS | Distributes and assembles scene objects, props and prefabs under placement rules, clearances and artist overrides. |
| **Jerico Surface & Damage** | EPLS | Applies environmental aging and surface layers, and requests geometric chips or physical fractures from their owning systems. |
| **Jerico Brush** | RBrush | Paints or edits material, vertex, mask, decal and damage data on selected surfaces. |
| **Jerico Modular** | RMod | Builds modular pieces and routes such as pipes, rails, connectors and supports using compatible parts and sockets. |

## Current evidence versus targets

| Area | What can be said today |
| --- | --- |
| Native editor tools | Selected editor-tool, GraphModel/GraphCanvas and material-editor integration slices have passed focused Windows/Profile/Vulkan build, GUI and persistence checks. This does not close the entire editor. |
| Renderer | Focused Vulkan runtime and DDGI/RT-related integration tests exist; global image quality, performance, Linux and VR acceptance remain open. |
| BSP / geometry | Native CSG and geometry work has verified bounded test cases. The complete room/corridor/door/window authoring-to-cook-to-runtime workflow is still an acceptance target. |
| Materials and assets | Work includes explicit PBR channel assignment, arbitrary source texture names, image processing and graph tooling. End-to-end format coverage, KTX/KTX2, reimport, cooking and shipping are not fully certified. |
| Procedural authoring | Procedural Studio, Geology, **Jerico Brush (RBrush)**, **Jerico Modular (RMod)**, **Jerico Assembly (PWAS)** and **Jerico Surface & Damage (EPLS)** have defined ownership and technical contracts. Their complete integrated workflows are **not** verified engine features. |
| Interaction and content | Jolt physics, Luau, Motion Studio, Sequencer, UI, audio, VFX, AI/navigation and other local systems remain on the roadmap; individual inherited or experimental components do not prove full Jerico integration. |

The separate [Jerico Playground](https://github.com/JericoFX/JericoGeologyPlayground) contains mathematical experiments and historical test evidence. Its code and application are **not** part of the production JE runtime; any useful algorithm must be reviewed, adapted and retested in the current engine.

## Start here

- [Current goal and product constraints](GOAL.md)
- [Contributor and branch rules](AGENTS.md) · [Codex starting point](CODEX_START.md)
- [Documentation index](docs/README.md) · [Roadmap](docs/ROADMAP.md)
- [Active work and recorded results](docs/work/ACTIVE_SLICE.md) · [Handoff](docs/work/HANDOFF.md)
- [Native ownership contract](docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md)
- [Jerico Assembly (PWAS) contract](docs/PWAS_TECHNICAL_MASTER.md) · [Jerico Surface & Damage (EPLS) contract](docs/EPLS_TECHNICAL_MASTER.md)
- [Playground-to-JE integration gates](docs/PLAYGROUND_TO_JE_INTEGRATION_MASTER.md)
- [Third-party licensing and release checklist](docs/legal/O3DE_LICENSE_COMPLIANCE_CHECKLIST_2026_10_13.md)

**Branch policy:** `development` is the sole working and publication branch. Historical branches and earlier Jerico repositories are reference material, not alternative active engines. Read `GOAL.md`, `AGENTS.md` and the active work records before attempting builds or changes; inherited dependency/download settings must not be used blindly.

## License and origin

JericoFX-owned original code and copyrightable modifications are offered under **GNU GPL v3.0 only** ([full GPLv3 license](LICENSE); [scope and exceptions](LICENSE_POLICY.md)). This does **not** revoke or replace the original licensing of inherited and third-party material.

This repository began from O3DE source commit `241c78b8064898a709cf1c5dd3535c6406763ef3`. O3DE-covered source retains its original Apache-2.0 or MIT terms and notices ([O3DE license statement](LICENSE.txt), [Apache 2.0](LICENSE_APACHE2.TXT), [MIT](LICENSE_MIT.TXT)); other dependencies and assets have separate licenses. A GPLv3-compatible, redistributable engine build still requires the pending component-by-component legal audit.
