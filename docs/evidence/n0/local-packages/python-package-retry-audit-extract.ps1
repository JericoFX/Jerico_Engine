$ErrorActionPreference = 'Stop'
$root = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $root 'build\n0-validation\local-packages'
$pkgDir = Join-Path $base 'python-v143-rev2\package-retry'
$name = 'python-3.10.13-je-rev1-windows'
$contents = Join-Path $pkgDir ($name + '.contents')
$archive = Join-Path $pkgDir ($name + '.tar.xz')
$filesPath = Join-Path $pkgDir ($name + '.files')
$archiveHashPath = Join-Path $pkgDir ($name + '.tar.xz.SHA256SUMS')
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$extract = Join-Path $base ('python-v143-rev2\extracted\' + $name)
if (Test-Path -LiteralPath $extract) { throw "extraction destination already exists: $extract" }
$tarArgv = [string[]]@('-E', 'tar', 'tf', $archive)
if ($tarArgv.Count -ne 4 -or $tarArgv[0] -ne '-E' -or $tarArgv[1] -ne 'tar' -or $tarArgv[2] -ne 'tf') { throw 'tar listing argv invalid' }
$listing = & $cmake @tarArgv 2>&1
$tarListExit = $LASTEXITCODE
$listing | Out-String | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-archive-listing.txt') -Encoding UTF8
$tarListExit | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-archive-listing.exitcode') -Encoding ASCII
if ($tarListExit -ne 0) { throw "archive listing failed: $tarListExit" }
$listingPaths = @($listing | ForEach-Object { [string]$_ } | Where-Object { $_ -ne '' })
$sourceList = @(Get-Content -LiteralPath $filesPath | ForEach-Object { $_ -replace '^--add-file=', '' })
$badPaths = @($listingPaths | Where-Object { $_ -match '^(?:/|[A-Za-z]:)|(^|/)\.\.(?:/|$)|\\' })
if ($badPaths.Count -ne 0) { throw ('unsafe archive paths: ' + ($badPaths -join ', ')) }
if ($listingPaths.Count -ne $sourceList.Count -or (Compare-Object ($listingPaths | Sort-Object) ($sourceList | Sort-Object)).Count -ne 0) { throw 'archive member list differs from explicit package files list' }
$archiveChecksum = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$archiveHashRecord = (Get-Content -LiteralPath $archiveHashPath -Raw).Trim()
if ($archiveHashRecord -ne ($archiveChecksum + ' *' + $name + '.tar.xz')) { throw 'archive checksum sidecar mismatch' }
$manifestPath = Join-Path $contents 'SHA256SUMS'
$manifestRows = @(Get-Content -LiteralPath $manifestPath)
$manifestMap = @{}
foreach ($row in $manifestRows) {
  if ($row -notmatch '^([0-9a-fA-F]{64}) \*(.+)$') { throw "invalid SHA256SUMS row: $row" }
  $manifestMap[$Matches[2]] = $Matches[1].ToLowerInvariant()
}
$manifestFailures = [System.Collections.Generic.List[string]]::new()
foreach ($relative in $manifestMap.Keys) {
  $full = Join-Path $contents ($relative -replace '/', '\')
  if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { $manifestFailures.Add("missing:$relative"); continue }
  if ((Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifestMap[$relative]) { $manifestFailures.Add("hash:$relative") }
}
if ($manifestFailures.Count -ne 0) { throw ('package manifest failures: ' + ($manifestFailures -join ';')) }
$manifestFiles = @(Get-ChildItem -LiteralPath $contents -File -Recurse | Where-Object { $_.Name -ne 'SHA256SUMS' })
if ($manifestFiles.Count -ne $manifestMap.Count) { throw "manifest count mismatch: expected $($manifestFiles.Count), got $($manifestMap.Count)" }
$info = Get-Content -LiteralPath (Join-Path $contents 'PackageInfo.json') -Raw | ConvertFrom-Json
if ($info.name -ne $name -or $info.version -ne '3.10.13-je-rev1' -or $info.license -ne 'PSF-2.0' -or $info.source.commit -ne '49965601d6afedafe47cc85556d99b7a24981051' -or $info.source.sha256 -ne '12c564214c8a3b109d294143cb439e8bccb1cf6be677b52e98c5e2c41941ea44') { throw 'package metadata mismatch' }
$prov = Get-Content -LiteralPath (Join-Path $contents 'JEProvenance.json') -Raw | ConvertFrom-Json
$null = New-Item -ItemType Directory -Path $extract
$extractArgv = [string[]]@('-E', 'tar', 'xf', $archive)
if ($extractArgv.Count -ne 4 -or $extractArgv[0] -ne '-E' -or $extractArgv[1] -ne 'tar' -or $extractArgv[2] -ne 'xf') { throw 'tar extraction argv invalid' }
Push-Location -LiteralPath $extract
try {
  $sw = [Diagnostics.Stopwatch]::StartNew()
  $extractOutput = & $cmake @extractArgv 2>&1
  $extractExit = $LASTEXITCODE
  $sw.Stop()
} finally { Pop-Location }
$extractOutput | Out-String | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-extract.log') -Encoding UTF8
$extractExit | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-extract.exitcode') -Encoding ASCII
('{0:N3}s' -f $sw.Elapsed.TotalSeconds) | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-extract.elapsed.txt') -Encoding ASCII
if ($extractExit -ne 0) { throw "archive extraction failed: $extractExit" }
$extractFailures = [System.Collections.Generic.List[string]]::new()
foreach ($relative in $manifestMap.Keys) {
  $full = Join-Path $extract ($relative -replace '/', '\')
  if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { $extractFailures.Add("missing:$relative"); continue }
  if ((Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifestMap[$relative]) { $extractFailures.Add("hash:$relative") }
}
if ($extractFailures.Count -ne 0) { throw ('extracted manifest failures: ' + ($extractFailures -join ';')) }
$treeFiles = @(Get-ChildItem -LiteralPath $extract -File -Recurse)
$result = [pscustomobject]@{ archive_sha256=$archiveChecksum; archive_members=$listingPaths.Count; manifest_entries=$manifestMap.Count; metadata_name=$info.name; metadata_version=$info.version; metadata_license=$info.license; metadata_source_commit=$info.source.commit; metadata_source_sha256=$info.source.sha256; externals=$prov.externals.Count; extracted_files=$treeFiles.Count; extracted_bytes=($treeFiles | Measure-Object Length -Sum).Sum; extraction=$extract; extraction_exit=$extractExit; extraction_seconds=$sw.Elapsed.TotalSeconds }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $base 'python-package-retry-audit.json') -Encoding UTF8
$result | Format-List
