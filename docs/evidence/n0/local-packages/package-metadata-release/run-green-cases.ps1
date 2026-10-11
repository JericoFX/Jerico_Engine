$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\package-metadata-release'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$helper='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
$test=Join-Path $ev 'metadata-tests.cmake'
$out=Join-Path $ev 'green-cases'
$expectedHelper='ADB8601D690D3FF8E37F1E8E1897790CE94A1E1361B32270E4959886A8BD3382'
$expectedTest='013840C891E337AEB17DD60C136FA32C6BDBC17FF5B84C2FD0FBF6C61C051A4E'
if (Test-Path -LiteralPath $out) { throw 'fresh output path exists; not launching' }
$helperHash=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash
$testHash=(Get-FileHash -LiteralPath $test -Algorithm SHA256).Hash
if ($helperHash -ne $expectedHelper -or $testHash -ne $expectedTest) { throw 'frozen input hash mismatch; not launching' }
$argv=[string[]]@("-DJE_TEST_OUTPUT:PATH=$out",'-P',$test)
if ($argv.Count -ne 3 -or $argv[1] -ne '-P' -or $argv[2] -ne $test) { throw 'argv guard failed; not launching' }
$start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$cmake;$start.WorkingDirectory=(Get-Location).Path;$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($arg in $argv){[void]$start.ArgumentList.Add($arg)}
$before=[ordered]@{helper_sha256=$helperHash;test_sha256=$testHash;output_exists=$false;executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$start.WorkingDirectory;start_utc=[DateTime]::UtcNow.ToString('o')}
$before|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'green-cases-inputs-before.json') -Encoding utf8
$p=[Diagnostics.Process]::new();$p.StartInfo=$start;$timer=[Diagnostics.Stopwatch]::StartNew();if(-not $p.Start()){throw 'Process.Start returned false'}
$stdoutTask=$p.StandardOutput.ReadToEndAsync();$stderrTask=$p.StandardError.ReadToEndAsync();$p.WaitForExit();$stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult();$exit=$p.ExitCode;$timer.Stop()
[IO.File]::WriteAllText((Join-Path $ev 'green-cases.stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $ev 'green-cases.stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
$cases=@('release','commit','both','neither','invalid-release','invalid-commit','invalid-hash')
$caseRecords=@();foreach($caseName in $cases){$log=Join-Path $out "$caseName.log";$caseOutput=Join-Path $out $caseName;$caseRecords += [pscustomobject]@{case=$caseName;log_exists=(Test-Path -LiteralPath $log);log_sha256=if(Test-Path -LiteralPath $log){(Get-FileHash -LiteralPath $log -Algorithm SHA256).Hash}else{$null};output_exists=(Test-Path -LiteralPath $caseOutput);package_info_exists=(Test-Path -LiteralPath (Join-Path $caseOutput 'JEMetadataWitness.contents\PackageInfo.json'))}}
$after=[ordered]@{exit_code=$exit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');helper_sha256=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash;test_sha256=(Get-FileHash -LiteralPath $test -Algorithm SHA256).Hash;output_exists=(Test-Path -LiteralPath $out);stdout_bytes=([Text.Encoding]::UTF8.GetByteCount($stdout));stderr_bytes=([Text.Encoding]::UTF8.GetByteCount($stderr));case_count=$cases.Count;case_logs=@($caseRecords);output_packageinfo_count=@(Get-ChildItem -LiteralPath $out -Recurse -File -Filter 'PackageInfo.json' -ErrorAction SilentlyContinue).Count}
$after|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $ev 'green-cases-result.json') -Encoding utf8
$p.Dispose();exit $exit