# Local Expat 2.4.2 Windows package

Rebuild the original Windows 2.4.2-rev2 interface from the audited tag commit,
using installed VS2022 x64 v143. Static Release `/MD` supplies all consumer
configurations through `3rdParty::expat`, `expat::expat` and `EXPAT::EXPAT`.
The payload is `expat/include/{expat.h,expat_external.h,expat_config.h}` and
`expat/lib/libexpatMD.lib`, plus complete notices and package metadata.

The producer requires four explicit inputs and a new output beneath this
checkout's `build`. It validates local paths, the exact source URL/hash and
original recipe patch before creating output. It extracts only the audited
`libexpat-e26a263aa9182d1e4552cd615857f0f61df3ba42/expat/*` subtree, verifies
the pristine/patched CMake byte hashes, and builds only upstream `expat`.
Failed/new outputs remain available for inspection; no overwrite or download.

Commands for the assigned validation owner (not execution evidence):

```powershell
$repo = 'F:/JericoEngine/JericoEngine'
$output = "$repo/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1"
cmake '-DJE_EXPAT_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/expat-e26a263aa9182d1e4552cd615857f0f61df3ba42.tar.gz' '-DJE_EXPAT_SOURCE_SHA256=a6130cab18d9df124dbfe4045f57f8c2180811c35d37bc357a42cff61afa4057' '-DJE_EXPAT_SOURCE_URL=https://codeload.github.com/libexpat/libexpat/tar.gz/e26a263aa9182d1e4552cd615857f0f61df3ba42' "-DJE_EXPAT_OUTPUT=$output" -P "$repo/cmake/Tools/LocalPackages/Expat/BuildExpatPackage.cmake"
$package = "$repo/build/n0-validation/local-packages/expat-rev1-validation/extracted"
if (Test-Path -LiteralPath $package) { throw 'Consumer extraction must be new' }
New-Item -ItemType Directory -Path $package | Out-Null
Push-Location $package
try {
    cmake -E tar xJf "$output/packages/expat-2.4.2-je-rev1-windows.tar.xz"
    if ($LASTEXITCODE -ne 0) { throw 'Package extraction failed' }
} finally { Pop-Location }
$consumer = "$repo/build/n0-validation/local-packages/expat-rev1-validation/consumer"
cmake -S "$repo/cmake/Tools/LocalPackages/Expat/tests" -B $consumer -G 'Visual Studio 17 2022' -A x64 -T v143 "-DJE_EXPAT_PACKAGE_ROOT=$package"
cmake --build $consumer --config Release
ctest --test-dir $consumer -C Release --output-on-failure
cmake --build $consumer --config Debug
ctest --test-dir $consumer -C Debug --output-on-failure
```

Validate archive extraction/manifest too; `.contents` alone does not establish
archive integrity. Four native cases exercise actual streaming UTF-8/internal
entities, namespaces, malformed/external-entity rejection and reset recovery,
custom allocator ownership and parser-buffer APIs. Every case checks exact
runtime 2.4.2/features/ABI and that parser destruction frees custom allocations.
The external callback deliberately rejects input without opening any file/URL.

Focal producer refusals: remote/UNC or relative source/output, missing archive,
wrong hash/URL, output outside build, existing output; all must fail before
configure/build/output creation. Package lookup must fail for missing header
or library, version mismatch and remote/UNC package roots. The authoring
baseline is not a security upgrade, historical binary match or Editor PASS.

Executed validation: producer exit 0 in 55.096 s; archive 196,028 bytes,
SHA256 `c102370257ed28d9f9ea5e61ca10432edc14e8389aa781380beaa9b2a8ae6925`.
Fresh extraction verified 38 members and 37 payload hashes, with no links.
Native consumers passed Release 4/4 and Debug 4/4. Eight guards rejected with
their expected diagnostics; outside-build used a preexisting source directory,
so the original runner failed its output-existence classification. That capture
limitation is preserved, with no guard retry; symlink validation was not run.
[Actual logs and results](../../../../docs/evidence/n0/local-packages/expat-rev1-validation/expat-final-validation-summary.json)
do not prove CrySystem, Editor or runtime. The original freeze14 remains a
historical test-input record; this paragraph is a later documentation-only change.
