#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
$extra=Get-Content -LiteralPath "$PSScriptRoot/profile-build-inputs.json" -Raw|ConvertFrom-Json
function AssertProfileInputs([string]$receiptName){
    $checks=@(foreach($entry in $extra.entries){
        $hash=GetHash $entry.path
        [ordered]@{path=$entry.path;sha256=$hash;matches=($hash -ceq $entry.sha256 -and ([IO.FileInfo]::new($entry.path)).Length -eq $entry.bytes)}
    })
    WriteReceipt $receiptName $checks
    if(@($checks|Where-Object {-not $_.matches}).Count){throw 'Profile build generated/configuration input mismatch'}
}
BeginRun 'profile-build-attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try{
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Original configure input mismatch'}
    AssertProfileInputs 'supplement-preflight.json'
    $phase=Native 'build-profile' $script:CMake ([string[]]$extra.arguments)
    $status='BUILD_OBSERVED'
    if($phase.exitCode -ne 0){throw "PhysX profile compilation failed: $($phase.exitCode)"}
    $checks=@([ordered]@{scope='SDK static profile libraries only; consumer, package and editor not verified';exitCode=$phase.exitCode})
}catch{$failure=$_.Exception.Message;$status='BUILD_DRIVER_FAILED'}
finally{
    try{AssertProfileInputs 'supplement-postflight.json'}catch{$failure=$_.Exception.Message;$status='BUILD_DRIVER_FAILED'}
    $result=EndRun $status $failure $checks
    $result|ConvertTo-Json -Depth 10
}
if($failure){exit 1}
exit 0