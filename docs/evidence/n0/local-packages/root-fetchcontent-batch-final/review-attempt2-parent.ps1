#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference = 'Stop'
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskBase = Join-Path $taskRepo 'build/n0-validation/local-packages/root-fetchcontent-batch-final'
$taskRun = Join-Path $taskBase 'attempt2'
function Read-TaskJson([string]$path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
$taskAssignment = Read-TaskJson (Join-Path $taskBase 'root-batch-assignment.json')
$taskArgv = Read-TaskJson (Join-Path $taskRun 'configure-argv.json')
if ($taskArgv.arguments.Count -ne 61 -or $taskAssignment.arguments.Count -ne 61) { throw 'Argv count differs' }
for ($taskIndex = 0; $taskIndex -lt 61; $taskIndex++) {
    if ($taskArgv.arguments[$taskIndex] -cne $taskAssignment.arguments[$taskIndex]) { throw "Argv differs at $taskIndex" }
}
$taskResult = Read-TaskJson (Join-Path $taskRun 'configure-result.json')
$taskStderr = Get-Content -LiteralPath (Join-Path $taskRun 'configure.stderr.log') -Raw
if ($taskResult.exitCode -ne 1 -or $taskStderr -notmatch '^CMake Error at cmake/3rdPartyPackages.cmake:331' -or
    $taskStderr -notmatch 'vulkan-validationlayers-1\.3\.261-rev1-windows' -or
    $taskStderr -notmatch '79132c6379e0d167c7a2f5a8e251861759a71d92f1fb8cc3b873df821d83ae51' -or
    $taskStderr -notmatch 'Gems/Atom/RHI/Vulkan/Code/CMakeLists.txt:177') { throw 'Native failure differs' }
$taskDeltas = @(Read-TaskJson (Join-Path $taskRun 'input-postflight-deltas.json'))
$taskChecks = [Collections.Generic.List[object]]::new()
foreach ($taskDelta in $taskDeltas) {
    $taskStem = $taskDelta.manifest -replace '-before$', ''
    $taskBefore = Read-TaskJson (Join-Path $taskRun "$taskStem-before.json")
    $taskAfter = Read-TaskJson (Join-Path $taskRun "$taskStem-after.json")
    $taskRows = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($taskRow in $taskBefore.files) {
        $taskKey = if ($taskRow.path) { $taskRow.path.Replace('\', '/') } else { $taskRow.relativePath.Replace('\', '/') }
        $taskRows.Add($taskKey, $taskRow)
    }
    $taskChanged = [Collections.Generic.List[string]]::new()
    foreach ($taskRow in $taskAfter.files) {
        $taskKey = if ($taskRow.path) { $taskRow.path.Replace('\', '/') } else { $taskRow.relativePath.Replace('\', '/') }
        if (-not $taskRows.ContainsKey($taskKey)) { $taskChanged.Add($taskKey); continue }
        $taskOld = $taskRows[$taskKey]
        if ($taskRow.sha256 -ne $taskOld.sha256 -or $taskRow.bytes -ne $taskOld.bytes -or $taskRow.exists -ne $taskOld.exists) { throw "Changed existing input: $taskKey" }
        [void]$taskRows.Remove($taskKey)
    }
    if ($taskRows.Count -ne 0 -or $taskChanged.Count -ne $taskDelta.deltaCount -or
        $taskBefore.files.Count -ne $taskDelta.beforeCount -or $taskAfter.files.Count -ne $taskDelta.afterCount) { throw "Snapshot comparison differs: $taskStem" }
    if ($taskStem -ne 'engine-packages' -and $taskChanged.Count -ne 0) { throw "Unexpected delta: $taskStem" }
    $taskChecks.Add([ordered]@{manifest=$taskStem;before=$taskBefore.files.Count;after=$taskAfter.files.Count;added=$taskChanged.Count;changedExisting=0;removed=0})
}
$taskPackageAfter = Read-TaskJson (Join-Path $taskRun 'engine-packages-after.json')
$taskProviderSummary = Read-TaskJson (Join-Path $taskRepo 'build/n0-validation/local-packages/d3dx12-rev1-validation/d3dx12-final-validation-summary.json')
$taskStage = $taskProviderSummary.package.extractRoot
$taskVerifiedPayloads = 0
foreach ($taskRow in $taskPackageAfter.files | Where-Object {$_.relativePath -like 'd3dx12-headers-je-rev1-windows\*'}) {
    $taskTail = $taskRow.relativePath.Substring('d3dx12-headers-je-rev1-windows\'.Length)
    $taskLive = Join-Path $taskPackageAfter.path $taskRow.relativePath
    $taskProvider = Join-Path $taskStage $taskTail
    if ((Get-FileHash -LiteralPath $taskLive -Algorithm SHA256).Hash -ne $taskRow.sha256 -or
        (Get-FileHash -LiteralPath $taskProvider -Algorithm SHA256).Hash -ne $taskRow.sha256) { throw "d3dx12 extraction differs: $taskTail" }
    $taskVerifiedPayloads++
}
if ($taskVerifiedPayloads -ne 14) { throw 'Expected fourteen accepted d3dx12 payloads plus stamp' }
$taskFreezeBefore = Read-TaskJson (Join-Path $taskRun 'root-freeze-before.json')
$taskFreezeAssignment = @(Read-TaskJson (Join-Path $taskBase 'source-freeze-root-batch.json'))
if ($taskFreezeAssignment.Count -ne 15529 -or $taskFreezeBefore.files.Count -ne 15529) { throw 'Root freeze count differs' }
$taskFreezeMap = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($taskRow in $taskFreezeAssignment) {
    $taskFullPath = if ([IO.Path]::IsPathRooted($taskRow.path)) { $taskRow.path } else { Join-Path $taskRepo $taskRow.path }
    $taskFreezeMap.Add([IO.Path]::GetFullPath($taskFullPath).Replace('\','/'), $taskRow)
}
foreach ($taskRow in $taskFreezeBefore.files) {
    $taskExpected = $taskFreezeMap[$taskRow.path.Replace('\','/')]
    if (-not $taskExpected -or $taskExpected.sha256 -ne $taskRow.sha256 -or $taskExpected.bytes -ne $taskRow.bytes -or -not $taskRow.exists) { throw "Frozen input differs: $($taskRow.path)" }
}
$taskAttempt1 = Read-TaskJson (Join-Path $taskBase 'attempt1/luna-driver-failure-result.json')
if ($taskAttempt1.nativeCMakeStatus -ne 'NOT_RUN' -or $null -ne $taskAttempt1.nativeCMakeExitCode -or $taskAttempt1.postflight -ne 'NOT_REACHED') { throw 'Invalid attempt1 attribution' }
$taskReviewedFiles = @('root-batch-assignment.json','source-freeze-root-batch.json','run-root-configure-batch-attempt2.ps1','attempt2-launcher-preflight.json','attempt1/luna-driver-failure-result.json','attempt2/configure-result.json','attempt2/configure-argv.json','attempt2/configure.stdout.log','attempt2/configure.stderr.log','attempt2/input-postflight-deltas.json','attempt2/cmake-tool-version.json')
$taskHashes = foreach ($taskName in $taskReviewedFiles) { [ordered]@{path=$taskName;sha256=(Get-FileHash -LiteralPath (Join-Path $taskBase $taskName) -Algorithm SHA256).Hash} }
$taskReceipt = [ordered]@{
    status='REVIEWED_CONFIGURE_FAILURE_ONLY';timestampUtc=[DateTime]::UtcNow.ToString('o');argumentsMatched=61;rootFreezeMatched=15529
    nativeCMakeExitCode=$taskResult.exitCode;elapsedSeconds=$taskResult.elapsedSeconds
    firstFatalPackage='vulkan-validationlayers-1.3.261-rev1-windows';packageSha256='79132c6379e0d167c7a2f5a8e251861759a71d92f1fb8cc3b873df821d83ae51'
    snapshotsCompared=$taskChecks.ToArray();acceptedD3dx12PayloadHashesMatched=14;acceptedD3dx12StampAdded=1
    memoryScope=$taskResult.memoryScope;cmakeParentPeakWorkingSetBytes=$taskResult.cmakeParentPeakWorkingSetBytes
    attempt1='Launcher failure, CMake NOT_RUN; preserved, no postflight evidence';engineBuild='NOT_RUN';editorRuntimeGpu='NOT_RUN'
    limitations='13 source overrides are supplied, but not all configured before fatal. No engine/Editor success or performance acceptance.'
    reviewedFiles=$taskHashes
}
$taskReceipt | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $taskBase 'parent-attempt2-result-review.json') -Encoding utf8
[ordered]@{snapshots=$taskChecks.Count;freezeMatched=15529;argvMatched=61;acceptedPayloads=14;receiptSHA256=(Get-FileHash -LiteralPath (Join-Path $taskBase 'parent-attempt2-result-review.json') -Algorithm SHA256).Hash} | ConvertTo-Json -Compress
