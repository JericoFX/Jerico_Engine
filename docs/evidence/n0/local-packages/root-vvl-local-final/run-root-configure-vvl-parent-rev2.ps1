#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference = 'Stop'
function Get-TaskInputHash {
    param([string]$LiteralPath, [string]$Algorithm='SHA256')
    if ($Algorithm -ne 'SHA256') { throw 'Only SHA256 is assigned' }
    $stream = [IO.File]::OpenRead($LiteralPath)
    try { [pscustomobject]@{Hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream));Algorithm='SHA256';Path=$LiteralPath} }
    finally { $stream.Dispose() }
}
$repo = 'F:\JericoEngine\JericoEngine'
$evidenceBase = Join-Path $repo 'build/n0-validation/local-packages/root-vvl-local-final'
$evidence = Join-Path $evidenceBase 'attempt1'
if (Test-Path -LiteralPath $evidence) { throw 'fresh attempt1 evidence directory already exists' }
[IO.Directory]::CreateDirectory($evidence) | Out-Null
$assignmentPath = Join-Path $repo 'build/n0-validation/local-packages/root-vvl-local-final/root-batch-assignment.json'
$freezePath = Join-Path $repo 'build/n0-validation/local-packages/root-vvl-local-final/source-freeze-root-batch.json'
$assignmentSha = '7C4BAE03D9641E44F1AE0A4F7DD615081A357DBBAB176FFE5A2FCF55438D572A'
$freezeSha = 'C92B8061F8E5E0FB48FBD419DBCB51DA2689355C6F9322B6A9282F4B73FF8A2E'
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash -ne $assignmentSha) { throw 'assignment JSON SHA mismatch' }
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $freezePath).Hash -ne $freezeSha) { throw 'root freeze SHA mismatch' }
$assignment = Get-Content -Raw -LiteralPath $assignmentPath | ConvertFrom-Json
$freeze = Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$arguments = [string[]]$assignment.arguments
if ($arguments.Count -ne 61 -or $freeze.Count -ne 15597) { throw "argv/freeze count guard failed: $($arguments.Count)/$($freeze.Count)" }
if ($arguments[0] -ne '-S' -or $arguments[2] -ne '-B') { throw 'CMake mode guard failed' }
if ($arguments -notcontains '-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON' -or $arguments -notcontains '-DLY_PACKAGE_SERVER_URLS:STRING=') { throw 'offline argv guards missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST:PATH=F:/JericoEngine/Dependencies/googletest-v1.18.0') { throw 'exact local GoogleTest override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_ANTLR4:PATH=F:/JericoEngine/Dependencies/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b') { throw 'exact local ANTLR4 override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_JSONCPP:PATH=F:/JericoEngine/Dependencies/jsoncpp-1.6.2') { throw 'exact local JSONCpp override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_CLI11:PATH=F:/JericoEngine/Dependencies/cli11-2.6.2') { throw 'exact local CLI11 override missing' }
$expectedPriorArguments = @(
    '-S',
    'F:/JericoEngine/JericoEngine',
    '-B',
    'F:/JericoEngine/JericoEngine/build/n0-validation/original-engine',
    '-G',
    'Visual Studio 17 2022',
    '-A',
    'x64',
    '-DLY_PACKAGE_SERVER_URLS:STRING=',
    '-DLY_3RDPARTY_PATH:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps',
    '-DLY_PACKAGE_DOWNLOAD_CACHE_LOCATION:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache',
    '-DLY_PACKAGE_VALIDATE_CONTENTS=ON',
    '-DLY_PACKAGE_VALIDATE_PACKAGE=ON',
    '-DFETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON',
    '-DJE_PYTHON_ROOT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine-deps/Python-rev3',
    '-DJE_PYTHON_WHEELHOUSE:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/root-package-cache/python-wheelhouse',
    '-DCMAKE_MESSAGE_LOG_LEVEL:STRING=VERBOSE',
    '-DFETCHCONTENT_SOURCE_DIR_MIMALLOC:PATH=F:/JericoEngine/Dependencies/mimalloc-3.5.2',
    '-DFETCHCONTENT_SOURCE_DIR_RAPIDJSON:PATH=F:/JericoEngine/Dependencies/rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f',
    '-DLY_PROJECTS:STRING=F:/JericoEngine/JericoEngine/Projects/JEEditorFixture',
    '-DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK:PATH=F:/JericoEngine/Dependencies/googlebenchmark-1.7.0',
    '-DFETCHCONTENT_SOURCE_DIR_SQLITE:PATH=F:/JericoEngine/Dependencies/sqlite-amalgamation-3530400',
    '-DFETCHCONTENT_SOURCE_DIR_MANIFOLD:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/manifold-3.5.1-root',
    '-DFETCHCONTENT_SOURCE_DIR_CLIPPER2:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root',
    '-DFETCHCONTENT_SOURCE_DIR_Clipper2:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/clipper2-46f639177fe418f9689e8ddb74f08a870c71f5b4-root',
    '-DFETCHCONTENT_SOURCE_DIR_OPENMESH:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/OpenMesh-11.0.0-root',
    '-DMANIFOLD_CROSS_SECTION:BOOL=ON',
    '-DMANIFOLD_CROSS_SECTION_BACKEND:STRING=clipper2',
    '-DMANIFOLD_USE_BUILTIN_CLIPPER2:BOOL=ON',
    '-DMANIFOLD_DOWNLOADS:BOOL=OFF',
    '-DMANIFOLD_PAR:BOOL=OFF',
    '-DMANIFOLD_TEST:BOOL=OFF',
    '-DMANIFOLD_CBIND:BOOL=OFF',
    '-DMANIFOLD_PYBIND:BOOL=OFF',
    '-DMANIFOLD_FUZZ:BOOL=OFF',
    '-DTRACY_ENABLE:BOOL=OFF',
    '-DASSIMP_ENABLE:BOOL=OFF',
    '-DMANIFOLD_NO_IOSTREAM:BOOL=OFF',
    '-DCLIPPER2_UTILS:BOOL=OFF',
    '-DCLIPPER2_EXAMPLES:BOOL=OFF',
    '-DCLIPPER2_TESTS:BOOL=OFF',
    '-DCLIPPER2_USINGZ:STRING=OFF',
    '-DUSE_EXTERNAL_GBENCHMARK:BOOL=OFF',
    '-DOPENMESH_BENCHMARK_DIR:PATH=',
    '-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST:PATH=F:/JericoEngine/Dependencies/googletest-v1.18.0',
    '-DFETCHCONTENT_SOURCE_DIR_ANTLR4:PATH=F:/JericoEngine/Dependencies/antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b',
    '-DFETCHCONTENT_SOURCE_DIR_JSONCPP:PATH=F:/JericoEngine/Dependencies/jsoncpp-1.6.2',
    '-DFETCHCONTENT_SOURCE_DIR_CLI11:PATH=F:/JericoEngine/Dependencies/cli11-2.6.2'
)
for ($index = 0; $index -lt 48; $index++) {
    if ($arguments[$index] -cne $expectedPriorArguments[$index]) { throw "prior48 argv prefix differs at $index" }
}
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_XXHASH:PATH=F:/JericoEngine/Dependencies/xxhash-0.8.3') { throw 'exact local xxhash override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_D3D12MEMORYALLOCATOR:PATH=F:/JericoEngine/Dependencies/D3D12MemoryAllocator-3.2.0') { throw 'exact local D3D12MemoryAllocator override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_MESHOPTIMIZER:PATH=F:/JericoEngine/Dependencies/meshoptimizer-1.2') { throw 'exact local meshoptimizer override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_SQUISH-CCR:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/squish-ccr-batch-root') { throw 'exact local squish-ccr override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_SSE2NEON:PATH=F:/JericoEngine/Dependencies/sse2neon-1.9.1') { throw 'exact local sse2neon override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_MIKKTSPACE:PATH=F:/JericoEngine/Dependencies/mikktspace-3e895b49d05ea07e4c2133156cfa94369e19e409') { throw 'exact local mikktspace override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_OGG:PATH=F:/JericoEngine/Dependencies/ogg-1.3.6') { throw 'exact local ogg override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_VORBIS:PATH=F:/JericoEngine/Dependencies/vorbis-1.3.7') { throw 'exact local vorbis override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_MINIAUDIO:PATH=F:/JericoEngine/Dependencies/miniaudio-0.11.22') { throw 'exact local miniaudio override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_V-HACD:PATH=F:/JericoEngine/Dependencies/v-hacd-4.1.0') { throw 'exact local v-hacd override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_RECASTNAVIGATION:PATH=F:/JericoEngine/Dependencies/RecastNavigation-1.6.0') { throw 'exact local RecastNavigation override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_ASSIMP:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/assimp-6.0.4-batch-root-rev2') { throw 'exact local assimp override missing' }
if ($arguments -notcontains '-DFETCHCONTENT_SOURCE_DIR_TINYUSDZ_REPO:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/original-engine/_deps/tinyusdz_repo-src') { throw 'exact local tinyusdz_repo override missing' }
$mcppRev2Path = Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-root-current17.json'
$mcppRev2ExpectedSha = '99A6E519ABB8E7E39C7DE401DBAD1E86D57C07A55A83C656646ABB8177A45361'
if (-not (Test-Path -LiteralPath $mcppRev2Path -PathType Leaf) -or (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash -ne $mcppRev2ExpectedSha) { throw 'mcpp rev2 freeze changed or missing before configure' }
$mcppRev2 = Get-Content -Raw -LiteralPath $mcppRev2Path | ConvertFrom-Json
if ($mcppRev2.inputCount -ne 17) { throw "mcpp freeze input count mismatch: $($mcppRev2.inputCount)" }
foreach ($item in $mcppRev2.inputs) {
    if (-not (Test-Path -LiteralPath $item.path -PathType Leaf)) { throw "mcpp frozen input missing: $($item.path)" }
    $fi = Get-Item -LiteralPath $item.path
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { throw "mcpp frozen input mismatch: $($item.path)" }
}
$googleTestFreezePath = Join-Path $repo 'build/n0-validation/local-packages/googletest-audit/source-freeze-googletest-current.json'
$googleTestFreezeSha = '9238D2FFB4B465D196167504CBDD9AB7134F6167D6AD2374C047C1A9F6571FDB'
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $googleTestFreezePath).Hash -ne $googleTestFreezeSha) { throw 'GoogleTest freeze SHA mismatch' }
$googleTestFreeze = Get-Content -Raw -LiteralPath $googleTestFreezePath | ConvertFrom-Json
if ($googleTestFreeze.fileCount -ne 260) { throw "GoogleTest freeze count mismatch: $($googleTestFreeze.fileCount)" }
foreach ($item in $googleTestFreeze.files) {
    if (-not (Test-Path -LiteralPath $item.path -PathType Leaf)) { throw "GoogleTest frozen input missing: $($item.path)" }
    $fi = Get-Item -LiteralPath $item.path
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { throw "GoogleTest frozen input mismatch: $($item.path)" }
}
$splashFreezePath = Join-Path $repo 'build/n0-validation/local-packages/startup-splash-audit/source-freeze.json'
$splashFreezeSha = 'BEBD49E54ECEDC444898E4D1C17F7FC5101DB00CCB34DBCF39A2D0F2DE876DB2'
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $splashFreezePath).Hash -ne $splashFreezeSha) { throw 'splash freeze SHA mismatch' }
$splashFreeze = Get-Content -Raw -LiteralPath $splashFreezePath | ConvertFrom-Json
if ($splashFreeze.inputCount -ne 6) { throw "splash freeze count mismatch: $($splashFreeze.inputCount)" }
foreach ($item in $splashFreeze.inputs) {
    $path = Join-Path $repo $item.path
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "splash frozen input missing: $path" }
    $fi = Get-Item -LiteralPath $path
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($fi.Length -ne $item.bytes -or $hash -ne $item.sha256) { throw "splash frozen input mismatch: $path" }
}
$rootBad = @()
foreach ($item in $freeze) {
    $path = if ([IO.Path]::IsPathRooted($item.path)) { $item.path } else { Join-Path $repo $item.path }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $rootBad += "MISSING $path"; continue }
    $fi = Get-Item -LiteralPath $path
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
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
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $item.path).Hash.ToLowerInvariant()
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
    $hash = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
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
if (@($cacheFiles | Where-Object { $_.DirectoryName -eq $cache }).Count -ne 34 -or $wheelFiles.Count -ne 44) { throw 'local archive cache/wheelhouse counts changed' }
function Write-FileManifest([string]$name,[string[]]$paths) {
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { [void]$rows.Add([pscustomobject]@{path=$path;exists=$false}); continue }
        $fi = Get-Item -LiteralPath $path
        [void]$rows.Add([pscustomobject]@{path=$path;exists=$true;bytes=$fi.Length;sha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()})
    }
    [ordered]@{name=$name;fileCount=$rows.Count;files=$rows.ToArray()} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name.json") -Encoding utf8
}
function Write-TreeManifest([string]$name,[string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { throw "snapshot directory missing: $path" }
    $full = (Resolve-Path -LiteralPath $path).Path
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($fi in Get-ChildItem -LiteralPath $full -File -Recurse -Force) {
        $relative = $fi.FullName.Substring($full.Length).TrimStart('\')
        [void]$rows.Add([pscustomobject]@{relativePath=$relative;bytes=$fi.Length;sha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $fi.FullName).Hash.ToLowerInvariant()})
    }
    [ordered]@{name=$name;path=$full;fileCount=$rows.Count;files=$rows.ToArray()} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence "$name.json") -Encoding utf8
}
$rootPaths = @($freeze | ForEach-Object { if ([IO.Path]::IsPathRooted($_.path)) { $_.path } else { Join-Path $repo $_.path } })
Write-FileManifest 'root-freeze-before' $rootPaths
Write-FileManifest 'googlebenchmark-freeze-before' @($googleFreeze.inputs | ForEach-Object path)
Write-FileManifest 'googletest-freeze-before' @($googleTestFreeze.files | ForEach-Object path)
Write-FileManifest 'mcpp-freeze-before' @($mcppRev2.inputs | ForEach-Object path)
Write-FileManifest 'splash-freeze-before' @($splashFreeze.inputs | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'sqlite-root-inputs-before' @($sqliteRootItems | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'root-package-cache-before' @($cacheFiles | ForEach-Object FullName)
Write-FileManifest 'wheelhouse-before' @($wheelFiles | ForEach-Object FullName)
$splashRccOutput = Join-Path $repo 'build/n0-validation/local-packages/editor-splash-native-validation/green/qrc_StartupLogoDialog.cpp'
Write-FileManifest 'splash-rcc-output-before' @($splashRccOutput)
$treePaths = @(
    @{name='mimalloc-source-before';path='F:\JericoEngine\Dependencies\mimalloc-3.5.2'},
    @{name='rapidjson-source-before';path='F:\JericoEngine\Dependencies\rapidjson-24b5e7a8b27f42fa16b96fc70aade9106cf7102f'},
    @{name='googlebenchmark-tree-before';path='F:\JericoEngine\Dependencies\googlebenchmark-1.7.0'},
    @{name='googletest-tree-before';path='F:\JericoEngine\Dependencies\googletest-v1.18.0'},
    @{name='antlr4-tree-before';path='F:\JericoEngine\Dependencies\antlr4-8e6fd9147b3c9d36b60e2b6656871a55227efb1b'},
    @{name='jsoncpp-tree-before';path='F:\JericoEngine\Dependencies\jsoncpp-1.6.2'},
    @{name='cli11-tree-before';path='F:\JericoEngine\Dependencies\cli11-2.6.2'},
    @{name='batch-xxhash-original-before';path='F:\JericoEngine\Dependencies\xxhash-0.8.3'},
    @{name='batch-D3D12MemoryAllocator-original-before';path='F:\JericoEngine\Dependencies\D3D12MemoryAllocator-3.2.0'},
    @{name='batch-meshoptimizer-original-before';path='F:\JericoEngine\Dependencies\meshoptimizer-1.2'},
    @{name='batch-squish-ccr-original-before';path='F:\JericoEngine\Dependencies\squish-ccr-deb557d2fa647b191b37a2d8682df54ec8a7cfba'},
    @{name='batch-squish-ccr-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\squish-ccr-batch-root'},
    @{name='batch-sse2neon-original-before';path='F:\JericoEngine\Dependencies\sse2neon-1.9.1'},
    @{name='batch-mikktspace-original-before';path='F:\JericoEngine\Dependencies\mikktspace-3e895b49d05ea07e4c2133156cfa94369e19e409'},
    @{name='batch-ogg-original-before';path='F:\JericoEngine\Dependencies\ogg-1.3.6'},
    @{name='batch-vorbis-original-before';path='F:\JericoEngine\Dependencies\vorbis-1.3.7'},
    @{name='batch-miniaudio-original-before';path='F:\JericoEngine\Dependencies\miniaudio-0.11.22'},
    @{name='batch-v-hacd-original-before';path='F:\JericoEngine\Dependencies\v-hacd-4.1.0'},
    @{name='batch-RecastNavigation-original-before';path='F:\JericoEngine\Dependencies\RecastNavigation-1.6.0'},
    @{name='batch-assimp-original-before';path='F:\JericoEngine\Dependencies\assimp-6.0.4'},
    @{name='batch-assimp-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\assimp-6.0.4-batch-root-rev2'},
    @{name='batch-tinyusdz_repo-original-before';path='F:\JericoEngine\Dependencies\tinyusdz-6050eef932f7d2788656d63297aa488fb0961ed1'},
    @{name='batch-tinyusdz_repo-private-before';path='F:\JericoEngine\JericoEngine\build\n0-validation\original-engine\_deps\tinyusdz_repo-src'},
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
$batchPatchPairs = @(
    @{name='squish-ccr';receipt='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\remaining-fetchcontent-batch-acquisition\squish-ccr\private-patch-receipt.json';sha256='085166C2A6EAA6CD0850A5DD80D9CCFBDD8E267BBF1546DB959DCB067E00C456'},
    @{name='assimp';receipt='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\remaining-fetchcontent-batch-acquisition\assimp\private-patch-receipt.json';sha256='4EA45CD24053E2E112D8FE6F1999751F16AB3FFFA2179D8F6670C09DF0842B8E'},
    @{name='tinyusdz_repo';receipt='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\remaining-fetchcontent-batch-acquisition\tinyusdz_repo\private-patch-receipt.json';sha256='D7945870E4F0858CA6EE21D8CF438ADB775599F05E04326808E41F412DF8895F'}
)
function Compare-BatchPatchBaseline([string]$phase) {
    $results = @()
    foreach ($pair in $batchPatchPairs) {
        if ((Get-TaskInputHash -LiteralPath $pair.receipt).Hash -ne $pair.sha256) { throw "assigned patch receipt differs: $($pair.name)" }
        $expected = Get-Content -LiteralPath $pair.receipt -Raw | ConvertFrom-Json
        $source = Get-Content -LiteralPath (Join-Path $evidence "batch-$($pair.name)-original-$phase.json") -Raw | ConvertFrom-Json
        $copy = Get-Content -LiteralPath (Join-Path $evidence "batch-$($pair.name)-private-$phase.json") -Raw | ConvertFrom-Json
        $errors = @()
        foreach ($check in @(@{actual=$source;expected=$expected.immutableBefore;kind='immutable'},@{actual=$copy;expected=$expected.privateBaseline;kind='patched-private'})) {
            $map = @{}; foreach ($row in $check.expected) { $map[$row.relative]=$row }
            foreach ($row in $check.actual.files) {
                $relative = $row.relativePath.Replace('\','/')
                if (-not $map.ContainsKey($relative) -or $map[$relative].sha256 -ne $row.sha256 -or $map[$relative].bytes -ne $row.bytes) { $errors += "$($check.kind):changed-or-added:$relative" }
                $map.Remove($relative)
            }
            foreach ($missing in $map.Keys) { $errors += "$($check.kind):missing:$missing" }
            if ($check.actual.fileCount -ne $expected.fileCount) { $errors += "$($check.kind):file-count-differs" }
        }
        $results += [pscustomobject]@{name=$pair.name;immutableFiles=$source.fileCount;patchedPrivateFiles=$copy.fileCount;expectedPatchedFiles=$expected.changedFiles.Count;baselineErrors=$errors}
    }
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence "batch-private-copy-comparison-$phase.json") -Encoding utf8
    if (@($results | Where-Object { $_.baselineErrors.Count -ne 0 }).Count) { throw "batch assigned patched baseline differs: $phase" }
}
Compare-BatchPatchBaseline 'before'
$packagePairs = @('lz4-1.9.4-je-rev1-windows.tar.xz','OpenSSL-1.1.1o-je-rev1-windows.tar.xz','mcpp-2.7.2_az.2-je-rev1-windows.tar.xz','expat-2.4.2-je-rev1-windows.tar.xz','tiff-4.2.0.15-je-rev1-windows.tar.xz','png-1.6.37-je-rev1-windows.tar.xz','d3dx12-headers-je-rev1-windows.tar.xz','OpenEXR-3.1.3-je-rev1-windows.tar.xz','vulkan-validationlayers-1.3.261-je-rev1-windows.tar.xz')
$archiveRows = @()
foreach ($name in $packagePairs) {
    $path = Join-Path $cache $name
    $sidecar = Get-Content -Raw -LiteralPath "$path.SHA256SUMS"
    $actual = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    $archiveRows += [pscustomobject]@{name=$name;bytes=(Get-Item -LiteralPath $path).Length;sha256=$actual;sidecar=$sidecar.Trim();sidecarMatches=$sidecar.Trim().StartsWith($actual)}
}
if (@($archiveRows | Where-Object { -not $_.sidecarMatches }).Count) { throw 'accepted native root cache sidecar mismatch' }
$archiveRows | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'accepted-native-packages-before.json') -Encoding utf8
$requiredPreflightFiles = @(
    'root-freeze-before.json','googlebenchmark-freeze-before.json','googletest-freeze-before.json',
    'mcpp-freeze-before.json','splash-freeze-before.json','splash-rcc-output-before.json',
    'sqlite-root-inputs-before.json','root-package-cache-before.json','wheelhouse-before.json',
    'private-copy-comparison-before.json','accepted-native-packages-before.json','batch-private-copy-comparison-before.json'
)
foreach($tree in $treePaths){$requiredPreflightFiles += "$($tree.name).json"}
foreach($name in $requiredPreflightFiles){if(-not (Test-Path -LiteralPath (Join-Path $evidence $name) -PathType Leaf)){throw "preflight evidence missing before native invocation: $name"}}
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
$cmake = 'C:/Program Files/CMake/bin/cmake.exe'
if ((Get-TaskInputHash -LiteralPath $cmake).Hash -ne '1AA884BF1F4949327FFFCC8EE4A97C2D684BDC1D0A64B71F01DC16321C7FBC64') { throw 'Audited CMake tool drift' }
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
[ordered]@{path=$cmake;sha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant();fileVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($cmake).FileVersion;exitCode=$versionExit;stdout=$versionOut;stderr=$versionErr} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $evidence 'cmake-tool-version.json') -Encoding utf8
$argvFile = Join-Path $evidence 'configure-argv.json'
$launch = [ordered]@{executable=$cmake;executableSha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $cmake).Hash.ToLowerInvariant();arguments=$arguments;argumentCount=$arguments.Count;assignmentSha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $assignmentPath).Hash.ToLowerInvariant();freezeSha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $freezePath).Hash.ToLowerInvariant();mcppRev2FreezeSha256=(Get-TaskInputHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash.ToLowerInvariant();workingDirectory=$evidence;processEnvironment=@{LY_PACKAGE_SERVER_URLS='';PIP_NO_INDEX='1';PIP_DISABLE_PIP_VERSION_CHECK='1';PIP_CONFIG_FILE='NUL';PYTHONNOUSERSITE='1';FETCHCONTENT_FULLY_DISCONNECTED='ON';removedInherited=@('LY_PACKAGE_DOWNLOAD_CACHE_LOCATION','LY_PACKAGE_UNPACK_PATH','PIP_INDEX_URL','PIP_EXTRA_INDEX_URL','PIP_FIND_LINKS')}}
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
Write-FileManifest 'googletest-freeze-after' @($googleTestFreeze.files | ForEach-Object path)
Write-FileManifest 'mcpp-freeze-after' @($mcppRev2.inputs | ForEach-Object path)
Write-FileManifest 'splash-freeze-after' @($splashFreeze.inputs | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'splash-rcc-output-after' @($splashRccOutput)
Write-FileManifest 'sqlite-root-inputs-after' @($sqliteRootItems | ForEach-Object { Join-Path $repo $_.path })
Write-FileManifest 'root-package-cache-after' @((Get-ChildItem -LiteralPath $cache -File -Force) | ForEach-Object FullName)
Write-FileManifest 'wheelhouse-after' @((Get-ChildItem -LiteralPath $wheelhouse -File -Force) | ForEach-Object FullName)
foreach ($tree in $treePaths) { Write-TreeManifest ($tree.name -replace '-before$','-after') $tree.path }
$comparisonRows = @()
foreach ($name in @('root-freeze','googlebenchmark-freeze','googletest-freeze','mcpp-freeze','splash-freeze','splash-rcc-output','sqlite-root-inputs','root-package-cache','wheelhouse')) {
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
Compare-BatchPatchBaseline 'after'
$mcppAfterSha = (Get-TaskInputHash -Algorithm SHA256 -LiteralPath $mcppRev2Path).Hash
if ($mcppAfterSha -ne $mcppRev2ExpectedSha) { throw 'mcpp rev2 freeze changed during root configure' }
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $googleTestFreezePath).Hash -ne $googleTestFreezeSha) { throw 'GoogleTest freeze changed during root configure' }
if ((Get-TaskInputHash -Algorithm SHA256 -LiteralPath $splashFreezePath).Hash -ne $splashFreezeSha) { throw 'splash freeze changed during root configure' }
[pscustomobject]@{ExitCode=$nativeExit;ElapsedSeconds=$result.elapsedSeconds;PeakCMakeWorkingSetBytes=$peakWorkingSet;Samples=$samples;Evidence=$evidence}

exit $nativeExit
