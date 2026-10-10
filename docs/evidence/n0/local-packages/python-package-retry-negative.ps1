$ErrorActionPreference = 'Stop'
$root = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $root 'build\n0-validation\local-packages'
$stage = Join-Path $base 'python-v143-rev2\stage'
$oldOut = Join-Path $base 'python-v143-rev2\packages'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$helper = Join-Path $root 'cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
$freeze = Join-Path $base 'source-freeze-python-parent-rev2.json'
$prefix = Join-Path $base 'python-package-retry-negative'
function Snapshot-Tree([string]$path, [string]$dest) {
  $items = @(Get-ChildItem -LiteralPath $path -File -Recurse | ForEach-Object {
    $h = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
    [pscustomobject]@{ path=$_.FullName.Substring($path.Length).TrimStart('\'); bytes=$_.Length; sha256=$h.Hash }
  } | Sort-Object path)
  $items | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $dest -Encoding UTF8
  return [pscustomobject]@{ files=$items.Count; bytes=($items | Measure-Object bytes -Sum).Sum; snapshot=$dest }
}
function Snapshot-Inputs([string]$label) {
  $stageInfo = Snapshot-Tree $stage ("$prefix-stage-$label.json")
  $freezeInfo = Get-FileHash -LiteralPath $freeze -Algorithm SHA256
  $helperInfo = Get-FileHash -LiteralPath $helper -Algorithm SHA256
  $cmakeInfo = Get-FileHash -LiteralPath $cmake -Algorithm SHA256
  [pscustomobject]@{ stage=$stageInfo; freeze_sha256=$freezeInfo.Hash; helper_sha256=$helperInfo.Hash; cmake_sha256=$cmakeInfo.Hash; cmake_version=(((& $cmake --version) | Select-Object -First 1) -join '') }
}
$argv = [string[]]@('-DJE_PACKAGE_STAGE=' + $stage, '-DJE_PACKAGE_NAME=python-3.10.13-je-rev1-windows', '-DJE_PACKAGE_OUTPUT=' + $oldOut, '-DJE_PACKAGE_VERSION=3.10.13-je-rev1', '-DJE_PACKAGE_SOURCE_COMMIT=49965601d6afedafe47cc85556d99b7a24981051', '-DJE_PACKAGE_SOURCE_SHA256=12c564214c8a3b109d294143cb439e8bccb1cf6be677b52e98c5e2c41941ea44', '-DJE_PACKAGE_LICENSE=PSF-2.0', '-DJE_PACKAGE_SOURCE_URL=https://codeload.github.com/python/cpython/tar.gz/49965601d6afedafe47cc85556d99b7a24981051', '-P', $helper)
$meta = [pscustomobject]@{ cwd=(Get-Location).Path; executable=$cmake; argv=@($argv); mode='-P'; started=(Get-Date).ToString('o'); before=(Snapshot-Inputs 'before'); old_contents_sha256=(Get-FileHash -LiteralPath (Join-Path $oldOut 'python-3.10.13-je-rev1-windows.contents\SHA256SUMS') -Algorithm SHA256).Hash }
$meta | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$prefix-argv-before.json" -Encoding UTF8
$sw = [Diagnostics.Stopwatch]::StartNew()
$output = & $cmake @argv 2>&1
$exit = $LASTEXITCODE
$sw.Stop()
$output | Out-String | Set-Content -LiteralPath "$prefix.log" -Encoding UTF8
$exit | Set-Content -LiteralPath "$prefix.exitcode" -Encoding ASCII
('{0:N3}s' -f $sw.Elapsed.TotalSeconds) | Set-Content -LiteralPath "$prefix.elapsed.txt" -Encoding ASCII
$after = Snapshot-Inputs 'after'
$newHash = (Get-FileHash -LiteralPath (Join-Path $oldOut 'python-3.10.13-je-rev1-windows.contents\SHA256SUMS') -Algorithm SHA256).Hash
$meta2 = [pscustomobject]@{ started=$meta.started; finished=(Get-Date).ToString('o'); exitcode=$exit; elapsed_seconds=$sw.Elapsed.TotalSeconds; before=$meta.before; after=$after; old_contents_sha256_before=$meta.old_contents_sha256; old_contents_sha256_after=$newHash; output_changed=($meta.old_contents_sha256 -ne $newHash) }
$meta2 | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$prefix-inputs-after.json" -Encoding UTF8
Write-Output ("exit=$exit elapsed={0:N3}s output_changed={1}" -f $sw.Elapsed.TotalSeconds, ($meta.old_contents_sha256 -ne $newHash))
