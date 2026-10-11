#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
BeginRun 'attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try {
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Configure input preflight mismatch'}
    $assignment=Get-Content -LiteralPath "$PSScriptRoot/assignment.json" -Raw|ConvertFrom-Json
    if(Test-Path -LiteralPath $assignment.buildRoot){throw 'Fresh build directory required'}
    $phase=Native 'configure' $script:CMake ([string[]]$assignment.configure.arguments)
    $status='CONFIGURE_OBSERVED'
    if($phase.exitCode -ne 0){throw "PhysX CPU configure failed: $($phase.exitCode)"}
    $checks=@([ordered]@{scope='Configure only; no SDK library build or consumer execution';exitCode=$phase.exitCode})
}catch{$failure=$_.Exception.Message;$status='CONFIGURE_DRIVER_FAILED'}
finally {
    $assignment=Get-Content -LiteralPath "$PSScriptRoot/assignment.json" -Raw|ConvertFrom-Json
    $baseline=@($script:InputFreeze.entries|Where-Object {$_.path.StartsWith($assignment.privateRoot+'/',[StringComparison]::OrdinalIgnoreCase)}|ForEach-Object path)
    $additions=@(Get-ChildItem -LiteralPath $assignment.privateRoot -File -Recurse -Force|ForEach-Object {$_.FullName.Replace('\','/')}|Where-Object {$_ -notin $baseline})
    $allowed=$assignment.privateRoot+'/physx/include/PxConfig.h'
    if(@($additions|Where-Object {$_ -cne $allowed}).Count){$failure='Unexpected private source additions';$status='CONFIGURE_DRIVER_FAILED'}
    WriteReceipt 'private-source-additions.json' ([ordered]@{additions=$additions;onlyAllowedGeneratedFile=$allowed;existingFiles='All immutable entries checked by Common postflight'})
    $result=EndRun $status $failure $checks;$result|ConvertTo-Json -Depth 10
}
if($failure){exit 1}
exit 0
