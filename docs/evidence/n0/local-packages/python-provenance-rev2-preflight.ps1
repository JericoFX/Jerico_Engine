$ErrorActionPreference = 'Stop'
$root = 'F:\JericoEngine\JericoEngine'
$workRoot = Join-Path $root 'build\n0-validation\local-packages'
$depRoot = 'F:\JericoEngine\Dependencies'
$freezePath = Join-Path $workRoot 'source-freeze-python-provenance-parent.json'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$freezeRows = @()
foreach ($entry in $freeze) {
  $path = Join-Path $root $entry.path
  $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
  $freezeRows += [pscustomobject]@{ path=$entry.path; expected=$entry.sha256.ToLowerInvariant(); actual=$actual; match=($actual -eq $entry.sha256.ToLowerInvariant()) }
}
$helperPath = Join-Path $workRoot 'python-package-helper.cmake'
if (-not (Test-Path -LiteralPath $helperPath)) { $helperPath = Join-Path $root 'cmake\Tools\LocalPackages\JECreateLocalPackage.cmake' }
$helperActual = (Get-FileHash -LiteralPath $helperPath -Algorithm SHA256).Hash.ToLowerInvariant()
$helperExpected = 'b067c707919808dda7697ce79efec94a31029ff03b6cc383e374209cc8b68e38'
$snapshotNames = @('cpython-python-input-snapshot-rev2.json','expat-python-input-snapshot.json','bzip2-1.0.8-python-input-snapshot.json','sqlite-3.40.1.0-python-input-snapshot.json','xz-5.2.5-python-input-snapshot.json','zlib-1.2.13-python-input-snapshot.json','libffi-3.3.0-python-input-snapshot.json','openssl-bin-1.1.1u-python-input-snapshot.json','tcltk-8.6.12.0-python-input-snapshot.json')
$snapshotRows = @()
foreach ($name in $snapshotNames) {
  $snap = Get-Content -LiteralPath (Join-Path $workRoot $name) -Raw | ConvertFrom-Json
  $base = if ($name -eq 'cpython-python-input-snapshot-rev2.json') { Join-Path $workRoot 'source-work\cpython-49965601d6afedafe47cc85556d99b7a24981051-provenance-rev2' } else { $snap.work }
  $expectedMap = @{}
  foreach ($f in $snap.files) { $expectedMap[$f.path.Replace('\','/')] = $f }
  $actualMap = @{}
  foreach ($f in Get-ChildItem -LiteralPath $base -File -Recurse -Force) {
    $rel = [IO.Path]::GetRelativePath($base, $f.FullName).Replace('\','/')
    $actualMap[$rel] = $f
  }
  $bad = @()
  foreach ($rel in $expectedMap.Keys) {
    if (-not $actualMap.ContainsKey($rel)) { $bad += [pscustomobject]@{path=$rel; issue='missing'}; continue }
    $actualFile = $actualMap[$rel]
    $expectedFile = $expectedMap[$rel]
    if ($actualFile.Length -ne [long]$expectedFile.bytes) { $bad += [pscustomobject]@{path=$rel; issue='size'; expected=$expectedFile.bytes; actual=$actualFile.Length}; continue }
    $hash = (Get-FileHash -LiteralPath $actualFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $expectedFile.sha256.ToLowerInvariant()) { $bad += [pscustomobject]@{path=$rel; issue='hash'; expected=$expectedFile.sha256; actual=$hash} }
  }
  $unexpected = @($actualMap.Keys | Where-Object { -not $expectedMap.ContainsKey($_) })
  $sumBytes = ($actualMap.Values | Measure-Object -Property Length -Sum).Sum
  $snapshotRows += [pscustomobject]@{ snapshot=$name; root=$base; expected_files=$expectedMap.Count; actual_files=$actualMap.Count; expected_bytes=(($snap.files | Measure-Object -Property bytes -Sum).Sum); actual_bytes=$sumBytes; mismatches=$bad; unexpected=$unexpected; match=($bad.Count -eq 0 -and $unexpected.Count -eq 0 -and $actualMap.Count -eq $expectedMap.Count) }
}
$manifest = Get-Content -LiteralPath (Join-Path $workRoot 'python-externals-manifest.json') -Raw | ConvertFrom-Json
$archives = @()
$archives += [pscustomobject]@{name='cpython'; path=(Join-Path $depRoot 'archives\cpython-49965601d6afedafe47cc85556d99b7a24981051.tar.gz'); expected='12c564214c8a3b109d294143cb439e8bccb1cf6be677b52e98c5e2c41941ea44'}
$archives += [pscustomobject]@{name='expat'; path=(Join-Path $depRoot 'archives\expat-65a21f2b2a306d29b44e70264aca948aa0454219.tar.gz'); expected='54fd00059462c81e8e3c8eef7e90169fca1b48d89122fbc97039d7007190d9c8'}
foreach ($a in $manifest) { $archives += [pscustomobject]@{name=$a.name; path=$a.archive; expected=$a.sha256} }
$archiveRows = @()
foreach ($a in $archives) {
  $item = Get-Item -LiteralPath $a.path
  $hash = (Get-FileHash -LiteralPath $a.path -Algorithm SHA256).Hash.ToLowerInvariant()
  $archiveRows += [pscustomobject]@{name=$a.name; path=$a.path; bytes=$item.Length; expected=$a.expected.ToLowerInvariant(); actual=$hash; match=($hash -eq $a.expected.ToLowerInvariant())}
}
$toolRows = @()
$tools = @(
  [pscustomobject]@{name='bootstrap';path='C:\Users\Jerico\AppData\Local\Programs\Python\Python312\python.exe';expected='805fd29612c221ebea570d372ec5ed46093c25a1033f1765de9b2f8103868886'},
  [pscustomobject]@{name='msbuild';path='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe';expected='0f7daba445ba37b652ae4180270ab3eff27acbbe5d411d3b05dad51aab16404c'}
)
foreach ($tool in $tools) { $hash=(Get-FileHash -LiteralPath $tool.path -Algorithm SHA256).Hash.ToLowerInvariant(); $toolRows += [pscustomobject]@{name=$tool.name;path=$tool.path;expected=$tool.expected;actual=$hash;match=($hash -eq $tool.expected)} }
$result = [pscustomobject]@{ freeze_sha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash; freeze_rows=$freezeRows; helper_path=$helperPath; helper_expected=$helperExpected; helper_actual=$helperActual; helper_match=($helperExpected -eq $helperActual); snapshots=$snapshotRows; archives=$archiveRows; tools=$toolRows; output_absent=(-not (Test-Path -LiteralPath (Join-Path $workRoot 'python-v143-provenance-rev2'))); source_work='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\source-work\cpython-49965601d6afedafe47cc85556d99b7a24981051-provenance-rev2' }
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $workRoot 'python-provenance-rev2-preflight.json') -Encoding utf8
$result | ConvertTo-Json -Depth 8


