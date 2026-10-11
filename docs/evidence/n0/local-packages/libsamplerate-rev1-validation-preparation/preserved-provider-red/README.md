This candidate builds the original libsamplerate 0.2.1 C API, statically, from
commit `a147b6dc4046deda22b2aa772b348717de772e2b`. Native validation is NOT RUN.
The current Windows caller is `Gem::Microphone.Static`:
`MicrophoneSystemComponent_Windows.cpp` uses short/float conversions,
`src_new(SRC_SINC_MEDIUM_QUALITY)`, streaming `src_process`, and `src_delete`.
Configuring that Gem does not prove game linking, microphone/device behavior,
or Editor/AssetProcessor acceptance.

The original O3DE recipe at `70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`
selects Windows Release, target `samplerate`, using VS2019. This new JE artifact
uses VS2022/v143/x64 and explicit Release `/MD`, retains C99 and all FAST,
MEDIUM, BEST, linear and zero-order converters. Shared library, tests, examples
and install are OFF. SndFile and its auxiliary codec discovery are confined
upstream to the disabled tests/examples branch; the library has no additional
Windows native link dependencies. No feature or source patch is introduced.
The upstream processor-clipping configure probes remain original.

The source URL is the official codeload archive at the exact commit. Audited
archive SHA256 is `f90e46bbf9bae05139e297ceebd5394ef77ce5e294e6b46beffcc15b9446a041`,
3,954,416 bytes. The 99-file archive has no links/path collisions. 98 files
match raw Git blobs. `Win32/libsamplerate-0.def` differs only by 30 CRLF/LF
pairs, attributed to committed `.gitattributes` (`*.def text eol=crlf`), with
the primary raw response and decoded API blob byte-identical. All snapshot and
build input bytes remain as downloaded; comparison does not rewrite sources.
That `.def` is used only for shared Windows builds, which this recipe does not
select. Full BSD2 COPYING and compiled-library/header notices are shipped.
The upstream GPL2-or-later `src/check_asm.sh` helper is neither run nor shipped.

Original Find contract/layout is preserved:
`3rdParty::libsamplerate` INTERFACE IMPORTED GLOBAL,
`libsamplerate/include/samplerate.h`, `libsamplerate/lib/samplerate.lib`, no
extra compile definitions/link options. New identity:
`libsamplerate-0.2.1-je-rev1-windows`; its actual archive hash is pending Luna.
The legacy `0.2.1-rev2` package hash and ABI are not claimed for this artifact.

Producer command (Luna only, new output, no retries or global install):

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' '-DJE_SAMPLERATE_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz' '-DJE_SAMPLERATE_SOURCE_MANIFEST=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-audit/source-manifest.json' '-DJE_SAMPLERATE_OUTPUT=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-build' -P 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/LibSamplerate/BuildLibSampleratePackage.cmake'
```

After independent archive audit and fresh extraction, set the package root
to that extracted SDK, then configure/build/test this consumer (Luna only):

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' -S 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/LibSamplerate/tests' -B 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-consumer' -G 'Visual Studio 17 2022' -A x64 -T v143 '-DJE_SAMPLERATE_PACKAGE_ROOT=<absolute audited extracted SDK>'
& 'C:/Program Files/CMake/bin/cmake.exe' --build 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-consumer' --config Release
& 'C:/Program Files/CMake/bin/ctest.exe' --test-dir 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-consumer' -C Release --output-on-failure
```

Repeat only the build/test commands with Debug after Release. The consumer is
`/MD` in Release and `/MDd` in Debug, while the archive is Release `/MD` only,
matching the original single-configuration recipe. Debug is a bounded consumer
check; possible CRT linker diagnostics must be retained and general mixed-CRT
compatibility is not certified. The library owns/frees its opaque states; the
consumer owns its sample buffers. No allocator crosses those owners.

Seven cases check linked version/all converters, conversions and saturation,
streaming/partial consumption/end-of-input/flush at three ratios for each sinc
quality, mathematical frame duration and stereo sine timing/amplitude,
reset/replay, linear/ZOH and invalid converter/channels/ratio/data/overlap.
No microphone hardware, real-time budget or sound-quality ranking is claimed.
Negative producer guards require a specific diagnostic, nonzero native exit,
and no new output; generic command failure is not a passing guard.
