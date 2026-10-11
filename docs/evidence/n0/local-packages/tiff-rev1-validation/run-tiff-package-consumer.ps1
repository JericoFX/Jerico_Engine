$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine';$base=Join-Path $repo 'build/n0-validation/local-packages/tiff-rev1-validation'
$archive=Join-Path $repo 'build/n0-validation/local-packages/tiff-rev1/packages/tiff-4.2.0.15-je-rev1-windows.tar.xz'
$packageHash='d54ecd9a2463472e0df48e9baff07744ca1512fe761bc142ade353b347bf1dd2'
$extract=Join-Path $base 'extracted';$consumer=Join-Path $repo 'build/n0-validation/local-packages/tiff-consumer-rev1'
$cmake='C:/Program Files/CMake/bin/cmake.exe';$ctest='C:/Program Files/CMake/bin/ctest.exe';$tar='C:/Windows/system32/tar.exe'
function Invoke-Captured {
 param([string]$Label,[string]$Executable,[string[]]$ArgumentVector,[string]$WorkingDirectory)
 $argvPath=Join-Path $base "$Label-argv.json";$outPath=Join-Path $base "$Label.stdout.log";$errPath=Join-Path $base "$Label.stderr.log";$resPath=Join-Path $base "$Label-result.json"
 [IO.File]::WriteAllText($argvPath,(@{executable=$Executable;workingDirectory=$WorkingDirectory;arguments=$ArgumentVector}|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
 $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$Executable;$psi.WorkingDirectory=$WorkingDirectory;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
 foreach($arg in $ArgumentVector){[void]$psi.ArgumentList.Add($arg)}
 $proc=[Diagnostics.Process]::new();$proc.StartInfo=$psi;$sw=[Diagnostics.Stopwatch]::StartNew();if(-not $proc.Start()){throw "Failed to start $Label"}
 $ot=$proc.StandardOutput.ReadToEndAsync();$et=$proc.StandardError.ReadToEndAsync();$proc.WaitForExit();$exit=$proc.ExitCode;$sw.Stop();$out=$ot.GetAwaiter().GetResult();$err=$et.GetAwaiter().GetResult()
 [IO.File]::WriteAllText($outPath,$out,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText($errPath,$err,[Text.UTF8Encoding]::new($false))
 $result=[ordered]@{label=$Label;exitCode=$exit;elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3);stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($out);stderrBytes=[Text.Encoding]::UTF8.GetByteCount($err)}
 [IO.File]::WriteAllText($resPath,($result|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false));return [pscustomobject]$result
}
$freezePath=Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/source-freeze-tiff-candidate-v2.json';$freeze=Get-Content -Raw $freezePath|ConvertFrom-Json
$freezeBefore=@{};foreach($entry in $freeze.entries){$freezeBefore[[string]$entry.path]=(Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash.ToUpperInvariant()}
$actual=(Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant();if($actual -ne $packageHash){throw "Archive SHA mismatch $actual"}
if(-not (Select-String -Path "$archive.SHA256SUMS" -Pattern $packageHash)){throw 'Archive sidecar mismatch'}
if(Test-Path -LiteralPath $extract){throw 'Extraction path already exists'};[IO.Directory]::CreateDirectory($extract)|Out-Null
$ex=Invoke-Captured 'tiff-archive-extract' $cmake ([string[]]@('-E','tar','xJf',$archive)) $extract;if($ex.exitCode -ne 0){throw 'TIFF archive extraction failed'}
$list=Invoke-Captured 'tiff-archive-tar-list' $tar ([string[]]@('-tf',$archive)) $base;if($list.exitCode -ne 0){throw 'TIFF archive listing failed'}
$verbose=Invoke-Captured 'tiff-archive-tar-verbose' $tar ([string[]]@('-tvf',$archive)) $base;if($verbose.exitCode -ne 0){throw 'TIFF verbose archive listing failed'}
$members=@(Get-Content (Join-Path $base 'tiff-archive-tar-list.stdout.log')|Where-Object{$_ -ne ''});$links=@(Get-Content (Join-Path $base 'tiff-archive-tar-verbose.stdout.log')|Where-Object{$_ -match '^l|^h'})
$sumLines=@(Get-Content (Join-Path $extract 'SHA256SUMS')|Where-Object{$_ -match '^[0-9a-fA-F]{64} \*'});$verified=@()
foreach($line in $sumLines){if($line -notmatch '^([0-9a-fA-F]{64}) \*(.+)$'){throw "Bad payload manifest line $line"};$expected=$Matches[1].ToLowerInvariant();$relative=$Matches[2];$file=Join-Path $extract $relative;if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw "Manifest file missing: $relative"};$hash=(Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant();if($hash -ne $expected){throw "Payload SHA mismatch: $relative"};$verified += [ordered]@{path=$relative;bytes=(Get-Item $file).Length;sha256=$hash}}
if($sumLines.Count -eq 0 -or $verified.Count -ne $sumLines.Count){throw 'TIFF payload manifest incomplete'}
$reparse=@(Get-ChildItem -LiteralPath $extract -Recurse -Force|Where-Object{$_.Attributes -band [IO.FileAttributes]::ReparsePoint})
$info=Get-Content -Raw (Join-Path $extract 'PackageInfo.json')|ConvertFrom-Json
$recipe=Get-Content -Raw (Join-Path $extract 'tiff/JERecipe.json')|ConvertFrom-Json
$audit=[ordered]@{archiveSHA256=$actual;archiveBytes=(Get-Item $archive).Length;memberCount=$members.Count;members=$members;linkEntryCount=$links.Count;manifestEntryCount=$verified.Count;verifiedPayload=$verified;packageInfo=$info;recipe=$recipe;reparsePointCount=$reparse.Count}
[IO.File]::WriteAllText((Join-Path $base 'tiff-package-audit.json'),($audit|ConvertTo-Json -Depth 9),[Text.UTF8Encoding]::new($false))
if($links.Count -ne 0 -or $reparse.Count -ne 0){throw 'TIFF package has unexpected links/reparse points'}
$configureManifest=Get-Content -Raw (Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/validation-arguments.json')|ConvertFrom-Json
$configureArgs=[string[]]$configureManifest.consumerConfigure.argv
$configureArgs[10]="-DJE_TIFF_PACKAGE_ROOT:PATH=$extract"
if($configureArgs.Count -ne 12 -or $configureArgs[0] -ne '-S' -or $configureArgs[2] -ne '-B' -or $configureArgs[3] -ne $consumer){throw 'TIFF consumer argv guard failed'}
if(Test-Path -LiteralPath $consumer){throw 'TIFF consumer build path exists; refusing overwrite'}
$conf=Invoke-Captured 'tiff-consumer-configure' $cmake $configureArgs $base;if($conf.exitCode -ne 0){throw "TIFF consumer configure failed: $($conf.exitCode)"}
$build=Invoke-Captured 'tiff-consumer-build-release' $cmake ([string[]]@('--build',$consumer,'--config','Release','--target','JE_TiffConsumer')) $base;if($build.exitCode -ne 0){throw "TIFF consumer build failed: $($build.exitCode)"}
$test=Invoke-Captured 'tiff-consumer-ctest-release' $ctest ([string[]]@('--test-dir',$consumer,'-C','Release','--output-on-failure','--verbose')) $base
$lastTest=Join-Path $consumer 'Testing/Temporary/LastTest.log';if(Test-Path $lastTest){Copy-Item $lastTest (Join-Path $base 'LastTest-Release.log')};if($test.exitCode -ne 0){throw "TIFF consumer CTest failed: $($test.exitCode)"}
$freezeAfter=@();foreach($entry in $freeze.entries){$path=[string]$entry.path;$hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant();$freezeAfter += [ordered]@{path=$path;sha256=$hash;matchesFreeze=($hash -eq [string]$entry.sha256);matchesBefore=($hash -eq $freezeBefore[$path])}}
[IO.File]::WriteAllText((Join-Path $base 'tiff-inputs-after-consumer.json'),(@{inputCount=$freezeAfter.Count;allMatch=(@($freezeAfter|Where-Object{!$_.matchesFreeze}).Count -eq 0);inputs=$freezeAfter}|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
$summary=[ordered]@{archiveSHA256=$actual;memberCount=$members.Count;payloadCount=$verified.Count;links=$links.Count;packageInfo=$info;recipe=$recipe;configureExit=$conf.exitCode;buildExit=$build.exitCode;ctestExit=$test.exitCode;ctestSummary=(Select-String -Path (Join-Path $base 'tiff-consumer-ctest-release.stdout.log') -Pattern '100% tests passed'|ForEach-Object{$_.Line});freezeCount=$freezeAfter.Count;freezeMatches=(@($freezeAfter|Where-Object{!$_.matchesFreeze}).Count -eq 0)}
[IO.File]::WriteAllText((Join-Path $base 'tiff-consumer-summary.json'),($summary|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false));Write-Output ($summary|ConvertTo-Json -Compress -Depth 6)
