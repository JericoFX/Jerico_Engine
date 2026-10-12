# Native room in the separate game launcher

2026-10-12, Windows Profile, local tree based on `bf623aeccd27fe98802998b21633c3fcee96faa0`. Existing compiled products and cooked cache were reused; no new build or download. This is not clean-checkout or source-free distribution acceptance.

The parent observed the room, corridor, door opening and grid floor in the separate game launcher with Vulkan (attempt2) and DX12 (attempt1). Both runs loaded `Geometry.dll` and `RenderMaterials.dll`. The complete loaded-module inventories contain no `Geometry.Authoring`, `EditorMaterials`, Qt, Python or PySide DLLs. This observation does not establish absence of statically linked consumers or files from the final package.

Vulkan attempt1 is retained as a rejected no-AP test: `connect_to_remote=0` means listen for an Asset Processor connection, not disable connections. The launcher spawned AP PID27912. The corrected runs used `--bg_ConnectToAssetProcessor=false`; their process snapshots show the launcher without Editor or AP. Launch arguments, executable hashes and full-resolution images are retained per attempt.

| Observation | Vulkan attempt2 | DX12 attempt1 |
|---|---:|---:|
| Loaded modules | 128 | 128 |
| Working set snapshot, bytes | 588607488 | 982958080 |
| Private memory snapshot, bytes | 1721815040 | 1999585280 |
| Overlay resident VRAM, MiB | 708.06 | 691.62 |
| Overlay reserved VRAM, MiB | 960.84 | 706.06 |

These are point observations, not a controlled performance comparison or frame-pacing benchmark. DX12 visibly produces brighter lighting; parity is not accepted. Both logs report a missing `game.cfg`; both no-AP runs still expose the inherited RemoteConsole listener on `0.0.0.0:4600`. Therefore this is not network-free acceptance. Normal launcher shutdown was not verified: Alt+F4 and the attempted console exit did not close the initial launcher, and the parent explicitly terminated only the owned test processes after capturing evidence.

Original prefab SHA256 remains `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA`. No UV reset or scene edit was performed. The earlier authored-UV guard remains a known geometry-edit limitation. Movement/collision, visual quality, all openings, VR, Linux, distributable closure and the remaining Gem absorption are not accepted by this test. See `parent-result.json` and the per-run raw evidence. All owned launcher/AP processes were absent at the final check.
