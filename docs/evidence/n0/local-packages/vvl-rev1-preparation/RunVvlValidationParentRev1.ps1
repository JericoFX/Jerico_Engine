#Requires -Version 7.0
$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'PowerShell Core is required' }
$evidenceRoot = $PSScriptRoot
$checkoutRoot = 'F:/JericoEngine/JericoEngine'
$runRoot = "$checkoutRoot/build/n0-validation/local-packages/vvl-rev1-validation/attempt1"
$freezePath = Join-Path $evidenceRoot 'source-freeze-vvl-provider-rev1-rev2.json'
$assignmentPath = Join-Path $evidenceRoot 'validation-assignment-vvl-rev1-rev2.json'
$approvalPath = Join-Path $evidenceRoot 'parent-source-review-vvl-rev1.json'
if ((Get-FileHash -LiteralPath $freezePath).Hash -ne 'C516374E93BED5F7B829A92CD0356D25A96A7289A9C17BE2D2FAA590AA3F90E3') { throw 'Freeze witness differs' }
if ((Get-FileHash -LiteralPath $assignmentPath).Hash -ne 'AF316CA5C13A937819B9B729AF5E249569C54013FF766BDC7CFD2779E127CA93') { throw 'Assignment witness differs' }
$approval = Get-Content -LiteralPath $approvalPath -Raw | ConvertFrom-Json
if ($approval.checkedInputs -ne 6471 -or $approval.nativeOwner -ne 'Luna') { throw 'Parent review is missing' }
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$assignment = Get-Content -LiteralPath $assignmentPath -Raw | ConvertFrom-Json
foreach ($path in @($runRoot, $assignment.outputMustBeFresh, $assignment.consumerOutputMustBeFresh)) {
    $resolved = [System.IO.Path]::GetFullPath($path)
    if (-not $resolved.StartsWith([System.IO.Path]::GetFullPath("$checkoutRoot/build/"), [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Output escaped checkout/build' }
    if (Test-Path -LiteralPath $resolved) { throw "Output exists; preserve attempt: $resolved" }
}
function Test-FrozenInputs {
    $changes = @()
    foreach ($entry in $freeze.inputs) {
        if (-not (Test-Path -LiteralPath $entry.path -PathType Leaf)) { $changes += $entry.path; continue }
        $item = Get-Item -LiteralPath $entry.path
        if ($item.Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $entry.path).Hash -ne $entry.sha256) { $changes += $entry.path }
    }
    return ,$changes
}
$preflight = Test-FrozenInputs
if ($preflight.Count) { throw "Inputs changed before native execution: $($preflight -join ', ')" }
foreach ($tool in $approval.toolWitnesses) {
    if ((Get-FileHash -LiteralPath $tool.path).Hash -ne $tool.sha256) { throw "Tool changed: $($tool.path)" }
}
[System.IO.Directory]::CreateDirectory($runRoot) | Out-Null
function Write-Evidence($path, $value) {
    [System.IO.File]::WriteAllText($path, ($value | ConvertTo-Json -Depth 12) + "`n", [System.Text.UTF8Encoding]::new($false))
}
Write-Evidence (Join-Path $runRoot 'preflight.json') @{status='PASS'; checkedInputs=$freeze.inputs.Count; powershellVersion=$PSVersionTable.PSVersion.ToString(); edition=$PSVersionTable.PSEdition}
$nativeResults = @()
$failed = $false
try {
    foreach ($command in $assignment.commands) {
        if ($command.label -eq 'extract-produced-archive') {
            if (-not (Test-Path -LiteralPath "$($assignment.outputMustBeFresh)/package-ready.json" -PathType Leaf)) { throw 'Producer did not produce a ready witness' }
            [System.IO.Directory]::CreateDirectory($command.workingDirectory) | Out-Null
        }
        $prefix = Join-Path $runRoot $command.label
        Write-Evidence "$prefix.argv.json" @{executable=$command.executable; argv=@($command.argv); workingDirectory=$command.workingDirectory}
        $start = [System.Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $command.executable
        $start.WorkingDirectory = if ($command.workingDirectory) { $command.workingDirectory } else { $checkoutRoot }
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        $start.Environment['PYTHONDONTWRITEBYTECODE'] = '1'
        foreach ($argument in $command.argv) { $start.ArgumentList.Add([string]$argument) }
        $stdout = [System.IO.File]::Create("$prefix.stdout.txt")
        $stderr = [System.IO.File]::Create("$prefix.stderr.txt")
        $process = [System.Diagnostics.Process]::new()
        $process.StartInfo = $start
        $watch = [System.Diagnostics.Stopwatch]::StartNew()
        try {
            if (-not $process.Start()) { throw 'Failed to start native command' }
            $outTask = $process.StandardOutput.BaseStream.CopyToAsync($stdout)
            $errTask = $process.StandardError.BaseStream.CopyToAsync($stderr)
            Write-Evidence "$prefix.running.json" @{pid=$process.Id; startUTC=[DateTime]::UtcNow.ToString('o'); label=$command.label}
            $peakWorkingSet = $null
            while (-not $process.WaitForExit(1000)) {
                try {
                    $process.Refresh()
                    $sample = $process.PeakWorkingSet64
                    if ($null -eq $peakWorkingSet -or $sample -gt $peakWorkingSet) { $peakWorkingSet = $sample }
                } catch {
                    # A process may exit between the wait and memory query.
                }
            }
            $nativeExit = $process.ExitCode
            $watch.Stop()
            [System.Threading.Tasks.Task]::WaitAll(@($outTask, $errTask))
            $result = [ordered]@{label=$command.label; nativeExit=$nativeExit; elapsedSeconds=$watch.Elapsed.TotalSeconds; sampledNativeProcessPeakWorkingSetBytes=$peakWorkingSet; memoryScope='Only invoked native process, not descendant builds or engine; null means not sampled'; status=if($nativeExit -eq 0){'PASS'}else{'FAIL'}}
            Write-Evidence "$prefix.result.json" $result
            $nativeResults += $result
            Write-Host "$($command.label): exit=$nativeExit elapsed=$([math]::Round($watch.Elapsed.TotalSeconds,3))s"
            if ($nativeExit -ne 0) { $failed=$true; break }
        } finally {
            $stdout.Dispose(); $stderr.Dispose(); $process.Dispose()
        }
    }
} catch {
    $failed = $true
    Write-Evidence (Join-Path $runRoot 'driver-error.json') @{message=$_.Exception.Message; scriptStack=$_.ScriptStackTrace}
} finally {
    $postflight = Test-FrozenInputs
    Write-Evidence (Join-Path $runRoot 'postflight.json') @{status=if($postflight.Count){'FAIL'}else{'PASS'}; checkedInputs=$freeze.inputs.Count; changedPaths=@($postflight)}
    if ($postflight.Count) { $failed=$true }
    Write-Evidence (Join-Path $runRoot 'validation-result.json') @{status=if($failed){'FAIL; PRESERVE ACTUAL RESULTS'}else{'FOCAL NATIVE COMMANDS PASS; PARENT REVIEW/NEGATIVE GUARDS/ROOT/EDITOR PENDING'}; commands=@($nativeResults); inputsChecked=$freeze.inputs.Count; changedPaths=@($postflight)}
}
if ($failed) { exit 1 }
exit 0
