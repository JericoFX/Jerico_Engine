#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference = 'Stop'
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskRoot = $PSScriptRoot
function Get-TaskHash([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try { [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream)) }
    finally { $stream.Dispose() }
}
$taskReviewPath = Join-Path $taskRoot 'parent-native-result-review.json'
if ([IO.File]::Exists($taskReviewPath)) { throw 'Fresh parent review required' }
$taskLunaPath = Join-Path $taskRoot 'luna-reuse-review-rev1.json'
$taskLuna = Get-Content -Raw -LiteralPath $taskLunaPath | ConvertFrom-Json
$taskAssignment = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'root-batch-assignment.json') | ConvertFrom-Json
$taskArgv = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'attempt1/configure-argv.json') | ConvertFrom-Json
$taskResult = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'attempt1/configure-result.json') | ConvertFrom-Json
$taskDeltas = @(Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'attempt1/input-postflight-deltas.json') | ConvertFrom-Json)
if ($taskAssignment.arguments.Count -ne 61 -or $taskArgv.arguments.Count -ne 61) { throw 'Root argv count differs' }
for ($i = 0; $i -lt 61; $i++) {
    if ($taskAssignment.arguments[$i] -cne $taskArgv.arguments[$i]) { throw 'Root argv differs from reviewed assignment' }
}
if ((Get-TaskHash (Join-Path $taskRoot 'root-batch-assignment.json')) -ne $taskArgv.assignmentSha256 -or
    (Get-TaskHash (Join-Path $taskRoot 'source-freeze-root-batch.json')) -ne $taskArgv.freezeSha256 -or
    (Get-TaskHash (Join-Path $taskRoot 'run-root-configure-dxc-parent-rev1.ps1')) -ne $taskLuna.driver.expectedSHA256) { throw 'Reviewed Root provenance differs' }
if ($taskLuna.freeze.entryCount -ne 15674 -or $taskLuna.freeze.mismatches.Count -ne 0 -or
    $taskLuna.rehashedInputs.cache.checkedCount -ne 36 -or $taskLuna.rehashedInputs.cache.mismatches.Count -ne 0 -or
    $taskLuna.rehashedInputs.wheelhouse.checkedCount -ne 44 -or $taskLuna.rehashedInputs.wheelhouse.mismatches.Count -ne 0 -or
    $taskLuna.activeCMakeMSBuildCompilerProcesses.Count -ne 0) { throw 'Current reuse review is inconsistent' }
if ($taskDeltas.Count -ne 41 -or $taskLuna.postflight.manifestCount -ne 41 -or $taskLuna.postflight.unexpectedDeltaManifests.Count -ne 0) { throw 'Postflight count/deltas differ' }
$taskChanged = @($taskDeltas | Where-Object deltaCount -ne 0)
if ($taskChanged.Count -ne 1 -or $taskChanged[0].manifest -ne 'engine-packages-before' -or
    $taskChanged[0].beforeCount -ne 6573 -or $taskChanged[0].afterCount -ne 6589 -or $taskChanged[0].deltaCount -ne 16) { throw 'Unexpected deployment delta' }
if ($taskLuna.dxc.payloadComparedCount -ne 15 -or $taskLuna.dxc.payloadMismatch.Count -ne 0 -or
    $taskLuna.dxc.archiveMemberCount -ne 15 -or $taskLuna.dxc.sha256ManifestPayloadRecords -ne 14 -or
    -not $taskLuna.dxc.archiveMatchesAcceptedAudit -or -not $taskLuna.dxc.catalogHashMatches -or
    -not $taskLuna.dxc.stampPresent -or $taskLuna.dxc.stampBytes -ne 0 -or
    $taskLuna.dxc.archiveSHA256 -ne '62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782') { throw 'DXC deployment review differs' }
$taskStderr = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'attempt1/configure.stderr.log')
if ($taskResult.exitCode -ne 1 -or $taskResult.elapsedSeconds -ne 29.346 -or
    $taskStderr -notmatch 'JE package unavailable offline: astc-encoder-3\.2-rev2-windows' -or
    $taskStderr -notmatch '17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295') { throw 'Recorded native blocker differs' }
$taskLoose = Join-Path $taskRepo 'build/n0-validation/local-packages/PrepareRootDxcParentRev1.ps1'
$taskMirror = Join-Path $taskRoot 'PrepareRootDxcParentRev1.ps1'
if ([IO.File]::Exists($taskMirror)) { throw 'Fresh preparation mirror required' }
[IO.File]::Copy($taskLoose, $taskMirror, $false)
if ((Get-TaskHash $taskLoose) -ne (Get-TaskHash $taskMirror)) { throw 'Preparation mirror bytes differ' }
$taskReview = [ordered]@{
    status = 'PARENT REVIEWED RECORDED BLOCKED CONFIGURE; REUSE FOR NEXT BLOCKER ONLY'
    recordedNativeExit = 1; elapsedSeconds = 29.346; arguments = 61; freezeCount = 15674
    cacheCount = 36; wheelhouseCount = 44; postflightManifests = 41; unexpectedDeltas = 0
    deployedDxcMembers = 15; deployedDxcStampBytes = 0; enginePackagesBefore = 6573; enginePackagesAfter = 6589
    firstFatalPackage = 'astc-encoder-3.2-rev2-windows'
    firstFatalSHA256 = '17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295'
    cmakeParentPeakWorkingSetBytes = $taskResult.cmakeParentPeakWorkingSetBytes
    lunaReviewSHA256 = Get-TaskHash $taskLunaPath
    nativeResultSHA256 = Get-TaskHash (Join-Path $taskRoot 'attempt1/configure-result.json')
    argvSHA256 = Get-TaskHash (Join-Path $taskRoot 'attempt1/configure-argv.json')
    postflightSHA256 = Get-TaskHash (Join-Path $taskRoot 'attempt1/input-postflight-deltas.json')
    prepareMirrorSHA256 = Get-TaskHash $taskMirror
    attribution = 'The retained configure records match current inputs but omit wrapper/launcher attribution. The later Luna dispatch was NOT_RUN because attempt1 existed. No rerun and no attribution to that dispatch.'
    acceptance = 'DXC focal package accepted separately. No complete Root generation, engine build, Editor/AP/runtime, device/frame or dependency-separation acceptance.'
}
[IO.File]::WriteAllText($taskReviewPath, ($taskReview | ConvertTo-Json -Depth 6) + "`n", [Text.UTF8Encoding]::new($false))
[ordered]@{status=$taskReview.status;sha256=(Get-TaskHash $taskReviewPath)} | ConvertTo-Json -Compress
