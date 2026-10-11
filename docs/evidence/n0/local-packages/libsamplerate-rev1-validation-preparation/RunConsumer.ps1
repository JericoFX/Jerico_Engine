# Execute only after explicit parent dispatch to Luna.
. (Join-Path $PSScriptRoot 'CommonExecution.ps1')
$authority='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-audit/validation-arguments-rev1.json'
if((GetHash $authority) -cne 'B623FE1D877D8F83A22E9D5D124C13E3DDEF875AD0A9BDC0D06FD821093BC585'){throw 'Consumer argument authority changed'}
$plan=Get-Content -LiteralPath $authority -Raw|ConvertFrom-Json
$package='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-package-audit/extracted/libsamplerate-0.2.1-je-rev1-windows'
$build='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/libsamplerate-rev1-consumer'
$library=Join-Path $package 'libsamplerate/lib/samplerate.lib'
if(Test-Path -LiteralPath $build){throw 'Consumer directory must be fresh'}
BeginRun 'consumer-run'
$failure=$null;$checks=@();$status='FAIL_OR_INCOMPLETE'
try{
    if($script:Before.matched -ne $script:InputFreeze.count){throw 'Preflight changed; no native phase dispatched'}
    $configureArguments=[string[]]@('-S','F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/LibSamplerate/tests','-B',$build,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_SAMPLERATE_PACKAGE_ROOT=$package")
    if($configureArguments.Count -ne 11 -or $plan.consumer.count -ne 11 -or $plan.consumer.arguments.Count -ne 11){throw 'Consumer argument count differs'}
    for($i=0;$i -lt 11;$i++){if($configureArguments[$i] -cne $plan.consumer.arguments[$i]){throw "Consumer argument $i differs from frozen authority"}}
    $phase=Native 'configure' $script:CMake $configureArguments
    if($phase.exitCode -ne 0){throw 'Consumer configure failed; no retry'}
    foreach($config in @('Release','Debug')){
        $phase=Native ('build-'+$config) $script:CMake ([string[]]@('--build',$build,'--config',$config))
        if($phase.exitCode -ne 0){throw "$config consumer build failed; no retry"}
        $exe=Join-Path $build "$config/JELibSamplerateConsumer.exe"
        $junit=Join-Path $script:RunRoot ($config+'-junit.xml')
        $phase=Native ('ctest-'+$config) $script:CTest ([string[]]@('--test-dir',$build,'-C',$config,'--output-on-failure','--output-junit',$junit))
        $last=Join-Path $build 'Testing/Temporary/LastTest.log'
        if(Test-Path -LiteralPath $last){[IO.File]::Copy($last,(Join-Path $script:RunRoot ($config+'-LastTest.raw.log')),$false)}
        if($phase.exitCode -ne 0){throw "$config CTest failed; no retry"}
        [xml]$xml=Get-Content -LiteralPath $junit -Raw
        $cases=@($xml.SelectNodes('//testcase'))
        $expected=@('api','conversions','fast','medium','best','reset','errors')|ForEach-Object {'JE.LocalPackages.LibSamplerate.'+$_}
        if($cases.Count -ne 7 -or @(Compare-Object ($cases.name|Sort-Object) ($expected|Sort-Object)).Count -or $xml.SelectNodes('//failure|//error|//skipped').Count){throw 'JUnit case coverage/outcome differs'}
        $checks+=[pscustomobject]@{configuration=$config;cases=$cases.Count;names=@($cases.name);junit=$junit;junitSHA256=(GetHash $junit);executable=$exe;executableSHA256=(GetHash $exe);staticLibraryPath=$library;staticLibrarySHA256=(GetHash $library);linkedVersionProof='Frozen api case requires exact src_get_version() libsamplerate-0.2.1 string';CRT='Release /MD library; consumer /MD or /MDd; raw diagnostics retained, general mixed-CRT compatibility not certified';status='NATIVE_CASES_PASSED'}
    }
    $status='CONSUMER_CASES_PASSED_ONLY'
}catch{$failure=$_.Exception.ToString()}
$result=EndRun $status $failure $checks
if($failure){exit 1}
exit 0