$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$provider = 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc'
$proofRoot = Join-Path $PSScriptRoot 'private-overlay-proof'
if (Test-Path -LiteralPath $proofRoot) { throw 'Private proof exists' }
$approval = Join-Path $PSScriptRoot 'parent-implementation-approval-dxc-rev1.json'
if ((Get-FileHash -LiteralPath $approval).Hash -ne '31BC4468E2C83C59E19843A1130646C7057A90614115E1079CD9CE000EF782A5') { throw 'Implementation approval changed' }
$dxc = Join-Path $PSScriptRoot 'acquisition/dxc/snapshot'
$tools = Join-Path $PSScriptRoot 'acquisition/spirv-tools/snapshot'
$beforeQueries = @'
def git_get_commit_hash():
    try:
        return subprocess.check_output(
            ['git', 'rev-parse', '--short=8', 'HEAD']).decode('ascii').strip()
    except subprocess.CalledProcessError:
        return "00000000"


def git_get_commit_count():
    try:
        return subprocess.check_output(
            ['git', 'rev-list', '--count', 'HEAD']).decode('ascii').strip()
    except subprocess.CalledProcessError:
        return 0
'@
$afterQueries = @'
def git_get_commit_hash():
    # Audited archive1cc0ab76; original runtime allocations require8 characters.
    return "1cc0ab76"


def git_get_commit_count():
    # Pinned GitHub REST unfiltered history cardinality; not local git rev-list.
    return 4954
'@
$overlays = @(
    @{ id = 'commit-info'; source = 'dxc'; path = 'utils/GetCommitInfo.py'; replacements = @(@{before = "import subprocess`n"; after = ''}, @{before = $beforeQueries; after = $afterQueries}) },
    @{ id = 'fixed-version'; source = 'dxc'; path = 'utils/version/version.inc'; replacements = @(@{before = '#define RC_PRODUCT_VERSION "1.8.2505.0"'; after = '#define RC_PRODUCT_VERSION "1.8.2505.0 JE archive 1cc0ab76dafe08a4144e419e619a8951a3d219db"'}) },
    @{ id = 'spirv-description'; source = 'spirv-tools'; path = 'utils/update_build_version.py'; replacements = @(@{before = '    description = os.getenv("FORCED_BUILD_VERSION_DESCRIPTION", describe(repo_path))'; after = '    description = "a62abcb402009b9ca5975e6167c09f237f630e0e (archive)"'}) }
)
[IO.Directory]::CreateDirectory($proofRoot) | Out-Null
$encoding = [Text.UTF8Encoding]::new($false, $true)
$proofs = @()
foreach ($overlay in $overlays) {
    $snapshot = if ($overlay.source -eq 'dxc') { $dxc } else { $tools }
    $source = Join-Path $snapshot $overlay.path
    [byte[]]$bytes = [IO.File]::ReadAllBytes($source)
    $before = $encoding.GetString($bytes)
    if ($before.Contains("`r")) { throw 'Only audited LF overlay sources permitted' }
    $after = $before
    foreach ($hunk in $overlay.replacements) {
        if ([regex]::Matches($after, [regex]::Escape($hunk.before)).Count -ne 1) { throw 'Overlay context must be exact/unique' }
        $after = $after.Replace($hunk.before, $hunk.after)
    }
    $inverse = $after
    for ($index = $overlay.replacements.Count - 1; $index -ge 0; --$index) {
        $hunk = $overlay.replacements[$index]
        if ($hunk.after -eq '') { $inverse = $inverse.Replace("import os`n", "import os`n" + $hunk.before) }
        else { $inverse = $inverse.Replace($hunk.after, $hunk.before) }
    }
    if (-not [Linq.Enumerable]::SequenceEqual[byte]($encoding.GetBytes($inverse), $bytes)) { throw 'Overlay inverse is not exact bytes' }
    $beforePath = Join-Path $proofRoot ($overlay.id + '.before')
    $afterPath = Join-Path $proofRoot ($overlay.id + '.after')
    [IO.File]::WriteAllBytes($beforePath, $bytes)
    [IO.File]::WriteAllText($afterPath, $after, $encoding)
    $overlay.beforeSHA256 = (Get-FileHash -LiteralPath $beforePath).Hash.ToLowerInvariant()
    $overlay.afterSHA256 = (Get-FileHash -LiteralPath $afterPath).Hash.ToLowerInvariant()
    $proofs += [ordered]@{ id = $overlay.id; immutableSource = $source; beforePath = $beforePath; afterPath = $afterPath; beforeSHA256 = $overlay.beforeSHA256; afterSHA256 = $overlay.afterSHA256; inverseByteExact = $true; upstreamSnapshotUnchanged = (Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant() -eq $overlay.beforeSHA256 }
}
$specPath = Join-Path $provider 'PrivateProvenance.json'
if (Test-Path -LiteralPath $specPath) { throw 'Source overlay specification exists' }
[ordered]@{schemaVersion = 1; overlays = $overlays; historyCountMethod = 'Pinned GitHub REST unfiltered list cardinality4954, not local git rev-list'; originalNumericVersion = '1.8.2505.0'; fixedVersionGuard = 'HLSL_ENABLE_FIXED_VER=ON; HLSL_FIXED_VERSION_LOCATION empty; upstream uses private utils/version/version.inc without invoking gen_version.py'; parentApprovalSHA256 = (Get-FileHash -LiteralPath $approval).Hash } | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $specPath -Encoding utf8
$noticePaths = @(
    @{ root = $dxc; path = 'LICENSE.TXT' }, @{root = $dxc; path = 'ThirdPartyNotices.txt'},
    @{root = $dxc; path = 'lib/Support/COPYRIGHT.regex'}, @{root = $dxc; path = 'lib/DxilCompression/LICENSE.TXT'},
    @{root = (Join-Path $PSScriptRoot 'acquisition/spirv-headers/snapshot'); path = 'LICENSE'}, @{root = $tools; path = 'LICENSE'}
)
$notices = [Text.StringBuilder]::new()
foreach ($item in $noticePaths) {
    [void]$notices.Append("===== $($item.path) [$($item.root)] =====`n")
    [void]$notices.Append([IO.File]::ReadAllText((Join-Path $item.root $item.path)))
    [void]$notices.Append("`n`n")
}
foreach ($relative in @('lib/Support/ConvertUTF.c', 'lib/Support/MD5.cpp', 'lib/DxilCompression/miniz.c', 'lib/DxilCompression/miniz.h', 'include/miniz/miniz.h')) {
    $text = [IO.File]::ReadAllText((Join-Path $dxc $relative))
    $firstDirective = [regex]::Match($text, '(?m)^\s*#(?:include|define|pragma|ifndef|ifdef)\b')
    if (-not $firstDirective.Success) { throw 'No clear notice boundary' }
    [void]$notices.Append("===== full pre-directive source notices: $relative =====`n")
    [void]$notices.Append($text.Substring(0, $firstDirective.Index)); [void]$notices.Append("`n`n")
}
$noticePath = Join-Path $provider 'SourceNotices.txt'
if (Test-Path -LiteralPath $noticePath) { throw 'Source notices exist' }
[IO.File]::WriteAllText($noticePath, $notices.ToString(), $encoding)
$sources = @()
foreach ($id in @('dxc', 'spirv-headers', 'spirv-tools')) {
    $dir = Join-Path $PSScriptRoot "acquisition/$id"
    $receipt = Get-Content -LiteralPath (Join-Path $dir 'source-acquisition.json') -Raw | ConvertFrom-Json
    $snapshot = Join-Path $dir 'snapshot'
    $sources += [ordered]@{ id = $id; repository = $receipt.repository; commit = $receipt.commit; url = $receipt.url; sha256 = $receipt.sha256; archiveRelativePath = "$id/$([IO.Path]::GetFileName($receipt.archivePath))"; archiveRoot = "$($receipt.repository.Split('/')[1])-$($receipt.commit)"; cmakeSHA256 = (Get-FileHash -LiteralPath (Join-Path $snapshot 'CMakeLists.txt')).Hash.ToLowerInvariant() }
}
$pins = [ordered]@{schemaVersion = 1; sources = $sources; privateProvenanceSHA256 = (Get-FileHash -LiteralPath $specPath).Hash.ToLowerInvariant(); sourceNoticeSHA256 = (Get-FileHash -LiteralPath $noticePath).Hash.ToLowerInvariant(); sourceVersion = '1.8.2505.0'; originalRecipeRef = 'release-1.8.2505.1-o3de (branch)'; originalPackageHashNotSourceHash = '224a34691a17234c2294202368d9756635f11d66ac9bc528f929d84cf4acac8e'}
$pinsPath = Join-Path $provider 'SourcePins.json'
if (Test-Path -LiteralPath $pinsPath) { throw 'Pins exist' }
$pins | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $pinsPath -Encoding utf8
[ordered]@{ overlays = $proofs; nativeExecuted = $false; exactInverseCount = 3; parentApprovedAdaptation = 'Private version.inc +fixedON/locationempty; historical external-location proposal would still invoke generatorGit'; fixedVersionCmake = [ordered]@{ path = Join-Path $dxc 'utils/version/CMakeLists.txt'; sha256 = (Get-FileHash -LiteralPath (Join-Path $dxc 'utils/version/CMakeLists.txt')).Hash }; sourceInputs = @($specPath, $pinsPath, $noticePath); snapshotWrites = 0 } | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath (Join-Path $proofRoot 'private-provenance-proof.json') -Encoding utf8
@($specPath, $pinsPath, $noticePath, (Join-Path $proofRoot 'private-provenance-proof.json')) | ForEach-Object { [pscustomobject]@{ path = $_; bytes = (Get-Item -LiteralPath $_).Length; sha256 = (Get-FileHash -LiteralPath $_).Hash } } | ConvertTo-Json
