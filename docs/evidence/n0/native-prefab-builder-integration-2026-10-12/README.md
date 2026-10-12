# Native PrefabBuilder integration — 2026-10-12

Implemented and verified locally on Windows Profile from `development`, based on `ebcdd06de6fc24d653f6ffaf0dc0f8219cd77332`. This accepts the PrefabBuilder absorption slice, not N0 or the full engine goal.

## Implementation

`Engine/Assets/PrefabBuilder` owns the real implementation in `JE::Assets::Prefab`, exported by `JE::PrefabBuilder`. Editor, Asset Processor/Batch, AssetBuilder and SerializeContextTools compose its factory directly. It remains host-tools-only; the game has no builder dependency. The four Atom tools preserve their former exclusion of this capability.

The migration preserves the module and component UUIDs, serialized fields/versions, behavior names, `GameObjectCreation` pipeline, `Prefabs` job key and existing SceneAPI lifecycle. Two descriptors remain; no required system component or parallel cooker was added. Both public buses have one exported implementation. The old Gem's source/build targets and project/catalog activations are retired; historical metadata and attribution remain. Public C++ include paths and namespaces intentionally change.

## Verification

- Offline configure: exit 0, 57.845 s. No package downloads. Current product registries no longer reference `PrefabBuilder.Builders`.
- First build exposed two incorrectly namespace-qualified `BehaviorValueProperty` macros. The two-line correction is the only source change between freeze revisions. The failed build is retained.
- Final build: all nine assigned targets, exit 0, 330.070 s. Reused the unchanged successful configure. Three CTest suites passed in 9.23 s: PrefabBuilder 22 cases, EditorLib 72, SerializeContextTools 1; 95 total, zero failures/errors/disabled cases. The 21 original PrefabBuilder cases remain.
- Final source freeze: 52/52 inputs unchanged. The original writer receipt had stale byte counts after whitespace removal; the integrated inventories contain measured lengths and hashes. CTest inventory parsing and a registry-receipt correction affected evidence only, not source or test outcomes.
- Asset Processor PID 83520 and ten owned AssetBuilder processes loaded `PrefabBuilder.dll`, never the old Gem DLL. The supplied reprocess list regenerated both room spawnables at 10:00:46 UTC with hashes identical to the baseline. Both source prefabs also remained byte-identical.
- The audited 30,656-byte Cube fixture produced 21 products, including `nativeprefabcube_fbx.procprefab`, mesh, material and buffers. Editor PID 11996 opened JENativeIntegration on Vulkan, instantiated this procedural prefab, displayed its cube, accepted a position edit, and undid both changes. Save restored exactly SHA `4C5E436524DD9EA5EB4FAE0B8BF61DF1024D5B7784D4C92EA242A28298C70040`. Original room SHA remained `FEE55849A032D29059D4369112047DC27E281D39631D0F0DF87D8BCF5EF517AA`.
- Independent launchers rendered the original room with Vulkan (PID 276180) and DX12 (PID 261816). Each had 20 sampled TCP/UDP checks with zero endpoints and no Editor/AP process. Loaded-module inventories contain no PrefabBuilder, Qt, embedded Python, EditorCamera, EditorMaterials or Geometry.Authoring. Captures are original window images, not mockups.

## Size and remaining limits

New PrefabBuilder DLL: 1,296,384 bytes, SHA `215A85ADF5BA17C94F395A0472D63CF2E3C310FBF656B198A27A4FED9EA91153`. Old Gem DLL: 1,199,104 bytes. **This module grew by 97,280 bytes (8.11%); no product size saving is claimed.** Native composition and explicit exports are verified; size optimization remains open.

After verifying no active consumer, only the obsolete Gem DLL and PDB were removed from the current build profile, reclaiming 42,679,296 bytes of local build output. This is not a shipping-size measurement. Native outputs, other builds and unrelated changes were preserved.

AP also reported failures for unrelated inherited assets during its scan; the complete captured stdout is retained. This is a focal prefab/scene acceptance, not a clean full-asset audit. Existing dark lighting and DX12/Vulkan brightness differences remain visible. No Beauty, frame-time percentile, performance improvement, Linux or headset/VR acceptance is claimed. Editor closed through the UI; AP/children and launchers were explicitly terminated after evidence capture, so their graceful shutdown remains unverified.

The separate White Box UV warning was reproduced without resetting authored UVs; source preservation passed, but geometry edits needing reprojection remain blocked. No UV fix is included here.

`evidence-manifest.json` pins the copied receipts, logs, XML and captures. The full local working evidence is under `build/n0-validation/native-prefab-builder-rev1`.
