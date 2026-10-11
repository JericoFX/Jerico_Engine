# Local ISPCTexComp input candidate

NOT VERIFIED: producer, eight native consumers, guards and original Atom integration have not run. Luna owns native validation. This is a Windows x64 Release DLL/import-library package, not a new codec or renderer.

The Windows recipe at o3de/3p-package-source `70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`, `package-system/ISPCTexComp/build_package_image.py`, pins encoder `36b80aac50ea401a9adde44d86712c2241b2acfd` and external ISPC 1.16.1. Its older upstream README mentioning 1.9.2 does not replace that recipe. The current catalog uses `ISPCTexComp-36b80aa-rev1-windows`; the new artifact is `ISPCTexComp-36b80aa-je-rev1-windows`, with a new measured package SHA after validation. No inherited package hash or historical binary ABI is claimed.

The original `ispc_texcomp.vcxproj` builds unchanged with explicit v143/14.44.35207 and SDK 10.0.26100.0 overrides. Its Release dynamic-library configuration uses `/MD` through audited `Microsoft.Cl.Common.props:103`. User property sheets and Directory.Build overrides are isolated. Original `-O2 --target=sse2,sse4,avx,avx2 --opt=fast-math` kernels and dispatcher remain. The immutable source/tool manifests and archive hashes are checked before creating output or invoking either compiler. Only a new private work copy receives generated files. No upstream patch, Python invocation, installation, PATH change outside the child process, or acquisition occurs.

The official source archive SHA256 is locally measured `029373e6ec5b10d7e29eefdc5a231fb86c79b76a4a39c58c2727c6729de28ab7` (6,435,298 bytes). All 238 entries were compared to pinned primary blobs: 72 byte-exact, 166 only CRLF versus LF under committed DXUT/DirectXTex attributes. The eight library files are byte-exact. Extraction retains archive bytes; no blanket normalization or export-cause claim. ISPC ZIP SHA256 `b34de2c36aff2afaa56b669ea41f9e614a045564ca74fc0b138e17ccea4880b7` (44,708,214 bytes) is a local measurement; no official asset digest was published. Compiler provenance/notices are retained, but neither ispc.exe nor ispcrt.dll enters the product payload.

Original caller: `Gem::ImageProcessingAtom.Editor.Static`, `Source/Compressors/ISPCTextureCompressor.cpp`, includes `ISPC/ispc_texcomp.h` and links `3rdParty::ISPCTexComp`. Atom uses BC3, unsigned half HDR BC6H and RGB/alpha BC7; it also registers ASTC profiles but deliberately routes ASTC encoding elsewhere. The package preserves all 26 original exports, including BC1/ETC1/ASTC. Find preserves the original include/bin layout, interface target and runtime DLL deployment hook.

Native consumer cases check all exports and the loaded DLL path, BC1/BC3 constant color and BC3 alpha via a test-only block oracle, half HDR BC6H modes, all BC7 profiles/mode prefixes, ETC1 and all ASTC profiles at 4/6/8 square blocks, reproducibility, raster block order, stride, bounds, source ownership and 32/64-bit border replication. ETC1/ASTC/BC6H/BC7 checks do not certify decoded image quality. Public compression APIs return void and require valid block-aligned inputs/buffers; invalid pointers/dimensions are not error-return APIs and are not invoked. Negative checks belong to real producer path/hash/manifest/output guards. Public host dispatch is exercised; ISA forcing and cross-machine dispatch coverage remain pending.

Commands for the validation owner, fresh directories only:

```powershell
$engine = 'F:/JericoEngine/JericoEngine'
$audit = "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-audit"
$output = "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-build"
cmake '-DJE_ISPC_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd.tar.gz' `
  '-DJE_ISPC_SOURCE_ROOT=F:/JericoEngine/Dependencies/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd' `
  "-DJE_ISPC_SOURCE_MANIFEST=$audit/source-immutable-manifest.json" `
  '-DJE_ISPC_TOOL_ARCHIVE=F:/JericoEngine/Dependencies/archives/ispc-v1.16.1-windows.zip' `
  '-DJE_ISPC_TOOL_ROOT=F:/JericoEngine/Dependencies/tools/ispc-v1.16.1-windows' `
  "-DJE_ISPC_TOOL_MANIFEST=$audit/tool-immutable-manifest.json" `
  '-DJE_ISPC_VS_ROOT=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools' `
  '-DJE_ISPC_POWERSHELL=C:/Users/Jerico/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe' `
  "-DJE_ISPC_OUTPUT=$output" -P "$engine/cmake/Tools/LocalPackages/ISPCTexComp/BuildISPCTexCompPackage.cmake"
cmake -S "$engine/cmake/Tools/LocalPackages/ISPCTexComp/tests" `
  -B "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-consumer" `
  -G 'Visual Studio 17 2022' -A x64 `
  "-DJE_ISPC_PACKAGE_ROOT=$output/packages/ISPCTexComp-36b80aa-je-rev1-windows.contents"
cmake --build "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-consumer" --config Release
ctest --test-dir "$engine/build/n0-validation/local-packages/ispc-texcomp-rev1-consumer" -C Release --output-on-failure
```

Repeat the consumer build/tests with Debug separately if assigned; the SDK DLL remains Release `/MD` and this does not certify general mixed-CRT compatibility. Atom/editor/AP integration, actual asset cooking, runtime rendering and performance are pending. The pinned legacy tool/library versions retain security/maintenance debt; this provider does not silently upgrade them.
