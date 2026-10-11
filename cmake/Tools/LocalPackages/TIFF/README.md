# TIFF local Windows candidate

This reproduces the Windows `tiff-4.2.0.15-rev3` recipe from
`o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`.
The official `v4.2.0` tag resolves to commit
`4862b0d7bcc786304ff4e8c31e8d5ccfb868fb99`; the upstream version remains
**4.2.0**, including `TIFFLIB_VERSION=20201219` and `TIFFGetVersion()`.
`4.2.0.15` is the original O3DE package/Find identity.

`BuildTiffPackage.cmake` accepts only an explicitly supplied, locally audited
archive, the exact JE zlib 1.2.11 package and a fresh output below this
checkout's `build/`. It checks URI/UNC/link ancestors, the source archive hash,
the original patch hash and four zlib input hashes before creating outputs.
It extracts to private work; the immutable snapshot is preserved. The exact
two original O3DE patch hunks change the CMake policy baseline to 3.5 and
prefer `long long` for 64-bit integers; full pre/post hashes guard the patch.

The build uses VS2022 x64/v143, static `tiff`, Release and the DLL CRT `/MD`
(upstream VS default and the engine Release policy, made explicit). It builds
only the original `tiff` target and manually stages the original four public
headers and `tiff.lib`. It performs no network acquisition, global install,
Git command or interpreter embedding. Original internal codecs remain on:
CCITT, PackBits, LZW, ThunderScan, NeXT, LogLuv and MDI. The original Windows
recipe already disables JPEG, old JPEG, PixarLog, LZMA, WebP, JBIG, Zstd,
JPEG12, libdeflate and the C++ stream wrapper; zlib/Deflate remains enabled.
No additional codecs are removed. Upstream always configures tools/tests/
contrib/docs; they are not built or installed by this library-only recipe.

The new identity is `tiff-4.2.0.15-je-rev1-windows`, using the shared local
package helper and its measured archive SHA. The historical package SHA
`c6000a906e6d2a0816b652e93dfbeab41c9ed73cdd5a613acd53e553d0510b60`
is not the source SHA or the new package SHA. Original `TIFF::TIFF`,
`3rdParty::TIFF`, TIFF include/library/version variables and the interface
link to `ZLIB::ZLIB` are retained. Engine use preserves its associated-package
hook; standalone use requires an explicit local zlib root when the target is
missing. `TIFF_CXX_FOUND` remains false. Full notices are listed in NOTICE.txt.

Only Luna runs the following validation after parent review/freeze. Paths
refer to the real audited inputs; output directories must not already exist.

```powershell
cmake -DJE_TIFF_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/tiff-audit/libtiff-4862b0d7bcc786304ff4e8c31e8d5ccfb868fb99.tar.gz -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents -DJE_TIFF_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/tiff-rev1 -P cmake/Tools/LocalPackages/TIFF/BuildTiffPackage.cmake
cmake -S cmake/Tools/LocalPackages/TIFF/tests -B build/n0-validation/local-packages/tiff-consumer-rev1 -G "Visual Studio 17 2022" -A x64 -T v143 -DJE_TIFF_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/tiff-rev1/packages/tiff-4.2.0.15-je-rev1-windows.contents -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents
cmake --build build/n0-validation/local-packages/tiff-consumer-rev1 --config Release --target JE_TiffConsumer
ctest --test-dir build/n0-validation/local-packages/tiff-consumer-rev1 -C Release --output-on-failure
```

Seven consumer cases cover exact linked/header versions, original codec
closure, RGB scanline/strip roundtrips (none/PackBits/LZW/Deflate, including
BigTIFF), Deflate tiled pixels, bit-exact IEEE float scanlines, callback
errors for truncated/corrupt headers and an invalid row, handle/storage
ownership, and actual `TIFFOpen` Windows file I/O/cleanup. These exercise
APIs used by Atom frame capture and ImageProcessingAtom TIFFLoader; they do
not link those engine targets or prove Editor, Asset Processor or runtime.
`/W4 /WX` applies to this C++ consumer; dependency headers are SYSTEM.

Required negative guards before acceptance: URI/UNC output; output outside
checkout/build; an existing output; missing archive; changed archive bytes;
missing/wrong zlib input hash; and an EXACT Find version other than 4.2.0.15.
Each guard must reject without configuring/building native tools or mutating
inputs; invalid harness invocations do not count as passing guard evidence.
Native validation completed with producer exit 0 in 106.736 seconds. The
produced archive is 452,948 bytes, SHA256
`d54ecd9a2463472e0df48e9baff07744ca1512fe761bc142ade353b347bf1dd2`.
Fresh extraction verified 90 members, 89 hashed payloads and no links.
The actual Release consumer passed all seven cases, using `/MD` and the
extracted `tiff.lib` plus the audited zlib static library. All 663 frozen
inputs and four zlib inputs remained intact through validation.

Seven producer guards and the wrong EXACT version lookup rejected as
expected. An initial missing-archive invocation retained an existing
output, so its diagnostic tested the wrong condition; it is preserved and
excluded. The corrected invocation used a fresh output and rejected the
missing archive. An initial consumer harness failed before CMake on its
path comparison; the corrected literal vector ran once. Neither harness
failure is counted as a native failure or a passing test. Raw evidence:
`docs/evidence/n0/local-packages/tiff-rev1-validation/`.
Engine consumer compilation and Editor/runtime acceptance remain pending.

This historical libtiff/zlib baseline retains known security maintenance
debt. No new-version security or ABI compatibility claim is made; upgrading
is a separate reviewed change. Library tests do not authorize untrusted
asset processing or establish product acceptance.
