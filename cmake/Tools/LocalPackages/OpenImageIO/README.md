# Local native OpenImageIO/OpenColorIO provider

The Windows x64 package preserves `3rdParty::OpenImageIO`, `3rdParty::OpenImageIO_Util`,
`3rdParty::OpenColorIO`, OCIO Runtime and the two native Tools::Binaries targets,
with original headers, Debug/Release DLL names and six OIIO/eight OCIO Release
executables. GradientSignal and CommonFeatures select these for Editor/host tools.
Python bindings/Plugins are excluded; pystring is native C++. No Runtime use is
claimed. This JE identity does not match the historical rev4 archive/hash.

Nine immutable source snapshots/archives and six existing native package roots
are fixed in the separately hash-pinned `SourceInputs.cmake` witness. Every
regular member, size, archive SHA and exact snapshot membership is checked before
output creation. Boost is the official full 1.76.0 source release under BSL-1.0.
OIIO's 15 approved test links are not materialized, and JPEG's seven upstream
export-ignore paths remain absent. The producer copies only verified regular
files to new private work. It never runs a downloader, Git, installer or updater.

Obtain the witness digest with `Get-FileHash -Algorithm SHA256 -LiteralPath` on
the `JE_OIIO_INPUTS` file and compare it with the parent-audited witness SHA.
The example below pins that existing witness; do not replace it with an unaudited digest.

Luna alone executes the parent-reviewed producer, once, with a fresh output:

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' `
  '-DJE_OIIO_INPUTS=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/oiio-rev1-preparation/input-witness/SourceInputs.cmake' `
  '-DJE_OIIO_INPUTS_SHA256=F411B122BCB84851715ED98E171F6FEE54A7477A3EDEB409AC525421D550A298' `
  '-DJE_OIIO_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/oiio-local-rev1' `
  '-DJE_OIIO_PARALLEL_JOBS=6' `
  -P 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/OpenImageIO/BuildOpenImageIOPackage.cmake'
```

The producer privately initializes the existing v143 toolchain, bootstraps b2
from the audited source and builds the original Boost modules. It builds JPEG
Release/SIMD with the hash-pinned local NASM, then YAML/pystring/lcms2, OCIO native,
OIIO native and a second OCIO pass for OIIO-dependent apps. Original stacktrace
and rpath edits have exact pre/post hashes. CMake minimum compatibility 3.5 is
confined to children; compiler/runtime are x64 v143 MD/MDd. Upstream fixtures,
Python, documentation and optional historically excluded dependencies are OFF.
`OIIO_BUILD_TESTS=OFF` and `OCIO_INSTALL_EXT_PACKAGES=NONE` are the real upstream
options. Debug YAML imports are explicitly verified against the private MDd file.

Per-phase logs retain argv, stdout/stderr, result and UTC start/end. The parent
harness records the producer CMake PID; execute_process does not expose child
PIDs. Failure preserves private work/logs and stages no package. Only after all
native phases succeed is a fresh selective stage passed to JECreateLocalPackage:
`packages/openimageio-opencolorio-2.3.17-je-rev1-windows.tar.xz`,
`.tar.xz.SHA256SUMS` and `.files`. Partial native files are not an accepted
package. Full source and embedded codec license notices are retained.

After audit and new extraction of the real archive, configure `tests` with
VS2022/x64/v143, `JE_OIIO_PACKAGE_ROOT` pointing to that extraction and the same
`JE_OIIO_INPUTS` witness. Build/CTest Release and Debug separately. Nine cases per
config cover GradientSignal's R8/TIFF, R16/PNG, R32/EXR and RGBA/PNG publication,
JPEG quality100 RGB compression/decompression (maximum error4/mean1), OCIO matrix
and inverse/config serialization, malformed image and color requests with
unmodified outputs, actual oiiotool/iconvert/idiff/maketx outputs/mip readback,
and ociocheck of an owned serialized config. Tool subprocesses are real, not
symbol-only tests. Tests require fresh owned output paths; parent freezes inputs.

Producer negatives proposed for later Luna dispatch: missing/changed source or
witness, URI/UNC/link ancestors, reused/out-of-build output, missing/changed native
tools/packages, jobs missing/zero/noninteger and patch mismatch (PATCH-HELPER).
Native producer rev4, configuration repack rev7, archive data audit rev5 and
all nine consumer cases in each of Release/Debug passed. Parent integrated the
rev7 archive (SHA256 268ecbe940f16f729248679b586542e0154195eb66ae7ac29bccc409e5cf6611)
into the Windows catalogue and local Root cache. The earlier readback access
violation is retained as not reproduced; no causal repair is asserted. Source
snapshot negatives, general CRT/ABI (JPEG uses Release MD), product execution,
shipping separation and legal/distribution acceptance remain open. These focal
results do not certify Editor/AP/runtime or dependency size reductions.
