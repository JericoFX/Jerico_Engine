#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
$extra=Get-Content -LiteralPath "$PSScriptRoot/remaining-build-inputs.json" -Raw|ConvertFrom-Json
function AssertBuildInputs([string]$receiptName){
    $checks=@(foreach($entry in $extra.entries){
        $hash=GetHash $entry.path
        [ordered]@{path=$entry.path;sha256=$hash;matches=($hash -ceq $entry.sha256 -and ([IO.FileInfo]::new($entry.path)).Length -eq $entry.bytes)}
    })
    WriteReceipt $receiptName $checks
    if(@($checks|Where-Object {-not $_.matches}).Count){throw 'Remaining build configuration input mismatch'}
}
BeginRun 'remaining-builds-attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try{
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Original configure input mismatch'}
    AssertBuildInputs 'supplement-preflight.json'
    foreach($configuration in @('debug','checked','release')){
        [string[]]$buildArguments=$extra.arguments.Clone()
        $configIndex=[Array]::IndexOf($buildArguments,'--config')
        if($configIndex -lt 0){throw 'Missing configuration argument'}
        $buildArguments[$configIndex+1]=$configuration
        $phase=Native ('build-'+$configuration) $script:CMake $buildArguments
        if($phase.exitCode -ne 0){throw "PhysX $configuration compilation failed: $($phase.exitCode)"}
        $checks+=@([ordered]@{scope='SDK static libraries only; consumer/package/editor not verified';configuration=$configuration;exitCode=$phase.exitCode})
    }
    $status='BUILDS_OBSERVED'
}catch{$failure=$_.Exception.Message;$status='BUILD_DRIVER_FAILED'}
finally{
    try{AssertBuildInputs 'supplement-postflight.json'}catch{$failure=$_.Exception.Message;$status='BUILD_DRIVER_FAILED'}
    $result=EndRun $status $failure $checks
    $result|ConvertTo-Json -Depth 10
}
if($failure){exit 1}
exit 0