$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-rev3-vctools-validation'
$ctest='C:\Program Files\CMake\bin\ctest.exe'
$argv=[string[]]@('--test-dir',"$ev\consumer-md",'-C','Release','--output-on-failure')
if ($argv.Count -ne 5 -or $argv[0] -ne '--test-dir' -or $argv[2] -ne '-C' -or $argv[3] -ne 'Release') { throw 'argv guard failed; not launching ctest' }
$record=[ordered]@{executable=$ctest;argv=$argv;argc=$argv.Count;cwd=(Get-Location).Path;start_utc=[DateTime]::UtcNow.ToString('o')}
$record | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$ev\consumer-ctest-argv.json" -Encoding utf8
$timer=[Diagnostics.Stopwatch]::StartNew()
& $ctest @argv 1> "$ev\consumer-ctest.stdout.log" 2> "$ev\consumer-ctest.stderr.log"
$nativeExit=$LASTEXITCODE
$timer.Stop()
[ordered]@{exit_code=$nativeExit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath "$ev\consumer-ctest-result.json" -Encoding utf8
exit $nativeExit