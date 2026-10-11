# OpenEXR local Windows candidate

The selected consumer is `Gem::ImageProcessingAtom.Editor.Static` and its
`Gems/Atom/Asset/ImageProcessingAtom/Code/Source/ImageLoader/ExrLoader.cpp`.
It uses scanline `InputFile`, channel/header/data-window inspection,
HALF/FLOAT `FrameBuffer`/`Slice`, default alpha 1, `readPixels`, and
`isOpenExrFile` with rejection of tiled/deep/multipart images. The native
candidate does not replace this loader or broaden its supported formats.

Original recipe: `o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`,
`package-system/OpenEXR`, Windows 3.1.3-rev5. Upstream OpenEXR `v3.1.3`
is commit `697cc449cc589ed780acda88e79e6145ddcd121f`; the recipe's upstream
`IMATH_TAG=v3.1.3` is commit `6dc0820482aeb24a22f520d7b165a410589648c3`.
Both tags were resolved through the official ASWF repositories. The manually
acquired codeload archives have SHA256 respectively
`5ed17a3be8ae029e26a7314a2d41e6febc2102cef1eafb1ffe6c0550ea1993be`
and `52aeb73f94911a4e107d0a2870ec98d7eb9d3907d25238b12e496c71a5fca4a3`.
Both complete archives were audited before immutable extraction, including
all paths, entry types, collisions, LFS pointers, lengths and file hashes.

The producer checks explicit local archive/zlib paths and exact hashes before
creating a fresh output beneath checkout/build. It extracts private writable
copies there; immutable snapshots are never build inputs for upstream writes.
No acquisition, Git, installer, global install or interpreter runs in the
producer. The shared path/package helpers are reused unchanged.

Original feature flags remain `BUILD_TESTING=OFF`, `OPENEXR_BUILD_EXAMPLES=OFF`,
`OPENEXR_INSTALL_EXAMPLES=OFF`, `OPENEXR_BUILD_TOOLS=OFF`,
`OPENEXR_INSTALL_TOOLS=OFF`, `OPENEXR_RUN_FUZZ_TESTS=OFF`,
`BUILD_SHARED_LIBS=OFF`, `DOCS=OFF`, `PYTHON=OFF`. Threading, all compressors,
half lookup tables, namespaces, library/API suffixes and other defaults remain.
Imath and OpenEXR each build/install Debug and Release into a private stage,
as in the original Windows recipe. VS2022 x64/v143 replaces VS2019.
CRT is `/MD` Release and `/MDd` Debug via CMP0091 and its explicit per-config
runtime property. The original JE zlib 1.2.11 archive is Release `/MD` only;
Debug's mixed zlib CRT must be reported from native validation rather than
treated as general CRT compatibility. No upstream source patch is applied.

OpenEXR v3.1.3's root includes `cmake/LibraryDefine.cmake`, which only
requires C++; the unused `OpenEXRLibraryDefine.cmake` contains the private
C11 requirement. MSVC 19.44.35228.0 supplies `stdatomic.h`, so upstream's
header-presence check enters C atomics even when C11 is not enabled. Luna's
isolated CMake 4.3.4 probe established two failures: default C mode requires
C11, and C11 alone leaves `__STDC_NO_ATOMICS__` defined. Explicit C11,
extensions off, and `/experimental:c11atomics` compiled and ran real atomic
init/store/load in both Debug `/MDd` and Release `/MD`. Receipt:
`build/n0-validation/local-packages/openexr-atomics-diagnostic/diagnostic-summary.json`,
SHA256 `9891E1044EF2C9A8CA5BFCE2737295E9D5F9D7C7C8173067F42D795BE765C2FD`.
The producer applies `CMAKE_C_STANDARD=11`, `CMAKE_C_STANDARD_REQUIRED=ON`,
`CMAKE_C_EXTENSIONS=OFF`, and `CMAKE_C_FLAGS=/experimental:c11atomics`
only to the OpenEXR child configure. Imath, engine flags, C++, features,
source pins and CRT remain unchanged. The experimental flag is a requirement
of this exact toolchain; the probe does not establish general compatibility
or OpenEXR package acceptance. Subsequent native package validation is recorded below.

`OfflineDependencies.cmake` is injected immediately after upstream
`project(OpenEXR)`. It requires exact local ZLIB and Imath targets before
`OpenEXRSetup.cmake` can enter its zlib ExternalProject or Imath FetchContent
fallback. Imath is first built and staged independently, preserving both
upstream export configurations. The OpenEXR child also sets
`FETCHCONTENT_FULLY_DISCONNECTED=ON`. Imath's Python subdirectory is gated
off. Documentation, tests, tools and example targets are gated off by the
same original recipe choices; full native library features stay present.
No engine CMake policies are reduced.

The new package is `OpenEXR-3.1.3-je-rev1-windows`, with a new measured hash.
Validation uses fresh `openexr-rev2`/`openexr-consumer-rev2` output directories.
The package identity stays rev1 because the first failed attempt produced no
package archive; all failed outputs and the original freeze are preserved.
The old catalog SHA
`bff6dc78412bb1b04ded243753bee36e9229fdaf9a9e1fa85b1059238fba4c9b`
is neither a source hash nor the new package hash. Payload layout remains
`OpenEXR/include/{OpenEXR,Imath}`, `OpenEXR/lib`, and upstream exported CMake
configurations. `FindImath` and `FindOpenEXR` preserve upstream component
targets, original `3rdParty` aliases, System includes, configuration mapping,
MSVC exception option correction and ZLIB link interface. They require the
exact staged 3.1.3 configs; the engine's existing associated-ZLIB hook is
retained. Standalone resolution needs an explicit local zlib path.
Full BSD licenses, contributor notices and DreamWorks patent grant/alternate
license are preserved, as detailed in NOTICE.txt.

Only Luna runs the following after parent review and source freeze. Use fresh
outputs and preserve failed attempts. Structured argv/counts and input freeze
are in `build/n0-validation/local-packages/openexr-audit`.

```powershell
cmake -DJE_OPENEXR_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/openexr-audit/openexr-697cc449cc589ed780acda88e79e6145ddcd121f.tar.gz -DJE_OPENEXR_IMATH_ARCHIVE:FILEPATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/openexr-audit/Imath-6dc0820482aeb24a22f520d7b165a410589648c3.tar.gz -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents -DJE_OPENEXR_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/openexr-rev2 -P cmake/Tools/LocalPackages/OpenEXR/BuildOpenEXRPackage.cmake
cmake -S cmake/Tools/LocalPackages/OpenEXR/tests -B build/n0-validation/local-packages/openexr-consumer-rev2 -G "Visual Studio 17 2022" -A x64 -T v143 -DJE_OPENEXR_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/openexr-rev2/packages/OpenEXR-3.1.3-je-rev1-windows.contents -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents
cmake --build build/n0-validation/local-packages/openexr-consumer-rev2 --config Release --target JE_OpenEXRConsumer
ctest --test-dir build/n0-validation/local-packages/openexr-consumer-rev2 -C Release --output-on-failure
cmake --build build/n0-validation/local-packages/openexr-consumer-rev2 --config Debug --target JE_OpenEXRConsumer
ctest --test-dir build/n0-validation/local-packages/openexr-consumer-rev2 -C Debug --output-on-failure
```

Six meaningful consumer cases in each configuration cover linked OpenEXRCore
version, Imath binary half representation/denormal conversion, zlib version,
HALF/FLOAT HDR pixels through five lossless codecs, missing alpha fill,
bounded memory callbacks, truncation/invalid-signature exceptions and recovery,
and a real native scanline file matching the selected loader's classification.
The failure-publication check is a harness oracle, not uncompiled engine
transaction/ownership acceptance. RAII keeps caller buffers alive until file
destruction; failed reads publish only after a complete successful decode.
`/W4 /WX` remains enabled for the consumer, with original dependency includes
marked System. Import checks require all original aliases and Debug/Release
static libraries inside this payload.

Guards to validate: URI/UNC output, output outside build, existing output,
missing/changed OpenEXR or Imath archive, missing/changed pinned zlib,
and wrong EXACT OpenEXR/Imath version. Frozen inputs must remain unchanged.

This candidate is prepared, not natively compiled or tested by the writer.
Library evidence cannot certify the engine loader, Editor, Asset Processor,
asset cooking or runtime. OpenEXR/Imath/zlib historical security maintenance
debt remains; there is no silent upgrade or claim of historical binary ABI
equivalence or safety for hostile assets. This selected dependency is not
asserted to be the latest root-configure fatal error.

Native validation after parent review: producer exit0/202.319s; archive
6,880,112 bytes, SHA256
`cd06701c7710fcaf68b29d99a1a4e110ea4c274b0656e1cc2182a1a7003f7fa0`.
Fresh archive extraction matched 584/584 payload hashes, 585 regular files,
12 static libraries and zero DLLs. Release and Debug builds each passed six
consumer CTests; ten guards reached their expected rejection diagnostics.
Debug retains LNK4098 for the documented Release-zlib CRT mix. The OpenEXR
wrong-EXACT guard compiled CMake's CompilerIdCXX, but no consumer/library target;
initial invalid harness attempts remain NOTPASS. Native freeze1182 and zlib4
matched before/after. This status-only README update follows that freeze;
producer, code, tests and archive bytes stay unchanged. Full raw receipt:
`docs/evidence/n0/local-packages/openexr-rev2-validation/validation-summary.json`.
The parent accepted this package scope and integrated its catalogue/cache;
engine, Editor, ImageProcessingAtom loader and graphical acceptance remain open.
