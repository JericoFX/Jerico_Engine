$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$engine = 'F:\JericoEngine\JericoEngine'
$prep = Split-Path $root -Parent
$oldPath = Join-Path $prep 'provider-candidate-rev2/source-freeze-dxc-provider-rev2-final.json'
if ((Get-FileHash -LiteralPath $oldPath).Hash -ne '123AE0639F373D0EF16E6147FC74765CBCBBD7AF42CCEF1535D58A5B7A1C5CCD') { throw 'Approved freeze differs' }
$old = Get-Content -LiteralPath $oldPath -Raw | ConvertFrom-Json
if ($old.files.Count -ne 23348) { throw 'Source count differs' }
foreach ($file in $old.files) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash -ne $file.sha256) { throw "Unapproved before delta: $($file.path)" } }
$cpp = Join-Path $engine 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
$beforePath = Join-Path $root 'DirectXShaderCompilerDxcConsumer.rev1.before.cpp'
if (Test-Path -LiteralPath $beforePath) { throw 'Previous evidence exists' }
$beforeBytes = [IO.File]::ReadAllBytes($cpp)
[IO.File]::WriteAllBytes($beforePath,$beforeBytes)
$text = [Text.UTF8Encoding]::new($false).GetString($beforeBytes)
$from = '        Require(major == 1 && minor == 8, "compiler1.8 API version");'
$to = @'
        // GetVersion reports DXIL API1.9, distinct from PE1.8.2505.0/package identity.
        std::printf("OBSERVED DXIL API version: %u.%u\n", static_cast<unsigned>(major), static_cast<unsigned>(minor));
        Require(major == 1 && minor == 9, "upstream DXIL1.9 API version");
'@
$to = $to.Replace("`r`n","`n")
if ([regex]::Matches($text,[regex]::Escape($from)).Count -ne 1) { throw 'Unique old oracle absent' }
$updated = $text.Replace($from,$to)
if ($updated.Replace($to,$from) -cne $text) { throw 'Exact replacement inverse differs' }
[IO.File]::WriteAllText($cpp,$updated,[Text.UTF8Encoding]::new($false))
$prefix = @($text.Substring(0,$text.IndexOf($from)).Split("`n"))
$line = $prefix.Count
$relative = 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/tests/DirectXShaderCompilerDxcConsumer.cpp'
$diff = "diff --git a/$relative b/$relative`n--- a/$relative`n+++ b/$relative`n@@ -$line,1 +$line,3 @@`n-$from`n" + (($to.Split("`n") | ForEach-Object { "+$_" }) -join "`n") + "`n"
[IO.File]::WriteAllText((Join-Path $root 'source-diff-rev1-to-rev2.patch'),$diff,[Text.UTF8Encoding]::new($false))
$packageRoot = Join-Path $engine 'build/n0-validation/local-packages/dxc-native-extracted-rev1'
$payloadFiles = @(Get-ChildItem -LiteralPath $packageRoot -Recurse -File)
if ($payloadFiles.Count -ne 15) { throw 'Accepted payload membership differs' }
foreach ($lineText in Get-Content -LiteralPath (Join-Path $packageRoot 'SHA256SUMS')) {
    if ($lineText -notmatch '^([a-f0-9]{64}) \*([^:]+)$') { throw 'Actual payload manifest malformed' }
    $digest = $Matches[1]
    $path = [IO.Path]::GetFullPath((Join-Path $packageRoot $Matches[2]))
    if (-not $path.StartsWith([IO.Path]::GetFullPath($packageRoot) + '\',[StringComparison]::OrdinalIgnoreCase) -or (Get-FileHash -LiteralPath $path).Hash -ne $digest) { throw 'Accepted extracted payload changed' }
}
$oldAssignment = Get-Content -LiteralPath (Join-Path $prep 'provider-candidate-rev2/validation-assignment.json') -Raw | ConvertFrom-Json -AsHashtable
$assignment = [ordered]@{ status = 'CORRECTED CONSUMER CANDIDATE; PENDING NATIVE'; nativeOwner = 'Luna only after parent review'; consumerRevision = 2; producerRebuild = $false; extraction = 'Reuse accepted real extracted archive read-only; no re-extraction'; archiveSHA256 = '62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782'; packageRoot = $packageRoot; consumerConfigure = $oldAssignment.consumerConfigure; consumerBuild = $oldAssignment.consumerBuild; consumerCTest = $oldAssignment.consumerCTest; correctedOracle = 'Exact pinned DXIL API1.9; observed unsigned values printed before Require'; preservedOracles = 'PE1.8.2505.0/full40 pin, hash8/count4954/CoTaskMemFree, HRESULT+status, retail hash, actual validator, SPIRV, dxsc and malformed rejection'; nativeExecuted = $false; acceptance = $false }
foreach ($command in @('consumerConfigure','consumerBuild','consumerCTest')) { $assignment[$command].argv = @($assignment[$command].argv | ForEach-Object { $_.Replace('dxc-native-consumer-rev1','dxc-native-consumer-rev2') }) }
$assignment | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $root 'validation-assignment.json') -Encoding utf8
$receipt = [ordered]@{ status = 'ONE SOURCE ORACLE CORRECTION; NATIVE NOT RUN'; path = $cpp; beforeBytes = $beforeBytes.Length; beforeSHA256 = (Get-FileHash -LiteralPath $beforePath).Hash; afterBytes = (Get-Item -LiteralPath $cpp).Length; afterSHA256 = (Get-FileHash -LiteralPath $cpp).Hash; exactInverse = $true; sourceContract = @('dxcompiler/dxcompilerobj.cpp:1708-1714 returns DXIL::kDxilMajor/kDxilMinor','include/dxc/DXIL/DxilConstants.h:29,32 are1,9','dxildll/dxcvalidator.cpp:97-100 delegates getValidationVersion','dxcvalidator/dxcvalidator.cpp:278-282 delegates generated GetValidationVersion','build-Release/lib/DxilValidation/DxilValidationImpl.inc:533-534 actual generated1,9','utils/version/version.inc:RC_VERSION_FIELD_1..4/RC_FILE_VERSION remain1.8.2505.0'); nativeObservedFailure = 'Historical consumer-attempt1 CTest failed compiler1.8 oracle before compile; dxsc dependency had no emitted fixture'; producerOrPackageChange = $false; bypassAdded = $false }
$receipt | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root 'source-change-receipt.json') -Encoding utf8
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in $old.files) {
    $path = [IO.Path]::GetFullPath($file.path)
    if ($path -eq $cpp) { $inputs.Add($path,[ordered]@{ path = $path; bytes = $receipt.afterBytes; sha256 = $receipt.afterSHA256; role = 'one corrected consumer DXIL API oracle; no producer/package changes' }) }
    else { if ((Get-Item -LiteralPath $path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $path).Hash -ne $file.sha256) { throw "Unexpected historical delta: $path" }; $inputs.Add($path,$file) }
}
$archive = Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz'
if ((Get-FileHash -LiteralPath $archive).Hash -ne $assignment.archiveSHA256) { throw 'Actual production archive changed' }
$paths = @($oldPath,$archive,(Join-Path $engine 'build/n0-validation/local-packages/dxc-rev1-validation/consumer-attempt1/consumer-result.json'),(Join-Path $engine 'build/n0-validation/local-packages/dxc-rev1-validation/consumer-attempt1/ctest.junit.xml'),(Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/build-Release/lib/DxilValidation/DxilValidationImpl.inc')) + @($payloadFiles | ForEach-Object FullName) + @(Get-ChildItem -LiteralPath $root -File | ForEach-Object FullName)
foreach ($path in $paths) { $full = [IO.Path]::GetFullPath($path); if (-not $inputs.ContainsKey($full)) { $inputs.Add($full,[ordered]@{ path = $full; bytes = (Get-Item -LiteralPath $full).Length; sha256 = (Get-FileHash -LiteralPath $full).Hash; role = 'immutable real package / historical failure / exact corrective evidence' }) } }
$files = @($inputs.Values | Sort-Object path)
foreach ($file in $files) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash -ne $file.sha256) { throw "Final input delta: $($file.path)" } }
$freeze = Join-Path $root 'source-freeze-dxc-consumer-rev2.json'
$bare = Join-Path $root 'source-freeze-dxc-consumer-rev2-bare.json'
[ordered]@{ status = 'PENDING NATIVE; CORRECTED EXACT UPSTREAM API ORACLE'; fileCount = $files.Count; approvedHistoricalInputs = 23348; authorizedHistoricalDeltas = 1; unchangedHistorical = 23347; payloadFiles = 15; sourceChange = $receipt; nativeExecuted = $false; files = $files } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $freeze -Encoding utf8
ConvertTo-Json -InputObject $files -Depth 12 | Set-Content -LiteralPath $bare -Encoding utf8
$proof = [ordered]@{ verified = $files.Count; historicalInputs = 23348; deltas = 1; unknownDeltas = 0; CPP_SHA256 = $receipt.afterSHA256; diffSHA256 = (Get-FileHash -LiteralPath (Join-Path $root 'source-diff-rev1-to-rev2.patch')).Hash; assignmentSHA256 = (Get-FileHash -LiteralPath (Join-Path $root 'validation-assignment.json')).Hash; freezeSHA256 = (Get-FileHash -LiteralPath $freeze).Hash; bareFreezeSHA256 = (Get-FileHash -LiteralPath $bare).Hash; archiveSHA256 = $assignment.archiveSHA256; payloadFiles = 15; nativeExecuted = $false; acceptance = $false }
$proof | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root 'candidate-proof.json') -Encoding utf8
$proof | ConvertTo-Json -Depth 12
