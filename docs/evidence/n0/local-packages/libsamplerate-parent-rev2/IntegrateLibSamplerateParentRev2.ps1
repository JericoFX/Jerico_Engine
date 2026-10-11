$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = 'F:/JericoEngine/JericoEngine'
$Base = "$Repo/build/n0-validation/local-packages"
function JEHash([string]$Path) {
    $Stream = [IO.File]::OpenRead($Path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Stream)) }
    finally { $Stream.Dispose() }
}
function JELocal([string]$Path) {
    if (-not [IO.Path]::IsPathFullyQualified($Path) -or $Path -match '://|^[/\\]{2}') { throw 'Absolute local path required' }
    $Node = [IO.Path]::GetFullPath($Path)
    while ($Node) {
        if (([IO.File]::Exists($Node) -or [IO.Directory]::Exists($Node)) -and
            (([IO.File]::GetAttributes($Node) -band [IO.FileAttributes]::ReparsePoint) -ne 0)) { throw 'Reparse integration path' }
        $Node = [IO.Path]::GetDirectoryName($Node)
    }
}
$AcceptancePath = "$Base/libsamplerate-rev2-validation-preparation/parent-focal-acceptance-rev1.json"
if ((JEHash $AcceptancePath) -cne 'A6AAB1FA968FCC9FA4BB44056457F452E29E8F34BBFCFCD318CCA361ACECB316') { throw 'Acceptance changed' }
$Acceptance = [IO.File]::ReadAllText($AcceptancePath) | ConvertFrom-Json
if (-not $Acceptance.acceptedForCatalogIntegration -or $Acceptance.consumerCases -ne 14 -or
    $Acceptance.guardCases -ne 8 -or $Acceptance.sourceInputDeltas -ne 0 -or $Acceptance.engineEditorRuntimeAccepted) { throw 'Focal acceptance scope differs' }
$ProviderRoot = "$Repo/cmake/Tools/LocalPackages/LibSamplerate"
$Inputs = ([IO.File]::ReadAllText("$Base/libsamplerate-rev2-guards-remaining-preparation/review2/driver-freeze.json") | ConvertFrom-Json).entries
$Providers = @(Get-ChildItem -LiteralPath $ProviderRoot -File -Recurse)
if ($Providers.Count -ne 9) { throw 'Provider membership differs' }
foreach ($File in $Providers) {
    $InputRecord = @($Inputs | Where-Object { [IO.Path]::GetFullPath($_.path) -eq $File.FullName })
    if ($InputRecord.Count -ne 1 -or $InputRecord[0].bytes -ne $File.Length -or (JEHash $File.FullName) -cne $InputRecord[0].sha256) { throw 'Accepted provider changed' }
}
$PackageName = 'libsamplerate-0.2.1-je-rev2-windows'
$Archive = "$Base/libsamplerate-rev2-build/packages/$PackageName.tar.xz"
$ArchiveHash = '01086CD19A1D241B4B0504FE16142C80BF29AF329543E67FF773301EDAA95122'
$Cache = "$Base/root-package-cache"
$Catalog = "$Repo/cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake"
$ReceiptPath = "$PSScriptRoot/parent-package-integration.json"
$BackupPath = "$PSScriptRoot/BuiltInPackages_windows.before-libsamplerate.cmake"
foreach ($Path in @($Archive, $Cache, $Catalog, $ReceiptPath, $BackupPath)) { JELocal $Path }
foreach ($Path in @($ReceiptPath, $BackupPath, "$Cache/$PackageName.tar.xz", "$Cache/$PackageName.tar.xz.SHA256SUMS")) {
    if (Test-Path -LiteralPath $Path) { throw 'Existing integration output preserved' }
}
if ((JEHash $Archive) -cne $ArchiveHash -or [IO.FileInfo]::new($Archive).Length -ne 3954496) { throw 'Archive changed' }
$ExpectedSidecar = $ArchiveHash.ToLowerInvariant() + ' *' + $PackageName + '.tar.xz'
if ([IO.File]::ReadAllText($Archive + '.SHA256SUMS').Trim() -cne $ExpectedSidecar) { throw 'Sidecar identity differs' }
$CacheBefore = @(Get-ChildItem -LiteralPath $Cache -File -Force | ForEach-Object {
    [ordered]@{ path=$_.FullName; bytes=$_.Length; sha256=JEHash $_.FullName }
})
if ($CacheBefore.Count -ne 40) { throw 'Expected forty existing cache files' }
$CatalogBeforeHash = JEHash $Catalog
if ($CatalogBeforeHash -cne '333778663011B38AF79966FAFA55FDA85DB7C29DD58BFE40CA0746754DC62F49') { throw 'Catalog differs from reviewed state' }
$CatalogBytes = [IO.File]::ReadAllBytes($Catalog)
$Before = [Text.Encoding]::UTF8.GetString($CatalogBytes)
$OldName = 'libsamplerate-0.2.1-rev2-windows'
$OldHash = 'dcf3c11a96f212a52e2c9241abde5c364ee90b0f32fe6eeb6dcdca01d491829f'
foreach ($Fragment in @($OldName, $OldHash)) {
    if ([regex]::Matches($Before, [regex]::Escape($Fragment)).Count -ne 1) { throw 'Original binding must be unique' }
}
$After = $Before.Replace($OldName, $PackageName).Replace($OldHash, $ArchiveHash.ToLowerInvariant())
if ($After.Replace($PackageName, $OldName).Replace($ArchiveHash.ToLowerInvariant(), $OldHash) -cne $Before) { throw 'Catalog inverse differs' }
[IO.File]::Copy($Catalog, $BackupPath, $false)
[IO.File]::Copy($Archive, "$Cache/$PackageName.tar.xz", $false)
[IO.File]::Copy($Archive + '.SHA256SUMS', "$Cache/$PackageName.tar.xz.SHA256SUMS", $false)
[IO.File]::WriteAllBytes($Catalog, [Text.Encoding]::UTF8.GetBytes($After))
foreach ($Entry in $CacheBefore) {
    if ([IO.FileInfo]::new($Entry.path).Length -ne $Entry.bytes -or (JEHash $Entry.path) -cne $Entry.sha256) { throw 'Existing cache bytes changed' }
}
if (@(Get-ChildItem -LiteralPath $Cache -File -Force).Count -ne 42 -or
    (JEHash "$Cache/$PackageName.tar.xz") -cne $ArchiveHash) { throw 'Integrated cache differs' }
$Receipt = [ordered]@{
    utc = [DateTime]::UtcNow.ToString('o')
    status = 'PARENT_FOCAL_LIBSAMPLERATE_REV2_CATALOG_AND_LOCAL_CACHE_INTEGRATED'
    packageName = $PackageName
    archiveBytes = 3954496
    archiveSHA256 = $ArchiveHash
    acceptanceSHA256 = JEHash $AcceptancePath
    providerFiles = @($Providers | ForEach-Object { [ordered]@{path=[IO.Path]::GetRelativePath($Repo,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=JEHash $_.FullName} })
    consumerCases = 14
    guardCases = 8
    existingCacheFilesBytePreserved = 40
    cacheFilesAfter = 42
    catalogBeforeSHA256 = $CatalogBeforeHash
    catalogAfterSHA256 = JEHash $Catalog
    catalogInverseExact = $true
    debugWarning = 'LNK4098 retained; original Release /MD static library, bounded Debug /MDd consumer only'
    originalSourceModified = $false
    engineEditorRuntimeAccepted = $false
    rootReconfigured = $false
    dependencyFootprintReduced = $false
}
$Bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Receipt | ConvertTo-Json -Depth 10) + "`n")
$Stream = [IO.File]::Open($ReceiptPath, [IO.FileMode]::CreateNew)
try { $Stream.Write($Bytes, 0, $Bytes.Length) } finally { $Stream.Dispose() }
$Receipt | ConvertTo-Json -Depth 10
"receiptSHA256=$(JEHash $ReceiptPath)"
