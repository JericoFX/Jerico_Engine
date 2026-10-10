$ErrorActionPreference = 'Stop'
$root = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $root 'build\n0-validation\local-packages'
$src = Join-Path $root 'cmake\Tools\LocalPackages\Python\tests'
$build = Join-Path $base 'python-v143-rev2\consumer-retry'
$package = Join-Path $base 'python-v143-rev2\extracted\python-3.10.13-je-rev1-windows'
$freeze = Join-Path $base 'source-freeze-python-parent-rev2.json'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$prefix = Join-Path $base 'python-consumer-retry-configure'
if (Test-Path -LiteralPath $build) { throw "consumer build already exists: $build" }
function Snapshot-Inputs([string]$label) {
  $manifest = Get-Content -LiteralPath $freeze -Raw | ConvertFrom-Json
  $sourceItems = @($manifest | ForEach-Object { $p=Join-Path $root $_.path; [pscustomobject]@{path=$_.path; sha256=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash} })
  $packageRows = @(Get-Content -LiteralPath (Join-Path (Split-Path $package -Parent | Split-Path -Parent) 'package-retry\python-3.10.13-je-rev1-windows.contents\SHA256SUMS'))
  $packageHash = Get-FileHash -LiteralPath (Join-Path (Split-Path $package -Parent | Split-Path -Parent) 'package-retry\python-3.10.13-je-rev1-windows.tar.xz') -Algorithm SHA256
  [pscustomobject]@{ freeze_sha256=(Get-FileHash -LiteralPath $freeze -Algorithm SHA256).Hash; source=$sourceItems; extracted_files=(Get-ChildItem -LiteralPath $package -File -Recurse | Measure-Object).Count; package_sha256=$packageHash.Hash; package_manifest_rows=$packageRows.Count; cmake_sha256=(Get-FileHash -LiteralPath $cmake -Algorithm SHA256).Hash }
}
$a1='-S'; $a2=$src; $a3='-B'; $a4=$build; $a5='-G'; $a6='Visual Studio 17 2022'; $a7='-A'; $a8='x64'; $a9="-DJE_PYTHON_PACKAGE_ROOT=$package"; $a10='-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON'
[string[]]$argv=@($a1,$a2,$a3,$a4,$a5,$a6,$a7,$a8,$a9,$a10)
if($argv.Count -ne 10 -or $argv[0] -ne '-S' -or $argv[2] -ne '-B' -or $argv[4] -ne '-G' -or $argv[6] -ne '-A' -or $argv[8] -notlike '-DJE_PYTHON_PACKAGE_ROOT=*'){throw 'configure argv invalid'}
$envBefore=[pscustomobject]@{PIP_NO_INDEX=$env:PIP_NO_INDEX;PIP_DISABLE_PIP_VERSION_CHECK=$env:PIP_DISABLE_PIP_VERSION_CHECK;PYTHONNOUSERSITE=$env:PYTHONNOUSERSITE}
$env:PIP_NO_INDEX='1';$env:PIP_DISABLE_PIP_VERSION_CHECK='1';$env:PYTHONNOUSERSITE='1'
$meta=[pscustomobject]@{cwd=(Get-Location).Path;executable=$cmake;argv=@($argv);argv_count=$argv.Count;mode='configure';started=(Get-Date).ToString('o');env_before=$envBefore;inputs_before=(Snapshot-Inputs 'before')}
$meta|ConvertTo-Json -Depth 8|Set-Content -LiteralPath "$prefix-argv-before.json" -Encoding UTF8
$sw=[Diagnostics.Stopwatch]::StartNew();$output=& $cmake @argv 2>&1;$exit=$LASTEXITCODE;$sw.Stop()
$output|Out-String|Set-Content -LiteralPath "$prefix.log" -Encoding UTF8
$exit|Set-Content -LiteralPath "$prefix.exitcode" -Encoding ASCII
('{0:N3}s' -f $sw.Elapsed.TotalSeconds)|Set-Content -LiteralPath "$prefix.elapsed.txt" -Encoding ASCII
$after=Snapshot-Inputs 'after'
[pscustomobject]@{started=$meta.started;finished=(Get-Date).ToString('o');exitcode=$exit;elapsed_seconds=$sw.Elapsed.TotalSeconds;before=$meta.inputs_before;after=$after;env_after=[pscustomobject]@{PIP_NO_INDEX=$env:PIP_NO_INDEX;PIP_DISABLE_PIP_VERSION_CHECK=$env:PIP_DISABLE_PIP_VERSION_CHECK;PYTHONNOUSERSITE=$env:PYTHONNOUSERSITE}}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath "$prefix-inputs-after.json" -Encoding UTF8
Write-Output ("exit=$exit elapsed={0:N3}s" -f $sw.Elapsed.TotalSeconds)
