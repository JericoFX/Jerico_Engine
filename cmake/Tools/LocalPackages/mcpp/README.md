Local Windows x64 producer for the mcpp 2.7.2_az.2 interface used by Atom.
The source archive and original O3DE patch are pinned; only a newly extracted
work copy under this checkout's `build` directory is modified. Acquisition,
Git, upstream configure scripts and global installation are absent from the producer.

The producer preserves the original DLL/import library and CLI layout. The shared
JE package helper must support `JE_PACKAGE_SOURCE_RELEASE`; no upstream commit is
invented for this historical release. On Windows x64, the assigned native producer
and consumers passed: four CTests in Release and four in Debug, plus eight producer
refusal cases. The measured JE archive SHA256 is
`4c4ae3b71be82837b18090029d7ec49a19af80c12b35008b70a0d6f967acc589`.
See `docs/evidence/n0/local-packages/mcpp-rev2-validation/` for the actual commands
and logs; the earlier capture with empty arguments is NOT_RUN. This focal result
does not establish a working ShaderBuilder or Editor.

Producer command template (use a new absolute output each time):

```powershell
cmake -DJE_MCPP_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz `
  -DJE_MCPP_SOURCE_SHA256=3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864 `
  -DJE_MCPP_SOURCE_URL=https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download `
  -DJE_MCPP_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-local-v1 `
  -P cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake
```

After hashing and extracting the produced archive to a separate local directory:

```powershell
cmake -S cmake/Tools/LocalPackages/mcpp/tests `
  -B F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 `
  -G "Visual Studio 17 2022" -A x64 -T v143 `
  -DJE_MCPP_PACKAGE_ROOT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-extracted-v1
cmake --build F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 --config Release
ctest --test-dir F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 -C Release --output-on-failure
cmake --build F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 --config Debug
ctest --test-dir F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/mcpp-consumer-v1 -C Debug --output-on-failure
```

The real consumer imports the DLL as Atom does and tests nested include callbacks,
macro expansion/stringizing/pasting, absolute line origins, the _az.2 comment-line
fix, invalid inputs and recovery without leaked macro state. The original CLI has
a separate preprocessing test. Producer refusals cover URI/UNC paths, missing or
incorrect source hash, different release URL, existing output and output outside
checkout/build. Symlink refusal remains unexecuted; no privilege changes were made.
These gates do not prove ShaderBuilder or Editor
execution, complete legal redistribution clearance, or engine performance.

## Pre-open include resolver: package validated for CoreFeature102 R35

The `4c4ae3...` archive and validation above describe the historical package,
not the pre-open ABI. The package actually validated for the new reader is:

- Name: `mcpp-2.7.2_az.2-je-preopen-r32-windows`.
- Archive SHA256: `ace0ea137a8056cf9a0c41820e13f413644a5fda0618922eb86542ef0e9c19f3`.
- ABI version 1: `mcpp_set_resolve_include_callback`, called after include macro
  expansion and before file search/open. The existing include-report callback
  still reports the file opened. No JE path policy is implemented inside mcpp.

`McppPreopenExtensionManifest.json` and `mcpp_preopen_read.patch` contain the
complete extra C/header ABI changes to `src/mcpp_lib.h`, `src/internal.H`,
`src/support.c`, `src/mcpp_lib.def` and `src/system.c`. The producer applies the
unchanged historical `_az.2` patch first, then this separately pinned extension.
Do not substitute a generated header for the source patch or mix header,
import library and DLL from different packages. `Findmcpp.cmake` requires
`JEPreopenAbi.json` version 1 and verifies all three artifact hashes; the old
package cannot satisfy the new reader's unconditional callback API call.

The original source remains mcpp 2.7.2 at the SourceForge URL and archive hash
in the producer command below. The URL is provenance, not an automatic download.
The source release has no established upstream Git commit. The original
BSD-2-Clause license and contributor notices, the historical O3DE adaptation,
and the Apache-2.0 OR MIT modification notices remain applicable. The producer
retains the original license/readme/source notices and both patches in its
package; see `NOTICE.txt`, `mcpp/LICENSE` and `LICENSES/` in the resulting package.

### Rebuild locally from the pinned original source

Supply the original archive locally through an allowed source and verify its
SHA256; the producer neither acquires dependencies nor invokes git-lfs. This is
the audited command interface, with a new output beneath this checkout's `build`
directory (the example output must not already exist):

```powershell
cmake -DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz `
  -DJE_MCPP_SOURCE_SHA256:STRING=3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864 `
  -DJE_MCPP_SOURCE_URL:STRING=https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download `
  -DJE_MCPP_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/mcpp-preopen-rebuild-r37 `
  -P cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake
```

Use the corresponding absolute local paths on another checkout. Windows x64,
CMake 3.24 or newer and Visual Studio 2022 with v143 are required. The producer
configures and builds the existing `JEMcppDll` and `JEMcppCli` targets serially.
It writes the archive and its `.tar.xz.SHA256SUMS` under the new output's
`packages/` directory, alongside the `.contents` payload. That payload contains
`mcpp/JEPreopenAbi.json` with the measured header/import-library/DLL hashes.
The existing standalone consumer recipe under `tests/` accepts this `.contents`
directory through `JE_MCPP_PACKAGE_ROOT` as shown above; no new harness is needed.

### Select the exact package without bypassing its pins

The engine's Windows `BuiltInPackages_windows.cmake` associates `mcpp` with the
pre-open package name and the full `ace0ea...` archive hash above. The validated
local archive and checksum currently reside at:

```text
F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache/mcpp-2.7.2_az.2-je-preopen-r32-windows.tar.xz
F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache/mcpp-2.7.2_az.2-je-preopen-r32-windows.tar.xz.SHA256SUMS
```

That is a machine-local audit location, not a committed payload or a download
service. The current offline engine configuration uses that directory as
`LY_PACKAGE_DOWNLOAD_CACHE_LOCATION`, has empty `LY_PACKAGE_SERVER_URLS`, and
unpacks through the existing package resolver under `LY_PACKAGE_UNPACK_LOCATION`.
To consume the already validated bytes elsewhere, an approved artifact source
must supply that exact archive and checksum to the selected local download
cache. Preserve the association hash and let the existing resolver and ABI
checks validate and select it; do not overwrite an old package in place or mix
its artifacts with the new one. No remote hosting or availability is established
by this README.

A rebuild from the pinned original source is the alternative when those accepted
archive bytes are unavailable. Reproducing the accepted `ace0ea...` archive hash
on a different toolchain or date has not been established. If the new archive
hash differs, preserve it, its measured checksum and coherent ABI metadata for
explicit review before changing any package association or binding. Never
rename different bytes as the accepted archive, disable hash/ABI checks, or
present the historical `4c4ae3...` package as the new ABI. Another machine still
needs either the exact approved archive or an explicitly reviewed local rebuild
and its new pins; the private cache alone is not a reproducibility instruction.

These source recipes and package bindings do not establish remote availability,
Shipping support, clean-HEAD replay, or whole-Gem acceptance. Existing native
consumer and shader validation evidence is separate from those pending gates.
