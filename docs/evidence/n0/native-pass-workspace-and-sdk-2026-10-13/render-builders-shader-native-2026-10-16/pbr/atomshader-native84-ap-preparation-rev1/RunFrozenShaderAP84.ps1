[CmdletBinding()]
param([switch]$Execute,[string]$EvidenceDirectory,[string]$FreezePath,[string]$FreezeSHA256,[int]$InputCount,[string]$ParentReview82Path,[string]$ParentReview82SHA256)
if (!$Execute) { Write-Output 'PREPARED_NOT_RUN; parent reviews82, stages exact fixture and authorizes one AP84 execution before SDK83.'; return }
$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$pbr=Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/material-pbr-preview-integration-rev1'
$build=Join-Path $repo 'build/n0-validation/original-engine'
$cache=Join-Path $build 'CMakeCache.txt'
$RamGuardPercent=80
$script:phases=[Collections.Generic.List[object]]::new()
$script:moduleCaptureKeys=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
if (!$EvidenceDirectory -or !$FreezePath -or $FreezeSHA256 -cnotmatch '^[0-9A-F]{64}$' -or $InputCount -ne 14037 -or !$ParentReview82Path -or $ParentReview82SHA256 -cnotmatch '^[0-9A-F]{64}$') { throw 'Exact source84 freeze and parent source82 review recipe required.' }
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
if (!$evidence.StartsWith(($pbr+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $evidence)) { throw 'New owned evidence directory required.' }
if ((Get-FileHash -LiteralPath $FreezePath).Hash -cne $FreezeSHA256) { throw 'Source84 freeze changed.' }
$freeze=Get-Content -LiteralPath $FreezePath -Raw|ConvertFrom-Json
if ($freeze.revision -ne 84 -or $freeze.inputs.Count -ne $InputCount) { throw 'Frozen count/revision differs.' }
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
$pinManifest=Join-Path $PSScriptRoot 'preparation-pins.json'
$pins=Get-Content -LiteralPath $pinManifest -Raw|ConvertFrom-Json
foreach($pin in $pins){if((Get-Item -LiteralPath $pin.path).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $pin.path).Hash -cne $pin.sha256){throw "Preparation dependency drift: $($pin.path)"}}
. (Join-Path $PSScriptRoot 'Snapshot-AssetDatabase78.ps1')
. (Join-Path $PSScriptRoot 'Observe-AssetModules84.ps1')
. (Join-Path $PSScriptRoot 'Review-ShaderSources84.ps1')
$workload=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'workload.json') -Raw|ConvertFrom-Json
$sources=@($workload.sources)
$listPath=Join-Path $PSScriptRoot 'reprocess-sources.txt'
$list=@([IO.File]::ReadAllLines($listPath))
if($sources.Count -ne 7 -or $list.Count -ne 5 -or ($list -join '|') -cne ($workload.reprocessSources -join '|')){throw 'Exact workload/list differs.'}
$baseFreezePath=Join-Path $pbr 'atomshader-native82-private-include-preparation-rev1/freeze82-native-shader.json'
if((Get-FileHash -LiteralPath $baseFreezePath).Hash -cne 'B65743A66E3837A917C49F479AD4F2F6DEEBBAF5719C49ECE30302A1DE05B0EC'){throw 'Source82 base manifest differs.'}
$baseFreeze=Get-Content -LiteralPath $baseFreezePath -Raw|ConvertFrom-Json
if($baseFreeze.inputs.Count -ne 14034){throw 'Source82 base count differs.'}
$manifestMap=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($entry in $freeze.inputs){$path=[string]$entry.path;if(![IO.Path]::IsPathRooted($path)){$path=Join-Path $repo $path};$path=[IO.Path]::GetFullPath($path);if(!$manifestMap.TryAdd($path,$entry)){throw 'Duplicate freeze84 source path.'}}
foreach($entry in $baseFreeze.inputs){$path=[string]$entry.path;if(![IO.Path]::IsPathRooted($path)){$path=Join-Path $repo $path};$path=[IO.Path]::GetFullPath($path);if(!$manifestMap.ContainsKey($path) -or $manifestMap[$path].bytes -ne $entry.bytes -or $manifestMap[$path].sha256 -cne $entry.sha256){throw 'Source84 alters source82 baseline.'};[void]$manifestMap.Remove($path)}
$stageSources=@($sources|Where-Object {!$_.alreadyPresent})
if($manifestMap.Count -ne 3 -or $stageSources.Count -ne 3){throw 'Exactly three temporary staged inputs may extend source82.'}
foreach($entry in $stageSources){$path=[IO.Path]::GetFullPath($entry.path);if(!$manifestMap.ContainsKey($path) -or $manifestMap[$path].bytes -ne $entry.bytes -or $manifestMap[$path].sha256 -cne $entry.sha256){throw 'Temporary fixture differs from approved source plan.'};[void]$manifestMap.Remove($path)}
if($manifestMap.Count){throw 'Unexpected freeze84 input.'}
if((Get-FileHash -LiteralPath $ParentReview82Path).Hash -cne $ParentReview82SHA256){throw 'Parent source82 review receipt differs.'}
$parent82=Get-Content -LiteralPath $ParentReview82Path -Raw|ConvertFrom-Json
if($parent82.approved -isnot [bool] -or !$parent82.approved -or $parent82.reviewer -cne '/root'){throw 'Explicit parent source82 approval required.'}
$hostReview=[pscustomobject]@{binaryPins=@(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'binary-pins82.json') -Raw|ConvertFrom-Json)}
$native82Path=Join-Path $pbr 'native-shader-compiler-source82-attempt1/postflight.json'
$native82=Get-Content -LiteralPath $native82Path -Raw|ConvertFrom-Json
if($native82.fatal -or !$native82.allFrozenInputsMatched -or $native82.preCount -ne 14034 -or $native82.postCount -ne 14034 -or $native82.freezeSHA256 -cne 'B65743A66E3837A917C49F479AD4F2F6DEEBBAF5719C49ECE30302A1DE05B0EC' -or $native82.nativeCases.Count -ne 811 -or @($native82.suites).Count -ne 8 -or @($native82.phases|Where-Object {$_.exitCode -ne 0 -or $_.ramGuardTriggered}).Count){throw 'Closed native82 CPU/hosts evidence differs; no AP acceptance inferred.'}
if(@($native82.phases|Where-Object phase -ceq 'build-shader-hosts82').Count -ne 1){throw 'Actual source82 hosts phase missing.'}
$recorded82=@(Get-Content -LiteralPath (Join-Path $pbr 'native-shader-compiler-source82-attempt1/product-binary-pins.json') -Raw|ConvertFrom-Json)
foreach($pin in $hostReview.binaryPins){
 if((Get-Item -LiteralPath $pin.path).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $pin.path).Hash -cne $pin.sha256){throw 'Current AP/module binary differs from preparation pin.'}
 if($pin.name -cne 'Atom_RPI.Public.dll'){$prior=@($recorded82|Where-Object path -ieq $pin.path);if($prior.Count -ne 1 -or $prior[0].sha256 -cne $pin.sha256 -or $prior[0].bytes -ne $pin.bytes){throw 'AP/producer pin is not actual source82 host output.'}}
}
$protected=@((Get-Content -LiteralPath (Join-Path $repo 'build/n0-validation/native-passgraph-workspace-rev1/real-pass-editor-ap-attempt1/protected-authored-preflight.json') -Raw|ConvertFrom-Json).inputs)
$ownerFiles=@((Get-Content -LiteralPath (Join-Path $repo 'build/n0-validation/native-render-builders-integration-rev1/folder-drop-regression-rev1/owner-folder-before-ui.json') -Raw|ConvertFrom-Json).inputs)
$logDirectory=Join-Path $repo 'build/n0-validation/ap84-logs'
if(Test-Path -LiteralPath $logDirectory){throw 'Preserve existing AP84 logs; no overwrite/retry.'}
$exe=Join-Path $build 'bin/profile/AssetProcessorBatch.exe'
$powerShell=Join-Path $PSHOME 'pwsh.exe'
$auditScript=Join-Path $PSScriptRoot 'Query-ShaderSources84Snapshot.ps1'
$nativeNames=@('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','JEEditorFixture.GameLauncher.exe','cmake.exe','MSBuild.exe','cl.exe','link.exe','ctest.exe','AzTestRunner.exe','MaterialEditor.exe','MaterialCanvas.exe','PassCanvas.exe','ShaderManagementConsole.exe')
$before=$null;$after=$null;$beforeQuery=$null;$afterQuery=$null;$native=$null;$fatal=$null;$cacheBefore=$null;$cacheAfter=$null;$authoredAfter=$null;$ownerAfter=$null;$focal=$null
try {
 $active=@(Get-CimInstance Win32_Process|Where-Object Name -in $nativeNames)
 if($active.Count){throw 'Another native owner exists; do not launch AP.'}
 $before=Get-InputMap
 $sourceBefore=Verify-FileEntries $sources 7 'Focal physical sources including staged fixture'
 $authoredBefore=Verify-FileEntries $protected 40 'Authored'
 $ownerBefore=Verify-FileEntries $ownerFiles 26 'OwnerTextures'
 $cacheBefore=Get-OfflineCacheState
 if(!$cacheBefore.valid -or $cacheBefore.sha256 -cne '583D5EC400A54D4A036414F26F332B01CDD6CD3B91B011B6B12DD484B71827BC'){throw 'Reviewed offline cache differs; no configure permitted.'}
 $legacyRegistry=@(Get-ChildItem -LiteralPath (Join-Path $build 'bin/profile/Registry') -Filter 'cmake_dependencies*.setreg' -File|Where-Object {[IO.File]::ReadAllText($_.FullName) -match 'AtomShader[.]Builders|Atom_RPI[.]Builders'})
 if($legacyRegistry.Count){throw 'Retired builder remains in active generated registry.'}
 Save-Json (Join-Path $evidence 'preflight.json') @{freeze=$FreezePath;freezeSHA256=$FreezeSHA256;inputCount=$before.Count;authored=$authoredBefore;ownerTextures=$ownerBefore;physicalSources=$sourceBefore;cache=$cacheBefore;native82=$native82Path;binaryPins=$hostReview.binaryPins;noStagingByRunner=$true;noConfigureOrBuild=$true;GemIncrement=0;acceptance='Parent only; four shader builders need fresh jobs/products, no GPU acceptance.'}
 $beforeDatabase=Save-AssetDatabase78Snapshot 'before'
 $beforeQueryPath=Join-Path ([IO.Path]::GetDirectoryName($beforeDatabase)) 'readonly-query.json'
 Invoke-Phase 'before-snapshot-query' $powerShell @('-NoProfile','-NonInteractive','-File',$auditScript,'-Execute','-DatabaseCopyPath',$beforeDatabase,'-ResultPath',$beforeQueryPath)|Out-Null
 $beforeQuery=Get-Content -LiteralPath $beforeQueryPath -Raw|ConvertFrom-Json
 $failed=@(($beforeQuery.queries|Where-Object name -ceq 'globalNonCompleted').rows|Where-Object Status -in @(2,3)|Select-Object -ExpandProperty SourceGuidHex -Unique)
 if($failed.Count -ne 347){throw 'Historical347 failed-source baseline differs before AP.'}
 [void][IO.Directory]::CreateDirectory($logDirectory)
 $phaseArguments=@('--engine-path=F:/JericoEngine/JericoEngine','--project-path=F:/JericoEngine/JericoEngine/Projects/JEEditorFixture','--platforms=pc',('--logDir='+$logDirectory),'--zeroAnalysisMode',('--reprocessFileList='+$listPath),'--regset=/Amazon/AssetProcessor/Settings/Jobs/maxJobs=1','--regset=/Amazon/AssetProcessor/Settings/Jobs/AlwaysUseMaxJobs=false')
 $native=Invoke-Phase 'assetprocessorbatch-shader84' $exe $phaseArguments -AllowNonZero
} catch {$fatal=$_.Exception.Message}
finally {
 try {
  $batchPhases=@($script:phases|Where-Object phase -ceq 'assetprocessorbatch-shader84')
  if($batchPhases.Count -eq 1 -and $beforeQuery){
   $afterDatabase=Save-AssetDatabase78Snapshot 'after'
   $afterQueryPath=Join-Path ([IO.Path]::GetDirectoryName($afterDatabase)) 'readonly-query.json'
   Invoke-Phase 'after-snapshot-query' $powerShell @('-NoProfile','-NonInteractive','-File',$auditScript,'-Execute','-DatabaseCopyPath',$afterDatabase,'-ResultPath',$afterQueryPath)|Out-Null
   $afterQuery=Get-Content -LiteralPath $afterQueryPath -Raw|ConvertFrom-Json
   try {$focal=Review-ShaderSources84 $beforeQuery $afterQuery $batchPhases[0];Save-Json (Join-Path $evidence 'four-builders-review.json') $focal} catch {Save-Json (Join-Path $evidence 'four-builders-review.json') @{state='INCOMPLETE_OR_FAILED';error=$_.Exception.Message;nativeExit=$batchPhases[0].exitCode;before=$beforeQueryPath;after=$afterQueryPath};throw}
  }
 } catch {$fatal=([string]$fatal+'; focal audit: '+$_.Exception.Message).Trim('; ')}
 try {
  $after=Get-InputMap
  $sourceAfter=Verify-FileEntries $sources 7 'Focal physical sources including staged fixture'
  $authoredAfter=Verify-FileEntries $protected 40 'Authored'
  $ownerAfter=Verify-FileEntries $ownerFiles 26 'OwnerTextures'
  $cacheAfter=Get-OfflineCacheState
  if(!$cacheAfter.valid -or $cacheAfter.sha256 -cne $cacheBefore.sha256){throw 'Offline cache changed during AP.'}
  foreach($pin in $hostReview.binaryPins){if((Get-Item -LiteralPath $pin.path).Length -ne $pin.bytes -or (Get-FileHash -LiteralPath $pin.path).Hash -cne $pin.sha256){throw 'Binary changed during AP.'}}
  $remaining=@(Get-CimInstance Win32_Process|Where-Object Name -in $nativeNames)
  Save-Json (Join-Path $evidence 'final-native-census.json') @($remaining|Select-Object ProcessId,ParentProcessId,Name,ExecutablePath,CreationDate)
  if($remaining.Count){throw 'Native processes remain; preserve identities for owner.'}
  $moduleLog=Join-Path $evidence 'assetprocessorbatch-shader84.modules.jsonl'
  $modules=@(if(Test-Path -LiteralPath $moduleLog){Get-Content -LiteralPath $moduleLog|ForEach-Object {$_|ConvertFrom-Json}})
  Save-Json (Join-Path $evidence 'native-module-observation.json') @{observations=$modules;scope='Owned PID/exe/creation captures only; not lifetime absence/device proof.'}
  if(!$modules.Count -or @($modules|Where-Object {$_.state -cne 'CAPTURED' -or $_.nativeActivation -cne 'BOTH_NATIVE_DLLS_OBSERVED_AND_PINNED' -or $_.legacyBuilderObserved}).Count){throw 'Pinned native modules not observed or retired AtomShader/Atom_RPI builder loaded.'}
  if($batchPhases.Count -eq 1){
   $phase=$batchPhases[0];$console=[IO.File]::ReadAllText($phase.stdout)
   $slotVerified=$console -match '(?m)Process up to 1 jobs in parallel' -and $console -notmatch 'Auto \(0\) selected for maxJobs'
   Save-Json (Join-Path $evidence 'slot-limit-observation.json') @{verified=$slotVerified;nativeExit=$phase.exitCode;stdoutSHA256=$phase.stdoutSha256;expected='Process up to 1 jobs in parallel';limits='CreateJobs worker pool may be distinct; no single-PID-global claim.'}
   if(!$slotVerified){throw 'Literal one-job scheduler confirmation absent.'}
  }
 } catch {$fatal=([string]$fatal+'; postflight: '+$_.Exception.Message).Trim('; ')}
 Save-Json (Join-Path $evidence 'postflight.json') @{freeze=$FreezePath;freezeSHA256=$FreezeSHA256;preCount=if($before){$before.Count}else{0};postCount=if($after){$after.Count}else{0};allFrozenInputsMatched=($null-ne $after -and $after.Count -eq $InputCount);authored=$authoredAfter;ownerTextures=$ownerAfter;cache=$cacheAfter;phases=$script:phases.ToArray();focalComplete=($null-ne $focal);fatal=$fatal;globalAPAcceptance='NOT_CLAIMED; raw exit preserved incl1';GemIncrement=0;limits='Actual fresh builder jobs/products and dependencies for parent review only; no runtime Ready/render/GPU/Gem acceptance.'}
}
if($fatal){throw $fatal}
Write-Output 'FOCAL84_EVIDENCE_CAPTURED_PARENT_REVIEW_REQUIRED; NO_GLOBAL_AP_PASS'
