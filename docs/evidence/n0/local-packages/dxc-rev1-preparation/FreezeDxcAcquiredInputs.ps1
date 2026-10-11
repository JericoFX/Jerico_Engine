$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$oldPath = Join-Path $root 'acquisition-readonly-pending-review-freeze.json'
$newPath = Join-Path $root 'source-freeze-dxc-acquired-final.json'
$proofPath = Join-Path $root 'freeze-verification-dxc-acquired-final.json'
foreach ($path in @($newPath, $proofPath)) { if (Test-Path -LiteralPath $path) { throw 'New freeze output exists' } }
if ((Get-FileHash -LiteralPath $oldPath).Hash -ne '1F6E7C9D5DD8C6FEE5A6EB823D8DBE89BC80223C9F71B81D4C70BC03D22A96ED') { throw 'Historical freeze changed' }
$old = Get-Content -LiteralPath $oldPath -Raw | ConvertFrom-Json
if ($old.files.Count -ne 1966) { throw 'Historical count changed' }
$inputs = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in $old.files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Unexpected historical delta: $($file.path)" }
    $inputs.Add([IO.Path]::GetFullPath($file.path), $file)
}
$newFiles = 0
foreach ($file in Get-ChildItem -LiteralPath $root -Recurse -File) {
    $path = [IO.Path]::GetFullPath($file.FullName)
    if (-not $inputs.ContainsKey($path)) {
        $inputs.Add($path, [ordered]@{ path = $path; bytes = $file.Length; sha256 = (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant(); role = 'owned DXC acquisition/snapshot/read-only plan evidence' })
        ++$newFiles
    }
}
$counts = [ordered]@{}
foreach ($id in @('dxc', 'spirv-headers', 'spirv-tools')) {
    $dir = Join-Path $root "acquisition/$id"
    $name = if ($id -eq 'dxc') { 'source-extraction-rev2.json' } else { 'source-extraction.json' }
    $receipt = Get-Content -LiteralPath (Join-Path $dir $name) -Raw | ConvertFrom-Json
    if (@(Get-ChildItem -LiteralPath $receipt.snapshotPath -Recurse -File).Count -ne $receipt.fileCount) { throw "Unexpected tree membership: $id" }
    foreach ($file in $receipt.files) {
        if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Snapshot delta: $($file.path)" }
    }
    $counts[$id] = $receipt.fileCount
}
if ($counts.dxc -ne 17851 -or $counts['spirv-headers'] -ne 104 -or $counts['spirv-tools'] -ne 1706) { throw 'Snapshot counts changed' }
$files = @($inputs.Values | Sort-Object path)
foreach ($file in $files) {
    if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant() -ne $file.sha256) { throw 'Final rehash delta' }
}
[ordered]@{ status = 'THREE EXACT SOURCES RAW EXTRACTED/PRIMARY VERIFIED; OVERLAYS/PRODUCER/NATIVE PENDING PARENT AUTHORIZATION'; inputCount = $files.Count; historicalUnchanged = 1966; newInputs = $newFiles; snapshots = $counts; exactDxcRawGitBlobMatches = 17832; exactDxcEolDerivedProofs = 19; rawPayloadNormalized = $false; files = $files } | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $newPath -Encoding utf8
[ordered]@{ inputCount = $files.Count; verified = $files.Count; historicalUnchanged = 1966; newInputs = $newFiles; freezePath = $newPath; freezeSHA256 = (Get-FileHash -LiteralPath $newPath).Hash; snapshotFiles = 19661; sourceArchives = 3; deltaToPrior = 'Only new own acquisition/snapshot/evidence files; no source/helper/catalogue/Root inputs edited'; nativeExecuted = $false } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $proofPath -Encoding utf8
@($newPath, $proofPath) | ForEach-Object { [pscustomobject]@{ path = $_; bytes = (Get-Item -LiteralPath $_).Length; sha256 = (Get-FileHash -LiteralPath $_).Hash } } | ConvertTo-Json
