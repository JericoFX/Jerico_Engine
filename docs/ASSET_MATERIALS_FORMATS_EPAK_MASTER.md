# Jerico Materials, Assets, Runtime Formats and EPAK — authoritative contract

**Decision:** 2026-10-10. **Status:** DESIGN / CONTRACT ADOPTED; O3DE implementation NOT VERIFIED. **Owners:** N2 Assets/Material/Atom, N3 Luau, N5 Animation, N6 UI, N7 Audio/VFX, N11 Packaging/Runtime. Higher authority: root GOAL.md and AGENTS.md.

This is a deliberate clarification of the material, proprietary-format and packaging commitments that existed in JericoEngine1/JericoEngine2. **O3DE is the implementation base, not the final on-disk game contract.** Preserve and adapt its Asset Processor, Material Editor/Canvas, shader tools, Asset Bundler, VFS, identifiers and dependency tracking. Do not duplicate them and do not declare a Jerico format finished by renaming an O3DE file.

## 1. Mandatory ownership and boundary

- JericoAssetRegistry/identity is the **Jerico-facing contract** over one actual persistence/lookup implementation. Reconcile AZ::Data::AssetId, source GUID, product ID and stable Jerico GUID explicitly. No second independent registry with different revision truth.
- JericoCooker and JericoShaderCook are product names/workflows built by integrating the real O3DE Asset Processor/builders and Atom shader pipeline; not duplicate stand-alone executable graphs.
- JericoMaterials is the product-level contract; Atom material type/instance, shader variants and passes are the actual initial implementation. Do not introduce a competing PBR shader engine.
- The editor stores editable source metadata/graphs/recipes. Runtime and Shipping consume validated cooked products, not authoring input formats or hardwired O3DE tool internals.
- License/provenance of every imported source/library/asset is checked separately. Do not copy the Cry renderer, Cry material implementation or other proprietary code; historical Jerico codecs remain donor candidates until per-file audit.

## 2. Source formats and importing

### 2.1 Required intake

| Source category | Target authoring inputs | Boundary |
|---|---|---|
| Scene / meshes | glTF 2.0, GLB, FBX via ufbx, OBJ/MTL | Editor/Tools-only parsers, Canonical IR; Shipping never imports them |
| Textures | PNG, JPEG, TGA, TIFF, HDR, EXR, DDS, KTX2, Basis Universal | Tool-side decode/metadata and conversion; treat KTX2 as source/interchange |
| Terrain / height | PNG16, RAW16/32, EXR, TIFF and GeoTIFF when dependency admitted | Height semantics and coordinate/unit metadata must be explicit |
| Audio source | WAV, FLAC, OGG | Tool-side metadata/transcode to runtime audio data |
| Rig / clips | glTF/FBX animation/skin/morph where verified, native authoring | Convert to separate skeleton, rig, clip products |
| Procedural authoring | RMod TOML, graph/recipe metadata, RBrush, rui2d/rui3d, rlang | Authoring only by default; build into owned runtime formats |

No Autodesk FBX SDK or ATL/MFC may become an imposed new owner dependency. Do not assert full support merely because an extension is recognized. Track submesh/material slots, transforms, nested nodes, skinned meshes, morphs, animation, multiple UV sets, texture transforms, instancing, glTF extensions and unknown required extensions individually.

All source paths from imported files are untrusted: normalize, bound lengths/counts, reject escape/symlink and case-fold collisions, and enforce budgets before memory allocation. External textures and linked dependencies are captured with hashes and safe relative paths. Cancel/error preserves the last active asset revision; no half-published prefabs or materials.

### 2.2 Canonical import and cooked product graph

**Canonical single route:**

1. Read source and authored mapping metadata → validate and assign persistent source identity.
2. Build immutable import snapshot (source bytes + external dependency bytes + import tool/version + settings).
3. Normalize to one Jerico Canonical IR while reusing O3DE's actual scene/source pipeline where it can express the contract.
4. Register typed dependencies, stable GUID/namespace, content and recipe hashes.
5. Cook through the existing Asset Processor graph with per-platform target profiles and bounded intermediate data.
6. Publish candidate mesh, material, texture, skeleton, collision and other required products under reproducible identity.
7. Atom Editor preview selects only the validated candidate after its whole dependency closure is available.
8. Commit active selector/scene reference last and retain old products while live readers/fences reference them.
9. Cook runtime/package closure and verify source-free execution from an isolated installation.

Incremental invalidation affects only products whose inputs/dependencies/policies changed. A texture reimport must not rebuild geometry and physics merely due to a new file timestamp. Version recipe and builder code, not just source bytes. Unloading or hot reloading a GPU asset requires safe lifetime/fence retirement; a stale source reference must fail without corrupting the current scene.

## 3. JericoMaterials — semantic PBR authoring

Jerico's editor must expose **semantic inputs**, never filename magic inherited from Cry:
Base Color/Albedo, Normal, Roughness, Metallic, Ambient Occlusion, Emission, Opacity/Cutout; separate physical material and collision/friction settings. Optional staged material families: clearcoat, anisotropy, transmission/rough glass, subsurface/skin, height/parallax and layered/wetness. None of these optional lobes is implemented solely by appearing in the UI.

Material slot/source filenames are unconstrained. Names such as `_diff`, `_ddn`, `_spec` or `_orm` MAY suggest a mapping but MUST NOT define or override authoritative semantics. Inspector metadata determines color space, channel swizzle, normal-map convention and UV index/transform; selections survive Save/Undo/Redo/duplicating prefab/reimport/cook.

### 3.1 Both ORM and separate maps are mandatory

- **Packed ORM:** R = Ambient Occlusion, G = perceptual Roughness, B = Metallic; data channels are linear, not decoded as sRGB.
- **Separate maps:** AO, Roughness and Metallic accepted independently; author can choose source channel and scalar defaults.
- Both routes must produce equivalent shading for equivalent values, subject to explicit compression/precision tolerances.
- Scalar overrides, tint factors and per-channel strengths remain editable regardless of packing. Missing texture uses semantic defaults and a visible warning when user-specified required data is absent.
- BaseColor/emission color is sRGB where authored as color; normals, heights, roughness, metal, occlusion and masks are linear. Explicit metadata can override default source color-space where supported; never guess from the extension alone.
- Normal maps expose tangent-space orientation (OpenGL versus DirectX Y), normal strength and tangent compatibility; the cooker may flip channels without rewriting original files. Store convention in authoring metadata.
- Explicit alpha mode Opaque/Mask/Blend with cutoff, sorting/OIT fallback and two-sided/winding behavior. Transparent shaders are not declared equivalent to opaque PBR just because texture decoding works.

### 3.2 Geometry and tangent correctness

Preserve normals under inverse-transpose transforms and orthogonalized tangents under negative/nonuniform scaling; correct handedness when reflecting geometry/winding. Preserve authored tangents if valid; otherwise deterministic MikkTSpace-like admitted path with appropriate splitting. Reject a normal-mapped mesh with degenerate UVs when a correct tangent basis cannot be produced. A fallback tangent on an untextured mesh is not equivalent evidence.

Textures require complete deterministic mip chains, odd-dimension edge handling, linear filtering of data channels and signed vector-space averaging/renormalization of normals. Alpha coverage policy and specular anti-aliasing/normal variance are distinct decisions. Choose BC1/BC3/BC4/BC5/BC7 or other capability-supported targets by semantic/platform budget, with ASTC/other compression deferred until profile tested. Do not assume an image's container implies correct GPU compression.

### 3.3 Material Graph and shader cooker

Reuse Atom Material Editor, Material Canvas, Shader Builder/AZSL, HLSL→DXC→SPIR-V, SRGs and the one material/pass/variant cache. Author graph is editable/source; final material instance/product .rmat is validated cooked data and refers to .rtext/other GUID dependencies. Bound shader permutations and provide diagnostics for missing pins, inconsistent channels, incompatible shader variants and failed cooks.

GraphCanvas/GraphModel can be shared as a visual library; material evaluation belongs to Atom/Material owner. Do not introduce a second general shader VM or a second Vulkan pipeline to support our graph. O3DE's existing Lua material functors may remain **private tool/runtime implementation details only after explicit audit**; they must not turn into a second public gameplay VM beside Luau.

### 3.4 Material layering, Vertex Paint and RBrush

RBrush preserves BRUSH (coverage), PAYLOAD (material/mask/scalar/decal/damage) and TARGET (vertex, surface, terrain, decal instances, geometry). Material layering/vertex colors, terrain paint and projected decals can share UI gestures and preview but not their output ownership. Editable strokes retain seed/GUID/sort/masks/locks/overrides and survive Undo/Redo and rebuild.

For decals, audit Atom's current array/atlas strategy first; pay attention to metallic surfaces, normal/roughness blending, gradients/mips, projection bounds, receiver masks and deterministic overlap. SIGGRAPH/idTech6 concepts are design/reference only; do not copy proprietary shaders.

## 4. Jerico binary product catalog

The following names are **final product contracts**. Their mere listing does not mean codecs are implemented. Exact binary revisions and field layouts must be frozen through ADR/tests when each real consumer enters:

| Runtime product | Authoritative meaning |
|---|---|
| .rproject | Resolved project/session boot metadata |
| .rlevel | World layer/cell/scene entities and dependencies |
| .rpref | Prefab/component composition and variants |
| .rmesh | Mesh streams, LOD, meshlet tier if admitted, skin bindings |
| .rtext | Texture dimensions/format/color-space/mip and streaming metadata |
| .rmat | Material parameters, semantic texture refs and cooked shader variant identity |
| .rcoll | Physics collision/shape and parameters admitted to Jolt |
| .rnav | Cooked navigation data |
| .rskel | Skeleton hierarchy/reference pose |
| .rrig | Rig roles/IK/FK constraints, sockets and controls |
| .ranim | Clip/curves/events |
| .rgraph | Cooked animation-graph product or independent graph lifecycle when justified |
| .rseq | Sequencer/animation/camera/UI/audio timeline when reusable |
| .rscript | Luau typechecked/cooked module with dependencies |
| .rinput | Input contexts/actions/profile defaults |
| .rui | UI runtime layout/events/bindings/accessibility/local animation |
| .rloc | Localization runtime strings/layout metadata |
| .rfont | Glyph metrics/font subsets/fallbacks and .rtext atlas dependencies |
| .raudio | Audio sample/stream data |
| .rsound | SoundEvent definition and policy |
| .rsoundsurface | Physical material/contact sound sets |
| .rpart | **VFX/particle** runtime data — not a modular pipe/part file |
| .rsave | Versioned, bounded save/provider records |
| .epak | Indexed package, content pack and delta/overlay container |

Authoring extensions include .rui2d, .rui3d, .rlang, .rbrush and .rmod.toml. .rmod.toml yields ordinary .rmesh/.rcoll/.rpref dependencies; it is not a Shipping format by default. **.rtext is canonical** (not a conflicting .rtex alias). Do not invent an independent format for every graph tab or UI animation.

### 4.1 Shared binary safety rules

Every new runtime codec must define and test:

- Unique magic/type ID, major/minor schema, required/optional flags, deterministic byte order and exact file length.
- Chunk directory with bounded count, sorted IDs/order policy, offsets, lengths, alignment, overflow detection and non-overlap rules. Unknown required chunks reject; optional chunks have explicit forward-compatibility policy.
- Dependency IDs/GUIDs and a version/content digest, platform/profile/cooker or schema identity when relevant.
- Bounded allocations/decompression and validated enums/floats (finite units/transforms, no bad NaN/Inf values entering physics/render).
- Stable writer ordering, canonical normalization and reproducible output; a hash verifies integrity, **not provenance authenticity**.
- Corrupt/truncated, huge count/zip bomb, pointer overflow, bad nested reference, cyclic dependency, mismatched hash, unsupported required version and duplicate key tests.
- A validator/dumper, round-trip tests, version-migration tests and a real reader in Editor/Runtime/Shipping. The same validation layer should be reused, not bypassed by the editor.
- No serialized ABI-specific C++ structs/pointers, VM closures, raw engine/AZ/EMotionFX/Jolt types or absolute source paths.

The former JericoEngine2 JRMS/JRMT/JRTX/JEPK wire layouts are **historical design/test evidence**, not automatically copied. A new file extension or wire change needs an ADR specifying owner, migration strategy, negative tests and real consumer.

## 5. .epak, source-free Shipping, incremental cook and updates

An .epak is not merely a renamed O3DE .pak. The admitted direction is an indexed, chunk-addressable, optionally compressed/protected, signed product package. The existing O3DE Asset Bundler/VFS is the integration point to audit and adapt; a second independent package loader would duplicate ownership.

### 5.1 Packaging/streaming invariants

- Package header and index identify format version, package/content-pack ID, build/project/schema version, file directory, typed dependencies and content hashes.
- Individual assets/chunks are independently addressable for streaming and random access; bounds, decompression ratio/output byte limits and integrity are checked before consumers receive bytes.
- Versioned compression policy: **Zstandard (Zstd) per independently readable chunk** is the selected design candidate; compare existing O3DE compression/assets and pin a legally allowed implementation before code.
- **BLAKE3 content digests** are the preferred design candidate from JericoEngine1. Persist an algorithm ID; validate against package and chunk fixtures. SHA-256 may remain where source provenance/cook/tooling explicitly uses it. Do not silently change existing digests or confuse digest with signature.
- Protect against unauthenticated/partial patch publication. **Official release packages and official updates require an authenticated signature policy**, with explicit trust roots, signer identity, key rotation/revocation and fail-closed verification. Ed25519 through a licensed and vetted crypto dependency (libsodium a candidate) is the prior design direction, not a dependency integrated by writing this doc.
- Configurable encryption for product content (including the previously requested Shipping-default protection policy) is evaluated with explicit key distribution/threat model. Never embed claims that encryption prevents extraction or authenticates content; client-local keys have known limits. Keep encryption, signing and integrity as three distinct mechanisms.
- Development/debug packages may be unsigned only behind a non-Shipping policy with visible warning; Shipping must not silently accept them.

### 5.2 Delta/overlay packages and rollback

One packaging authority supports Base.epak, update/delta .epak, DLC/content packs and optional compaction/rebase. **Do not invent .epatch** solely to implement patches.

- Delta generation compares exact content hashes and dependencies, packages changed/new assets plus valid removals/tombstones where the schema allows.
- Overlay precedence and project/build compatibility are explicit, deterministic and validated before activating an update.
- Applying an update is transactional: download/stage candidate, validate closure/signature/space, then atomically switch active manifest. On failure or interruption keep the last known-good version.
- Subsequent compaction/rebase can create a new base package without altering authoring GUIDs or silently losing patches/DLC. Migration between major schema versions needs an explicit compatibility policy.
- Streaming respects live resource lifetimes; don't unload old mesh/texture/audio bytes while GPU, audio or physics readers still reference them.
- Verify cold launch, no source, fresh install, base+delta, interrupted update/rollback, duplicate/unknown dependency, hash mismatch, invalid signature and disk-full conditions.

### 5.3 DLC and campaigns

ContentPack = installable/versioned bundle with namespace/dependencies/entitlements or local admission policy, never arbitrary DLL execution. Campaign = playable experience/save scope selecting content via CampaignRegistry/Context. Multiple campaigns can share one executable and immutable cooked assets by GUID/dependency closure; user saves have an explicit CampaignId and compatibility/migration policy. Adding campaign content is not a reason to fork JericoEngine or duplicate the engine/runtime.

The final Shipping directory has executable, .epak content and required DLLs in the chosen Jerico product layout, preserving legally needed third-party resources/notices. Nothing requires the development checkout, offline importer, shader source/compiler, AWS login or legacy Cry formats. Proof must run with source paths unavailable and network disconnected.

## 6. Editor workflows and acceptance fixtures

**MATERIAL-01:** import a GLB with two materials and independently named source maps, bind semantic channels in Inspector, set roughness/metal/normal, save/reopen, cook and display both materials using Atom/Vulkan. Change filenames to unrelated text and prove no required naming suffix.

**MATERIAL-02:** use ORM and three separate AO/Roughness/Metallic textures carrying equivalent values; same UV set, scalar settings and lighting. Compare linear sampled values and rendered output within justified tolerance. Repeat with a different color-space declaration, normal-Y convention and negative-scale mesh.

**MATERIAL-03:** edit a material layer/vertex paint/RBrush decal, Undo/Redo, reimport one source texture, verify locked/manual stroke/slot data is not lost. A canceled or corrupt import leaves previous product active.

**FORMAT-01:** for each admitted Jerico format, use a real writer/reader and round-trip/endianness/version/chunk tests, then a consuming runtime object. Corrupt offsets, truncated chunk, unsupported version, missing dependencies, huge sizes and inconsistent GUID must fail closed.

**EPAK-01:** source-free package startup with correct asset closure and texture/mesh/material/sound/script products; remove SourceAssets and development tools before test. Build reproducibility and read limits recorded.

**EPAK-02:** base + delta containing only modified .rtext/.rmat and a removed product; verify precedence, rollback after interruption, signature failure rejection and an independently installed DLC/ContentPack with shared assets.

**PERF-01:** trace import/cook time, memory peak and loaded VRAM, frame quality/performance and hot-reload stalls. A 512MiB on-disk bound does not prove 512MiB resident RAM. Fixed camera/scene/hardware and p95/p99 matter.

These fixtures are specifications for later N2/N11 implementation. They are not claimed to pass now. Each implementation slice must link producer, consumer, tests and evidence to the corresponding IDs in REQUIREMENTS_MATRIX.

## 7. Primary source map

- Previous catalog: JericoFX/JericoEngine2@304d9d2e13704e4967e82a8d5a13a5a506be01d4/docs/RUNTIME_FORMATS.md.
- Prior direct authoring contract: same commit's GOAL.md, asset/material clauses, and JERICOENGINE1_TRANSFER_MAP.
- JericoEngine1 donor specifications: ASSET_IMPORT_AND_COOKING.md, RUNTIME_FORMATS_AND_CHUNKS.md, JERICO_INCREMENTAL_COOK_DELTA_EPAK.md, JERICO_HOT_RELOAD_LIVE_ASSET_REFRESH.md, and Jerico's materials/Shader Master. Source code reuse requires per-file provenance.
- Current O3DE: Code/Tools/AssetProcessor; AssetBundler; Gems/SceneProcessing; Gems/Atom/Asset/Shader; Gems/Atom/Tools/MaterialEditor/MaterialCanvas; Gems/Atom/RPI and Feature/Common.
- Licenses and package rules are governed by docs/legal/PROVENANCE_AND_DEPENDENCIES.md and the effective licenses/versions, not by this technical file alone.
