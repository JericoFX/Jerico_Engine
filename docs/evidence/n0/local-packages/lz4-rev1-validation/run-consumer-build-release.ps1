$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$build='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-consumer-rev1'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
if(-not(Test-Path -LiteralPath (Join-Path $build 'JELocalLz4Consumer.sln'))){throw 'Consumer solution missing after configure.'}
$argv=[string[]]@('--build',$build,'--config','Release','--target','JELz4Consumer')
if($argv.Count -ne 6 -or $argv[0] -ne '--build' -or $argv[1] -ne $build -or $argv[2] -ne '--config' -or $argv[3] -ne 'Release' -or $argv[4] -ne '--target' -or $argv[5] -ne 'JELz4Consumer'){throw 'Release build argv guard failed.'}
$argv|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $ev 'consumer-build-release.argv.json') -Encoding utf8
$stdout=Join-Path $ev 'consumer-build-release.stdout.log';$stderr=Join-Path $ev 'consumer-build-release.stderr.log';$log=Join-Path $ev 'consumer-build-release.log'
$watch=[Diagnostics.Stopwatch]::StartNew()
& $cmake @argv 1> $stdout 2> $stderr
$nativeExit=$LASTEXITCODE
$watch.Stop()
$combined=(Get-Content -LiteralPath $stdout -Raw)+"`r`n"+(Get-Content -LiteralPath $stderr -Raw)
$combined|Set-Content -LiteralPath $log -Encoding utf8
$result=[ordered]@{executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$cwd;native_exit_code=$nativeExit;elapsed_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);stdout=$stdout;stderr=$stderr;log=$log;consumer_exe='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-consumer-rev1/Release/JELz4Consumer.exe'}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'consumer-build-release.result.json') -Encoding utf8
[pscustomobject]@{native_exit=$nativeExit;elapsed_seconds=$result.elapsed_seconds;log=$log}|Format-List
exit $nativeExit
