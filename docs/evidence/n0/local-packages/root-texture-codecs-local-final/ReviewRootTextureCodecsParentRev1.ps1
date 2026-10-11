#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskRun="$PSScriptRoot/attempt1"
$taskOutput="$PSScriptRoot/parent-native-result-review.json"
if(Test-Path -LiteralPath $taskOutput){throw 'Preserve existing parent review'}
function Get-TaskHash([string]$p){$s=[IO.File]::OpenRead($p);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
$taskResult=Get-Content -LiteralPath "$taskRun/configure-result.json" -Raw | ConvertFrom-Json
$taskLaunch=Get-Content -LiteralPath "$PSScriptRoot/launcher-preflight-rev1.json" -Raw | ConvertFrom-Json
$taskStart=Get-Content -LiteralPath "$taskRun/configure-native-start.json" -Raw | ConvertFrom-Json
$taskAssignment=Get-Content -LiteralPath "$PSScriptRoot/root-batch-assignment.json" -Raw | ConvertFrom-Json
if($taskResult.exitCode -ne 1 -or $taskLaunch.errors.Count -or $taskLaunch.powerShellVersion -ne '7.6.5' -or $taskStart.argumentCount -ne 61 -or ($taskStart.arguments -join "`n") -cne ($taskAssignment.arguments -join "`n") -or [DateTime]$taskLaunch.utc -ge [DateTime]$taskStart.nativeProcessStartUTC){throw 'Native invocation attribution differs'}
if(-not [IO.File]::Exists($taskLaunch.priorPreflightFailure)){throw 'Preserved initial metadata failure missing'}
$taskFatal=[IO.File]::ReadAllText("$taskRun/configure.stderr.log")
if(-not $taskFatal.Contains('JE package unavailable offline: openimageio-opencolorio-2.3.17-rev4-windows') -or -not $taskFatal.Contains('d542198653b33640268a999de542c9c5adf4a29322ed04858c730bc82eb34b09') -or -not $taskFatal.Contains('Gems/GradientSignal/Code/CMakeLists.txt:66')){throw 'Actual offline blocker differs'}
$taskComparisons=@(Get-Content -LiteralPath "$taskRun/input-postflight-deltas.json" -Raw | ConvertFrom-Json)
if($taskComparisons.Count -ne 41 -or @($taskComparisons | Where-Object {$_.manifest -ne 'engine-packages-before' -and $_.deltaCount -ne 0}).Count){throw 'Unexpected preservation delta'}
$taskPackageDelta=@($taskComparisons | Where-Object manifest -EQ 'engine-packages-before')
if($taskPackageDelta.Count -ne 1 -or $taskPackageDelta[0].beforeCount -ne 6589 -or $taskPackageDelta[0].afterCount -ne 7261 -or $taskPackageDelta[0].deltaCount -ne 672){throw 'Package additions differ'}
$taskSpecs=@(
    @('astc-encoder-3.2-je-rev1-windows',"$taskBase/astc-native-extracted-rev1",63),
    @('ISPCTexComp-36b80aa-je-rev1-windows',"$taskBase/ispc-texcomp-rev1-package-audit/extracted/ISPCTexComp-36b80aa-je-rev1-windows",21),
    @('OpenEXR-3.1.3-je-rev1-windows',"$taskBase/openexr-rev2/packages/OpenEXR-3.1.3-je-rev1-windows.contents",585)
)
$taskPackageRoot="$taskRepo/build/n0-validation/original-engine-deps/packages"
$taskExpected=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$taskPayloads=@()
foreach($spec in $taskSpecs){
    $taskOriginal=@(Get-ChildItem -LiteralPath $spec[1] -Recurse -File -Force)
    $taskInstalled="$taskPackageRoot/$($spec[0])"
    if($taskOriginal.Count -ne $spec[2] -or @(Get-ChildItem -LiteralPath $taskInstalled -Recurse -File -Force).Count -ne $spec[2]){throw 'Actual installed payload membership differs'}
    foreach($fi in $taskOriginal){
        $relative=[IO.Path]::GetRelativePath($spec[1],$fi.FullName).Replace([char]92,[char]47)
        $target="$taskInstalled/$relative"
        if([IO.FileInfo]::new($target).Length -ne $fi.Length -or (Get-TaskHash $target) -ne (Get-TaskHash $fi.FullName)){throw 'Original package bytes differ after native resolution'}
        [void]$taskExpected.Add("$($spec[0])/$relative")
    }
    $taskStamp="$taskPackageRoot/$($spec[0]).stamp"
    if(-not [IO.File]::Exists($taskStamp) -or [IO.FileInfo]::new($taskStamp).Length -ne 0){throw 'Expected validation stamp differs'}
    [void]$taskExpected.Add("$($spec[0]).stamp")
    $taskPayloads+=@{name=$spec[0];payloadFiles=$spec[2];actualByteExact=$true;emptyValidationStamp=$true}
}
foreach($delta in $taskPackageDelta[0].deltas){
    if(-not $delta.StartsWith('changed-or-added:') -or -not $taskExpected.Remove($delta.Substring(17).Replace([char]92,[char]47))){throw 'Unattributed package addition/removal/change'}
}
if($taskExpected.Count){throw 'Expected package addition missing'}
$taskFreeze=@(Get-Content -LiteralPath "$PSScriptRoot/source-freeze-root-batch.json" -Raw | ConvertFrom-Json)
if($taskFreeze.Count -ne 16868){throw 'Current Root freeze count differs'}
foreach($e in $taskFreeze){
    $p=if([IO.Path]::IsPathFullyQualified($e.path)){$e.path}else{Join-Path $taskRepo $e.path}
    if([IO.FileInfo]::new($p).Length -ne $e.bytes -or (Get-TaskHash $p) -ne $e.sha256){throw "Current Root input changed: $p"}
}
$taskPrivate=@(Get-Content -LiteralPath "$taskRun/private-copy-comparison-after.json" -Raw | ConvertFrom-Json)
if(@($taskPrivate | Where-Object {$_.unexpectedDeltas.Count -or $_.deltas.Count}).Count){throw 'Private copies changed'}
$taskReceipt=[ordered]@{
    status='ORIGINAL ROOT61 OFFLINE BLOCKER CONFIRMED; TEXTURE CODEC PACKAGE RESOLUTION VERIFIED'
    nativeExit=1;elapsedSeconds=$taskResult.elapsedSeconds;nativePID=$taskStart.nativePID
    frozenInputs=16868;currentInputsRehashed=16868;postflightManifests=41;unexpectedDeltas=0
    cacheFiles=40;wheelhouseFiles=44;enginePackagesBefore=6589;enginePackagesAfter=7261;attributedPackageAdditions=672;payloads=$taskPayloads
    blocker='openimageio-opencolorio-2.3.17-rev4-windows';blockerSHA256='d542198653b33640268a999de542c9c5adf4a29322ed04858c730bc82eb34b09';caller='Gems/GradientSignal/Code/CMakeLists.txt:66'
    cmakeParentPeakWorkingSetBytes=$taskResult.cmakeParentPeakWorkingSetBytes;memoryScope=$taskResult.memoryScope
    nativeResultSHA256=(Get-TaskHash "$taskRun/configure-result.json");stderrSHA256=(Get-TaskHash "$taskRun/configure.stderr.log");stdoutSHA256=(Get-TaskHash "$taskRun/configure.stdout.log")
    nativeStartSHA256=(Get-TaskHash "$taskRun/configure-native-start.json");launcherSHA256=(Get-TaskHash "$PSScriptRoot/launcher-preflight-rev1.json")
    preservedPreflightFailureSHA256=(Get-TaskHash $taskLaunch.priorPreflightFailure)
    attribution='First launcher metadata counted cache directories and failed before driver/native. Initial RED retained; corrected CreateNew preflight predates actual native PID receipt. Configure executed once.'
    limits='Configuration incomplete; no solution generation, build, Editor/AP/cook/runtime, GPU frame, product RAM/VRAM or effective dependency retirement acceptance.'
    engineAccepted=$false
}
$s=[IO.File]::Open($taskOutput,[IO.FileMode]::CreateNew)
try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($taskReceipt | ConvertTo-Json -Depth 9)+"`n");$s.Write($b)}finally{$s.Dispose()}
[ordered]@{status=$taskReceipt.status;sha256=(Get-TaskHash $taskOutput);currentInputs=16868;payloadFiles=669;emptyStamps=3} | ConvertTo-Json
