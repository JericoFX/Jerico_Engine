$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$draftPath = Join-Path $root 'source-freeze-dxc-provider-rev2.json'
if ((Get-FileHash -LiteralPath $draftPath).Hash -ne 'E9AA81DD7D8C8D4BA6E6EA104E8D0410E2DBAAEA4428D0D10A37E311E95E6AC5') { throw 'Rev2 draft changed' }
$draft = Get-Content -LiteralPath $draftPath -Raw | ConvertFrom-Json
foreach ($file in $draft.files) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Draft input delta: $($file.path)" } }
$readme = 'F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\DirectXShaderCompilerDxc\README.md'
$before = [IO.File]::ReadAllBytes($readme)
$oldLine = "  '-DJE_DXC_PARALLEL_JOBS=6' `` `n"
$newLine = "  '-DJE_DXC_PARALLEL_JOBS=6' ```n"
$text = [Text.UTF8Encoding]::new($false).GetString($before)
if ([regex]::Matches($text,[regex]::Escape($oldLine)).Count -ne 1) { throw 'Exact added README line absent' }
$updated = $text.Replace($oldLine,$newLine)
if ($updated.Replace($newLine,$oldLine) -cne $text) { throw 'README one-space inverse differs' }
[IO.File]::WriteAllBytes((Join-Path $root 'README-rev2-draft.before'),$before)
[IO.File]::WriteAllText($readme,$updated,[Text.UTF8Encoding]::new($false))
$receipt = Get-Content -LiteralPath (Join-Path $root 'source-change-receipt.json') -Raw | ConvertFrom-Json -AsHashtable
$entry = @($receipt.sourceDeltas | Where-Object { $_.path -eq $readme })[0]
$entry.afterBytes = (Get-Item -LiteralPath $readme).Length
$entry.afterSHA256 = (Get-FileHash -LiteralPath $readme).Hash.ToLowerInvariant()
$receipt.finalREADMECommandCorrection = @{ removedBytes = 1; scope = 'Only trailing space after newly added PowerShell continuation backtick'; draftSHA256 = '87477492e514d03b4a7b3be3e7472d6b1b7363788ff11447a9e955e1514731f6'; finalSHA256 = $entry.afterSHA256; inverseByteExact = $true; originalRev1InverseComposition = 'Final to draft exact one-space inverse, then six exact revision inverses already verified in source-change-receipt.json' }
$receipt | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $root 'source-change-receipt-final.json') -Encoding utf8
$patchPath = Join-Path $root 'source-diff-rev1-to-rev2.patch'
$patch = [IO.File]::ReadAllText($patchPath)
$oldAddedLine = "+  '-DJE_DXC_PARALLEL_JOBS=6' `` `n"
$newAddedLine = "+  '-DJE_DXC_PARALLEL_JOBS=6' ```n"
if ([regex]::Matches($patch,[regex]::Escape($oldAddedLine)).Count -ne 1) { throw 'Patch new line absent' }
[IO.File]::WriteAllText((Join-Path $root 'source-diff-rev1-to-rev2-final.patch'),$patch.Replace($oldAddedLine,$newAddedLine),[Text.UTF8Encoding]::new($false))
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in $draft.files) {
    if ($file.path -eq $readme) { $file.sha256 = $entry.afterSHA256; $file.bytes = $entry.afterBytes }
    $inputs.Add([IO.Path]::GetFullPath($file.path),$file)
}
foreach ($path in @($draftPath,(Join-Path $root 'source-freeze-dxc-provider-rev2-bare.json'),(Join-Path $root 'candidate-proof.json'),$PSCommandPath,(Join-Path $root 'README-rev2-draft.before'),(Join-Path $root 'source-change-receipt-final.json'),(Join-Path $root 'source-diff-rev1-to-rev2-final.patch'))) {
    $absolute = [IO.Path]::GetFullPath($path)
    if (-not $inputs.ContainsKey($absolute)) { $inputs.Add($absolute,[ordered]@{ path = $absolute; bytes = (Get-Item -LiteralPath $absolute).Length; sha256 = (Get-FileHash -LiteralPath $absolute).Hash.ToLowerInvariant(); role = 'preserved draft and final exact README command correction evidence' }) }
}
$files = @($inputs.Values | Sort-Object path)
foreach ($file in $files) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Final delta: $($file.path)" } }
$freeze = Join-Path $root 'source-freeze-dxc-provider-rev2-final.json'
$bare = Join-Path $root 'source-freeze-dxc-provider-rev2-final-bare.json'
if ((Test-Path -LiteralPath $freeze) -or (Test-Path -LiteralPath $bare)) { throw 'Final evidence exists' }
[ordered]@{ status = 'REV2 FINAL CANDIDATE; PENDING PARENT REVIEW/NATIVE'; fileCount = $files.Count; historicalInputCount = 23322; authorizedHistoricalDeltas = 2; historicalUnchanged = 23320; providerFileCount = 12; sourceDeltas = $receipt.sourceDeltas; parallelJobs = 6; nativeExecuted = $false; files = $files } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $freeze -Encoding utf8
ConvertTo-Json -InputObject $files -Depth 12 | Set-Content -LiteralPath $bare -Encoding utf8
$proof = [ordered]@{ status = 'STATIC REHASH AND EXACT INVERSE ONLY'; verified = $files.Count; historicalInputs = 23322; authorizedDeltas = 2; unknownDeltas = 0; providerSources = 12; unchangedProviderSources = 10; sourceDiffSHA256 = (Get-FileHash -LiteralPath (Join-Path $root 'source-diff-rev1-to-rev2-final.patch')).Hash; assignmentSHA256 = (Get-FileHash -LiteralPath (Join-Path $root 'validation-assignment.json')).Hash; receiptSHA256 = (Get-FileHash -LiteralPath (Join-Path $root 'source-change-receipt-final.json')).Hash; freezeSHA256 = (Get-FileHash -LiteralPath $freeze).Hash; bareFreezeSHA256 = (Get-FileHash -LiteralPath $bare).Hash; parallelJobs = 6; nativeExecuted = $false; acceptance = $false }
$proof | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root 'candidate-proof-final.json') -Encoding utf8
$proof | ConvertTo-Json -Depth 12
