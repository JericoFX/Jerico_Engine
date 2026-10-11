$ErrorActionPreference='Stop'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$source='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\LZ4\tests'
$build='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-consumer-rev1'
$package='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-extracted-rev1'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
if(Test-Path -LiteralPath $build){throw 'Consumer build directory already exists; refusing to overwrite.'}
if(-not(Test-Path -LiteralPath (Join-Path $package 'Findlz4.cmake'))){throw 'Extracted package is missing Findlz4.cmake.'}
$argv=[string[]]@('-S',$source,'-B',$build,'-G','Visual Studio 17 2022','-A','x64','-T','v143',"-DJE_LZ4_PACKAGE_ROOT:PATH=$package")
if($argv.Count -ne 11 -or $argv[0] -ne '-S' -or $argv[1] -ne $source -or $argv[2] -ne '-B' -or $argv[3] -ne $build -or $argv[4] -ne '-G' -or $argv[5] -ne 'Visual Studio 17 2022' -or $argv[6] -ne '-A' -or $argv[7] -ne 'x64' -or $argv[8] -ne '-T' -or $argv[9] -ne 'v143' -or $argv[10] -ne "-DJE_LZ4_PACKAGE_ROOT:PATH=$package"){throw 'Consumer configure argv guard failed.'}
$argv|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $ev 'consumer-configure.argv.json') -Encoding utf8
$stdout=Join-Path $ev 'consumer-configure.stdout.log';$stderr=Join-Path $ev 'consumer-configure.stderr.log';$log=Join-Path $ev 'consumer-configure.log'
$watch=[Diagnostics.Stopwatch]::StartNew()
& $cmake @argv 1> $stdout 2> $stderr
$nativeExit=$LASTEXITCODE
$watch.Stop()
$combined=(Get-Content -LiteralPath $stdout -Raw)+"`r`n"+(Get-Content -LiteralPath $stderr -Raw)
$combined|Set-Content -LiteralPath $log -Encoding utf8
$result=[ordered]@{executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$cwd;native_exit_code=$nativeExit;elapsed_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);package_root=$package;stdout=$stdout;stderr=$stderr;log=$log;build_directory_created=(Test-Path -LiteralPath $build)}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'consumer-configure.result.json') -Encoding utf8
[pscustomobject]@{native_exit=$nativeExit;elapsed_seconds=$result.elapsed_seconds;build_created=$result.build_directory_created;log=$log}|Format-List
exit $nativeExit
