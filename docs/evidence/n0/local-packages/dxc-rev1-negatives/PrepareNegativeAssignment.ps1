$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($PSScriptRoot)
$attempt = Join-Path $root 'attempt1'
$engine = 'F:\JericoEngine\JericoEngine'
$prep = Join-Path $engine 'build/n0-validation/local-packages/dxc-rev1-preparation'
$previous = Join-Path $prep 'provider-candidate-rev2/validation-assignment.json'
if ((Get-FileHash -LiteralPath $previous).Hash -ne '5968A5F2785CC65BEBA8AD14EAB987C429645F2EC5BB3CCBBE3FA21902CE9585') { throw 'Reviewed positive assignment differs' }
$base = Get-Content -LiteralPath $previous -Raw | ConvertFrom-Json
$assignmentPath = Join-Path $root 'negative-assignment.json'
if (Test-Path -LiteralPath $assignmentPath) { throw 'Assignment exists; preserve it' }
function Full([string] $path) { [IO.Path]::GetFullPath($path) }
$empty = Full (Join-Path $attempt 'fixtures/empty-archives')
$existing = Full (Join-Path $attempt 'fixtures/existing-output')
$outside = Full (Join-Path $engine 'JE_DXC_negative_never_created')
$definitions = @(
    @{ label = 'missing-archive'; changes = @{ JE_DXC_ARCHIVES_ROOT = $empty }; error = 'dxc exact local archive file is missing' },
    @{ label = 'archive-sha'; changes = @{ JE_DXC_ARCHIVES_ROOT = (Full (Join-Path $attempt 'fixtures/changed-archives')) }; error = 'dxc exact local archive SHA256 mismatch' },
    @{ label = 'missing-tool'; changes = @{ JE_DXC_PYTHON = (Full (Join-Path $attempt 'fixtures/missing-python.exe')) }; error = 'audited existing external PYTHON file is missing' },
    @{ label = 'python-sha'; changes = @{ JE_DXC_PYTHON_SHA256 = ('0' * 64) }; error = 'audited existing external PYTHON SHA256 mismatch' },
    @{ label = 'ninja-sha'; changes = @{ JE_DXC_NINJA_SHA256 = ('0' * 64) }; error = 'audited existing external NINJA SHA256 mismatch' },
    @{ label = 'cl-sha'; changes = @{ JE_DXC_CL_SHA256 = ('0' * 64) }; error = 'audited existing external CL SHA256 mismatch' },
    @{ label = 'uri'; changes = @{ JE_DXC_ARCHIVES_ROOT = 'https://example.invalid/JE_DXC_never_contacted' }; error = 'JE_DXC_ARCHIVES_ROOT must not be a URI' },
    @{ label = 'unc'; changes = @{ JE_DXC_ARCHIVES_ROOT = '//localhost/JE_DXC_never_shared/archives' }; error = 'JE_DXC_ARCHIVES_ROOT must be an absolute local path' },
    @{ label = 'junction'; changes = @{ JE_DXC_ARCHIVES_ROOT = (Full (Join-Path $attempt 'fixtures/archive-junction')) }; error = 'JE_DXC_ARCHIVES_ROOT must not use symlink paths' },
    @{ label = 'ancestor-reparse'; changes = @{ JE_DXC_ARCHIVES_ROOT = (Full (Join-Path $attempt 'fixtures/archive-junction/missing-child')) }; error = 'JE_DXC_ARCHIVES_ROOT must not use symlink paths' },
    @{ label = 'existing-output'; changes = @{ JE_DXC_OUTPUT = $existing; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC output exists; previous attempts are retained' },
    @{ label = 'outside-build'; changes = @{ JE_DXC_OUTPUT = $outside; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC output must be below checkout/build' },
    @{ label = 'absent-vs'; changes = @{ JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC requires the existing VS2022/v143 x64 development environment'; unsetVS = $true },
    @{ label = 'jobs-absent'; changes = @{ JE_DXC_PARALLEL_JOBS = $null; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC: JE_DXC_PARALLEL_JOBS is required' },
    @{ label = 'jobs-zero'; changes = @{ JE_DXC_PARALLEL_JOBS = '0'; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC: JE_DXC_PARALLEL_JOBS must be a positive integer' },
    @{ label = 'jobs-negative'; changes = @{ JE_DXC_PARALLEL_JOBS = '-1'; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC: JE_DXC_PARALLEL_JOBS must be a positive integer' },
    @{ label = 'jobs-noninteger'; changes = @{ JE_DXC_PARALLEL_JOBS = 'six'; JE_DXC_ARCHIVES_ROOT = $empty }; error = 'JE DXC: JE_DXC_PARALLEL_JOBS must be a positive integer' }
)
$cases = @(foreach ($definition in $definitions) {
    $vars = [ordered]@{}
    foreach ($arg in $base.producer.argv) { if ($arg -match '^-D([^=]+)=(.*)$') { $vars[$Matches[1]] = $Matches[2] } }
    $output = Full (Join-Path $attempt "not-created-$($definition.label)")
    $vars['JE_DXC_OUTPUT'] = $output
    foreach ($key in $definition.changes.Keys) {
        if ($null -eq $definition.changes[$key]) { $vars.Remove($key) }
        else { $vars[$key] = $definition.changes[$key] }
    }
    $output = [string]$vars['JE_DXC_OUTPUT']
    $absent = if ($definition.label -eq 'existing-output') { @('logs','source-work','build-Release','stage','packages','package-ready.json') | ForEach-Object { Full (Join-Path $output $_) } } else { @($output) }
    $argv = @($vars.GetEnumerator() | ForEach-Object { "-D$($_.Key)=$($_.Value)" }) + @('-P',$base.producer.argv[-1])
    [ordered]@{ label = $definition.label; executable = $base.producer.executable; argv = $argv; expectedError = $definition.error; childEnvironment = @{ VSCMD_ARG_TGT_ARCH = if ($definition.ContainsKey('unsetVS')) { $null } else { 'x64' } }; mustRemainAbsent = @($absent); allowedEmptyProof = $null }
})
$overlayProof = Full (Join-Path $attempt 'overlay-proof')
$cases += [ordered]@{ label = 'overlay-mismatch'; executable = $base.producer.executable; argv = @("-DJE_WORK_dxc=$(Full (Join-Path $attempt 'fixtures/overlay-work/dxc'))","-DJE_WORK_spirv-tools=$(Full (Join-Path $attempt 'fixtures/overlay-work/spirv-tools'))","-DJE_DXC_PROOF_OUTPUT=$overlayProof",'-P',(Full (Join-Path $engine 'cmake/Tools/LocalPackages/DirectXShaderCompilerDxc/ApplyPrivateProvenance.cmake'))); expectedError = 'commit-info pristine private source SHA256 mismatch'; childEnvironment = @{}; mustRemainAbsent = @('stage','packages','build-Release','package-ready.json') | ForEach-Object { Full (Join-Path $attempt $_) }; allowedEmptyProof = $overlayProof }
if ($cases.Count -ne 18) { throw 'Case count changed' }
$witnessPaths = @(
    (Full (Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz')),
    (Full (Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/packages/DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz.SHA256SUMS')),
    (Full (Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/logs/configure-Release.result.json')),
    (Full (Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/logs/build-Release.result.json')),
    (Full (Join-Path $engine 'build/n0-validation/local-packages/dxc-local-rev1/package-ready.json'))
)
if ((Get-FileHash -LiteralPath $witnessPaths[0]).Hash -ne '62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782') { throw 'Reported real production archive differs' }
foreach ($path in $witnessPaths[2..3]) { if ((Get-Content -LiteralPath $path -Raw | ConvertFrom-Json).exitResult -ne '0') { throw 'Actual producer native stage did not return zero' } }
$witnesses = @(foreach ($path in $witnessPaths) { [ordered]@{ path = $path; bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-FileHash -LiteralPath $path).Hash } })
$assignment = [ordered]@{
    status = 'PREPARED ONLY; NOT RUN'; nativeOwner = 'Luna only after parent review'; nativeExecuted = $false; acceptance = $false
    sourceFreeze = @{ path = (Full (Join-Path $prep 'provider-candidate-rev2/source-freeze-dxc-provider-rev2-final.json')); sha256 = '123AE0639F373D0EF16E6147FC74765CBCBBD7AF42CCEF1535D58A5B7A1C5CCD'; files = 23348 }
    productionWitnesses = $witnesses
    snapshots = @(@{ id = 'dxc'; path = (Full (Join-Path $prep 'acquisition/dxc/snapshot')); files = 17851 },@{ id = 'spirv-headers'; path = (Full (Join-Path $prep 'acquisition/spirv-headers/snapshot')); files = 104 },@{ id = 'spirv-tools'; path = (Full (Join-Path $prep 'acquisition/spirv-tools/snapshot')); files = 1706 })
    fixtureSources = @{ archive = (Full (Join-Path $prep 'acquisition/dxc/dxc-1cc0ab76dafe08a4144e419e619a8951a3d219db.tar.gz')); commitInfo = (Full (Join-Path $prep 'acquisition/dxc/snapshot/utils/GetCommitInfo.py')) }
    pathsMustRemainAbsent = @($outside)
    cases = $cases
    fixtureDesign = @('Luna creates only attempt1 fixtures; writer did not create fixtures or execute native','Archive fixture is exact source archive copied plus one byte; original stays read-only','Three fixture files are hashed and membership-checked before/after cases','Junction target is private empty local directory, not any source snapshot; no link cleanup','Absent child of junction proves ancestor reparse rejection before file lookup','Existing-output marker preserved byte-exact; no production children permitted','Outside-build sentinel never written: empty archive root is a second early rejection if expected guard regresses','Jobs and absent-VS cases also have empty archives as a safe secondary guard','Child process VS variable set only for guard context or removed for absent-VS; host environment unmodified','Overlay uses actual helper with altered first pristine file; only an empty proof directory may be created, no successful patch or production')
    resultContract = 'Every case must return nonzero, match its exact normalized diagnostic, create no production output and preserve fixtures. All 23348 source inputs, 3 full tree memberships and 5 positive-production witnesses remain intact. Any failure stops commands and preserves evidence.'
    limits = 'Negative guards prove rejection only. Real archive payload/consumer/runtime/Root/DX12 acceptance belongs to parent and its separately assigned Luna driver.'
}
$encoding = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText($assignmentPath,($assignment | ConvertTo-Json -Depth 20) + "`n",$encoding)
$driver = Join-Path $root 'RunDxcNegatives.ps1'
$text = [IO.File]::ReadAllText($driver)
if ([regex]::Matches($text,'JE_ASSIGNMENT_SHA256').Count -ne 1) { throw 'Unique assignment hash slot absent' }
[IO.File]::WriteAllText($driver,$text.Replace('JE_ASSIGNMENT_SHA256',(Get-FileHash -LiteralPath $assignmentPath).Hash),$encoding)
[pscustomobject]@{ status = 'PREPARED; NOT RUN'; cases = $cases.Count; sourceInputs = 23348; snapshots = 19661; fixtureFilesCreatedByWriter = 0; assignmentSHA256 = (Get-FileHash -LiteralPath $assignmentPath).Hash; driverSHA256 = (Get-FileHash -LiteralPath $driver).Hash } | ConvertTo-Json
