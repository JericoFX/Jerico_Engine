#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core required' }
# Stream hashing avoids per-file cmdlet overhead; the SHA256 guard is unchanged.
function Get-JEFileSHA256([string] $path) {
    $stream = [IO.File]::OpenRead($path)
    try { return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
$root = [IO.Path]::GetFullPath($PSScriptRoot)
$attempt = Join-Path $root 'attempt-rev3'
if ($root -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\dxc-rev1-negatives') { throw 'Assigned output root differs' }
if (Test-Path -LiteralPath $attempt) { throw 'Attempt exists; retain it' }
$assignmentPath = Join-Path $root 'negative-assignment-rev3.json'
if ((Get-JEFileSHA256 $assignmentPath) -ne '63B815D3BA3E87586B0C1194ABF57620EB6076D55124DEF0F9F680A80D2B1DB3') { throw 'Negative assignment changed' }
$assignment = Get-Content -LiteralPath $assignmentPath -Raw | ConvertFrom-Json
$freezePath = $assignment.sourceFreeze.path
if ((Get-JEFileSHA256 $freezePath) -ne $assignment.sourceFreeze.sha256) { throw 'Approved DXC freeze changed' }
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
if ($freeze.files.Count -ne 23385 -or $assignment.cases.Count -ne 18) { throw 'Input/case counts differ' }
function Test-FrozenInputs {
    $changed = @()
    foreach ($file in $freeze.files) {
        if (-not (Test-Path -LiteralPath $file.path -PathType Leaf)) { $changed += $file.path; continue }
        if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-JEFileSHA256 $file.path) -ne $file.sha256) { $changed += $file.path }
    }
    foreach ($tree in $assignment.snapshots) {
        $prefix = [IO.Path]::GetFullPath($tree.path) + '\'
        $expected = @($freeze.files | Where-Object { [IO.Path]::GetFullPath($_.path).StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase) } | ForEach-Object { [IO.Path]::GetFullPath($_.path) } | Sort-Object)
        $actual = @(Get-ChildItem -LiteralPath $tree.path -Recurse -File | ForEach-Object { $_.FullName } | Sort-Object)
        if ($actual.Count -ne $tree.files -or $expected.Count -ne $tree.files -or ($actual -join "`n") -cne ($expected -join "`n")) { $changed += "Snapshot membership: $($tree.id)" }
    }
    foreach ($file in $assignment.productionWitnesses) {
        if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-JEFileSHA256 $file.path) -ne $file.sha256) { $changed += $file.path }
    }
    return ,$changed
}
if ((Test-FrozenInputs).Count) { throw 'Frozen sources/production witnesses changed before negatives' }
foreach ($path in $assignment.pathsMustRemainAbsent) { if (Test-Path -LiteralPath $path) { throw "Sentinel already exists: $path" } }
$encoding = [Text.UTF8Encoding]::new($false)
function Write-Json([string] $path, $value) { [IO.File]::WriteAllText($path,($value | ConvertTo-Json -Depth 16) + "`n",$encoding) }
New-Item -ItemType Directory -Path (Join-Path $attempt 'fixtures/empty-archives') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $attempt 'fixtures/changed-archives/dxc') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $attempt 'fixtures/existing-output') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $attempt 'fixtures/reparse-target') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $attempt 'fixtures/overlay-work/dxc/utils') | Out-Null
[IO.File]::WriteAllText((Join-Path $attempt 'fixtures/existing-output/retain.txt'),'Existing private fixture output must stay byte-identical.',$encoding)
$changedArchive = Join-Path $attempt 'fixtures/changed-archives/dxc/dxc-1cc0ab76dafe08a4144e419e619a8951a3d219db.tar.gz'
[IO.File]::Copy($assignment.fixtureSources.archive,$changedArchive,$false)
$stream = [IO.File]::Open($changedArchive,[IO.FileMode]::Append)
try { $stream.WriteByte(0) } finally { $stream.Dispose() }
$wrongOverlay = Join-Path $attempt 'fixtures/overlay-work/dxc/utils/GetCommitInfo.py'
[IO.File]::Copy($assignment.fixtureSources.commitInfo,$wrongOverlay,$false)
$stream = [IO.File]::Open($wrongOverlay,[IO.FileMode]::Append)
try { $bytes = $encoding.GetBytes("`n# JE negative fixture only; never a package input.`n"); $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
# Only a private empty target is exposed through the junction. No snapshot is a target.
$junction = Join-Path $attempt 'fixtures/archive-junction'
$junctionTarget = [IO.Path]::GetFullPath((Join-Path $attempt 'fixtures/reparse-target'))
New-Item -ItemType Junction -Path $junction -Value $junctionTarget | Out-Null
[IO.File]::WriteAllText($assignment.helperFixture.path,$assignment.helperFixture.content,$encoding)
$fixtureFiles = @((Join-Path $attempt 'fixtures/existing-output/retain.txt'),$changedArchive,$wrongOverlay,$assignment.helperFixture.path)
$fixtures = @(foreach ($path in $fixtureFiles) { [ordered]@{ path = $path; bytes = (Get-Item -LiteralPath $path).Length; sha256 = (Get-JEFileSHA256 $path) } })
Write-Json (Join-Path $attempt 'fixture-manifest.json') $fixtures
function Test-Fixtures {
    $changed = @()
    foreach ($file in $fixtures) { if ((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Get-JEFileSHA256 $file.path) -ne $file.sha256) { $changed += $file.path } }
    $actual = @(Get-ChildItem -LiteralPath (Join-Path $attempt 'fixtures') -Recurse -File | ForEach-Object FullName | Sort-Object)
    if (($actual -join "`n") -cne (($fixtureFiles | Sort-Object) -join "`n")) { $changed += 'Fixture membership changed' }
    $link = Get-Item -LiteralPath $junction
    if (-not ($link.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $link.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath([string]$link.Target) -ne $junctionTarget) { $changed += 'Private junction changed' }
    if (@(Get-ChildItem -LiteralPath $junctionTarget -Force).Count) { $changed += 'Private junction target changed' }
    return ,$changed
}
$results = @()
$failed = $false
try {
    foreach ($case in $assignment.cases) {
        if ((Test-Fixtures).Count) { throw 'Fixture changed before command' }
        $prefix = Join-Path $attempt $case.label
        Write-Json "$prefix.argv.json" @{ executable = $case.executable; argv = @($case.argv); childEnvironment = $case.childEnvironment }
        $info = [Diagnostics.ProcessStartInfo]::new()
        $info.FileName = $case.executable
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.RedirectStandardOutput = $true
        $info.RedirectStandardError = $true
        foreach ($arg in $case.argv) { $info.ArgumentList.Add([string]$arg) }
        foreach ($property in $case.childEnvironment.PSObject.Properties) {
            if ($null -eq $property.Value) { [void] $info.Environment.Remove($property.Name) }
            else { $info.Environment[$property.Name] = [string]$property.Value }
        }
        $process = [Diagnostics.Process]::new()
        $process.StartInfo = $info
        $watch = [Diagnostics.Stopwatch]::StartNew()
        try {
            if (-not $process.Start()) { throw 'CMake start failed' }
            $outTask = $process.StandardOutput.ReadToEndAsync()
            $errTask = $process.StandardError.ReadToEndAsync()
            $process.WaitForExit()
            $nativeExit = $process.ExitCode
            $watch.Stop()
            $stdout = $outTask.GetAwaiter().GetResult()
            $stderr = $errTask.GetAwaiter().GetResult()
            [IO.File]::WriteAllText("$prefix.stdout.txt",$stdout,$encoding)
            [IO.File]::WriteAllText("$prefix.stderr.txt",$stderr,$encoding)
            $diagnostic = ($stdout + "`n" + $stderr) -replace '\s+',' '
            $expected = $diagnostic -match [regex]::Escape($case.expectedError)
            $absent = $true
            foreach ($path in $case.mustRemainAbsent) { if (Test-Path -LiteralPath $path) { $absent = $false } }
            if ($case.label -eq 'overlay-mismatch' -and (Test-Path -LiteralPath $case.allowedEmptyProof)) {
                if (@(Get-ChildItem -LiteralPath $case.allowedEmptyProof -Force).Count) { $absent = $false }
            }
            $fixtureChanges = Test-Fixtures
            $pass = $nativeExit -ne 0 -and $expected -and $absent -and $fixtureChanges.Count -eq 0
            $result = [ordered]@{ label = $case.label; scope = $case.scope; secondaryMissingPath = $case.secondaryMissingPath; nativeExit = $nativeExit; elapsedSeconds = $watch.Elapsed.TotalSeconds; expectedError = $case.expectedError; observedExpectedError = $expected; outputCondition = $absent; fixtureChanges = @($fixtureChanges); status = if ($pass) { 'PASS REJECTION' } else { 'FAIL' } }
            Write-Json "$prefix.result.json" $result
            $results += $result
            Write-Host "$($case.label): exit=$nativeExit expected=$expected output=$absent"
            if (-not $pass) { $failed = $true; break }
        } finally { $process.Dispose() }
    }
} catch {
    $failed = $true
    Write-Json (Join-Path $attempt 'driver-error.json') @{ message = $_.Exception.Message; scriptStack = $_.ScriptStackTrace }
} finally {
    $sourceChanges = Test-FrozenInputs
    $fixtureChanges = Test-Fixtures
    if ($sourceChanges.Count -or $fixtureChanges.Count -or $results.Count -ne $assignment.cases.Count) { $failed = $true }
    foreach ($path in $assignment.pathsMustRemainAbsent) { if (Test-Path -LiteralPath $path) { $failed = $true } }
    Write-Json (Join-Path $attempt 'negative-result.json') @{ status = if ($failed) { 'FAIL; RETAIN ACTUAL OUTPUTS' } else { 'FOCAL REJECTIONS PASS; PARENT REVIEW PENDING' }; results = @($results); expectedCases = $assignment.cases.Count; sourceInputs = $freeze.files.Count; sourceChanges = @($sourceChanges); fixtureFiles = $fixtures.Count; fixtureChanges = @($fixtureChanges); productionArchiveSHA256 = $assignment.productionWitnesses[0].sha256; acceptance = $false }
}
if ($failed) { exit 1 }
exit 0
