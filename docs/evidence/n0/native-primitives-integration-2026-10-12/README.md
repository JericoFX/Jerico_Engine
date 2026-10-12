# PrimitiveAssets integrated as native engine content — 2026-10-12

Accepted scope: the existing primitive meshes, materials and textures are owned by `Engine/Assets/Primitives`, installed with engine content and discovered through the engine Asset Processor registry. PrimitiveAssets is removed from engine catalog, fixture, AutomatedTesting and both project-template activations. Its historical CMake entry rejects reactivation. This is asset-only integration: no new renderer, material system, builder or runtime DLL.

## Source integrity and configuration

- Twelve source/metadata files moved. Three original FBX payloads were recovered from `media.githubusercontent.com`, pinned at O3DE `241c78b8064898a709cf1c5dd3535c6406763ef3`, with redirects disabled and exact original LFS SHA-256 and byte sizes. Three TIFF payloads were already present locally with matching original OIDs. The six material/assetinfo files retain their bytes. See acquisition/origin-manifest.json and external/source-freeze.json. No LFS pull, AWS/S3/CloudFront or automatic dependency installation.
- The original portable scan-folder key remains intentionally stable to preserve database identity. The historical `Amazon/AssetProcessor/Settings` schema is not an AWS service integration. Source paths now resolve under the engine folder. StandardPBR still belongs to the remaining Atom dependency.
- Offline configure completed with exit 0 in 116.4274655 seconds using the existing VS2022/x64 tree. Existing disconnected/cache settings remained intact. All 24 frozen inputs and five product executable hashes were unchanged after configure. No C++ implementation changed, so no engine rebuild or unrelated unit-test repetition was required.
- Thirty-one generated active profile registry files were checked without PrimitiveAssets activation or the old Gem source path. Installed SDK execution was not tested.

## Real cook and editor checks

- Before: FBX jobs 913–915 failed with four errors each and no products. After original payload recovery and integration: all nine processing jobs completed, zero errors, 46 product rows. The other three files are metadata, not separate jobs. Use `assetdb-comparison-correction.json` for product counts; it supersedes the initial count calculation.
- All nine SourceID/SourceGuid identities survived. ScanFolderID 92 retains its portable key and points at `Engine/Assets/Primitives/Assets`. The obsolete empty registry scan folder disappeared. Products include three models, three collision meshes, material products and streaming textures.
- One retained FBX warning: the box embeds an unavailable historical absolute `_Prim_Grid.tif` path. See box-import.log line 130 and job-913-warning-review.json. It affects the automatically converted FBX material; the separate MiddleGrey material uses the recovered relative texture paths and was explicitly assigned in the editor. Do not call the import warning-free.
- Editor PID 250424 opened the existing JENativeIntegration scene in Vulkan. A box model from the native assets was instantiated, moved, given the existing native Material component, and assigned `_box_1x1_middlegrey` using the picker rooted at Engine/Assets/Primitives. Original full-window screenshots record the picker and visible checker material. The temporary box was raised to Z=1 to inspect it above the floor; this is not a room lighting or geometry correction.
- Undo removed the temporary material/component/transform/entity changes. Save returned the copy to SHA-256 `4C5E436524DD9EA5EB4FAE0B8BF61DF1024D5B7784D4C92EA242A28298C70040`. Original room remained `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA`. No Reset All UVs was used. The authored-UV geometry rejection remains an open behavior, not fixed by this integration.

## Standalone regression and limits

- Vulkan PID 154640 and DX12 PID 240208 visibly rendered the existing room after configure/cook, without Editor/AP running. Each probe collected 20 samples with no TCP/UDP endpoints or Editor/AP processes, and 126 loaded modules with no Python/PySide/Qt/PrefabBuilder/Geometry.Authoring/EditorMaterials matches. See raw inventories and original captures. DX12 attempt1 did not launch: the previous process was still exiting, so the guard stopped it; attempt2 ran after verifying no product process remained.
- These runtime probes are room regression checks, not a packaged primitive scene, collision interaction test, new material persistence test or clean-clone reproduction. Prior UV/Save/reopen evidence remains separate. No VR headset or Linux test occurred. DX12/Vulkan lighting parity remains unresolved.
- No binary size reduction is claimed: this integration adds no DLL and retains existing binaries. Source payloads replace unusable pointers. The FPS overlay is not a performance benchmark; CPU/GPU percentiles, load-time comparison and package-size acceptance remain open.
- Editor closed through Alt+F4 after restoration. Owned AP/builders and launchers were explicitly terminated after checks; normal AP/launcher shutdown is not accepted by that action. Other dirty work was preserved.

Next coherent integration: ImGui ownership and its consumers, already inventoried but not implemented here. Atom and PhysX5 remain seed Gems alongside ImGui; transitive Gems, N0, packaging, UV-preserving geometry operations and the full GOAL remain open.
