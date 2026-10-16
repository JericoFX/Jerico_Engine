[CmdletBinding()]
param([switch]$Execute,[string]$EvidenceDirectory,[string]$FreezePath,[string]$FreezeSHA256,[int]$InputCount)
if (!$Execute) { Write-Output 'PREPARED_NOT_RUN'; return }
$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$pbr=Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/material-pbr-preview-integration-rev1'
$build=Join-Path $repo 'build/n0-validation/original-engine'
$cache=Join-Path $build 'CMakeCache.txt'
$RamGuardPercent=80
$script:phases=[Collections.Generic.List[object]]::new()
$script:moduleCaptureKeys=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
if (!$EvidenceDirectory -or !$FreezePath -or $FreezeSHA256 -cnotmatch '^[0-9A-F]{64}$' -or $InputCount -lt 13995) { throw 'Explicit reviewed baseline recipe required.' }
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
if (!$evidence.StartsWith(($pbr+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $evidence)) { throw 'New owned evidence directory required.' }
if ((Get-FileHash -LiteralPath $FreezePath -Algorithm SHA256).Hash -cne $FreezeSHA256) { throw 'Baseline freeze changed.' }
$freeze=Get-Content -LiteralPath $FreezePath -Raw|ConvertFrom-Json
if ($freeze.revision -ne 80 -or $freeze.inputs.Count -ne $InputCount) { throw 'Baseline count/revision differs.' }
[void][IO.Directory]::CreateDirectory($evidence)
function Save-Json([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
}

function Get-InputMap {
    $map = [ordered]@{}
    foreach ($entry in $freeze.inputs) {
        $path = [string]$entry.path
        if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $repo $path }
        $path = [IO.Path]::GetFullPath($path)
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Frozen input missing: $path" }
        $item = Get-Item -LiteralPath $path
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($item.Length -ne [long]$entry.bytes -or $hash -cne [string]$entry.sha256) {
            throw "Frozen input mismatch: $path"
        }
        $map[$path] = [pscustomobject]@{ bytes = $item.Length; sha256 = $hash }
    }
    return $map
}

function Get-OfflineCacheState {
    $lines = Get-Content -LiteralPath $cache
    $required = @(
        'CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022',
        'CMAKE_GENERATOR_PLATFORM:INTERNAL=x64',
        'FETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON',
        'LY_PACKAGE_SERVER_URLS:STRING=',
        'O3DE_EXTERNAL_SUBDIRS:STRING=',
        'LY_PROJECTS:STRING=F:/JericoEngine/JericoEngine/Projects/JEEditorFixture'
    )
    $missing = @($required | Where-Object { $_ -cnotin $lines })
    [pscustomobject]@{
        path = $cache
        sha256 = (Get-FileHash -LiteralPath $cache -Algorithm SHA256).Hash
        requiredLines = $required
        missingLines = $missing
        valid = ($missing.Count -eq 0)
    }
}

function Get-RamSnapshot {
    $os = Get-CimInstance Win32_OperatingSystem
    $totalKb = [double]$os.TotalVisibleMemorySize
    $freeKb = [double]$os.FreePhysicalMemory
    $usedPercent = if ($totalKb -gt 0) { [Math]::Round((($totalKb - $freeKb) / $totalKb) * 100, 2) } else { 100.0 }
    [pscustomobject]@{
        capturedUtc = [DateTime]::UtcNow.ToString('o')
        totalBytes = [long]($totalKb * 1KB)
        freeBytes = [long]($freeKb * 1KB)
        usedPercent = $usedPercent
        thresholdPercent = $RamGuardPercent
    }
}

function Save-PhaseProcessSnapshot([string]$Base, [int]$RootId, [datetime]$RootCreationUtc, [string]$RootExecutable, $Ram) {
    $rows = @(Get-CimInstance Win32_Process)
    $byId = @{}
    foreach ($row in $rows) { $byId[[int]$row.ProcessId] = $row }
    $owned = [Collections.Generic.HashSet[int]]::new()
    $root = $byId[$RootId]
    if ($root -and $root.CreationDate -and $root.ExecutablePath -ieq $RootExecutable -and
        [Math]::Abs(($root.CreationDate.ToUniversalTime() - $RootCreationUtc).TotalMilliseconds) -lt 1) {
        [void]$owned.Add($RootId)
    }
    do {
        $added = $false
        foreach ($row in $rows) {
            if ($owned.Contains([int]$row.ParentProcessId) -and
                $row.CreationDate -and $byId[[int]$row.ParentProcessId].CreationDate -and
                $row.CreationDate -ge $byId[[int]$row.ParentProcessId].CreationDate -and
                $row.CreationDate.ToUniversalTime() -ge $RootCreationUtc -and
                $owned.Add([int]$row.ProcessId)) { $added = $true }
        }
    } while ($added)
    $nativeNames = @('cmake.exe','MSBuild.exe','cl.exe','Tracker.exe','link.exe','ctest.exe','AzTestRunner.exe','mspdbsrv.exe')
    $selected = @($rows | Where-Object {
        $owned.Contains([int]$_.ProcessId) -or $_.Name -in $nativeNames -or [long]$_.WorkingSetSize -ge 250MB
    })
    $snapshot = [ordered]@{
        capturedUtc = [datetime]::UtcNow.ToString('o'); ramUsedPercent = $Ram.usedPercent
        phaseRootId = $RootId; phaseRootCreationUtc = $RootCreationUtc.ToString('o')
        processes = @($selected | ForEach-Object {
            $isOwned = $owned.Contains([int]$_.ProcessId)
            $command = [string]$_.CommandLine
            [pscustomobject]@{
                pid = $_.ProcessId; parent = $_.ParentProcessId; name = $_.Name
                creationUtc = if ($_.CreationDate) { $_.CreationDate.ToUniversalTime().ToString('o') } else { $null }
                executable = $_.ExecutablePath
                workingSetBytes = [long]$_.WorkingSetSize; privateBytes = [long]$_.PrivatePageCount
                observedPhaseDescendant = $isOwned
                commandLine = if ($isOwned) { $command } else { $null }
                mentionsAuthoritativeCheckout = $command.Replace('/','\').IndexOf($repo,[StringComparison]::OrdinalIgnoreCase) -ge 0
                mentionsHawken = $command.IndexOf('Hawken',[StringComparison]::OrdinalIgnoreCase) -ge 0
            }
        })
    }
    [IO.File]::AppendAllText(($Base + '.memory-process-samples.jsonl'),($snapshot | ConvertTo-Json -Depth 5 -Compress) + [Environment]::NewLine,[Text.UTF8Encoding]::new($false))
    Observe-AssetModules78 $snapshot $Base
    return $snapshot
}

function Invoke-Phase([string]$Name, [string]$Executable, [string[]]$PhaseArguments, [switch]$AllowNonZero, [string]$WorkingDirectory) {
    $ramBefore = Get-RamSnapshot
    if ($ramBefore.usedPercent -ge $RamGuardPercent) { throw ("RAM guard blocks {0}: {1}% >= {2}%" -f $Name, $ramBefore.usedPercent, $RamGuardPercent) }
    $started = [DateTime]::UtcNow
    $base = Join-Path $evidence $Name
    $stdoutPath = $base + '.stdout.log'
    $stderrPath = $base + '.stderr.log'
    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Executable
    $psi.WorkingDirectory = if ($WorkingDirectory) { $WorkingDirectory } else { $repo }
    $psi.UseShellExecute = $false
    if ($Name.StartsWith('build-', [StringComparison]::Ordinal)) {
        # Task-local compiler concurrency: _CL_ is appended after command-line options.
        # /MP1 also limits compilation within the single MSBuild project; no cache/global edit.
        $inheritedTail = [string]$psi.Environment['_CL_']
        $psi.Environment['_CL_'] = ($inheritedTail + ' /MP1').Trim()
        $psi.Environment['MSBUILDDISABLENODEREUSE'] = '1'
    }
    $psi.CreateNoWindow = $true
    $psi.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    foreach ($argument in $PhaseArguments) { [void]$psi.ArgumentList.Add([string]$argument) }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $psi
    $stdoutStream = $null
    $stderrStream = $null
    try {
    $admission = Get-RamSnapshot
    if ($admission.usedPercent -ge 70 -or $admission.freeBytes -lt 10GB) {
        throw ("Phase admission blocks {0}: {1}% RAM, {2} free bytes; requires <70% and >=10 GiB" -f $Name, $admission.usedPercent, $admission.freeBytes)
    }
    if (-not $process.Start()) { throw "Could not start $Name" }
    $phaseOriginalCim = Get-CimInstance Win32_Process -Filter ("ProcessId={0}" -f $process.Id)
    if (-not $phaseOriginalCim -or $phaseOriginalCim.ExecutablePath -ine $Executable) { throw 'Original native process identity unavailable; preserve unknown processes' }
    $phaseOriginalCreationUtc = $phaseOriginalCim.CreationDate.ToUniversalTime()
    $startRecord = [pscustomobject]@{
        phase = $Name; startedUtc = $started.ToString('o'); pid = $process.Id
        processCreationUtc = $phaseOriginalCreationUtc.ToString('o'); ramBefore = $ramBefore
        executable = $Executable; argv = @($PhaseArguments); workingDirectory = $psi.WorkingDirectory
        stdout = $stdoutPath; stderr = $stderrPath
    }
    Save-Json ($base + '.start.json') $startRecord
    Write-Host ("PHASE START {0} PID {1} UTC {2}" -f $Name, $process.Id, $startRecord.startedUtc)
    [Console]::Out.Flush()
    $stdoutStream = [IO.File]::Open($stdoutPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    $stderrStream = [IO.File]::Open($stderrPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    $stdoutTask = $process.StandardOutput.BaseStream.CopyToAsync($stdoutStream)
    $stderrTask = $process.StandardError.BaseStream.CopyToAsync($stderrStream)
    $waitSamples = [System.Collections.Generic.List[object]]::new()
    $peakUsedPercent = $ramBefore.usedPercent
    $ramGuardTriggered = $null
    while (-not $process.WaitForExit(3000)) {
        $ramSample = Get-RamSnapshot
        $processSnapshot = $null
        try { $processSnapshot = Save-PhaseProcessSnapshot $base $process.Id ($phaseOriginalCreationUtc) $Executable $ramSample }
        catch { Write-Host ("Process memory observation failed; RAM guard remains active: {0}" -f $_.Exception.Message) }
        $ownedProcess = Get-Process -Id $process.Id -ErrorAction SilentlyContinue
        $waitSamples.Add([pscustomobject]@{
            capturedUtc = $ramSample.capturedUtc
            ramUsedPercent = $ramSample.usedPercent
            pid = $process.Id
            processCreationUtc = $phaseOriginalCreationUtc.ToString('o')
            workingSetBytes = if ($ownedProcess) { [long]$ownedProcess.WorkingSet64 } else { $null }
        })
        if ($ramSample.usedPercent -gt $peakUsedPercent) { $peakUsedPercent = $ramSample.usedPercent }
        if ($ramSample.usedPercent -ge $RamGuardPercent -and -not $process.HasExited) {
            $currentRoot = Get-CimInstance Win32_Process -Filter ("ProcessId={0}" -f $process.Id)
            if ($currentRoot -and -not $process.HasExited) {
                $creationDelta = [Math]::Abs(($currentRoot.CreationDate.ToUniversalTime() - $phaseOriginalCreationUtc).TotalMilliseconds)
                if ($currentRoot.Name -cne [IO.Path]::GetFileName($Executable) -or
                    $currentRoot.ExecutablePath -ine $Executable -or $creationDelta -ge 1) {
                    throw 'RAM guard root identity mismatch; preserve unknown processes'
                }
                $ramGuardTriggered = [pscustomobject]@{
                    capturedUtc = $ramSample.capturedUtc; thresholdPercent = $RamGuardPercent
                    observedPercent = $ramSample.usedPercent; pid = $process.Id
                    executable = $currentRoot.ExecutablePath
                    creationUtc = $currentRoot.CreationDate.ToUniversalTime().ToString('o')
                    action = 'Stop only the verified original phase process and its task descendants'
                }
                Save-Json ($base + '.ram-guard.json') $ramGuardTriggered
                Save-Json ($base + '.ram-guard-processes.json') $processSnapshot
                try { $process.Kill($true) } catch { if (-not $process.HasExited) { throw } }
            }
        }
    }
    $process.WaitForExit()
    $nativeExitCode = $process.ExitCode
    [IO.File]::WriteAllText(($base + '.native-exit.txt'), [string]$nativeExitCode, [Text.UTF8Encoding]::new($false))
    $stdoutTask.GetAwaiter().GetResult()
    $stderrTask.GetAwaiter().GetResult()
    $stdoutStream.Dispose()
    $stderrStream.Dispose()
    $finished = [DateTime]::UtcNow
    $result = [pscustomobject]@{
        phase = $Name; startedUtc = $started.ToString('o'); finishedUtc = $finished.ToString('o')
        elapsedSeconds = [Math]::Round(($finished - $started).TotalSeconds, 3); pid = $process.Id
        processCreationUtc = $phaseOriginalCreationUtc.ToString('o'); ramBefore = $ramBefore
        peakRamUsedPercent = $peakUsedPercent; ramWaitSamples = @($waitSamples.ToArray())
        ramGuardTriggered = $ramGuardTriggered; ramPollMilliseconds = 3000
        processMemorySamples = $base + '.memory-process-samples.jsonl'
        executable = $Executable; argv = @($PhaseArguments); exitCode = $process.ExitCode
        stdout = $stdoutPath; stdoutSha256 = (Get-FileHash -LiteralPath $stdoutPath -Algorithm SHA256).Hash
        stderr = $stderrPath; stderrSha256 = (Get-FileHash -LiteralPath $stderrPath -Algorithm SHA256).Hash
    }
    Save-Json ($base + '.result.json') $result
    $script:phases.Add($result)
    } finally {
        if ($stdoutStream) { $stdoutStream.Dispose() }
        if ($stderrStream) { $stderrStream.Dispose() }
        $process.Dispose()
    }
    if ($result.ramGuardTriggered) { throw "$Name stopped by task RAM guard at $($result.ramGuardTriggered.observedPercent)% (threshold $RamGuardPercent%); native exit $($result.exitCode)" }
    if ($result.exitCode -ne 0 -and -not $AllowNonZero) { throw "$Name exited $($result.exitCode)" }
    return $result
}

function Verify-FileEntries($entries,[int]$expected,[string]$label){
 if($entries.Count -ne $expected){throw "$label inventory count differs"}
 foreach($entry in $entries){
  if((Get-Item -LiteralPath $entry.path).Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $entry.path).Hash -cne $entry.sha256){throw "$label bytes differ: $($entry.path)"}
 }
 return [pscustomobject]@{label=$label;count=$expected;matched=$expected}
}
$moduleObserver=Join-Path $pbr 'ap-console-source79-preparation-rev1/Observe-AssetModules78.ps1'
if ((Get-FileHash -LiteralPath $moduleObserver -Algorithm SHA256).Hash -cne '898E333155F1AA5534C05BBE04C7F7CB55509650C9BF445B8FE168D8953550C6') { throw 'Reused process observer changed.' }
. $moduleObserver
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$ctest='C:\Program Files\CMake\bin\ctest.exe'
$xmlPath=Join-Path $build 'Testing/Gtest/Gem_AtomShader.Tests.xml'
$protected=@((Get-Content -LiteralPath (Join-Path $repo 'build/n0-validation/native-passgraph-workspace-rev1/real-pass-editor-ap-attempt1/protected-authored-preflight.json') -Raw|ConvertFrom-Json).inputs)
$ownerFiles=@((Get-Content -LiteralPath (Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/folder-drop-regression-rev1/owner-folder-before-ui.json') -Raw|ConvertFrom-Json).inputs)
$fatal=$null; $before=$null; $after=$null; $test=$null; $nativeCases=@()
try {
 $active=@(Get-CimInstance Win32_Process|Where-Object Name -in @('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','JEEditorFixture.GameLauncher.exe','cmake.exe','MSBuild.exe','cl.exe','link.exe','ctest.exe','AzTestRunner.exe'))
 if ($active.Count) { throw 'Another native owner exists.' }
 $before=Get-InputMap
 $cacheBefore=Get-OfflineCacheState
 if (!$cacheBefore.valid -or $cacheBefore.sha256 -cne '4469ECECFF474E20A81B5C0AE6D87BCA931EBAF10C24560E507F6A1DCA2E7651') { throw 'Audited offline cache changed.' }
 $authoredBefore=Verify-FileEntries $protected 40 'Authored'
 $ownerBefore=Verify-FileEntries $ownerFiles 26 'OwnerTextures'
 Save-Json (Join-Path $evidence 'preflight.json') @{freeze=$FreezePath;freezeSHA256=$FreezeSHA256;inputCount=$before.Count;cache=$cacheBefore;authored=$authoredBefore;ownerTextures=$ownerBefore;ram=Get-RamSnapshot;scope='Original17shader tests BEFORE ownership migration; no GPU/AP/UI acceptance';GemIncrement=0}
 Invoke-Phase 'build-original-shader-tests' $cmake @('--build',$build,'--config','profile','--target','AtomShader.Tests','AtomShader.Builders','--parallel','1','--','/m:1','/nodeReuse:false')|Out-Null
 Save-Json (Join-Path $evidence 'original-binary-pins.json') @(foreach($name in @('AtomShader.Tests.dll','AtomShader.Builders.dll')){$path=Join-Path $build ('bin/profile/'+$name);$item=Get-Item -LiteralPath $path;[pscustomobject]@{path=$item.FullName;bytes=$item.Length;lastWriteUtc=$item.LastWriteTimeUtc.ToString('o');sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}})
 $test=Invoke-Phase 'test-original-shader-tests' $ctest @('--test-dir',$build,'--build-config','profile','--output-on-failure','--output-junit',(Join-Path $evidence 'ctest.junit.xml'),'-j','1','-R','^Gem::AtomShader[.]Tests[.]main::TEST_RUN$')
 $xmlItem=Get-Item -LiteralPath $xmlPath
 if ($xmlItem.LastWriteTimeUtc -lt ([DateTimeOffset]$test.startedUtc).UtcDateTime) { throw 'Actual native XML is stale.' }
 $xml=[xml][IO.File]::ReadAllText($xmlPath)
 $cases=@($xml.SelectNodes('//testcase'))
 if ($cases.Count -ne 17 -or [int]$xml.testsuites.tests -ne 17 -or [int]$xml.testsuites.failures -ne 0 -or [int]$xml.testsuites.errors -ne 0 -or [int]$xml.testsuites.disabled -ne 0 -or $xml.SelectNodes('//failure|//error|//skipped').Count -ne 0 -or @($cases|Where-Object {$_.status -ne 'run' -or $_.result -ne 'completed'}).Count) { throw 'Original17 native cases are not all executed and green.' }
 [IO.File]::Copy($xmlPath,(Join-Path $evidence 'original17.native.xml'),$false)
 $nativeCases=@($cases|ForEach-Object {[pscustomobject]@{suite=$_.classname;name=$_.name;status=$_.status;result=$_.result}})
} catch { $fatal=$_.Exception.Message }
finally {
 try {
  $testPhases=@($script:phases|Where-Object phase -ceq 'test-original-shader-tests')
  if ($testPhases.Count -eq 1 -and (Test-Path -LiteralPath $xmlPath) -and (Get-Item -LiteralPath $xmlPath).LastWriteTimeUtc -ge ([DateTimeOffset]$testPhases[0].startedUtc).UtcDateTime -and !(Test-Path -LiteralPath (Join-Path $evidence 'original17.native.xml'))) { [IO.File]::Copy($xmlPath,(Join-Path $evidence 'original17.native.failed.xml'),$false) }
  $after=Get-InputMap
  $authoredAfter=Verify-FileEntries $protected 40 'Authored'
  $ownerAfter=Verify-FileEntries $ownerFiles 26 'OwnerTextures'
  $cacheAfter=Get-OfflineCacheState
  if (!$cacheAfter.valid -or $cacheAfter.sha256 -cne '4469ECECFF474E20A81B5C0AE6D87BCA931EBAF10C24560E507F6A1DCA2E7651') { throw 'Offline cache changed.' }
  $remaining=@(Get-CimInstance Win32_Process|Where-Object Name -in @('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','JEEditorFixture.GameLauncher.exe','cmake.exe','MSBuild.exe','cl.exe','link.exe','ctest.exe','AzTestRunner.exe'))
  if ($remaining.Count) { throw 'Native processes remain; preserve for owner review.' }
 } catch { $fatal=([string]$fatal+'; postflight: '+$_.Exception.Message).Trim('; ') }
 Save-Json (Join-Path $evidence 'postflight.json') @{freeze=$FreezePath;freezeSHA256=$FreezeSHA256;preCount=if($before){$before.Count}else{0};postCount=if($after){$after.Count}else{0};allFrozenInputsMatched=($null-ne $after -and $after.Count -eq $InputCount);authored=$authoredAfter;ownerTextures=$ownerAfter;cache=$cacheAfter;phases=$script:phases.ToArray();nativeCases=$nativeCases;fatal=$fatal;GemIncrement=0;limits='Original before-tests only; no migration/AP/GPU/UI/Shipping acceptance.'}
}
if ($fatal) { throw $fatal }
Write-Output 'ORIGINAL17_BEFORE_NATIVE_TESTS_PASSED_PARENT_REVIEW_REQUIRED'
