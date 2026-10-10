$ErrorActionPreference = 'Continue'
$root = 'F:\JericoEngine\JericoEngine'
$base = Join-Path $root 'build\n0-validation\local-packages'
$freezePath = Join-Path $base 'source-freeze-root-mimalloc.json'
$freezeShaExpected = 'FA4503E0A04FAB1CB36F56B62089CC3D69CF8B545EB08851CC99E619B1D468E4'
$tarAuditPath = Join-Path $base 'mimalloc-source-tar-audit.json'
$sourceRoot = 'F:\JericoEngine\Dependencies\mimalloc-3.5.2'
$cacheRoot = Join-Path $base 'root-package-cache'
$buildDir = Join-Path $root 'build\n0-validation\original-engine'
$depsDir = Join-Path $root 'build\n0-validation\original-engine-deps'
$wheelhouse = Join-Path $cacheRoot 'python-wheelhouse'
$prefix = Join-Path $base 'original-engine-mimalloc-configure'
$logPath = "$prefix.log"
$expectedCwd = $base
if ((Get-Location).Path -ne $expectedCwd) { throw "Unexpected CWD: $((Get-Location).Path)" }
foreach ($name in @('LY_PACKAGE_SERVER_URLS','LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_LOCATION')) { Remove-Item "Env:$name" -ErrorAction SilentlyContinue }
$env:PIP_NO_INDEX = '1'
$env:PIP_DISABLE_PIP_VERSION_CHECK = '1'
$env:PYTHONNOUSERSITE = '1'
$env:PIP_CONFIG_FILE = 'NUL'
$freezeRaw = Get-Content -LiteralPath $freezePath -Raw
$freezeSha = (Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash
if ($freezeSha -ne $freezeShaExpected) { throw "Freeze SHA mismatch: $freezeSha" }
$freeze = ConvertFrom-Json -InputObject $freezeRaw
$sourceChecks = @($freeze | ForEach-Object { $p = Join-Path $root $_.path; $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant(); [pscustomobject]@{path=$_.path; expected=$_.sha256; sha256=$h; match=($h -eq $_.sha256)} })
if (@($sourceChecks | Where-Object { -not $_.match }).Count -ne 0) { throw 'Frozen source mismatch before configure' }
$tarAudit = Get-Content -LiteralPath $tarAuditPath -Raw | ConvertFrom-Json
$archive = $tarAudit.archive
$archiveSha = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($archiveSha -ne $tarAudit.sha256) { throw "Mimalloc archive SHA mismatch: $archiveSha" }
$sourceEntries = @($tarAudit.entries | Where-Object { $_.type -eq 'file' } | ForEach-Object { $p = Join-Path $sourceRoot ($_.path.Substring('mimalloc-3.5.2/'.Length).Replace('/', '\')); $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant(); [pscustomobject]@{path=$_.path; expected=$_.sha256; sha256=$h; match=($h -eq $_.sha256)} })
if (@($sourceEntries | Where-Object { -not $_.match }).Count -ne 0) { throw 'Mimalloc extracted source mismatch before configure' }
function Get-TreeSnapshot([string]$path) { $items = @(Get-ChildItem -LiteralPath $path -File -Recurse | ForEach-Object { [pscustomobject]@{path=$_.FullName.Substring($path.Length).TrimStart('\'); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()} }); [pscustomobject]@{root=$path; files=$items.Count; bytes=($items | Measure-Object -Property bytes -Sum).Sum; items=$items} }
$cacheBefore = Get-TreeSnapshot $cacheRoot
$inputsBefore = [pscustomobject]@{freeze_sha256=$freezeSha; source_files=$sourceChecks; mimalloc_archive_sha256=$archiveSha; mimalloc_source_files=$sourceEntries; cache=$cacheBefore}
$argv = [string[]]@(
  '-S', 'F:/JericoEngine/JericoEngine',
  '-B', 'F:/JericoEngine/JericoEngine/build/n0-validation/original-engine',
  '-G', 'Visual Studio 17 2022',
  '-A', 'x64',
  '-DLY_PACKAGE_SERVER_URLS:STRING=',
  '-DLY_3RDPARTY_PATH:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps',
  '-DLY_PACKAGE_DOWNLOAD_CACHE_LOCATION:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache',
  '-DLY_PACKAGE_VALIDATE_CONTENTS=ON',
  '-DLY_PACKAGE_VALIDATE_PACKAGE=ON',
  '-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON',
  '-DJE_PYTHON_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python',
  '-DJE_PYTHON_WHEELHOUSE:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache/python-wheelhouse',
  '-DCMAKE_MESSAGE_LOG_LEVEL:STRING=VERBOSE',
  '-DFETCHCONTENT_SOURCE_DIR_MIMALLOC:PATH=F:/JericoEngine/Dependencies/mimalloc-3.5.2'
)
if ($argv.Count -ne 18) { throw "Argv count guard failed: $($argv.Count), expected 18" }
if ($argv[0] -ne '-S' -or $argv[2] -ne '-B' -or $argv[16] -ne '-DCMAKE_MESSAGE_LOG_LEVEL:STRING=VERBOSE' -or $argv[17] -ne '-DFETCHCONTENT_SOURCE_DIR_MIMALLOC:PATH=F:/JericoEngine/Dependencies/mimalloc-3.5.2') { throw 'Argv position guard failed' }
$argvRecord = [pscustomobject]@{cwd=(Get-Location).Path; expected_cwd=$expectedCwd; executable='C:\Program Files\CMake\bin\cmake.exe'; argv=$argv; argv_count=$argv.Count; mode='protected root configure with audited local mimalloc'; inputs_before=$inputsBefore; started=(Get-Date).ToString('o')}
$argvRecord | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$prefix-argv-before.json" -Encoding utf8
$inputsBefore | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$prefix-inputs-before.json" -Encoding utf8
$timer = [System.Diagnostics.Stopwatch]::StartNew()
& 'C:\Program Files\CMake\bin\cmake.exe' @argv 2>&1 | Tee-Object -FilePath $logPath
$nativeExit = $LASTEXITCODE
$timer.Stop()
$elapsed = $timer.Elapsed.TotalSeconds
$nativeExit | Set-Content -LiteralPath "$prefix.exitcode" -NoNewline
$elapsed.ToString('F6',[Globalization.CultureInfo]::InvariantCulture) | Set-Content -LiteralPath "$prefix.elapsed.txt" -NoNewline
$sourceAfter = @($freeze | ForEach-Object { $p = Join-Path $root $_.path; $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant(); [pscustomobject]@{path=$_.path; expected=$_.sha256; sha256=$h; match=($h -eq $_.sha256)} })
$sourceEntriesAfter = @($tarAudit.entries | Where-Object { $_.type -eq 'file' } | ForEach-Object { $p = Join-Path $sourceRoot ($_.path.Substring('mimalloc-3.5.2/'.Length).Replace('/', '\')); $h = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant(); [pscustomobject]@{path=$_.path; expected=$_.sha256; sha256=$h; match=($h -eq $_.sha256)} })
$cacheAfter = Get-TreeSnapshot $cacheRoot
$inputsAfter = [pscustomobject]@{freeze_sha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash; source_files=$sourceAfter; mimalloc_archive_sha256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant(); mimalloc_source_files=$sourceEntriesAfter; cache=$cacheAfter}
$inputsAfter | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$prefix-inputs-after.json" -Encoding utf8
$result = [pscustomobject]@{exitcode=$nativeExit; elapsed_seconds=$elapsed; argv_count=$argv.Count; freeze_match_before=(@($sourceChecks | Where-Object { -not $_.match }).Count -eq 0); freeze_match_after=(@($sourceAfter | Where-Object { -not $_.match }).Count -eq 0); mimalloc_source_match_before=(@($sourceEntries | Where-Object { -not $_.match }).Count -eq 0); mimalloc_source_match_after=(@($sourceEntriesAfter | Where-Object { -not $_.match }).Count -eq 0); cache_files_before=$cacheBefore.files; cache_files_after=$cacheAfter.files; cache_bytes_before=$cacheBefore.bytes; cache_bytes_after=$cacheAfter.bytes; cache_identical=(@(Compare-Object -ReferenceObject $cacheBefore.items -DifferenceObject $cacheAfter.items -Property path,bytes,sha256).Count -eq 0)}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$prefix-result.json" -Encoding utf8

