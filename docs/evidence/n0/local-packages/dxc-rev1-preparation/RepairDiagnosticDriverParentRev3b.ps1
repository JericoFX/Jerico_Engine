#Requires -Version 7.0
$ErrorActionPreference='Stop'
$taskOldDriver="$PSScriptRoot/RunDxcConsumerDiagnosticParentRev3.ps1"
$taskOldAssignment="$PSScriptRoot/parent-consumer-diagnostic-assignment-rev3.json"
$taskDriver="$PSScriptRoot/RunDxcConsumerDiagnosticParentRev3b.ps1"
$taskAssignment="$PSScriptRoot/parent-consumer-diagnostic-assignment-rev3b.json"
foreach($path in @($taskDriver,$taskAssignment)){if(Test-Path -LiteralPath $path){throw 'Preserve previous preparation'}}
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
if((Get-TaskHash $taskOldDriver) -ne '3EED1BC0C64262B2E1496E0DC32E96D7AC961D61A3A537FE23133528D842256E' -or (Get-TaskHash $taskOldAssignment) -ne '8B1869D8C9A7F3207FC5F04A3BFFF13ED8523A57D86F3018C687344A93FD52EB'){throw 'Reviewed historical evidence changed'}
$taskText=[IO.File]::ReadAllText($taskOldDriver)
$taskBad='source-freeze-dxc-consumer-rev2.json'
$taskGood='source-freeze-dxc-consumer-rev3.json'
if(([regex]::Matches($taskText,[regex]::Escape($taskBad))).Count -ne 1){throw 'Unexpected driver path delta'}
$taskFixed=$taskText.Replace($taskBad,$taskGood)
if($taskFixed.Replace($taskGood,$taskBad) -cne $taskText){throw 'Driver inverse differs'}
$tokens=$null;$errors=$null
[void][Management.Automation.Language.Parser]::ParseInput($taskFixed,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Driver parser errors'}
$taskFreeze="$PSScriptRoot/consumer-candidate-rev3/$taskGood"
if((Get-TaskHash $taskFreeze) -ne 'B06DB30C0DCDEC4EF685C8606C0A8FD19F54D1B1E967C90DA35DB4842750B909'){throw 'Actual candidate freeze differs'}
$taskWorkerAssignment="$PSScriptRoot/consumer-candidate-rev3/validation-assignment.json"
if((Get-TaskHash $taskWorkerAssignment) -ne 'A321C3DF24DF972D0FC963C024F733E73F6263E61D278DF288EEA012F2D34705'){throw 'Actual worker assignment differs'}
$taskInputCount=(Get-Content -Raw -LiteralPath $taskFreeze | ConvertFrom-Json).files.Count
if($taskInputCount -ne 23379){throw 'Actual freeze input count differs'}
foreach($path in @('F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-rev1-validation/consumer-diagnostic-attempt3','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-native-consumer-rev3')){if(Test-Path -LiteralPath $path){throw 'Native outputs already exist'}}
[IO.File]::WriteAllText($taskDriver,$taskFixed,[Text.UTF8Encoding]::new($false))
$taskReceipt=Get-Content -Raw -LiteralPath $taskOldAssignment | ConvertFrom-Json
$taskReceipt.driver=$taskDriver;$taskReceipt.driverSHA256=Get-TaskHash $taskDriver
$taskReceipt | Add-Member -NotePropertyName harnessCorrection -NotePropertyValue 'Correct exactly one frozen file basename rev2 to rev3. Previous preflight caught absence before any native execution; previous evidence retained.'
$taskReceipt | Add-Member -NotePropertyName inverseExact -NotePropertyValue $true
[IO.File]::WriteAllText($taskAssignment,($taskReceipt | ConvertTo-Json -Depth 12)+"`n",[Text.UTF8Encoding]::new($false))
[ordered]@{driverSHA256=$taskReceipt.driverSHA256;assignmentSHA256=(Get-TaskHash $taskAssignment);actualFreezeInputs=$taskInputCount;parserErrors=0;nativeExecuted=$false} | ConvertTo-Json
