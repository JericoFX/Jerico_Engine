#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
$taskBase = 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages'
$taskEvidence = "$taskBase/dxc-rev1-validation/pe-attempt1"
$taskPackage = "$taskBase/dxc-native-extracted-rev1"
$taskOutput = "$PSScriptRoot/parent-pe-review-rev1.json"
if (Test-Path -LiteralPath $taskOutput) { throw 'Review output exists; retain it' }
function Get-TaskHash([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
$taskPost = Get-Content -Raw -LiteralPath "$taskEvidence/postflight-v2.json" | ConvertFrom-Json
$taskSummary = @(Get-Content -Raw -LiteralPath "$taskEvidence/dumpbin-summary-v2.json" | ConvertFrom-Json)
if ($taskPost.status -ne 'POSTFLIGHT PASS' -or -not $taskPost.membershipUnchanged -or -not $taskPost.fourBinaryHashesAndSizesUnchanged -or $taskPost.extractedMembership.Count -ne 15 -or $taskSummary.Count -ne 4) { throw 'PE evidence membership/status differs' }
$taskArchive = "$taskBase/dxc-local-rev1/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz"
if ((Get-TaskHash $taskArchive) -ne '62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782') { throw 'Actual package changed' }
$taskDumpbin = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe'
if ((Get-TaskHash $taskDumpbin) -ne '12A1CD87238BD66DFDB788B4FCDCB91CE4B3F81236AAB12A7A29E5AE1D85AF50') { throw 'PE tool changed' }
$taskExpected = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($entry in $taskPost.extractedMembership) {
    if (-not $taskExpected.Add($entry.relativePath)) { throw 'Duplicate payload record' }
    $path = Join-Path $taskPackage $entry.relativePath
    if ([IO.FileInfo]::new($path).Length -ne $entry.bytes -or (Get-TaskHash $path) -ne $entry.sha256) { throw "Actual payload changed: $path" }
}
$taskActual = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($file in Get-ChildItem -LiteralPath $taskPackage -Recurse -File -Force) {
    if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Package reparse entry' }
    [void]$taskActual.Add([IO.Path]::GetRelativePath($taskPackage,$file.FullName).Replace([char]92,[char]47))
}
if (-not $taskExpected.SetEquals($taskActual)) { throw 'Package membership changed' }
$taskRecords = @(Get-Content -LiteralPath "$taskPackage/SHA256SUMS" | Where-Object { $_ -match '^[0-9a-fA-F]{64}\s+\*\S' })
if ($taskRecords.Count -ne 14) { throw 'Payload manifest record count differs' }
$taskReviewed = @()
foreach ($entry in $taskSummary) {
    $label = [IO.Path]::GetFileNameWithoutExtension($entry.name)
    $raw = [IO.File]::ReadAllText("$taskEvidence/$label.stdout-v2.txt")
    $native = Get-Content -Raw -LiteralPath "$taskEvidence/$label.result-v2.json" | ConvertFrom-Json
    if ($native.nativeExit -ne 0 -or [IO.FileInfo]::new("$taskEvidence/$label.stderr-v2.txt").Length -ne 0 -or $raw -notmatch '8664 machine \(x64\)') { throw "PE capture failed: $label" }
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo("$taskPackage/DirectXShaderCompilerDxc/bin/$($entry.name)")
    if ($version.FileVersion -ne '1.8.2505.0' -or $version.ProductVersion -ne '1.8.2505.0 JE archive 1cc0ab76dafe08a4144e419e619a8951a3d219db') { throw 'PE version/provenance differs' }
    if ($entry.name.EndsWith('.dll') -and ($raw -notmatch 'DxcCreateInstance = DxcCreateInstance' -or $raw -notmatch 'DxcCreateInstance2 = DxcCreateInstance2')) { throw 'DLL exports differ' }
    if (@($entry.imports | Where-Object { $_ -match 'python|metal|libxml|zlib' }).Count) { throw 'Unexpected dependency import' }
    $taskReviewed += [ordered]@{ name=$entry.name; rawCaptureSHA256=(Get-TaskHash "$taskEvidence/$label.stdout-v2.txt"); nativeExit=$native.nativeExit; fileVersion=$version.FileVersion; productVersion=$version.ProductVersion; machine='AMD64'; imports=@($entry.imports) }
}
$taskReceipt = [ordered]@{
    status='PARENT STATIC PE REVIEW PASS'; packageFiles=15; payloadManifestRecords=14
    packageSHA256=(Get-TaskHash $taskArchive); dumpbinSHA256=(Get-TaskHash $taskDumpbin)
    binaries=$taskReviewed; toolExecutedByParent=$false
    runtimeDependencies='Observed Windows/MSVC CRT imports; presence checks in Luna evidence. API-set contracts are not classified as missing physical DLLs.'
    limits='Static PE and local presence evidence only. No portability, signing, engine, device or frame acceptance.'
    engineAccepted=$false
}
[IO.File]::WriteAllText($taskOutput,($taskReceipt | ConvertTo-Json -Depth 8)+"`n",[Text.UTF8Encoding]::new($false))
[ordered]@{status=$taskReceipt.status; path=$taskOutput; sha256=(Get-TaskHash $taskOutput)} | ConvertTo-Json
