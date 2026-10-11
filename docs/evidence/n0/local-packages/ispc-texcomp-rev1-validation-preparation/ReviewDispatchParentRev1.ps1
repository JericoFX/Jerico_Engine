#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Get-TaskHash([string]$p){$s=[IO.File]::OpenRead($p);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
$taskPins=@{
    'assignment.json'='61D17229600D38C523CBACC907174C210E397F3B4833A9477BB3C64F664CA67C'
    'driver-freeze.json'='75F511CEA9147BA4FE821018CB96C9122F31151AB05090ACA65517CC33DBF2BA'
    'validation-inputs.json'='DF45F46828446FE0113777546CFDE2983FC099588404548821C9D24A7B6E8710'
    'CommonExecution.ps1'='8858FF1B4A7832596FEF54EA2F3464928FC22AA59A3AD539CAE2366FB8A2E329'
    'RunConsumer.ps1'='685C6B5D8CC0F267ECEB738629A249C006CCB532F1A644EF060A1A6C584DBA89'
    'RunGuards.ps1'='4D789E0AAE96A0751CF6DBEB857627EC57680E5982951C148FDBA4A0191416FB'
}
$taskOutput=Join-Path $PSScriptRoot 'parent-dispatch-review-rev1.json'
if(Test-Path -LiteralPath $taskOutput){throw 'Preserve existing review'}
foreach($name in $taskPins.Keys){if((Get-TaskHash (Join-Path $PSScriptRoot $name)) -ne $taskPins[$name]){throw "Preparation changed: $name"}}
$taskFreeze=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'driver-freeze.json') -Raw | ConvertFrom-Json
if($taskFreeze.count -ne 496 -or $taskFreeze.entries.Count -ne 496){throw 'Driver membership differs'}
foreach($e in $taskFreeze.entries){
    if(-not [IO.File]::Exists($e.path) -or [IO.FileInfo]::new($e.path).Length -ne $e.bytes -or (Get-TaskHash $e.path) -ne $e.sha256){throw "Frozen input changed: $($e.path)"}
    if([IO.File]::GetAttributes($e.path) -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse input rejected'}
}
$taskErrors=0
foreach($name in @('CommonExecution.ps1','RunConsumer.ps1','RunGuards.ps1')){
    $tokens=$null;$errors=$null
    [void][Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot $name),[ref]$tokens,[ref]$errors)
    $taskErrors+=$errors.Count
}
if($taskErrors){throw 'Driver syntax errors'}
$taskAssignment=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'assignment.json') -Raw | ConvertFrom-Json
if($taskAssignment.inputs.count -ne 490 -or $taskAssignment.consumer.cases -ne 8 -or $taskAssignment.guards.cases -ne 9){throw 'Assignment membership differs'}
foreach($p in @($taskAssignment.consumer.freshBuild,$taskAssignment.consumer.evidence,$taskAssignment.guards.fixtures,$taskAssignment.guards.evidence)){if(Test-Path -LiteralPath $p){throw 'Assigned native outputs must be fresh'}}
$taskReceipt=[ordered]@{
    decision='PARENT DISPATCH APPROVED; CONSUMER FIRST, GUARDS SEPARATELY; NATIVE NOT RUN'
    sourceAndDriverInputsRehashed=496;validationInputs=490;syntaxErrors=0;pins=$taskPins
    consumerCases=8;configurations=@('Release','Debug');producerGuards=9
    review='All three drivers, literal argv and original guard authority read. Fresh audited extracted SDK; no producer/re-extraction. Byte streams, immediate native exits, JUnit membership, pre/postflight and separate CreateNew receipts retained.'
    DLLProof='Expected copied path/hash recorded; actual loaded DLL parent asserted by frozen source, no path text captured.'
    nativeOwner='/root/validation_luna';acceptanceOwner='/root';engineAccepted=$false;catalogIntegrated=$false
}
$taskStream=[IO.File]::Open($taskOutput,[IO.FileMode]::CreateNew)
try{$taskBytes=[Text.UTF8Encoding]::new($false).GetBytes(($taskReceipt | ConvertTo-Json -Depth 8)+"`n");$taskStream.Write($taskBytes)}finally{$taskStream.Dispose()}
[ordered]@{decision=$taskReceipt.decision;sha256=(Get-TaskHash $taskOutput);inputs=496} | ConvertTo-Json
