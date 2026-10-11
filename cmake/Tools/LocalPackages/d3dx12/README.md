# Local original d3dx12 headers

Preserve the header-only Windows provider required by Atom RHI DX12:
`3rdParty::d3dx12` (`INTERFACE IMPORTED GLOBAL`), `d3dx12_INCLUDE_DIR` and
`d3dx12/include/d3dx12.h`. No new API, alias, DLL or SDK implementation.
The exact header source is the O3DE 3p archive at commit
`70f0b7b5257bf7cf935bf1cfd270e1b53ccead18`; its original metadata does not
identify a Microsoft commit/tag. `NOTICE.txt` preserves that distinction.

Four explicit producer inputs are required. Local absolute paths, source
URL/hash and a new output under this checkout's build are checked before
writing. Only the four audited files beneath the archive's literal
`package-system/d3dx12-windows/` subtree are extracted; each is hash checked.
The producer copies the unmodified header/license and uses JECreateLocalPackage.
It invokes no compiler, upstream CMake, download, installer or Git command.

Proposed commands for the assigned validation owner; not execution evidence:

```powershell
$repo = 'F:/JericoEngine/JericoEngine'
$output = "$repo/build/n0-validation/local-packages/d3dx12-rev1-validation/d3dx12-local-v1"
cmake '-DJE_D3DX12_SOURCE_ARCHIVE=F:/JericoEngine/Dependencies/archives/o3de-3p-70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.tar.gz' '-DJE_D3DX12_SOURCE_SHA256=f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042' '-DJE_D3DX12_SOURCE_URL=https://codeload.github.com/o3de/3p-package-source/tar.gz/70f0b7b5257bf7cf935bf1cfd270e1b53ccead18' "-DJE_D3DX12_OUTPUT=$output" -P "$repo/cmake/Tools/LocalPackages/d3dx12/BuildD3dx12Package.cmake"
if ($LASTEXITCODE -ne 0) { throw 'Producer failed' }
$package = "$repo/build/n0-validation/local-packages/d3dx12-rev1-validation/extracted"
if (Test-Path -LiteralPath $package) { throw 'Consumer extraction must be new' }
New-Item -ItemType Directory -Path $package | Out-Null
Push-Location $package
try {
    cmake -E tar xJf "$output/packages/d3dx12-headers-je-rev1-windows.tar.xz"
    if ($LASTEXITCODE -ne 0) { throw 'Package extraction failed' }
} finally { Pop-Location }
$consumer = "$repo/build/n0-validation/local-packages/d3dx12-rev1-validation/consumer"
cmake -S "$repo/cmake/Tools/LocalPackages/d3dx12/tests" -B $consumer -G 'Visual Studio 17 2022' -A x64 -T v143 "-DJE_D3DX12_PACKAGE_ROOT=$package"
cmake --build $consumer --config Release
ctest --test-dir $consumer -C Release --output-on-failure
cmake --build $consumer --config Debug
ctest --test-dir $consumer -C Debug --output-on-failure
```

The standalone C++ consumer uses the installed Windows SDK and nine actual
helper types used by Atom's DX12 Device/CommandList/Image/ImagePool/upload
consumers. It checks upload/default heap access, buffer/clear/range descriptors,
copy footprints/indices/boxes, boxed/packed tile regions and subresource
index/decomposition. No GPU device, resource, driver or render result is tested.
Verify the produced archive hash and its extracted SHA256SUMS separately.

Focal refusals before output creation: missing/remote/UNC/relative source,
wrong hash or URL, output outside checkout/build, existing output. Lookup must
reject missing/modified header and remote/UNC package roots before declaration.
Windows SDK availability/compatibility is tested only by the native consumer;
configuration or package creation alone is not DX12/Editor PASS.

Native validation: producer exit 0 in 0.268 seconds; package 16,652 bytes,
SHA256 `5ba66c64483f19e72beea0d1ccc94ad3e0175e88ec256668fc5b0725727ffb73`.
Fresh extraction verified 14 regular members, 13 payload hashes and no links;
header and license match the audited source byte for byte. SDK 10.0.26100.0,
VS 2022 v143 x64 consumers passed three cases in Release and Debug. All 12
frozen inputs remained unchanged through validation.

Six producer guards and three lookup guards rejected as expected. Three
initial matcher results were NOT_PASS because the diagnostic wrapped across
lines; the native rejections were correct. Additional identical native
checks were run before that harness issue was identified; their separate
records remain, and they add no necessary validation. Missing/tampered-header
lookup configurations perform CMake compiler discovery before Find rejects;
no product consumer executable is built by those guards.
Raw evidence: `docs/evidence/n0/local-packages/d3dx12-rev1-validation/`.
GPU, renderer and Editor acceptance remain pending.
