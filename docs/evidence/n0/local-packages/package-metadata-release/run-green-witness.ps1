$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\package-metadata-release'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$helper='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
$witness=Join-Path $ev 'release-witness.cmake'
$out=Join-Path $ev 'green-witness'
$expectedHelper='ADB8601D690D3FF8E37F1E8E1897790CE94A1E1361B32270E4959886A8BD3382'
$expectedWitness='C34909064FE9E852E5EC5A6B34A72B88433A6BEE57C7DE257AB1B23F9894F557'
if (Test-Path -LiteralPath $out) { throw 'fresh output path exists; not launching' }
$helperHash=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash
$witnessHash=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash
if ($helperHash -ne $expectedHelper -or $witnessHash -ne $expectedWitness) { throw 'frozen input hash mismatch; not launching' }
$argv=[string[]]@("-DJE_TEST_OUTPUT:PATH=$out",'-P',$witness)
if ($argv.Count -ne 3 -or $argv[1] -ne '-P' -or $argv[2] -ne $witness) { throw 'argv guard failed; not launching' }
$start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$cmake;$start.WorkingDirectory=(Get-Location).Path;$start.UseShellExecute=$false;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($arg in $argv){[void]$start.ArgumentList.Add($arg)}
$before=[ordered]@{helper_sha256=$helperHash;witness_sha256=$witnessHash;output_exists=$false;executable=$cmake;argv=$argv;argc=$argv.Count;cwd=$start.WorkingDirectory;start_utc=[DateTime]::UtcNow.ToString('o')}
$before|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'green-witness-inputs-before.json') -Encoding utf8
$p=[Diagnostics.Process]::new();$p.StartInfo=$start;$timer=[Diagnostics.Stopwatch]::StartNew();if(-not $p.Start()){throw 'Process.Start returned false'}
$stdoutTask=$p.StandardOutput.ReadToEndAsync();$stderrTask=$p.StandardError.ReadToEndAsync();$p.WaitForExit();$stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult();$exit=$p.ExitCode;$timer.Stop()
[IO.File]::WriteAllText((Join-Path $ev 'green-witness.stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $ev 'green-witness.stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
$info=Join-Path $out 'packages\JEReleaseMetadataWitness.contents\PackageInfo.json'
$after=[ordered]@{exit_code=$exit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');helper_sha256=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash;witness_sha256=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash;output_exists=(Test-Path -LiteralPath $out);package_info_exists=(Test-Path -LiteralPath $info);release_message_found=$stdout.Contains('Release identity recorded without an invented Git commit');stdout_bytes=([Text.Encoding]::UTF8.GetByteCount($stdout));stderr_bytes=([Text.Encoding]::UTF8.GetByteCount($stderr))}
if(Test-Path -LiteralPath $info){$json=Get-Content -LiteralPath $info -Raw|ConvertFrom-Json;$after.package_info_sha256=(Get-FileHash -LiteralPath $info -Algorithm SHA256).Hash;$after.package_identity=$json.name;$after.schema_version=$json.schemaVersion;$after.source=$json.source;$after.payload_sha256=(Get-FileHash -LiteralPath (Join-Path $out 'packages\JEReleaseMetadataWitness.contents\SHA256SUMS') -Algorithm SHA256).Hash}
$after|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $ev 'green-witness-result.json') -Encoding utf8
$p.Dispose();exit $exit