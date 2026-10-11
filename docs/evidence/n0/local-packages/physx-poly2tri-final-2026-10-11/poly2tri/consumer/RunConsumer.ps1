#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
BeginRun 'consumer-attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try {
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'poly2tri consumer preflight mismatch'}
    $assignment=Get-Content -LiteralPath "$PSScriptRoot/consumer-assignment.json" -Raw|ConvertFrom-Json
    if((GetHash $assignment.archivePath) -cne $assignment.archiveSHA256){throw 'Consumer archive pin mismatch'}
    $configured=Native 'consumer-configure' $script:CMake ([string[]]$assignment.consumer.arguments)
    if($configured.exitCode -ne 0){throw "Consumer configure failed: $($configured.exitCode)"}
    foreach($config in @('Debug','Profile','Release')) {
        $built=Native "consumer-build-$config" $script:CMake ([string[]]@('--build',$assignment.consumer.buildRoot,'--config',$config,'--parallel','6','--target','JEPoly2TriConsumer'))
        if($built.exitCode -ne 0){throw "Consumer build $config failed: $($built.exitCode)"}
        $junit=Join-Path $script:RunRoot ($config+'.junit.xml')
        $tested=Native "consumer-ctest-$config" $script:CTest ([string[]]@('--test-dir',$assignment.consumer.buildRoot,'-C',$config,'--output-on-failure','--no-tests=error','--output-junit',$junit))
        if($tested.exitCode -ne 0){throw "Consumer CTest $config failed: $($tested.exitCode)"}
        [xml]$xml=[IO.File]::ReadAllText($junit);$cases=@($xml.SelectNodes('//testcase'))
        if($cases.Count -ne 4 -or @($xml.SelectNodes('//failure|//error|//skipped')).Count -ne 0){throw 'Require exactly four cases per configuration with zero failures/skips'}
        foreach($mode in @('concave','hole','steiner','repeated')) {
            if(@($cases|Where-Object {$_.name -ceq "JE.LocalPackages.Poly2Tri.$mode"}).Count -ne 1){throw "Missing/duplicate test: $mode"}
        }
        $checks+=([ordered]@{configuration=$config;cases=4;failures=0;skips=0;libraryMapping=$(if($config -eq 'Debug'){'Debug/MDd'}else{'Release/MD'})})
    }
    $status='TWELVE_FOCAL_CONSUMER_CASES_COMPLETED; PARENT_ACCEPTANCE_PENDING'
}catch{$failure=$_.Exception.Message;$status='CONSUMER_DRIVER_FAILED'}
finally{$result=EndRun $status $failure $checks;$result|ConvertTo-Json -Depth 12}
if($failure){exit 1}
exit 0
