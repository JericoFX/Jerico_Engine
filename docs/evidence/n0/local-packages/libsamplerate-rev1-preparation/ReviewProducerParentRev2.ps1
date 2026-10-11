#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$taskRepo='F:\JericoEngine\JericoEngine'
$taskPrep=Join-Path $taskRepo 'build/n0-validation/local-packages/libsamplerate-rev1-preparation'
$taskRun=Join-Path $taskPrep 'producer-run'
$taskOutput=Join-Path $taskRepo 'build/n0-validation/local-packages/libsamplerate-rev1-build'
function Hash-TaskFile([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
function Read-TaskJson([string]$path){[IO.File]::ReadAllText($path)|ConvertFrom-Json}
$taskResult=Read-TaskJson (Join-Path $taskRun 'producer-result.json')
$taskStart=Read-TaskJson (Join-Path $taskRun 'native-start.json')
$taskWrapper=Read-TaskJson (Join-Path $taskRun 'wrapper-start.json')
$taskLauncher=Read-TaskJson (Join-Path $taskPrep 'luna-launch-preflight-rev1.json')
if($taskResult.nativeExitCode -ne 0 -or -not $taskResult.nativeAttempted -or $taskResult.wrapperFailure -or $taskResult.nativePID -ne $taskStart.PID -or $taskResult.wrapperPID -ne $taskWrapper.PID -or $taskStart.count -ne 5){throw 'Producer receipt inconsistent'}
foreach($taskPhase in @('preflight-136.json','postflight-136.json')){
    $taskReceipt=Read-TaskJson (Join-Path $taskRun $taskPhase)
    if($taskReceipt.count -ne 136 -or $taskReceipt.matched -ne 136 -or @($taskReceipt.entries|Where-Object{-not $_.matches}).Count){throw 'Producer preservation failed'}
}
foreach($taskLog in $taskResult.rawLogs){if([IO.FileInfo]::new($taskLog.path).Length -ne $taskLog.bytes -or (Hash-TaskFile $taskLog.path) -ne $taskLog.sha256){throw 'Native raw log drift'}}
foreach($taskLabel in @('configure','build-Release')){
    $taskStage=[IO.File]::ReadAllText((Join-Path $taskOutput "$taskLabel-result.txt"))
    if($taskStage -notmatch '(?m)^result=0\r?$'){throw 'Native stage result failed'}
}
$taskFreeze=Read-TaskJson (Join-Path $taskPrep 'driver-freeze.json')
if($taskFreeze.entries.Count -ne 142){throw 'Driver freeze count differs'}
foreach($taskEntry in $taskFreeze.entries){if([IO.FileInfo]::new($taskEntry.path).Length -ne $taskEntry.bytes -or (Hash-TaskFile $taskEntry.path) -ne $taskEntry.sha256){throw 'Current frozen input drift'}}
$taskArchive=Join-Path $taskOutput 'packages/libsamplerate-0.2.1-je-rev1-windows.tar.xz'
if([IO.FileInfo]::new($taskArchive).Length -ne 3947260 -or (Hash-TaskFile $taskArchive) -ne '4B202E429E792B7BA7CBE7B592A22CD4E1FEE5E84C47FA10E23B6C33AA5D3028'){throw 'Archive identity differs'}
$taskCMakeOut=[IO.File]::ReadAllText((Join-Path $taskOutput 'configure-stdout.log'))
$taskCMakeErr=[IO.File]::ReadAllText((Join-Path $taskOutput 'configure-stderr.log'))
if($taskCMakeOut -notmatch 'MSVC 19\.44\.35228\.0' -or $taskCMakeOut -notmatch 'Checking processor clipping capabilities\.\.\. none' -or $taskCMakeErr -notmatch 'CMake Deprecation Warning'){throw 'Expected native environment/probe/warning witness absent'}
$taskVerified=[ordered]@{status='PARENT PRODUCER RESULT REVIEWED; PACKAGE/CONSUMERS/GUARDS STILL PENDING';nativeExit=0;nativePID=$taskStart.PID;wrapperPID=$taskWrapper.PID;elapsedSeconds=$taskResult.nativeElapsedSeconds;preflightInputs=136;postflightInputs=136;currentInputsRehashed=142;inputDeltas=0;archiveBytes=3947260;archiveSHA256=(Hash-TaskFile $taskArchive);nativeResultSHA256=(Hash-TaskFile (Join-Path $taskRun 'producer-result.json'));nativeStartSHA256=(Hash-TaskFile (Join-Path $taskRun 'native-start.json'));launcherSHA256=(Hash-TaskFile (Join-Path $taskPrep 'luna-launch-preflight-rev1.json'));warning='Original minimum-version CMake deprecation retained';processorClipping='Original positive/negative probes returned false, no CPU clipping; not a failed configure/build';consumerAndGuardsExecuted=$false;engineAccepted=$false;catalogIntegrated=$false}
$taskStream=[IO.File]::Open((Join-Path $taskPrep 'parent-producer-result-review-rev1.json'),[IO.FileMode]::CreateNew)
try{$taskBytes=[Text.UTF8Encoding]::new($false).GetBytes(($taskVerified|ConvertTo-Json -Depth 5)+"`n");$taskStream.Write($taskBytes,0,$taskBytes.Length)}finally{$taskStream.Dispose()}
$taskVerified | ConvertTo-Json -Compress
