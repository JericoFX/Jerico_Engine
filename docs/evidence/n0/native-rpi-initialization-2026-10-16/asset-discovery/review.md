# RPI98 asset discovery: configuration-only diagnosis

Status: READ_ONLY / proposal not applied / native NOT_RUN. Parent alone chooses the configuration seam.

## Demonstrated comparison

`activation-plan-comparison.json` pins the 22 Profile plans in accepted SDK96 and the 22 current original-engine plans. All 22 baseline plans contain `/O3DE/Gems/Atom_RPI`; all 22 current plans omit it. Twenty baseline entries select `Atom_RPI.Editor`; the two GameLauncher entries select `Atom_RPI.Private`. The physical SceneAndViewSrgs.shader and .azsl are still byteexact to the current AP diagnosis. No compiler-failure conclusion follows from AssetStatus_Missing.

`cmake/SettingsRegistry.cmake`125–143 traverses GEM_MODULE dependencies;176–231 maps these into the active Gem objects;308–310 generates the plans. Native factories correctly replace module activation, while Atom_RPI.Public is a SHARED target without GEM_MODULE. Consequently linking Public does not replace the removed discovery metadata.

The W1 shared graph packet establishes the consumer chain: GemInfo.cpp27–67 enumerates only ActiveGemsRootKey, then obtains each visited name's path from ManifestGemsRootKey. A manifest path by itself therefore cannot create an RPI GemInfo. PlatformConfiguration.cpp771/778 calls GetGemsInfo then AddGemScanFolders. Its1923 derives the identity from CreateName("Atom_RPI0");1940–1955 adds the Assets recursive folder, and1957–1973 also adds Registry. Removing the active entry removes both automatic registrations.

The actual AP_GUI receipt records the nine SceneAndViewSrgs Vulkan products deleted at23:14:25Z with the native source-removed-or-builder-did-not-emit alternative; several RPI .azsli paths are outside input folders. These observations agree with the discovery loss. We have not captured the effective AP settings registry itself, so the plan-to-consumer chain and AP symptoms are evidence, not an invented measurement of its live folder list.

## Minimal configuration proposal for parent review

Reuse the existing native `Registry/render_integration_assetprocessor.setreg` / `shader_assetprocessor.setreg` pattern: explicitly register the existing RPI Assets root recursively with its historical `gemassets-{UUID}` portable key and relative product paths. Do not restore Private/Editor targets, a dynamic module, a Gem alias or an additional cooker. Obtain the exact historical UUID from existing ScanFolders evidence, or verified native CreateName("Atom_RPI0"); no invented UUID is supplied here. Preserve the original assets in place for this bounded fix. This is one configuration change and does not require changing C++ or DLLs.

This Assets-only proposal has an explicit boundary: AddGemScanFolders also registered Registry, and runtime MergeSettingsToRegistry_GemRegistries1186–1208 uses VisitActiveGems. Merely leaving `atom_rpi.release.setreg` and `asset_browser.setreg` byteexact in a nonvisited Gem directory does not prove their overrides remain applied. Root/W1 must select the matching native registry/alias discovery seam before accepting the complete configuration fix. An alternative metadata-only `/O3DE/Gems/Atom_RPI` object without Targets/Modules would preserve current GemInfo consumers, but is a distinct owner decision, not an implemented recommendation or a reintroduced factory. Shader include/alias consumers are under W1's focused trace; a scoped zero GetGemsInfo hit alone does not prove they have no dependence.

## Validation boundary

Keep R4 C++/DLLs and the actual588/588 evidence unchanged. After approved configuration edits, review only affected generated metadata/effective AP discovery; run one newly evidenced focal recook with current RunKeys/products/identities/backend and then the parent-owned Editor check. Do not rebuild11 hosts or full-replay SDK96. SDK98r2 remains NOT_RUN; update its freeze/metadata binding only when parent authorizes. Do not reinterpret raw AP failures or cached runtime success as this gate passing.

No source/config/Git/build/AP/GUI changes were made. W2 used complete unchanged cpp-pro/game-developer coverage and W1's shared Graft results; no graph process or persistent helper was started. Physical RAM at closure44.16%, free17.82GiB. Root resource/process ownership remains authoritative.
