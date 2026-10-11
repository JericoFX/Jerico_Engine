# PNG local Windows candidate

This reproduces the Windows `png-1.6.37-rev2` recipe from
`o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`.
The original SourceForge `v1.6.37` tag and official GitHub `pnggroup/libpng`
tag both resolve to `a40189cf881e9f0db80511c382292a5604c3c3d1`.
The manually acquired official commit archive has SHA256
`ed747514984fddf095142aa85e983111f13d64b877b1065ed0d990f7adb31eb7`.
Its immutable snapshot is preserved; the producer extracts a fresh private
work copy from the exact audited archive.

`BuildPngPackage.cmake` accepts explicit local archive/zlib paths and a fresh
output below checkout/build. Shared local path guards reject URI, UNC and
link ancestors. Archive and four JE zlib input hashes are checked before
outputs or native tools. There is no download, installation, interpreter
embedding, Git, source patch or new dependency.

The original feature choices remain `PNG_SHARED/TESTS/DEBUG=OFF`,
`PNG_STATIC=ON`, PIC, C++17, `BUILD_SHARED_LIBS=OFF`, `PNG_ARM_NEON=on`
(the last applies only to ARM upstream). The new toolchain is VS2022 x64/v143,
Release `/MD`, preserving the upstream VS DLL CRT default and engine Release
policy. The minimum policy compatibility is raised from upstream 3.1 to 3.5
only in the producer's child configure, without editing source or engine
policy. Only `png_static` is built, with no global install.

The explicit `AWK=AWK-NOTFOUND` selects upstream's ordinary Windows fallback
`scripts/pnglibconf.h.prebuilt`, avoiding ambient Git/Unix tool discovery.
That supplied header preserves the full read/write/transform API. The recipe
verifies its SHA, compares generated/header text with newline normalization,
and stages the actual generated header bytes unchanged. Hardware optimization
defaults are retained. Consumer capability macros and real APIs verify this
choice; no custom stripped header is generated.

The new package identity is `png-1.6.37-je-rev1-windows` with a new SHA measured
by the shared package helper. The historical package hash
`e16539a0fff26ac9ef80dd11ef0103eca91745519eacd41d41d96911c173589f`
is not reused as a source or new package hash. Original `PNG::PNG` and
`3rdParty::PNG`, include/lib/version variables, `IMPORTED_LINK_INTERFACE_LANGUAGES=C`
and interface `ZLIB::ZLIB` are retained. The engine's associated-package
hook is preserved; standalone use requires explicit local zlib when its
imported target is absent. Layout remains `png/include/{png.h,pngconf.h,pnglibconf.h}`
and `png/lib/libpng16_static.lib`. Complete notices are described in NOTICE.txt.

Only Luna executes validation after parent review and source freeze. Fresh
outputs are required; retain failed attempts rather than deleting them.

```powershell
cmake -DJE_PNG_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/png-audit/libpng-a40189cf881e9f0db80511c382292a5604c3c3d1.tar.gz -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents -DJE_PNG_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/png-rev1 -P cmake/Tools/LocalPackages/PNG/BuildPngPackage.cmake
cmake -S cmake/Tools/LocalPackages/PNG/tests -B build/n0-validation/local-packages/png-consumer-rev1 -G "Visual Studio 17 2022" -A x64 -T v143 -DJE_PNG_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/png-rev1/packages/png-1.6.37-je-rev1-windows.contents -DJE_ZLIB_PACKAGE_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents
cmake --build build/n0-validation/local-packages/png-consumer-rev1 --config Release --target JE_PngConsumer
ctest --test-dir build/n0-validation/local-packages/png-consumer-rev1 -C Release --output-on-failure
cmake --build build/n0-validation/local-packages/png-consumer-rev1 --config Debug --target JE_PngConsumer
ctest --test-dir build/n0-validation/local-packages/png-consumer-rev1 -C Debug --output-on-failure
```

Five native cases in Release and Debug mirror the original recipe's two
consumer configurations: exact linked/header PNG/zlib versions; RGB/RGBA
memory encode/decode at compression levels 0/9; caller transforms strip16,
gray-to-RGB, alpha removal and packed gray2 expansion; Adam7 bit-exact pixels;
truncation/critical CRC error callbacks, cleanup and valid decode after failure.
Tracked caller allocators verify every PNG-owned allocation is released.
Longjmp helpers contain only primitive automatic variables; their bounded
stream/image storage and C++ ownership live outside the setjmp frame. `/W4 /WX`
remains enabled, with only the same focused C4611 suppression required in
native PngFile.cpp. Failed pixel publication is a consumer-harness oracle,
not a claim about uncompiled engine caller behavior.

Guard checks: URI/UNC output; output outside checkout/build; existing output;
missing/changed archive; missing/changed pinned zlib; wrong EXACT PNG version.
Each must reject without native configure/build or changing frozen inputs.
Structured arguments and freeze receipts are under the owned png-audit folder.

The selected consumer is `Gem::Atom_Utils.Static` and its real
`Gems/Atom/Utils/Code/Source/PngFile.cpp` read/write callbacks and transforms.
Library checks do not link that engine target or prove Editor, Asset Processor,
cook/image integration or runtime. Native producer exit 0 took 16.823 seconds;
the 421,588-byte archive SHA256 is
`7d13e9a70172f713f66daf870a7359191096308ebe501c17121bfb24dbefe9b4`.
Fresh extraction verified 51 members, 50 payload hashes and no links.
Release and Debug consumers passed five cases each; all 514 frozen inputs
and four pinned zlib inputs remained intact. Seven producer guards and the
wrong EXACT 1.6.38 lookup rejected as expected, preserving the existing
output sentinel and leaving the outside-build output absent.

Release links the `/MD` libraries without a linker warning. The `/MDd` Debug
consumer emitted LNK4098 for the Release `/MD` PNG/zlib libraries; its five
passing cases do not establish general compatibility across these CRTs.
No warning suppression or CRT change was used to obtain the result.
Upstream's header fallback converted 219 LF endings to CRLF in the generated
header; normalized text matches exactly, while the actual packaged byte hash
is retained. Raw evidence: `docs/evidence/n0/local-packages/png-rev1-validation/`.
Engine target compilation and Editor/runtime acceptance remain pending.
This historical libpng/zlib baseline retains security maintenance debt; no
silent upgrade, historical ABI equivalence or safety for untrusted assets is
asserted by this reconstruction.
