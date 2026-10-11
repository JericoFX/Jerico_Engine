$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$evidence = Join-Path $repo 'build/n0-validation/local-packages/original-engine-native-python-retirement-configure'
$assignmentPath = Join-Path $repo 'build/n0-validation/local-packages/root-native-python-retirement-assignment.json'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/source-freeze-root-native-python-retirement.json'
$assignmentSha = 'A6C13DCD00EFAA1A965BEA52B499E9B9C76BCC305010842326E93515661AECEF'
$freezeSha = 'AC8E24A1E825FF95A8A90272F50AD92ADD140D7353E317D50DBFEFEDDBF88394'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash -ne $assignmentSha) { throw 'assignment JSON SHA mismatch' }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash -ne $freezeSha) { throw 'root freeze SHA mismatch' }
$assignment = Get-Content -Raw -LiteralPath $assignmentPath | ConvertFrom-Json
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
if ($assignment.arguments.Count -ne 44) { throw "argv count guard failed: $($assignment.arguments.Count)" }
if ($freeze.Count -ne 393) { throw "freeze count guard failed: $($freeze.Count)" }
if ($assignment.arguments[0] -ne '-S' -or $assignment.arguments[2] -ne '-B') { throw 'argv mode guard failed' }
if ($assignment.arguments -notcontains '-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON') { throw 'disconnected guard missing' }
if ($assignment.arguments -notcontains '-DLY_PACKAGE_SERVER_URLS:STRING=') { throw 'empty package-server guard missing' }
if ($assignment.arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_CLIPPER2:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root') { throw 'uppercase Clipper source override missing' }
if ($assignment.arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_Clipper2:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root') { throw 'mixed-case Clipper source override missing' }
$bad = @()
foreach ($item in $freeze) {
    $path = if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $bad += "MISSING $path"; continue }
    $fi = Get-Item -LiteralPath $path
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { $bad += "MISMATCH $path bytes=$($fi.Length) sha=$hash" }
}
if ($bad.Count) { throw "root input freeze failed: $($bad -join '; ')" }
$googleFreezePath = Join-Path $repo 'build/n0-validation/local-packages/googlebenchmark-audit/source-freeze.json'
$googleFreeze = Get-Content -Raw -LiteralPath $googleFreezePath | ConvertFrom-Json
if ($googleFreeze.inputs.Count -ne 174) { throw "GoogleBenchmark input count mismatch: $($googleFreeze.inputs.Count)" }
$googleBad = @()
foreach ($item in $googleFreeze.inputs) {
    if (-not (Test-Path -LiteralPath $item.path -PathType Leaf)) { $googleBad += "MISSING $($item.path)"; continue }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    if ((Get-Item -LiteralPath $item.path).Length -ne $item.bytes -or $hash -ne $item.sha256) { $googleBad += "MISMATCH $($item.path)" }
}
if ($googleBad.Count) { throw "GoogleBenchmark source freeze failed: $($googleBad -join '; ')" }
$rootSqlite = @($freeze | Where-Object { $_.path -match '(^|/)Code/3rdParty/sqlite/|^cmake/Tools/LocalPackages/SQLite/' })
if ($rootSqlite.Count -lt 7) { throw "SQLite root scope unexpectedly has only $($rootSqlite.Count) files" }
$sqliteAuditPath = Join-Path $repo 'build/n0-validation/local-packages/sqlite-3530400-audit/source-freeze-sqlite-current.json'
$sqliteAudit = Get-Content -Raw -LiteralPath $sqliteAuditPath | ConvertFrom-Json
$sqliteArchive = 'F:\JericoEngine\Dependencies\archives\sqlite-amalgamation-3530400.zip'
$sqliteSource = 'F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400'
$sqliteBad = @()
foreach ($item in $sqliteAudit.files) {
    $path = $item.path
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $sqliteBad += "MISSING $path"; continue }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ((Get-Item -LiteralPath $path).Length -ne $item.bytes -or $hash -ne $item.sha256) {
        if ($path -eq 'F:\JericoEngine\JericoEngine\Code\3rdParty\sqlite\CMakeLists.txt' -and @($freeze | Where-Object path -eq 'Code/3rdParty/sqlite/CMakeLists.txt').Count -eq 1) { continue }
        $sqliteBad += "MISMATCH $path"
    }
}
if ($sqliteBad.Count) { throw "SQLite input audit failed: $($sqliteBad -join '; ')" }
$cache = Join-Path $repo 'build/n0-validation/local-packages/root-package-cache'
$wheelhouse = Join-Path $cache 'python-wheelhouse'
$rootPython = Join-Path $repo 'build/n0-validation/original-engine-deps/Python-rev3'
$binary = Join-Path $repo 'build/n0-validation/original-engine'
$outputDir = Join-Path $evidence 'configure-output'
if (Test-Path -LiteralPath $outputDir) { throw 'fresh configure-output directory already exists' }
if (-not (Test-Path -LiteralPath $binary -PathType Container)) { throw "approved binary directory missing: $binary" }
if (-not (Test-Path -LiteralPath $rootPython -PathType Container)) { throw "Python rev3 root missing: $rootPython" }
[IO.Directory]::CreateDirectory($outputDir) | Out-Null
function Get-TreeSnapshot([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { return [pscustomobject]@{ path=$path; exists=$false; fileCount=0; bytes=0; files=@() } }
    $rootPath = (Resolve-Path -LiteralPath $path).Path
    $files = @(Get-ChildItem -LiteralPath $rootPath -File -Recurse -Force)
    $rows = @()
    [long]$total = 0
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($rootPath.Length).TrimStart('\')
        $rows += [pscustomobject]@{ relativePath=$relative; bytes=$file.Length; sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant() }
        $total += $file.Length
    }
    return [pscustomobject]@{ path=$rootPath; exists=$true; fileCount=$files.Count; bytes=$total; files=$rows }
}
function Get-RootInputSnapshot {
    $rows = @()
    foreach ($item in $freeze) {
        $path = if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path }
        $rows += [pscustomobject]@{ path=$item.path; bytes=(Get-Item -LiteralPath $path).Length; sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() }
    }
    return $rows
}
$copyRoots = @(
    'F:\JericoEngine\Dependencies\manifold-3.5.1',
    'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\manifold-3.5.1-root',
    'F:\JericoEngine\Dependencies\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4',
    'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root',
    'F:\JericoEngine\Dependencies\OpenMesh-11.0.0',
    'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\OpenMesh-11.0.0-root'
)
$sourceRoots = @(
    'F:\JericoEngine\Dependencies\mimalloc-3.5.2',
    'F:\JericoEngine\Dependencies\rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f',
    'F:\JericoEngine\Dependencies\googlebenchmark-1.7.0',
    'F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400'
)
$before = [ordered]@{
    timestampUtc=[DateTime]::UtcNow.ToString('o')
    rootFreeze=Get-RootInputSnapshot
    googleBenchmark=Get-TreeSnapshot 'F:\JericoEngine\Dependencies\googlebenchmark-1.7.0'
    sqliteAudit={ fileCount=$sqliteAudit.fileCount; source=Get-TreeSnapshot $sqliteSource; archive=Get-FileHash -Algorithm SHA256 -LiteralPath $sqliteArchive }
    sourceTrees=@($sourceRoots | ForEach-Object { Get-TreeSnapshot $_ })
    privateCopies=@($copyRoots | ForEach-Object { Get-TreeSnapshot $_ })
    packageCache=Get-TreeSnapshot $cache
    pythonRoot=Get-TreeSnapshot $rootPython
    existing3rdPartyPackages=Get-TreeSnapshot (Join-Path $repo 'build/n0-validation/original-engine-deps/packages')
    packageArchives=@('lz4-1.9.4-je-rev1-windows.tar.xz','OpenSSL-1.1.1o-je-rev1-windows.tar.xz') | ForEach-Object { $path=Join-Path $cache $_; [pscustomobject]@{path=$path;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $path).Length;sidecar=(Get-Content -Raw -LiteralPath "$path.SHA256SUMS").Trim()} }
    environmentBefore=@{LY_PACKAGE_SERVER_URLS=$env:LY_PACKAGE_SERVER_URLS;LY_PACKAGE_DOWNLOAD_CACHE_LOCATION=$env:LY_PACKAGE_DOWNLOAD_CACHE_LOCATION;LY_PACKAGE_UNPACK_PATH=$env:LY_PACKAGE_UNPACK_PATH;PIP_INDEX_URL=$env:PIP_INDEX_URL;PIP_EXTRA_INDEX_URL=$env:PIP_EXTRA_INDEX_URL;PIP_CONFIG_FILE=$env:PIP_CONFIG_FILE;PYTHONPATH=$env:PYTHONPATH;FETCHCONTENT_FULLY_DISCONNECTED=$env:FETCHCONTENT_FULLY_DISCONNECTED}
    rootCacheHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash.ToLowerInvariant()
}
$before | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $evidence 'inputs-before.json') -Encoding utf8
$cacheFileNames = @(Get-ChildItem -LiteralPath $cache -File -Force).Name
$wheelNames = @(Get-ChildItem -LiteralPath $wheelhouse -File -Force).Name
if ($cacheFileNames.Count -ne 20 -or $wheelNames.Count -ne 44) { throw "package cache/wheelhouse count changed: $($cacheFileNames.Count)/$($wheelNames.Count)" }
$cmake = (Get-Command cmake).Source
$arguments = [string[]]$assignment.arguments
if ($arguments.Count -ne 44 -or $arguments[3] -ne 'F:/JericoEngine/JericoEngine/build/n0-validation/original-engine') { throw 'final CMake argv guard failed' }
$cmakeVersion = [Diagnostics.ProcessStartInfo]::new()
$cmakeVersion.FileName = $cmake
$cmakeVersion.UseShellExecute = $false
$cmakeVersion.RedirectStandardOutput = $true
$cmakeVersion.RedirectStandardError = $true
[void]$cmakeVersion.ArgumentList.Add('--version')
$versionProc = [Diagnostics.Process]::new()
$versionProc.StartInfo = $cmakeVersion
[void]$versionProc.Start()
$versionOutTask = $versionProc.StandardOutput.ReadToEndAsync()
$versionErrTask = $versionProc.StandardError.ReadToEndAsync()
$versionProc.WaitForExit()
$versionExit = $versionProc.ExitCode
$versionOut = $versionOutTask.GetAwaiter().GetResult()
$versionErr = $versionErrTask.GetAwaiter().GetResult()
$toolVersion = [ordered]@{ path=$cmake; sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant(); fileVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($cmake).FileVersion; versionExit=$versionExit; stdout=$versionOut; stderr=$versionErr }
$toolVersion | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'cmake-tool-version.json') -Encoding utf8
$launch = [ordered]@{ executable=$cmake; workingDirectory=$evidence; arguments=$arguments; argumentCount=$arguments.Count; assignmentJsonSha256=$assignmentSha; sourceFreezeSha256=$freezeSha; tool=$toolVersion; processEnvironment=@{LY_PACKAGE_SERVER_URLS='';PIP_NO_INDEX='1';PIP_DISABLE_PIP_VERSION_CHECK='1';PIP_CONFIG_FILE='NUL';PYTHONNOUSERSITE='1';FETCHCONTENT_FULLY_DISCONNECTED='ON';inheritedOverridesRemoved=@('LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_PATH','PIP_INDEX_URL','PIP_EXTRA_INDEX_URL','PIP_FIND_LINKS')} }
$launch | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $evidence 'configure-argv.json') -Encoding utf8
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
$stdoutPath = Join-Path $evidence 'configure.stdout.log'
$stderrPath = Join-Path $evidence 'configure.stderr.log'
$stdoutStream = [IO.File]::Open($stdoutPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$stderrStream = [IO.File]::Open($stderrPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$proc = [Diagnostics.Process]::new()
$proc.StartInfo = $psi
$watch = [Diagnostics.Stopwatch]::StartNew()
[void]$proc.Start()
$stdoutCopy = $proc.StandardOutput.BaseStream.CopyToAsync($stdoutStream)
$stderrCopy = $proc.StandardError.BaseStream.CopyToAsync($stderrStream)
$peakWorkingSet = 0L
$peakVirtual = 0L
$samples = 0
while (-not $proc.HasExited) {
    try {
        $proc.Refresh()
        if ($proc.WorkingSet64 -gt $peakWorkingSet) { $peakWorkingSet = $proc.WorkingSet64 }
        if ($proc.VirtualMemorySize64 -gt $peakVirtual) { $peakVirtual = $proc.VirtualMemorySize64 }
        $samples++
    } catch {}
    Start-Sleep -Milliseconds 1000
}
$proc.WaitForExit()
$nativeExit = $proc.ExitCode
$watch.Stop()
$stdoutCopy.GetAwaiter().GetResult()
$stderrCopy.GetAwaiter().GetResult()
$stdoutStream.Dispose()
$stderrStream.Dispose()
$after = [ordered]@{
    timestampUtc=[DateTime]::UtcNow.ToString('o')
    rootFreeze=Get-RootInputSnapshot
    googleBenchmark=Get-TreeSnapshot 'F:\JericoEngine\Dependencies\googlebenchmark-1.7.0'
    sqliteAudit={ fileCount=$sqliteAudit.fileCount; source=Get-TreeSnapshot $sqliteSource; archive=Get-FileHash -Algorithm SHA256 -LiteralPath $sqliteArchive }
    sourceTrees=@($sourceRoots | ForEach-Object { Get-TreeSnapshot $_ })
    privateCopies=@($copyRoots | ForEach-Object { Get-TreeSnapshot $_ })
    packageCache=Get-TreeSnapshot $cache
    pythonRoot=Get-TreeSnapshot $rootPython
    existing3rdPartyPackages=Get-TreeSnapshot (Join-Path $repo 'build/n0-validation/original-engine-deps/packages')
    packageArchives=@('lz4-1.9.4-je-rev1-windows.tar.xz','OpenSSL-1.1.1o-je-rev1-windows.tar.xz') | ForEach-Object { $path=Join-Path $cache $_; [pscustomobject]@{path=$path;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $path).Length;sidecar=(Get-Content -Raw -LiteralPath "$path.SHA256SUMS").Trim()} }
}
$after | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $evidence 'inputs-after.json') -Encoding utf8
$summary = [ordered]@{
    exitCode=$nativeExit
    elapsedSeconds=[Math]::Round($watch.Elapsed.TotalSeconds,3)
    endedUtc=[DateTime]::UtcNow.ToString('o')
    cmakeParentPeakWorkingSetBytes=$peakWorkingSet
    cmakeParentPeakVirtualBytes=$peakVirtual
    cmakeParentSamples=$samples
    resourceScope='CMake parent process only; child MSBuild/compiler and other processes are not aggregated'
    rootFreezePath=$freezePath
    rootFreezeSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash.ToLowerInvariant()
    rootFreezeCount=$freeze.Count
    googleBenchmarkFreezeCount=$googleFreeze.inputs.Count
    sqliteCurrentRootFreezeCount=$rootSqlite.Count
    exactArgvCount=$arguments.Count
    existingBuildDirectory=$binary
    rootPython=$rootPython
}
$summary | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'configure-result.json') -Encoding utf8
[pscustomobject]@{ExitCode=$nativeExit;ElapsedSeconds=$summary.elapsedSeconds;CMakeParentPeakWorkingSetBytes=$peakWorkingSet;CMakeSamples=$samples;RootFreezeFiles=$freeze.Count;GoogleBenchmarkFiles=$googleFreeze.inputs.Count;SQLiteRootFreezeFiles=$rootSqlite.Count;ArgCount=$arguments.Count;Evidence=$evidence}
