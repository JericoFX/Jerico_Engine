$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$ctest='C:\Program Files\CMake\bin\ctest.exe'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$build='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-consumer-rev1'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
$argv=[string[]]@('--test-dir',$build,'-C','Debug','--output-on-failure')
if($argv.Count -ne 5 -or $argv[0] -ne '--test-dir' -or $argv[1] -ne $build -or $argv[2] -ne '-C' -or $argv[3] -ne 'Debug' -or $argv[4] -ne '--output-on-failure'){throw 'Debug CTest argv guard failed.'}
$argv|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $ev 'consumer-ctest-debug.argv.json') -Encoding utf8
$stdout=Join-Path $ev 'consumer-ctest-debug.stdout.log';$stderr=Join-Path $ev 'consumer-ctest-debug.stderr.log';$log=Join-Path $ev 'consumer-ctest-debug.log'
$watch=[Diagnostics.Stopwatch]::StartNew()
& $ctest @argv 1> $stdout 2> $stderr
$nativeExit=$LASTEXITCODE
$watch.Stop()
$combined=(Get-Content -LiteralPath $stdout -Raw)+"`r`n"+(Get-Content -LiteralPath $stderr -Raw)
$combined|Set-Content -LiteralPath $log -Encoding utf8
$result=[ordered]@{executable=$ctest;argv=$argv;argc=$argv.Count;cwd=$cwd;native_exit_code=$nativeExit;elapsed_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);stdout=$stdout;stderr=$stderr;log=$log;last_test_log=Join-Path $build 'Testing/Temporary/LastTest.log'}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'consumer-ctest-debug.result.json') -Encoding utf8
[pscustomobject]@{native_exit=$nativeExit;elapsed_seconds=$result.elapsed_seconds;log=$log}|Format-List
exit $nativeExit
