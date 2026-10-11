$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\package-metadata-release'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$helper='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
$witness=Join-Path $ev 'release-witness.cmake'
$out=Join-Path $ev 'red-native-capture'
$expectedHelper='B067C707919808DDA7697CE79EFEC94A31029FF03B6CC383E374209CC8B68E38'
$expectedWitness='C34909064FE9E852E5EC5A6B34A72B88433A6BEE57C7DE257AB1B23F9894F557'
if (Test-Path -LiteralPath $out) { throw 'fresh output path already exists; not launching' }
$helperHash=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash
$witnessHash=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash
if ($helperHash -ne $expectedHelper -or $witnessHash -ne $expectedWitness) { throw 'frozen input hash mismatch; not launching' }
$argv=[string[]]@("-DJE_TEST_OUTPUT:PATH=$out",'-P',$witness)
if ($argv.Count -ne 3 -or $argv[1] -ne '-P' -or $argv[2] -ne $witness) { throw 'argv guard failed; not launching' }
$start=[Diagnostics.ProcessStartInfo]::new()
$start.FileName=$cmake
$start.WorkingDirectory=(Get-Location).Path
$start.UseShellExecute=$false
$start.RedirectStandardOutput=$true
$start.RedirectStandardError=$true
foreach($arg in $argv){[void]$start.ArgumentList.Add($arg)}
$before=[ordered]@{helper_sha256=$helperHash;witness_sha256=$witnessHash;output_exists=(Test-Path -LiteralPath $out);executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$start.WorkingDirectory;start_utc=[DateTime]::UtcNow.ToString('o')}
$before | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ev 'red-native-capture-inputs-before.json') -Encoding utf8
$p=[Diagnostics.Process]::new()
$p.StartInfo=$start
$timer=[Diagnostics.Stopwatch]::StartNew()
if (-not $p.Start()) { throw 'Process.Start returned false' }
$stdoutTask=$p.StandardOutput.ReadToEndAsync()
$stderrTask=$p.StandardError.ReadToEndAsync()
$p.WaitForExit()
$stdout=$stdoutTask.GetAwaiter().GetResult()
$stderr=$stderrTask.GetAwaiter().GetResult()
$nativeExit=$p.ExitCode
$timer.Stop()
[IO.File]::WriteAllText((Join-Path $ev 'red-native-capture.stdout.log'),$stdout,[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $ev 'red-native-capture.stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
$stage=Join-Path $out 'stage\payload.txt'
$after=[ordered]@{exit_code=$nativeExit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');helper_sha256=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash;witness_sha256=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash;output_exists=(Test-Path -LiteralPath $out);stage_payload_exists=(Test-Path -LiteralPath $stage);stage_payload_bytes=if(Test-Path -LiteralPath $stage){(Get-Item -LiteralPath $stage).Length}else{$null};stage_payload_sha256=if(Test-Path -LiteralPath $stage){(Get-FileHash -LiteralPath $stage -Algorithm SHA256).Hash}else{$null};packages_dir_exists=(Test-Path -LiteralPath (Join-Path $out 'packages'));archive_exists=(Test-Path -LiteralPath (Join-Path $out 'packages\JEReleaseMetadataWitness.tar.xz'));package_info_exists=(Test-Path -LiteralPath (Join-Path $out 'packages\JEReleaseMetadataWitness.contents\PackageInfo.json'));stdout_bytes=([Text.Encoding]::UTF8.GetByteCount($stdout));stderr_bytes=([Text.Encoding]::UTF8.GetByteCount($stderr));expected_error_found=($stderr.Contains('JE_PACKAGE_SOURCE_COMMIT is required'))}
$after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ev 'red-native-capture-result.json') -Encoding utf8
$p.Dispose()
exit $nativeExit