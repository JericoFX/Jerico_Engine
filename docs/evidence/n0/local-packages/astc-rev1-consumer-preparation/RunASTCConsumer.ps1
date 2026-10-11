# Luna dispatch only. Source/payload inputs remain read-only; no production or re-extraction.
#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$here=$PSScriptRoot
$assignment=Get-Content "$here/consumer-assignment.json" -Raw | ConvertFrom-Json
$freeze=Get-Content "$here/source-freeze-astc-consumer.json" -Raw | ConvertFrom-Json
$attempt=$assignment.freshAttempt
if(Test-Path -LiteralPath $attempt){throw 'Fresh attempt required; historical outputs preserved'}
if(Test-Path -LiteralPath $assignment.freshBuild){throw 'Fresh build required; existing outputs preserved'}
New-Item -ItemType Directory -Path $attempt | Out-Null
function Hash([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
function Verify([string]$phase){$deltas=@();foreach($file in $freeze){if(-not(Test-Path -LiteralPath $file.path -PathType Leaf)){$deltas+=$file.path;continue};$item=Get-Item -LiteralPath $file.path;if($item.Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){$deltas+=$file.path}};[ordered]@{phase=$phase;count=$freeze.Count;deltas=$deltas} | ConvertTo-Json -Depth 5 | Set-Content "$attempt/$phase.json" -Encoding utf8NoBOM;if($deltas.Count){throw "$phase source/payload changes"}}
$results=[Collections.Generic.List[object]]::new()
function Run([string]$label,[string]$exe,[string[]]$nativeArgs){[ordered]@{executable=$exe;argv=$nativeArgs} | ConvertTo-Json -Depth 4 | Set-Content "$attempt/$label-argv.json" -Encoding utf8NoBOM;$clock=[Diagnostics.Stopwatch]::StartNew();& $exe @nativeArgs 1>"$attempt/$label-stdout.log" 2>"$attempt/$label-stderr.log";$code=$LASTEXITCODE;$clock.Stop();$record=[ordered]@{label=$label;exitCode=$code;seconds=$clock.Elapsed.TotalSeconds};$record | ConvertTo-Json | Set-Content "$attempt/$label-result.json" -Encoding utf8NoBOM;$results.Add($record);return $code}
Verify 'preflight'
$final=0
try{
    if((Run 'configure' $assignment.cmake @($assignment.configure)) -ne 0){throw 'Consumer configure failed; no builds attempted'}
    foreach($config in @('Release','Debug')){
        if((Run "build-$config" $assignment.cmake @('--build',$assignment.freshBuild,'--config',$config,'--parallel','8')) -ne 0){throw "Consumer $config build failed"}
        $junit="$attempt/ctest-$config.xml"
        $code=Run "ctest-$config" $assignment.ctest @('--test-dir',$assignment.freshBuild,'-C',$config,'--output-on-failure','--output-junit',$junit)
        $lastTest=Join-Path $assignment.freshBuild 'Testing/Temporary/LastTest.log'
        if(Test-Path -LiteralPath $lastTest){Copy-Item -LiteralPath $lastTest -Destination "$attempt/LastTest-$config.log"}
        if($code -ne 0){$final=1}
        if(-not(Test-Path -LiteralPath $junit)){throw "$config JUnit missing"}
        [xml]$xml=Get-Content -LiteralPath $junit -Raw
        $cases=@($xml.testsuite.testcase)
        $names=@($cases | ForEach-Object {$_.name} | Sort-Object)
        $expected=@($assignment.expectedTests | Sort-Object)
        $bad=@($cases | Where-Object {$_.failure -or $_.error -or $_.skipped})
        if($cases.Count -ne 9 -or $bad.Count -ne 0 -or (Compare-Object $names $expected)){$final=1}
        [ordered]@{configuration=$config;exitCode=$code;testCount=$cases.Count;failedOrSkipped=$bad.Count;names=$names;expected=$expected} | ConvertTo-Json -Depth 5 | Set-Content "$attempt/ctest-$config-verification.json" -Encoding utf8NoBOM
    }
}catch{$final=1;$_ | Out-String | Set-Content "$attempt/driver-error.log" -Encoding utf8NoBOM}
finally{try{Verify 'postflight'}catch{$final=1;$_ | Out-String | Set-Content "$attempt/postflight-error.log" -Encoding utf8NoBOM};[ordered]@{exitCode=$final;producerRebuilt=$false;archiveReExtracted=$false;stages=@($results)} | ConvertTo-Json -Depth 6 | Set-Content "$attempt/summary.json" -Encoding utf8NoBOM}
exit $final
