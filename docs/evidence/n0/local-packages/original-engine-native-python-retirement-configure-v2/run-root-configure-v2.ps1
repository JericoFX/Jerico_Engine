$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$evidenceBase = Join-Path $repo 'build/n0-validation/local-packages/original-engine-native-python-retirement-configure-v2'
$evidence = Join-Path $evidenceBase 'attempt2'
if (Test-Path -LiteralPath $evidence) { throw 'fresh attempt2 evidence directory already exists' }
[IO.Directory]::CreateDirectory($evidence) | Out-Null
$assignmentPath = Join-Path $repo 'build/n0-validation/local-packages/root-native-python-retirement-assignment.json'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/source-freeze-root-native-python-retirement.json'
$assignmentSha = 'A6C13DCD00EFAA1A965BEA52B499E9B9C76BCC305010842326E93515661AECEF'
$freezeSha = 'AC8E24A1E825FF95A8A90272F50AD92ADD140D7353E317D50DBFEFEDDBF88394'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash -ne $assignmentSha) { throw 'assignment JSON SHA mismatch' }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash -ne $freezeSha) { throw 'root freeze SHA mismatch' }
$assignment = Get-Content -Raw -LiteralPath $assignmentPath | ConvertFrom-Json
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$arguments = [string[]]$assignment.arguments
if ($arguments.Count -ne 44 -or $freeze.Count -ne 393) { throw "argv/freeze count guard failed: $($arguments.Count)/$($freeze.Count)" }
if ($arguments[0] -ne '-S' -or $arguments[2] -ne '-B') { throw 'CMake mode guard failed' }
if ($arguments -notcontains '-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON' -or $arguments -notcontains '-DLY_PACKAGE_SERVER_URLS:STRING=') { throw 'offline argv guards missing' }
$mcppRev2Path = Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-parent17-rev2.json'
$mcppRev2ExpectedSha = 'D7C8399550C2E48EDB6F6A160F6AC2EFE76C656BCCBACB6636CD35FBEEE596BF'
if (-not (Test-Path -LiteralPath $mcppRev2Path -PathType Leaf) -or (Get-FileHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash -ne $mcppRev2ExpectedSha) { throw 'mcpp rev2 freeze changed or missing before configure' }
$rootBad = @()
foreach ($item in $freeze) {
    $path = if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $rootBad += "MISSING $path"; continue }
    $fi = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { $rootBad += "MISMATCH $path bytes=$($fi.Length) sha=$hash" }
}
if ($rootBad.Count) { throw "root freeze failed: $($rootBad -join '; ')" }
$googleFreezePath = Join-Path $repo 'build/n0-validation/local-packages/googlebenchmark-audit/source-freeze.json'
$googleFreeze = Get-Content -Raw -LiteralPath $googleFreezePath | ConvertFrom-Json
if ($googleFreeze.inputs.Count -ne 174) { throw "GoogleBenchmark freeze count mismatch: $($googleFreeze.inputs.Count)" }
$googleBad = @()
foreach ($item in $googleFreeze.inputs) {
    if (-not (Test-Path -LiteralPath $item.path -PathType Leaf)) { $googleBad += "MISSING $($item.path)"; continue }
    $fi = Get-Item -LiteralPath $item.path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { $googleBad += "MISMATCH $($item.path)" }
}
if ($googleBad.Count) { throw "GoogleBenchmark freeze failed: $($googleBad -join '; ')" }
$sqliteCurrentPath = Join-Path $repo 'build/n0-validation/local-packages/sqlite-3530400-audit/source-freeze-sqlite-current.json'
$sqliteCurrent = Get-Content -Raw -LiteralPath $sqliteCurrentPath | ConvertFrom-Json
$sqliteRootItems = @($freeze | Where-Object { $_.path -match '(^|/)Code/3rdParty/sqlite/|^cmake/Tools/LocalPackages/SQLite/' })
if ($sqliteRootItems.Count -lt 7) { throw "SQLite source scope count mismatch: $($sqliteRootItems.Count)" }
$sqliteBad = @()
foreach ($item in $sqliteCurrent.files) {
    $path = $item.path
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $sqliteBad += "MISSING $path"; continue }
    $fi = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) {
        $currentRootEntry = @($freeze | Where-Object { $_.path -eq 'Code/3rdParty/sqlite/CMakeLists.txt' })
        if ($path -eq 'F:\JericoEngine\JericoEngine\Code\3rdParty\sqlite\CMakeLists.txt' -and $currentRootEntry.Count -eq 1) { continue }
        $sqliteBad += "MISMATCH $path"
    }
}
if ($sqliteBad.Count) { throw "SQLite source audit failed: $($sqliteBad -join '; ')" }
$cache = Join-Path $repo 'build/n0-validation/local-packages/root-package-cache'
$wheelhouse = Join-Path $cache 'python-wheelhouse'
$rootPython = Join-Path $repo 'build/n0-validation/original-engine-deps/Python-rev3'
$enginePackages = Join-Path $repo 'build/n0-validation/original-engine-deps/packages'
$binary = Join-Path $repo 'build/n0-validation/original-engine'
if (-not (Test-Path -LiteralPath $binary -PathType Container)) { throw 'approved root binary directory missing' }
if (-not (Test-Path -LiteralPath $rootPython -PathType Container)) { throw 'local Python rev3 root missing' }
$cacheFiles = @(Get-ChildItem -LiteralPath $cache -File -Force)
$wheelFiles = @(Get-ChildItem -LiteralPath $wheelhouse -File -Force)
if (@($cacheFiles | Where-Object { $_.DirectoryName -eq $cache }).Count -ne 20 -or $wheelFiles.Count -ne 44) { throw 'local archive cache/wheelhouse counts changed' }
function Write-FileManifest([string]$name,[string[]]$paths) {
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { [void]$rows.Add([pscustomobject]@{path=$path;exists=$false}); continue }
        $fi = Get-Item -LiteralPath $path
        [void]$rows.Add([pscustomobject]@{path=$path;exists=$true;bytes=$fi.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()})
    }
    [ordered]@{name=$name;fileCount=$rows.Count;files=$rows.ToArray()} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name.json") -Encoding utf8
}
function Write-TreeManifest([string]$name,[string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { throw "snapshot directory missing: $path" }
    $full = (Resolve-Path -LiteralPath $path).Path
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($fi in Get-ChildItem -LiteralPath $full -File -Recurse -Force) {
        $relative = $fi.FullName.Substring($full.Length).TrimStart('\')
        [void]$rows.Add([pscustomobject]@{relativePath=$relative;bytes=$fi.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $fi.FullName).Hash.ToLowerInvariant()})
    }
    [ordered]@{name=$name;path=$full;fileCount=$rows.Count;files=$rows.ToArray()} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name.json") -Encoding utf8
}
$rootPaths = @($freeze | ForEach-Object { if ([IO.Path]::IsPathRooted($_.path)) { $_.path } else { Join-Path $repo $_.path } })
Write-FileManifest 'root-freeze-before' $rootPaths
Write-FileManifest 'googlebenchmark-freeze-before' @($googleFreeze.inputs | ForEach-Object path)
Write-FileManifest 'sqlite-root-inputs-before' @($sqliteRootItems | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'root-package-cache-before' @($cacheFiles | ForEach-Object FullName)
Write-FileManifest 'wheelhouse-before' @($wheelFiles | ForEach-Object FullName)
$treePaths = @(
    @{name='mimalloc-source-before';path='F:\JericoEngine\Dependencies\mimalloc-3.5.2'},
    @{name='rapidjson-source-before';path='F:\JericoEngine\Dependencies\rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f'},
    @{name='googlebenchmark-tree-before';path='F:\JericoEngine\Dependencies\googlebenchmark-1.7.0'},
    @{name='sqlite-source-before';path='F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400'},
    @{name='manifold-original-before';path='F:\JericoEngine\Dependencies\manifold-3.5.1'},
    @{name='manifold-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\manifold-3.5.1-root'},
    @{name='clipper-original-before';path='F:\JericoEngine\Dependencies\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4'},
    @{name='clipper-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root'},
    @{name='openmesh-original-before';path='F:\JericoEngine\Dependencies\OpenMesh-11.0.0'},
    @{name='openmesh-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\OpenMesh-11.0.0-root'},
    @{name='python-rev3-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\original-engine-deps\Python-rev3'},
    @{name='engine-packages-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\original-engine-deps\packages'}
)
foreach ($tree in $treePaths) { Write-TreeManifest $tree.name $tree.path }
$privateComparison = @()
foreach ($pair in @(
    @{name='Manifold';stem='manifold';source='F:\JericoEngine\Dependencies\manifold-3.5.1';copy='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\manifold-3.5.1-root'},
    @{name='Clipper2';stem='clipper';source='F:\JericoEngine\Dependencies\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4';copy='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root'},
    @{name='OpenMesh';stem='openmesh';source='F:\JericoEngine\Dependencies\OpenMesh-11.0.0';copy='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\OpenMesh-11.0.0-root'}
)) {
    $sourceManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$($pair.stem)-original-before.json") | ConvertFrom-Json
    $copyManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$($pair.stem)-private-before.json") | ConvertFrom-Json
    $srcMap = @{}; foreach ($row in $sourceManifest.files) { $srcMap[$row.relativePath]=$row.sha256 }
    $diff = @(); foreach ($row in $copyManifest.files) { if (-not $srcMap.ContainsKey($row.relativePath) -or $srcMap[$row.relativePath] -ne $row.sha256) { $diff += $row.relativePath }; $srcMap.Remove($row.relativePath) }
    foreach ($extra in $srcMap.Keys) { $diff += "missing-copy:$extra" }
    $privateComparison += [pscustomobject]@{name=$pair.name;sourceFileCount=$sourceManifest.fileCount;copyFileCount=$copyManifest.fileCount;preconfigureDifferences=$diff}
}
$privateComparison | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'private-copy-comparison-before.json') -Encoding utf8
if (@($privateComparison | Where-Object { $_.preconfigureDifferences.Count -ne 0 }).Count) { throw 'private source copies differ from the assigned pristine source before configure' }
$packagePairs = @('lz4-1.9.4-je-rev1-windows.tar.xz','OpenSSL-1.1.1o-je-rev1-windows.tar.xz')
$archiveRows = @()
foreach ($name in $packagePairs) {
    $path = Join-Path $cache $name
    $sidecar = Get-Content -Raw -LiteralPath "$path.SHA256SUMS"
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    $archiveRows += [pscustomobject]@{name=$name;bytes=(Get-Item -LiteralPath $path).Length;sha256=$actual;sidecar=$sidecar.Trim();sidecarMatches=$sidecar.Trim().StartsWith($actual)}
}
if (@($archiveRows | Where-Object { -not $_.sidecarMatches }).Count) { throw 'accepted LZ4/OpenSSL root cache sidecar mismatch' }
$archiveRows | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'accepted-native-packages-before.json') -Encoding utf8
$envPresence = @{}
foreach ($name in @('PIP_INDEX_URL','PIP_EXTRA_INDEX_URL','LY_PACKAGE_SERVER_URLS','LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_PATH')) {
    $value = [Environment]::GetEnvironmentVariable($name,'Process')
    $envPresence[$name] = [pscustomobject]@{present=($null -ne $value);host=if ($value -and $name -like 'PIP_*URL') { try { ([Uri]$value).Host } catch { 'unparseable-redacted' } } else { $null }}
}
$envPresence | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'inherited-environment-presence-redacted.json') -Encoding utf8
$binary = Join-Path $repo 'build/n0-validation/original-engine'
foreach ($file in @('CMakeCache.txt','CMakeFiles/CMakeConfigureLog.yaml','CMakeFiles/CMakeOutput.log')) {
    $source = Join-Path $binary $file
    if (Test-Path -LiteralPath $source -PathType Leaf) { Copy-Item -LiteralPath $source -Destination (Join-Path $evidence ('before-' + ($file -replace '[\\/]','-'))) }
}
$cmake = (Get-Command cmake).Source
$versionPsi = [Diagnostics.ProcessStartInfo]::new()
$versionPsi.FileName = $cmake
$versionPsi.WorkingDirectory = $evidence
$versionPsi.UseShellExecute = $false
$versionPsi.RedirectStandardOutput = $true
$versionPsi.RedirectStandardError = $true
[void]$versionPsi.ArgumentList.Add('--version')
$versionProc = [Diagnostics.Process]::new()
$versionProc.StartInfo = $versionPsi
[void]$versionProc.Start()
$versionOutTask = $versionProc.StandardOutput.ReadToEndAsync()
$versionErrTask = $versionProc.StandardError.ReadToEndAsync()
$versionProc.WaitForExit()
$versionExit = $versionProc.ExitCode
$versionOut = $versionOutTask.GetAwaiter().GetResult()
$versionErr = $versionErrTask.GetAwaiter().GetResult()
[ordered]@{path=$cmake;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant();fileVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($cmake).FileVersion;exitCode=$versionExit;stdout=$versionOut;stderr=$versionErr} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'cmake-tool-version.json') -Encoding utf8
$argvFile = Join-Path $evidence 'configure-argv.json'
$launch = [ordered]@{executable=$cmake;executableSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant();arguments=$arguments;argumentCount=$arguments.Count;assignmentSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash.ToLowerInvariant();freezeSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash.ToLowerInvariant();mcppRev2FreezeSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash.ToLowerInvariant();workingDirectory=$evidence;processEnvironment=@{LY_PACKAGE_SERVER_URLS='';PIP_NO_INDEX='1';PIP_DISABLE_PIP_VERSION_CHECK='1';PIP_CONFIG_FILE='NUL';PYTHONNOUSERSITE='1';FETCHCONTENT_FULLY_DISCONNECTED='ON';removedInherited=@('LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_PATH','PIP_INDEX_URL','PIP_EXTRA_INDEX_URL','PIP_FIND_LINKS')}}
$launch | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $argvFile -Encoding utf8
$psi = [Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cmake
$psi.WorkingDirectory = $evidence
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
foreach ($arg in $arguments) { [void]$psi.ArgumentList.Add($arg) }
foreach ($name in @('LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_PATH','PIP_INDEX_URL','PIP_EXTRA_INDEX_URL','PIP_FIND_LINKS')) { [void]$psi.Environment.Remove($name) }
$psi.Environment['LY_PACKAGE_SERVER_URLS'] = ''
$psi.Environment['PIP_NO_INDEX'] = '1'
$psi.Environment['PIP_DISABLE_PIP_VERSION_CHECK'] = '1'
$psi.Environment['PIP_CONFIG_FILE'] = 'NUL'
$psi.Environment['PYTHONNOUSERSITE'] = '1'
$psi.Environment['FETCHCONTENT_FULLY_DISCONNECTED'] = 'ON'
$stdoutStream = [IO.File]::Open((Join-Path $evidence 'configure.stdout.log'),[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$stderrStream = [IO.File]::Open((Join-Path $evidence 'configure.stderr.log'),[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$process = [Diagnostics.Process]::new()
$process.StartInfo = $psi
$watch = [Diagnostics.Stopwatch]::StartNew()
[void]$process.Start()
$stdoutTask = $process.StandardOutput.BaseStream.CopyToAsync($stdoutStream)
$stderrTask = $process.StandardError.BaseStream.CopyToAsync($stderrStream)
$peakWorkingSet = 0L; $peakVirtual = 0L; $samples = 0
while (-not $process.HasExited) {
    try { $process.Refresh(); if ($process.WorkingSet64 -gt $peakWorkingSet) { $peakWorkingSet=$process.WorkingSet64 }; if ($process.VirtualMemorySize64 -gt $peakVirtual) { $peakVirtual=$process.VirtualMemorySize64 }; $samples++ } catch {}
    Start-Sleep -Milliseconds 1000
}
$process.WaitForExit()
$nativeExit = $process.ExitCode
$watch.Stop()
$stdoutTask.GetAwaiter().GetResult(); $stderrTask.GetAwaiter().GetResult()
$stdoutStream.Dispose(); $stderrStream.Dispose()
$result = [ordered]@{exitCode=$nativeExit;elapsedSeconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);cmakeParentPeakWorkingSetBytes=$peakWorkingSet;cmakeParentPeakVirtualBytes=$peakVirtual;cmakeParentSamples=$samples;memoryScope='direct CMake parent only; child MSBuild/compiler processes are not aggregated';endedUtc=[DateTime]::UtcNow.ToString('o')}
$result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'configure-result.json') -Encoding utf8
Write-FileManifest 'root-freeze-after' $rootPaths
Write-FileManifest 'googlebenchmark-freeze-after' @($googleFreeze.inputs | ForEach-Object path)
Write-FileManifest 'sqlite-root-inputs-after' @($sqliteRootItems | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'root-package-cache-after' @((Get-ChildItem -LiteralPath $cache -File -Force) | ForEach-Object FullName)
Write-FileManifest 'wheelhouse-after' @((Get-ChildItem -LiteralPath $wheelhouse -File -Force) | ForEach-Object FullName)
foreach ($tree in $treePaths) { Write-TreeManifest ($tree.name -replace '-before$','-after') $tree.path }
$comparisonRows = @()
foreach ($name in @('root-freeze','googlebenchmark-freeze','sqlite-root-inputs','root-package-cache','wheelhouse')) {
    $beforeManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$name-before.json") | ConvertFrom-Json
    $afterManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$name-after.json") | ConvertFrom-Json
    $beforeMap = @{}; foreach ($row in $beforeManifest.files) { $beforeMap[$row.path]=$row.sha256 }
    $delta = @(); foreach ($row in $afterManifest.files) { if (-not $beforeMap.ContainsKey($row.path) -or $beforeMap[$row.path] -ne $row.sha256) { $delta += "changed-or-added:$($row.path)" }; $beforeMap.Remove($row.path) }
    foreach ($removed in $beforeMap.Keys) { $delta += "removed:$removed" }
    $comparisonRows += [pscustomobject]@{manifest=$name;beforeCount=$beforeManifest.fileCount;afterCount=$afterManifest.fileCount;deltaCount=$delta.Count;deltas=$delta}
}
foreach ($tree in $treePaths) {
    $beforeManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$($tree.name).json") | ConvertFrom-Json
    $afterName = $tree.name -replace '-before$','-after'
    $afterManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$afterName.json") | ConvertFrom-Json
    $beforeMap = @{}; foreach ($row in $beforeManifest.files) { $beforeMap[$row.relativePath]=$row.sha256 }
    $delta = @(); foreach ($row in $afterManifest.files) { if (-not $beforeMap.ContainsKey($row.relativePath) -or $beforeMap[$row.relativePath] -ne $row.sha256) { $delta += "changed-or-added:$($row.relativePath)" }; $beforeMap.Remove($row.relativePath) }
    foreach ($removed in $beforeMap.Keys) { $delta += "removed:$removed" }
    $comparisonRows += [pscustomobject]@{manifest=$tree.name;beforeCount=$beforeManifest.fileCount;afterCount=$afterManifest.fileCount;deltaCount=$delta.Count;deltas=$delta}
}
$comparisonRows | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'input-postflight-deltas.json') -Encoding utf8
$privateAfter = @()
foreach ($pair in @(
    @{name='Manifold';source='manifold-original-after';copy='manifold-private-after';allowed=@('src/csg_tree.h','src/edge_op.cpp')},
    @{name='Clipper2';source='clipper-original-after';copy='clipper-private-after';allowed=@('CPP/Clipper2Lib/include/clipper2/clipper.version.h')},
    @{name='OpenMesh';source='openmesh-original-after';copy='openmesh-private-after';allowed=@()}
)) {
    $sourceManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$($pair.source).json") | ConvertFrom-Json
    $copyManifest = Get-Content -Raw -LiteralPath (Join-Path $evidence "$($pair.copy).json") | ConvertFrom-Json
    $sourceMap = @{}; foreach ($row in $sourceManifest.files) { $sourceMap[$row.relativePath]=$row.sha256 }
    $deltas = @(); foreach ($row in $copyManifest.files) { if (-not $sourceMap.ContainsKey($row.relativePath) -or $sourceMap[$row.relativePath] -ne $row.sha256) { $deltas += $row.relativePath }; $sourceMap.Remove($row.relativePath) }
    foreach ($removed in $sourceMap.Keys) { $deltas += "missing-copy:$removed" }
    $unexpected = @($deltas | Where-Object { $_ -notin $pair.allowed })
    $privateAfter += [pscustomobject]@{name=$pair.name;sourceFileCount=$sourceManifest.fileCount;copyFileCount=$copyManifest.fileCount;deltas=$deltas;unexpectedDeltas=$unexpected}
}
$privateAfter | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'private-copy-comparison-after.json') -Encoding utf8
$mcppAfterSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash
if ($mcppAfterSha -ne $mcppRev2ExpectedSha) { throw 'mcpp rev2 freeze changed during root configure' }
[pscustomobject]@{ExitCode=$nativeExit;ElapsedSeconds=$result.elapsedSeconds;PeakCMakeWorkingSetBytes=$peakWorkingSet;Samples=$samples;Evidence=$evidence}
