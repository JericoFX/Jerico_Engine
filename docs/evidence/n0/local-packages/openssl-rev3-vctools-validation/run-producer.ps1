$ErrorActionPreference = 'Continue'
$ev = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-rev3-vctools-validation'
$base = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$freeze = Join-Path $base 'openssl-perl-tool-audit\source-freeze-openssl-vctools-version.json'
$driver = Join-Path $base 'openssl-perl-tool-audit\producer-candidate-inputs-rev3.cmake'
$out = Join-Path $base 'openssl-build-rev3'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$argv = [string[]]@('-P', $driver)
if ($argv.Count -ne 2 -or $argv[0] -ne '-P' -or $argv[1] -ne $driver) { throw 'argv guard failed' }
if (Test-Path -LiteralPath $out) { throw 'output unexpectedly exists' }
$freezeHash = (Get-FileHash -LiteralPath $freeze -Algorithm SHA256).Hash
$driverHash = (Get-FileHash -LiteralPath $driver -Algorithm SHA256).Hash
$producer = 'F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\OpenSSL\BuildOpenSSLPackage.cmake'
$producerHash = (Get-FileHash -LiteralPath $producer -Algorithm SHA256).Hash
if ($freezeHash -ine '8DEF2ABEAE3AA8803CEB425F3D1BBE0F1643F1052EFC6D835E93ED4109C2C35C' -or $driverHash -ine '7C45E7993362430FD081924C934EE889AB15315683EDA61C68AB642585CE3CDE' -or $producerHash -ine 'E8A5316A6B0048F931965355D4276C89DFECF43A9FAAEECE091E1279279CA3A8') { throw 'frozen input hash mismatch' }
$meta = [ordered]@{ executable=$cmake; argv=$argv; argc=$argv.Count; cwd=(Get-Location).Path; freeze_sha256=$freezeHash; driver_sha256=$driverHash; producer_sha256=$producerHash; output=$out; output_existed_before=$false; start_utc=(Get-Date).ToUniversalTime().ToString('o') }
$meta | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ev 'producer-argv.json') -Encoding utf8
$timer = [Diagnostics.Stopwatch]::StartNew()
& $cmake @argv 1> (Join-Path $ev 'producer.stdout.log') 2> (Join-Path $ev 'producer.stderr.log')
$nativeExitCode = $LASTEXITCODE
$timer.Stop()
$result = [ordered]@{ native_exit_code=$nativeExitCode; elapsed_seconds=$timer.Elapsed.TotalSeconds; end_utc=(Get-Date).ToUniversalTime().ToString('o'); output_exists_after=(Test-Path -LiteralPath $out) }
$result | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $ev 'producer-result.json') -Encoding utf8
"CMAKE_EXIT=$nativeExitCode ELAPSED_SECONDS=$($timer.Elapsed.TotalSeconds) OUTPUT_EXISTS=$($result.output_exists_after)"
exit $nativeExitCode