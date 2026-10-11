$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$engine = 'F:\JericoEngine\JericoEngine'
$provider = Join-Path $engine 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc'
$out = Join-Path $root 'provider-candidate-rev1'
if (Test-Path -LiteralPath $out) { throw 'Candidate evidence exists; preserve previous records' }
$oldPath = Join-Path $root 'source-freeze-dxc-acquired-final.json'
if ((Get-FileHash -LiteralPath $oldPath).Hash -ne '7333734E7669C1151616CBED86D2C81A2E1FE8278E3CC01C719D87788475C93B') { throw 'Approved historical freeze changed' }
$old = Get-Content -LiteralPath $oldPath -Raw | ConvertFrom-Json
if ($old.files.Count -ne 19828) { throw 'Approved input count changed' }
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
function Add-Input([string] $path, [string] $role) {
    $absolute = [IO.Path]::GetFullPath($path)
    if (-not $inputs.ContainsKey($absolute)) {
        $item = Get-Item -LiteralPath $absolute
        if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Not a regular local input: $absolute" }
        $inputs.Add($absolute, [ordered]@{ path = $absolute; bytes = $item.Length; sha256 = (Get-FileHash -LiteralPath $absolute).Hash.ToLowerInvariant(); role = $role })
    }
}
foreach ($file in $old.files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Unapproved historical delta: $($file.path)" }
    $inputs.Add([IO.Path]::GetFullPath($file.path), $file)
}
# Reuse only the reviewed external CPython tool tree, not any VVL/product input.
$pythonRoot = Join-Path $engine 'build/n0-validation/original-engine-deps/Python-rev3/packages/python-3.10.13-je-rev3-windows/python'
$vvlPath = Join-Path $engine 'build/n0-validation/local-packages/vvl-rev1-preparation/consumer-rev4/source-freeze-vvl-provider-consumer-rev4-bare.json'
$pythonFiles = @(Get-Content -LiteralPath $vvlPath -Raw | ConvertFrom-Json | Where-Object { [IO.Path]::GetFullPath($_.path).StartsWith($pythonRoot + '\', [StringComparison]::OrdinalIgnoreCase) })
if ($pythonFiles.Count -ne @(Get-ChildItem -LiteralPath $pythonRoot -Recurse -File).Count) { throw 'External Python tree membership changed' }
foreach ($file in $pythonFiles) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "External Python tool delta: $($file.path)" }
    Add-Input $file.path 'unchanged audited external CPython generation tool; not product'
}
$sources = @(Get-ChildItem -LiteralPath $provider -Recurse -File | Sort-Object FullName)
if ($sources.Count -ne 12) { throw 'Provider membership differs from reviewed candidate scope' }
foreach ($file in $sources) { Add-Input $file.FullName 'new DXC provider source; native pending' }
foreach ($path in @('cmake/Tools/LocalPackages/JECreateLocalPackage.cmake','cmake/Tools/LocalPackages/Python/JERequireLocalPath.cmake','LICENSE.txt','LICENSE_APACHE2.TXT','LICENSE_MIT.TXT')) {
    Add-Input (Join-Path $engine $path) 'unchanged shared package helper/local guard/full O3DE notice'
}
Add-Input 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' 'existing FindDiaSDK executable; no install'
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root 'private-overlay-proof') -Recurse -File) { Add-Input $file.FullName 'three exact private overlays and inverse byte proofs; no source native executed' }
foreach ($name in @('PrepareDxcOverlayInputs.ps1','FreezeDxcProviderCandidate.ps1','parent-implementation-approval-dxc-rev1.json','source-freeze-dxc-acquired-final.json','freeze-verification-dxc-acquired-final.json')) { Add-Input (Join-Path $root $name) 'approved acquisition/provenance/own evidence preparation' }
New-Item -ItemType Directory -Path $out | Out-Null
# New directory: patch represents new source files only. Raw byte authority is the freeze.
$patch = [Text.StringBuilder]::new()
foreach ($file in $sources) {
    $relative = $file.FullName.Substring($engine.Length + 1).Replace('\','/')
    $lines = @([IO.File]::ReadAllLines($file.FullName))
    [void] $patch.Append("diff --git a/$relative b/$relative`nnew file mode 100644`n--- /dev/null`n+++ b/$relative`n@@ -0,0 +1,$($lines.Count) @@`n")
    foreach ($line in $lines) { [void] $patch.Append("+$line`n") }
}
$diffPath = Join-Path $out 'source-diff.patch'
[IO.File]::WriteAllText($diffPath, $patch.ToString(), [Text.UTF8Encoding]::new($false))
Add-Input $diffPath 'complete new-provider source diff; raw freeze is byte authority'
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
$package = 'DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows'
$nativeOut = Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1'
$consumerOut = Join-Path $engine 'build/n0-validation/local-packages/dxc-native-consumer-rev1'
$extract = Join-Path $engine 'build/n0-validation/local-packages/dxc-native-extracted-rev1'
$producerArgs = @(
    "-DJE_DXC_ARCHIVES_ROOT=$root/acquisition",
    "-DJE_DXC_PYTHON=$pythonRoot/python.exe", '-DJE_DXC_PYTHON_SHA256=6235abe2248c14ad86dfe00dbb7bc2396eb4b91f663412c211905e279bce9d6e',
    '-DJE_DXC_NINJA=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe', '-DJE_DXC_NINJA_SHA256=ff29bcb5f547174bae6b73c84278cce9b674e8ebc2f3c3926ab21030b33f34b8',
    '-DJE_DXC_CL=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe', '-DJE_DXC_CL_SHA256=88c8344236a27a6e727e0a8edc49aaa2690bdc7a9464b9d18cc7abe70a9f1c0d',
    "-DJE_DXC_OUTPUT=$nativeOut", '-P', "$provider/BuildDirectXShaderCompilerDxcPackage.cmake")
$assignment = [ordered]@{
    status = 'CANDIDATE FOR PARENT REVIEW; DO NOT EXECUTE UNTIL LUNA ASSIGNED'; nativeOwner = 'Luna only'; nativeExecuted = $false
    environment = 'Existing VS2022/v143 x64 development environment, inherited local SDK/DIA/ATL; no install'
    producer = @{ executable = $cmake; argv = $producerArgs; freshOutput = $nativeOut }
    package = @{ archive = "$nativeOut/packages/$package.tar.xz"; sidecar = "$nativeOut/packages/$package.tar.xz.sha256"; id = $package; sourceNumericVersion = '1.8.2505.0'; packageVersion = '1.8.2505.1'; sha256 = 'MEASURE AFTER REAL PRODUCTION, NEVER LEGACY HASH'; extractedRoot = $extract; extraction = 'Luna audits local produced tar paths/types/links/hashes before fresh private extraction; test this actual archive, not stage/contents' }
    consumerConfigure = @{ executable = $cmake; argv = @('-S',"$provider/tests",'-B',$consumerOut,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_DXC_PACKAGE_ROOT=$extract","-DJE_DXC_SOURCE_ROOT=$root/acquisition/dxc/snapshot",'-DFETCHCONTENT_FULLY_DISCONNECTED=ON') }
    consumerBuild = @{ executable = $cmake; argv = @('--build',$consumerOut,'--config','Release','--target','JEDirectXShaderCompilerDxcConsumer','--parallel') }
    consumerCTest = @{ executable = 'C:/Program Files/CMake/bin/ctest.exe'; argv = @('--test-dir',$consumerOut,'-C','Release','--output-on-failure'); cases = @('JE.LocalPackages.DXC.dxil','JE.LocalPackages.DXC.spirv','JE.LocalPackages.DXC.invalid','JE.LocalPackages.DXC.dxsc'); expected = '4/4 real cases; HRESULT and operationstatus, exact provenance, retail hash, actual dxsc and malformed rejection' }
    guards = @('missing archive','wrong archive SHA','wrong external tool SHA','remote URL path','UNC path','symlink path/ancestor','existing output','outside-build output','missing VS x64 environment','private provenance before/after/inverse mismatch')
    preservation = 'Preflight/postflight full frozen inputs and all three snapshot memberships; no output cleanup/overwrite; archive helper and Root unchanged'
    acceptanceLimits = @('Native producer and consumer NOT_RUN','No Root/AP/Editor/DX12 PSO/Vulkan acceptance','Actual optional LibXml2/zlib/Metal/DIA/CRT cache and PE imports to review','Full notices preserve provenance, not legal certification','Retail DXIL hash is upstream shader compatibility metadata, not Authenticode/security signature')
}
$assignmentPath = Join-Path $out 'validation-assignment.json'
$assignment | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $assignmentPath -Encoding utf8
Add-Input $assignmentPath 'proposed literal native commands for sole Luna owner after parent review'
$files = @($inputs.Values | Sort-Object path)
foreach ($file in $files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Final input delta: $($file.path)" }
}
$bare = Join-Path $out 'source-freeze-dxc-provider-rev1-bare.json'
ConvertTo-Json -InputObject $files -Depth 12 | Set-Content -LiteralPath $bare -Encoding utf8
$freeze = Join-Path $out 'source-freeze-dxc-provider-rev1.json'
[ordered]@{ status = 'IMPLEMENTED CANDIDATE; PENDING NATIVE EXECUTION/PARENT ACCEPTANCE'; fileCount = $files.Count; providerFileCount = $sources.Count; approvedHistoricalUnchanged = 19828; externalPythonFiles = $pythonFiles.Count; immutableSnapshotFiles = 19661; originalSourceArchives = 3; privateOverlays = 3; nativeExecuted = $false; files = $files } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $freeze -Encoding utf8
$proof = [ordered]@{ status = 'STATIC DATA REHASH ONLY'; verified = $files.Count; oldInputDeltas = 0; newProviderFiles = $sources.Count; externalPythonFiles = $pythonFiles.Count; sourceDiffSHA256 = (Get-FileHash -LiteralPath $diffPath).Hash; assignmentSHA256 = (Get-FileHash -LiteralPath $assignmentPath).Hash; freezeSHA256 = (Get-FileHash -LiteralPath $freeze).Hash; bareFreezeSHA256 = (Get-FileHash -LiteralPath $bare).Hash; fixedVersionPlanCorrection = 'Private utils/version/version.inc, HLSL_ENABLE_FIXED_VER=ON, HLSL_FIXED_VERSION_LOCATION empty; nonempty old-plan location would invoke Git. No upstream CMake patch.'; nativeExecuted = $false; acceptance = $false }
$proof | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $out 'candidate-proof.json') -Encoding utf8
$proof | ConvertTo-Json -Depth 12
