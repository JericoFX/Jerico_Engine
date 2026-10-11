#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$taskRepo='F:\JericoEngine\JericoEngine'
$taskAudit=Join-Path $taskRepo 'build/n0-validation/local-packages/libsamplerate-rev1-audit'
$taskPrep=Join-Path $taskRepo 'build/n0-validation/local-packages/libsamplerate-rev1-preparation'
function Hash-TaskFile([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$taskPins=@(
    @((Join-Path $taskAudit 'source-freeze-libsamplerate-rev1.json'),'56C34D9835C18C444EFA77F4B2042B1A31D938E49E0BF74948CB7DCA8550A986'),
    @((Join-Path $taskAudit 'validation-arguments-rev1.json'),'B623FE1D877D8F83A22E9D5D124C13E3DDEF875AD0A9BDC0D06FD821093BC585'),
    @((Join-Path $taskAudit 'candidate-receipt-rev1.json'),'317B13C9F763D36CD6F3AB0116435EB638E45F882D2F93DF8088F56412331090'),
    @((Join-Path $taskPrep 'RunProducer.ps1'),'72DE9CA77C2C481D8E1358E16BE5090D1C79752EB47690C4905A2B2E090EE4A2'),
    @((Join-Path $taskPrep 'assignment.json'),'1F8E767A764C9CD7902AC2CD97080D72431C9A219BB1A4E21B322B341E632D4C'),
    @((Join-Path $taskPrep 'driver-freeze.json'),'37ABA24BA679D6BE0F707E3096ABB5C66660D9D44E335BB59C33AF0E32D9BB4F'),
    @((Join-Path $taskRepo 'build/n0-validation/local-packages/libsamplerate-rev1-review-luna/review-rev1.json'),'FF788D325F8D5E1F3B1E8D519DABAFF44AD9D86907E93292EEA9111C6E0B850B'))
foreach($taskPin in $taskPins){if((Hash-TaskFile $taskPin[0]) -ne $taskPin[1]){throw "Dispatch authority drift: $($taskPin[0])"}}
$taskSourceFreeze=[IO.File]::ReadAllText($taskPins[0][0])|ConvertFrom-Json
$taskDriverFreeze=[IO.File]::ReadAllText($taskPins[5][0])|ConvertFrom-Json
if($taskSourceFreeze.entries.Count -ne 136 -or $taskDriverFreeze.entries.Count -ne 142){throw 'Freeze count mismatch'}
foreach($taskEntry in $taskDriverFreeze.entries){
    if(-not [IO.File]::Exists($taskEntry.path) -or [IO.FileInfo]::new($taskEntry.path).Length -ne $taskEntry.bytes -or (Hash-TaskFile $taskEntry.path) -ne $taskEntry.sha256){throw "Frozen input drift: $($taskEntry.path)"}
}
$taskSource='F:\JericoEngine\Dependencies\libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b'
$taskMembers=@(Get-ChildItem -LiteralPath $taskSource -File -Recurse -Force)
if($taskMembers.Count -ne 99){throw 'Snapshot membership differs'}
foreach($taskMember in Get-ChildItem -LiteralPath $taskSource -Recurse -Force){if($taskMember.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Snapshot reparse point'}}
$taskAuthority=[IO.File]::ReadAllText($taskPins[1][0])|ConvertFrom-Json
$taskAssignment=[IO.File]::ReadAllText($taskPins[4][0])|ConvertFrom-Json
if($taskAuthority.producer.count -ne 5 -or $taskAuthority.producer.arguments.Count -ne 5 -or $taskAssignment.nativeArgumentCount -ne 5){throw 'Producer argv differs'}
if([IO.Directory]::Exists($taskAssignment.freshOutput) -or [IO.Directory]::Exists($taskAssignment.runEvidence)){throw 'Producer output/attempt already exists'}
$taskTokens=$null;$taskErrors=$null
[void][Management.Automation.Language.Parser]::ParseFile($taskPins[3][0],[ref]$taskTokens,[ref]$taskErrors)
if($taskErrors.Count){throw 'Driver parser errors'}
$taskResult=[ordered]@{status='PARENT_SOURCE_AND_DISPATCH_REVIEWED';sourceFreezeEntries=136;currentDriverInputsRehashed=142;snapshotFiles=99;producerArgumentCount=5;parseErrors=0;sourceFilesRead=6;sourceAndAuditReaderRead=$true;lunaReviewSHA256=$taskPins[6][1];assignmentSHA256=$taskPins[4][1];nativeExecuted=$false;allowedNext='Luna single prepared producer only, preserved CreateNew prelaunch and native PID/argv/raw streams/prepost136; no retry, consumer, guards, Root or Git';engineAccepted=$false}
$taskPath=Join-Path $taskPrep 'parent-dispatch-review-rev1.json'
$taskStream=[IO.File]::Open($taskPath,[IO.FileMode]::CreateNew)
try{$taskBytes=[Text.UTF8Encoding]::new($false).GetBytes(($taskResult|ConvertTo-Json -Depth 5)+"`n");$taskStream.Write($taskBytes,0,$taskBytes.Length)}finally{$taskStream.Dispose()}
$taskResult | ConvertTo-Json -Compress
