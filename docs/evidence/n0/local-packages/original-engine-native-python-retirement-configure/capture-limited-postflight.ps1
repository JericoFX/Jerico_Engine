$ErrorActionPreference = 'Stop'
$repo = 'F:\JericoEngine\JericoEngine'
$evidence = Join-Path $repo 'build/n0-validation/local-packages/original-engine-native-python-retirement-configure'
function Get-FileRows([string[]]$paths) {
    $rows = @()
    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $rows += [pscustomobject]@{path=$path;exists=$false}; continue }
        $item = Get-Item -LiteralPath $path
        $rows += [pscustomobject]@{path=$path;exists=$true;bytes=$item.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()}
    }
    return $rows
}
function Write-TreeManifest([string]$name,[string]$path) {
    $rows = @()
    if (Test-Path -LiteralPath $path -PathType Container) {
        $rootPath = (Resolve-Path -LiteralPath $path).Path
        foreach ($file in Get-ChildItem -LiteralPath $rootPath -File -Recurse -Force) {
            $rows += [pscustomobject]@{relativePath=$file.FullName.Substring($rootPath.Length).TrimStart('\');bytes=$file.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()}
        }
    }
    $object = [ordered]@{path=$path;exists=(Test-Path -LiteralPath $path -PathType Container);fileCount=$rows.Count;files=$rows}
    $object | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name.json") -Encoding utf8
    return [pscustomobject]@{name=$name;fileCount=$rows.Count}
}
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/source-freeze-root-native-python-retirement.json'
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$freezeRows = @()
$bad = @()
foreach ($item in $freeze) {
    $path = if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $bad += "MISSING $path"; continue }
    $length = (Get-Item -LiteralPath $path).Length
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    $freezeRows += [pscustomobject]@{path=$item.path;bytes=$length;sha256=$hash;matchesFreeze=($length -eq $item.bytes -and $hash -eq $item.sha256)}
    if ($length -ne $item.bytes -or $hash -ne $item.sha256) { $bad += "MISMATCH $path" }
}
[ordered]@{freezePath=$freezePath;freezeSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $freezePath).Hash.ToLowerInvariant();fileCount=$freeze.Count;allMatch=($bad.Count -eq 0);mismatches=$bad;files=$freezeRows} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'root-freeze-postflight.json') -Encoding utf8
$googleFreezePath = Join-Path $repo 'build/n0-validation/local-packages/googlebenchmark-audit/source-freeze.json'
$googleFreeze = Get-Content -Raw -LiteralPath $googleFreezePath | ConvertFrom-Json
$googleRows = Get-FileRows @($googleFreeze.inputs | ForEach-Object path)
$googleBad = @()
for ($i = 0; $i -lt $googleFreeze.inputs.Count; $i++) {
    $expected = $googleFreeze.inputs[$i]
    $actual = $googleRows[$i]
    if (-not $actual.exists -or $actual.bytes -ne $expected.bytes -or $actual.sha256 -ne $expected.sha256) { $googleBad += $expected.path }
}
[ordered]@{freezePath=$googleFreezePath;fileCount=$googleRows.Count;allMatch=($googleBad.Count -eq 0);mismatches=$googleBad;files=$googleRows} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'googlebenchmark-postflight.json') -Encoding utf8
$cache = Join-Path $repo 'build/n0-validation/local-packages/root-package-cache'
$cacheFiles = @(Get-ChildItem -LiteralPath $cache -File -Force | ForEach-Object FullName)
$wheels = @(Get-ChildItem -LiteralPath (Join-Path $cache 'python-wheelhouse') -File -Force | ForEach-Object FullName)
Get-FileRows $cacheFiles | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'root-package-cache-postflight.json') -Encoding utf8
Get-FileRows $wheels | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'wheelhouse-postflight.json') -Encoding utf8
$trees = @(
    @{name='mimalloc-source-postflight';path='F:\JericoEngine\Dependencies\mimalloc-3.5.2'},
    @{name='rapidjson-source-postflight';path='F:\JericoEngine\Dependencies\rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f'},
    @{name='googlebenchmark-source-tree-postflight';path='F:\JericoEngine\Dependencies\googlebenchmark-1.7.0'},
    @{name='sqlite-source-postflight';path='F:\JericoEngine\Dependencies\sqlite-amalgamation-3530400'},
    @{name='manifold-dependency-source-postflight';path='F:\JericoEngine\Dependencies\manifold-3.5.1'},
    @{name='clipper-dependency-source-postflight';path='F:\JericoEngine\Dependencies\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4'},
    @{name='openmesh-dependency-source-postflight';path='F:\JericoEngine\Dependencies\OpenMesh-11.0.0'},
    @{name='manifold-private-copy-postflight';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\manifold-3.5.1-root'},
    @{name='clipper-private-copy-postflight';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root'},
    @{name='openmesh-private-copy-postflight';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\OpenMesh-11.0.0-root'}
)
$treeResults = @()
foreach ($tree in $trees) { $treeResults += Write-TreeManifest $tree.name $tree.path }
$sqliteRoot = @($freeze | Where-Object { $_.path -match '(^|/)Code/3rdParty/sqlite/|^cmake/Tools/LocalPackages/SQLite/' })
$sqlitePaths = @()
foreach ($item in $sqliteRoot) { $sqlitePaths += if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path } }
Get-FileRows $sqlitePaths | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'sqlite-root-inputs-postflight.json') -Encoding utf8
$receipt = [ordered]@{
    status='configure-not-launched'
    attemptedHarness=(Join-Path $evidence 'run-root-configure.ps1')
    attemptedHarnessSha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $evidence 'run-root-configure.ps1')).Hash.ToLowerInvariant()
    error='ConvertTo-Json -Depth 8 at run-root-configure.ps1 line 113 emitted a truncation warning, then failed with Insufficient memory to continue execution before ProcessStartInfo construction/start.'
    nativeCmakeExitCode=$null
    configureStdoutExists=(Test-Path -LiteralPath (Join-Path $evidence 'configure.stdout.log'))
    configureStderrExists=(Test-Path -LiteralPath (Join-Path $evidence 'configure.stderr.log'))
    evidenceScope='current postflight only; no complete before snapshot retained or reconstructed; root freeze393 checked against assigned original, plus separately captured current cache, wheelhouse, GoogleBenchmark, SQLite root files, source trees and private copies'
    googleBenchmarkFiles=$googleFreeze.inputs.Count
    sqliteRootInputFiles=$sqliteRoot.Count
    packageCacheFiles=$cacheFiles.Count
    wheelhouseFiles=$wheels.Count
    copyTreeFiles=1212
    treeCounts=$treeResults
    rootFreezeMismatches=$bad
}
$receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'configure-harness-failure-and-postflight.json') -Encoding utf8
$receipt | ConvertTo-Json -Depth 4
