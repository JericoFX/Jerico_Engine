# Execute only after explicit parent dispatch to Luna.
. (Join-Path $PSScriptRoot 'CommonExecution.ps1')
$package='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/ispc-texcomp-rev1-package-audit/extracted/ISPCTexComp-36b80aa-je-rev1-windows'
$build='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/ispc-texcomp-rev1-package-consumer'
if(Test-Path -LiteralPath $build){throw 'Consumer directory must be fresh'}
BeginRun 'consumer-run'
$failure=$null;$checks=@();$status='FAIL_OR_INCOMPLETE'
try{
    $configureArguments=[string[]]@('-S','F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/ISPCTexComp/tests','-B',$build,'-G','Visual Studio 17 2022','-A','x64',"-DJE_ISPC_PACKAGE_ROOT=$package")
    $phase=Native 'configure' $script:CMake $configureArguments
    if($phase.exitCode -ne 0){throw 'Consumer configure failed; no retry'}
    foreach($config in @('Release','Debug')){
        $phase=Native ('build-'+$config) $script:CMake ([string[]]@('--build',$build,'--config',$config))
        if($phase.exitCode -ne 0){throw "$config consumer build failed; no retry"}
        $exe=Join-Path $build "$config/JEISPCTexCompConsumer.exe"
        $dll=Join-Path $build "$config/ispc_texcomp.dll"
        if((GetHash $dll) -cne '3F6E8AD2AED34192836E39A2458FABC6ED48F937B3679D13A03315C3344F2C28'){throw 'Deployed DLL differs from audited SDK'}
        $junit=Join-Path $script:RunRoot ($config+'-junit.xml')
        $phase=Native ('ctest-'+$config) $script:CTest ([string[]]@('--test-dir',$build,'-C',$config,'--output-on-failure','--output-junit',$junit))
        $last=Join-Path $build 'Testing/Temporary/LastTest.log'
        if(Test-Path -LiteralPath $last){[IO.File]::Copy($last,(Join-Path $script:RunRoot ($config+'-LastTest.raw.log')),$false)}
        if($phase.exitCode -ne 0){throw "$config CTest failed; no retry"}
        [xml]$xml=Get-Content -LiteralPath $junit -Raw
        $cases=@($xml.SelectNodes('//testcase'))
        $expected=@('exports','bc1','bc3','bc6h','bc7','etc1','astc','borders')|ForEach-Object {'JE.LocalPackages.ISPCTexComp.'+$_}
        if($cases.Count -ne 8 -or @(Compare-Object ($cases.name|Sort-Object) ($expected|Sort-Object)).Count -or $xml.SelectNodes('//failure|//error|//skipped').Count){throw 'JUnit case coverage/outcome differs'}
        $checks+=[pscustomobject]@{configuration=$config;cases=$cases.Count;names=@($cases.name);junit=$junit;junitSHA256=(GetHash $junit);executable=$exe;executableSHA256=(GetHash $exe);expectedCopiedDLLPath=$dll;actualLoadedDLLPathText=$null;DLLCopySHA256=(GetHash $dll);loadedDLLProof='Frozen native Exports() calls GetModuleFileNameW and requires DLL parent==EXE parent in every case. Runtime path string not emitted by frozen source.';status='NATIVE_CASES_PASSED'}
    }
    $status='CONSUMER_CASES_PASSED_ONLY'
}catch{$failure=$_.Exception.ToString()}
$result=EndRun $status $failure $checks
if($failure){exit 1}
exit 0