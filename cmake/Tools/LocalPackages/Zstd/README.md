The new `zstd-1.3.5-je-rev1-windows` package builds the audited v1.3.5 commit
archive locally with Visual Studio 2022, x64 v143 and Release `/MD`. It uses
upstream `build/cmake` without source patches. `CMAKE_POLICY_VERSION_MINIMUM=3.5`
is confined to this standalone producer; engine policies are unchanged.
Legacy support is enabled (upstream level4), multithread support is enabled,
and the complete static library retains dictionary and deprecated APIs.
Shared library, CLI programs, upstream test binaries and contrib tools are
disabled. There are no downloads or global installations.

The original recipe at `o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`
maps `1.35-rev1` to tag `v1.3.5`, but supports only Mac ARM64. Its Find module,
archive layout and erroneous `0BSD` metadata do not establish the old Windows
package ABI or feature flags. Upstream Windows VS2010 enables legacy4 and
multithread; its old CRT choices are not copied. The current engine explicitly
uses `/MD` for Profile, and its Release MSVC runtime default is the DLL CRT;
the producer fixes `MultiThreadedDLL` explicitly. Generated Release project
flags and the actual archive must still be checked during validation.

Luna, the assigned validation owner, runs the producer with four explicit inputs:

```powershell
cmake `
  -DJE_ZSTD_SOURCE_ARCHIVE:FILEPATH=F:/JericoEngine/Dependencies/archives/zstd-90ae50224d15e8dbcb9fa26b9be096366733db8e.tar.gz `
  -DJE_ZSTD_SOURCE_SHA256=dfb594dc41e79c1304ef2c9232746c048e4837578c1ca88cf0e9b83c8cc36457 `
  -DJE_ZSTD_SOURCE_URL=https://codeload.github.com/facebook/zstd/tar.gz/90ae50224d15e8dbcb9fa26b9be096366733db8e `
  -DJE_ZSTD_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zstd-producer-rev1 `
  -P cmake/Tools/LocalPackages/Zstd/BuildZstdPackage.cmake
```

Output must be new and under this checkout's `build/`. The immutable snapshot
is never a work directory. The archive SHA is verified before extraction into
new source work. The shared local-path guard rejects URI/UNC/symlink ancestry;
the shared package helper records source provenance and the new archive SHA.
Payload layout is `Findzstd.cmake`, `JERequireLocalPath.cmake`, `zstd/include/`
and `zstd/lib/release/zstd_static.lib`, plus notices/provenance/licences.
The Find module defines the actual engine target `3rdParty::zstd` and supports
an exact 1.3.5 request without searching system locations.

Extract and audit the generated `.tar.xz` into a new local directory, then
configure `tests/` with `JE_ZSTD_PACKAGE_ROOT` pointing at that extracted root
and a new build directory under this checkout's `build/`. Build Release and
run CTest Release. The consumer checks header and linked library version,
basic byte-exact compression, bounded destinations, corrupt/truncated frames,
advanced custom allocator streams, partial buffers, flush/finish, stream reuse
through the 1.3.5 reset APIs, allocation ownership and allocation failure.
Guard negatives must reject wrong SHA/URL, existing output, output outside
checkout/build, UNC/URI paths and symlink ancestry before producer execution.

These direct package tests do not link AzCore or validate its current methods:
`AzCore/Compression/zstd_compression.cpp` currently has incomplete compression
and header/reset methods. Root resolution, engine compilation and productive
Editor/runtime behavior remain separate acceptance gates. The historical
source's security debt is explicit in `NOTICE.txt`; no automatic upgrade occurs.
