# JE Editor fixture

This in-tree project supplies a real native Editor scene with a focally verified
WhiteBox authoring, material/UV editing and Editor/runtime capture route. These
checks do not close N0/N1 or the complete product acceptance. It has no custom Gem and uses
the two remaining seed Gems (Atom and transitional PhysX5) in project.json; their transitive dependencies remain required. Geometry/WhiteBox, materials, cameras and the tool-only procedural prefab builder are composed directly from native engine subsystems. Primitive meshes and textures are native engine content. ImGui debug integration is owned by the native engine, reusing Dear ImGui. WhiteBox, Camera, CameraFramework, PrefabBuilder, PrimitiveAssets and ImGui are no longer activated as project Gems.
The retained template `script_only` flag means no custom project Gem; it does
not select a scripting runtime. Their actual native/asset dependencies
remain required; the fixture does not replace the engine renderer or cooker.

Configure the existing engine root with the explicit project selection:
`-DLY_PROJECTS:STRING=Projects/JEEditorFixture`. Keep the independently audited
offline dependency inputs used for the engine configuration. No global project
registration is needed. The existing targets are `Editor`, `AssetProcessor`,
`AssetProcessorBatch`, `JEEditorFixture.Assets` and `JEEditorFixture.GameLauncher`.
The project-centric EngineFinder path is retained from the template; validation
uses the engine root and its source build.

Open `Levels/JEEditorFixture/JEEditorFixture.prefab` in the Editor for this project.
Its baseline native camera looks along +Y from inside the corridor; the native
directional light and RoomLight/CorridorLight illuminate the scene. There are no
external models or HDR skybox. Choose **Tools > Author JE Room/Corridor Fixture**.
This C++ action requires the exact project UUID and local level already open
and focused at the root, with no unsaved edits or active undo batch. It never
changes an unrelated scene. Five native CSG calls build a 10x8x4 room with 0.25 walls,
the corridor, doorway and window from the native room/corridor test fixture.
It computes scratch meshes before creating a single entity, checks actual API
outcomes, mesh counts and all five fixture volumes, and serializes with WhiteBox's
own API. A native Editor undo batch captures the mesh/material/name in the prefab
template before saving through PrefabPublicInterface. It requires Asset Processor
and the catalog to provide two distinct native Atom material products:
`materials/presets/pbr/default_grid.azmaterial` and
`materials/presets/pbr/metal_iron.azmaterial`. The Atom Material component owns
their assets and overrides; WhiteBox binds full surface identities to slots 1/2.
Upward-facing surfaces use the grid slot; this group includes top surfaces and
does not mean only the floor. Reopen through the Editor's normal File menu.
The default WhiteBox shape selector also builds its five original primitives
natively, preserving their parameters, winding and planar UV generation.

A completed ownership marker makes reruns preserve the current mesh, including
manual edits and locks. Partial or ambiguous markers fail without rebuilding;
recover through the Editor's Undo or manual inspection. Automatic deletion
on failure is restricted to the unlocked entity just created in the same undo
batch. All CSG operands are scratch meshes, so no temporary operand entities
need deletion. The five operations and persistence calls do not replace the 117 native CSG
checks or prove runtime collision, complete topology acceptance or visual quality.

The Selected Faces Inspector provides a typed Atom **Face Material** picker and
explicit **Assign Material**, **Apply UV Transform** and **Reset All UVs** buttons.
The picker and UV fields prepare an edit; changing them alone does not apply it.
Assignments preserve compatible property/UV overrides, with explicit rejection
when they cannot be preserved. Surface bindings and Material component references
are saved in the scene; exported WhiteBox meshes retain geometry/UV/identity, not
scene material bindings. Tint/Use Texture and the project's
`Assets/materials/WhiteBoxDefault.material` remain legacy slot-0 fallback behavior,
not substitutes for the fixture's two required material assignments. The existing
project scan-folder priority shadows that fallback product only for this project.

Historical focal evidence (2026-10-11, original scene baseline):

- Native suites passed: **575/575 WhiteBox** and **68/68 Atom Feature Common**.
- [DX12 attempt9](../../build/n0-validation/editor-fixture-dx12-attempt9/verification.json)
  and [Vulkan attempt2](../../build/n0-validation/editor-fixture-vulkan-attempt2/verification.json)
  completed Save/reopen and real Atom Editor/runtime captures, with an Atom Material
  owner, 30 surface bindings, distinct grid/metal IDs and verified runtime identity/camera.
  The parent reviewed both 1199x583 images for each backend: floor and doorway are present.
  DX12 R10 capture retains the original DDS alongside its quantized PNG preview.
- [Vulkan face UI attempt2](../../build/n0-validation/editor-face-ui-vulkan-attempt2/parent-ui-result.json)
  verified all three buttons, selected-face UV Apply/Save/Undo/Redo and material
  picker Assign/Save/Undo/Redo. Redo through the Edit menu reproduced each applied
  prefab byte exactly. The emitted Redo shortcut did not invoke the action; its
  cause is not diagnosed. Preparatory picker/UV values were restored explicitly.
- [Vulkan window attempt2](../../build/n0-validation/editor-window-vulkan-attempt2/verification.json)
  completed Save/reopen and Editor/runtime captures from camera position (7,0,1.6),
  Z rotation 90 degrees. The parent reviewed both 1199x583 PNGs: the rectangular
  opening, interior and sill are visible, while the walls remain dark. This is a
  bounded geometry observation, not complete visual-quality acceptance.

The historical runs above restored the camera and scene to prefab SHA256
`4E337058DEC499E4387FC3022C869129943D9CF35037B8D1EC0F9749F313FBCD`.
The current fixture candidate is the subsequently configured scene
with SHA256 `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA`.
It retains the same room, camera, lights and two face-material references, plus
the `JE_HDR_LUT_validation` entity with HDR Color Grading enabled and exposure 1.
It references no generated LUT or user-imported asset. This configured scene is
not silently substituted for the historical capture inputs; LUT generation,
activation and their validation copies remain outside this fixture publication.
The UV variant subsequently completed Save/reopen and Editor/runtime captures in
`editor-uv-runtime-vulkan-attempt1`, preserving its exact saved file. A material
assignment on that edited UV was manually reopened and run in game mode; native
UI screenshots and the Editor log are retained in
`editor-material-uv-runtime-vulkan-attempt1`. The original fixture verifier rejected
the intentionally different face-slot distribution; its gate was not weakened.
Comparison confirmed material assignment preserved geometry/UV. The Editor was
closed before restoring the original saved baseline. Reset All UVs was visible
but not executed.
Standalone Profile launcher evidence is now published in
[the native runtime no-listener record](../../docs/evidence/n0/native-runtime-no-listener-2026-10-12/README.md).
Vulkan and DX12 visibly rendered this FEE scene without Editor or Asset Processor,
using `--bg_ConnectToAssetProcessor=false`,
`--regset=/Amazon/AzCore/Bootstrap/wait_for_connect=0` and
`--regset=/Amazon/AzCore/Bootstrap/remote_filesystem=0`.
RemoteConsole was disabled by its repaired default, with no console override.
Each backend had 20 process-owned TCP/UDP samples over approximately 25 seconds
and a later final check, all empty. These samples are not a packet trace or an
all-time absence guarantee; explicit RemoteConsole opt-in retains its legacy
listener. The local console was observed on Vulkan. The runs used the audited
local build and cooked cache and were explicitly terminated; normal exit,
source-free replay, complete packaging and DX12/Vulkan lighting parity remain
unverified.

Full BSP/CSG editing, grid/snap behavior, Hammer2/custom UX, runtime collision,
Linux Vulkan, comparable performance and VR/OpenXR remain pending. Backend brightness is not a controlled visual comparison. Atom viewport
captures do not themselves prove Qt/Inspector interaction; the separate UI evidence
covers only the actions listed above. An issued readback cannot be canceled and
may finish writing after verification has failed or been canceled.

The original Windows template GameSDK.ico and Splash.bmp are copied byte for
byte into Resources after manual acquisition and SHA256 verification. They are
distributed as regular Git binary files through two exact path exceptions,
not through the fixture's LFS endpoint. Their inherited payload identities are
`Resources/GameSDK.ico`: 148143 bytes, SHA256
`BA0704EFD586FEE72B3D45B19163A1B67CD5594BCBF57CB28D796E2919502ABA`, and
`Resources/Splash.bmp`: 1200056 bytes, SHA256
`BBD314F41B90BC9CBA384D86EED865568343CBD332568F74C8CA39A08BC43836`.
Both match the original ScriptOnlyProject resources at the pinned O3DE origin
described below, as well as the current DefaultProject and MinimalProject LFS
OIDs; the fixture retains their Apache-2.0 OR MIT notices and licenses.

The required `default_grid.material` references these three engine texture
sources. A clone containing only their LFS pointer text cannot cook the grid:

| Repository-relative source path | Payload bytes | SHA256 / LFS OID |
| --- | ---: | --- |
| `Gems/Atom/Feature/Common/Assets/Textures/Default/default_basecolor.tif` | 793918 | `5E9E1371FB108A057D246935B01374F2BB86AE0B126BAE471176CE62284EACD7` |
| `Gems/Atom/Feature/Common/Assets/Textures/Default/default_normal.tif` | 793918 | `1B56097F0DF01DA7ED49311462002F5E1A61B8496306C7AB3B12E17C27AFB28A` |
| `Gems/Atom/Feature/Common/Assets/Textures/Default/default_roughness.tif` | 810996 | `BA52FF6D59F50B4B5DEA6E266572C9E7FEB92D5FD85BD0738E0D07D42D234BF7` |

The local TIFF payloads were verified against those identities. Original
acquisition receipts [213](../../docs/evidence/n0/editor-fixture-publication-2026-10-12/acquisition/213.json), [215](../../docs/evidence/n0/editor-fixture-publication-2026-10-12/acquisition/215.json) and [216](../../docs/evidence/n0/editor-fixture-publication-2026-10-12/acquisition/216.json)
record direct HEAD and GET 200 on 2026-10-10, with no credentials or redirects,
from `https://media.githubusercontent.com/media/o3de/o3de/241c78b8064898a709cf1c5dd3535c6406763ef3/`
plus each exact repository-relative source path above. This is the audited
primary origin, not an instruction to use the blocked LFS/CloudFront endpoint.
Remote availability was not rechecked for this publication. The acquisition
receipts cover immutable snapshots; the later checkout materialization's author
and operation are not identified. Hash agreement is distinct from cook/runtime
acceptance, and the standalone evidence above depends on the existing cooked cache.

The common launcher icon, font files, active QRC icons and bounded Atom defaults
are also materialized from their exact existing LFS pointer identities; their
source hashes are recorded in the N0 acquisition evidence. The original Editor
splashscreen_background.png remains unavailable (HTTP 404), with no substitute
image. That historical asset failure does not negate the native Editor runs above;
resource availability is not blanket launcher/deployment acceptance. The legacy `Amazon` key in assetprocessor_settings.setreg
is the existing local Asset Processor settings namespace, not a download route.
Motion/performance and the remaining product gates require their own measured runs.
