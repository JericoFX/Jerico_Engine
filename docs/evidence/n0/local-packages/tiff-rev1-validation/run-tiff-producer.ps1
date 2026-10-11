$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages/tiff-rev1-validation'
$freezePath=Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/source-freeze-tiff-candidate-v2.json'
$freezeExpected='1F103DDA160910E2FC36CA45F908DDD19BAEBF12BE35EF0928B1F6C12587ADDC'
$argsPath=Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/validation-arguments.json'
$argsExpected='43C1E07842B66AA37FD05958F9418D6AF95385A991B3BDF419F7FB2A62C4942B'
if((Get-FileHash $freezePath -Algorithm SHA256).Hash -ne $freezeExpected){throw 'TIFF freeze manifest hash mismatch'}
if((Get-FileHash $argsPath -Algorithm SHA256).Hash -ne $argsExpected){throw 'TIFF validation arguments manifest hash mismatch'}
$freeze=Get-Content -Raw $freezePath|ConvertFrom-Json
$before=@();foreach($entry in $freeze.entries){$path=[string]$entry.path;$hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant();if($hash -ne [string]$entry.sha256){throw "Frozen TIFF input mismatch: $path"};$before += [ordered]@{path=$path;bytes=(Get-Item $path).Length;sha256=$hash}}
if($before.Count -ne 663){throw "Expected 663 frozen inputs, found $($before.Count)"}
$manifest=Get-Content -Raw $argsPath|ConvertFrom-Json
$output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/tiff-rev1'
if(Test-Path -LiteralPath $output){throw 'TIFF producer output exists; refusing overwrite'}
$arguments=[string[]]$manifest.producer.argv
if($arguments.Count -ne 5 -or $arguments[3] -ne '-P' -or $arguments[4] -ne 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/TIFF/BuildTiffPackage.cmake'){throw 'Producer argv mode/count guard failed'}
$cmake=[string]$manifest.producer.executable
[IO.File]::WriteAllText((Join-Path $base 'tiff-producer-argv.json'),(@{executable=$cmake;workingDirectory=$base;arguments=$arguments}|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $base 'tiff-inputs-before.json'),(@{freezeManifestSHA256=$freezeExpected;validationArgsSHA256=$argsExpected;inputCount=$before.Count;inputs=$before}|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
$psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$base;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
foreach($arg in $arguments){[void]$psi.ArgumentList.Add($arg)}
$proc=[Diagnostics.Process]::new();$proc.StartInfo=$psi;$sw=[Diagnostics.Stopwatch]::StartNew();if(-not $proc.Start()){throw 'CMake producer failed to start'}
$ot=$proc.StandardOutput.ReadToEndAsync();$et=$proc.StandardError.ReadToEndAsync();$proc.WaitForExit();$exit=$proc.ExitCode;$peakWS=$proc.PeakWorkingSet64;$cpuSeconds=$proc.TotalProcessorTime.TotalSeconds;$sw.Stop();$stdout=$ot.GetAwaiter().GetResult();$stderr=$et.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $base 'tiff-producer.stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $base 'tiff-producer.stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
$after=@();foreach($entry in $freeze.entries){$path=[string]$entry.path;$hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant();$after += [ordered]@{path=$path;sha256=$hash;matchesFreeze=($hash -eq [string]$entry.sha256)}}
[IO.File]::WriteAllText((Join-Path $base 'tiff-inputs-after-producer.json'),(@{inputCount=$after.Count;allMatch=(@($after|Where-Object{!$_.matchesFreeze}).Count -eq 0);inputs=$after}|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
$result=[ordered]@{exitCode=$exit;elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3);cpuSeconds=[Math]::Round($cpuSeconds,3);peakWorkingSetBytes=$peakWS;memoryScope='cmake.exe parent process only, not child MSBuild/cl/link';stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($stdout);stderrBytes=[Text.Encoding]::UTF8.GetByteCount($stderr);outputExists=(Test-Path -LiteralPath $output);frozenInputCount=$after.Count;frozenInputsMatch=(@($after|Where-Object{!$_.matchesFreeze}).Count -eq 0)}
[IO.File]::WriteAllText((Join-Path $base 'tiff-producer-result.json'),($result|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false));Write-Output ($result|ConvertTo-Json -Compress)
