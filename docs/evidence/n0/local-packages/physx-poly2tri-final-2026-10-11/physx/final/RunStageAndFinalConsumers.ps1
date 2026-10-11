#Requires -Version 7.0
#Requires -PSEdition Core
. "$PSScriptRoot/CommonExecution.ps1"
BeginRun 'attempt1'
$failure=$null;$status='NATIVE_NOT_RUN';$checks=@()
try {
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'PhysX final input preflight mismatch'}
    $assignment=Get-Content -LiteralPath "$PSScriptRoot/assignment.json" -Raw|ConvertFrom-Json
    if($assignment.consumers.Count -ne 4 -or $assignment.stage.argc -ne 8 -or $assignment.stage.arguments.Count -ne 8){throw 'PhysX assigned phase contract differs'}
    foreach($path in @($assignment.outputRoot)+@($assignment.consumers|ForEach-Object {$_.buildRoot})){
        if(Test-Path -LiteralPath $path){throw "PhysX assigned output must be fresh: $path"}
    }
    $stage=Native 'stage' $script:CMake ([string[]]$assignment.stage.arguments)
    if($stage.exitCode -ne 0){throw "PhysX stage failed: $($stage.exitCode)"}
    $witness=Get-Content -LiteralPath $assignment.artifactWitness -Raw|ConvertFrom-Json
    $main=@('PhysX','PhysXPvdSDK','PhysXVehicle','PhysXCharacterKinematic','PhysXExtensions','PhysXCooking','PhysXCommon','PhysXFoundation')
    $libraries=@($witness.libraries|Where-Object {$_.target -cin $main})
    if($libraries.Count -ne 32){throw 'Expected thirty-two main witness archives'}
    foreach($entry in $libraries){
        $path="$($assignment.packageRoot)/PhysX/physx/bin/static/$($entry.config)/$($entry.target)_static_64.lib"
        if((GetHash $path) -cne $entry.sha256 -or ([IO.FileInfo]::new($path)).Length -ne $entry.bytes){throw 'Staged main archive differs from original static output'}
    }
    $stagedLibraries=@(Get-ChildItem -LiteralPath "$($assignment.packageRoot)/PhysX/physx/bin/static" -Recurse -File)
    if($stagedLibraries.Count -ne 32 -or @($stagedLibraries|Where-Object {$_.Extension -cne '.lib'}).Count -ne 0){throw 'Staged static membership must contain only the thirty-two main archives'}
    if((GetHash "$($assignment.packageRoot)/PhysX/physx/include/PxConfig.h") -cne $witness.generatedHeader.sha256){throw 'Staged generated header differs'}
    $info=Get-Content -LiteralPath "$($assignment.packageRoot)/PackageInfo.json" -Raw|ConvertFrom-Json
    if($info.source.commit -cne '0bbcff3d0c541325f4d14c36ee18f24e22e35e6e'){throw 'Staged SDK source commit differs'}
    WriteReceipt 'stage-native-membership.json' ([ordered]@{validatedWitnessLibraries=56;stagedMainLibraries=32;matched=32;generatedHeaderMatched=$true;sourceCommit=$info.source.commit;scope='Data equality of existing SDK outputs only; tar/PE/COFF audits and acceptance remain independent'})
    foreach($consumer in $assignment.consumers){
        if($consumer.argc -ne 17 -or $consumer.arguments.Count -ne 17 -or $consumer.testCount -ne 6 -or $consumer.target -cne 'JEPhysXConsumer'){throw 'Assigned contractual consumer differs'}
        $configured=Native ("consumer-configure-"+$consumer.name) $script:CMake ([string[]]$consumer.arguments)
        if($configured.exitCode -ne 0){throw "PhysX $($consumer.name) configure failed: $($configured.exitCode)"}
        $built=Native ("consumer-build-"+$consumer.name) $script:CMake ([string[]]@('--build',$consumer.buildRoot,'--config',$consumer.config,'--parallel','6','--target','JEPhysXConsumer'))
        if($built.exitCode -ne 0){throw "PhysX $($consumer.name) build failed: $($built.exitCode)"}
        $junitPath=Join-Path $script:RunRoot ($consumer.name+'.xml')
        $tested=Native ("consumer-ctest-"+$consumer.name) $script:CTest ([string[]]@('--test-dir',$consumer.buildRoot,'-C',$consumer.config,'--output-on-failure','--no-tests=error','--output-junit',$junitPath))
        if($tested.exitCode -ne 0){throw "PhysX $($consumer.name) CTest failed: $($tested.exitCode)"}
        [xml]$junit=[IO.File]::ReadAllText($junitPath)
        $cases=@($junit.SelectNodes('//testcase'))
        if($cases.Count -ne 6 -or @($junit.SelectNodes('//failure|//error|//skipped')).Count -ne 0){throw 'Require exactly six executed cases, without failures/skips'}
        foreach($mode in @('lifecycle','cooking','scene','character','vehicle','invalid')){
            $name="JE.LocalPackages.PhysX.$mode"
            if(@($cases|Where-Object {$_.name -ceq $name}).Count -ne 1){throw "Missing/duplicate contractual case: $name"}
        }
        $checks+=[ordered]@{mapping=$consumer.name;configuration=$consumer.config;profileUsesChecked=$consumer.profileUsesChecked;target='3rdParty::PhysX5';mainLibraries=8;cases=6;failed=0;skipped=0}
    }
    $status='FINAL_CPU_CONSUMER_MATRIX_COMPLETED'
}catch{$failure=$_.Exception.Message;$status='STAGE_OR_CONSUMER_DRIVER_FAILED'}
finally {$result=EndRun $status $failure $checks;$result|ConvertTo-Json -Depth 12}
if($failure){exit 1}
exit 0
