$ErrorActionPreference='Stop'
$repo='F:\JericoEngine\JericoEngine'
$base=Join-Path $repo 'build\n0-validation\local-packages'
$evidence=Join-Path $base 'openexr-rev2-validation'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$argsPath=Join-Path $base 'openexr-audit\validation-arguments-v2.json'
$freezePath=Join-Path $base 'openexr-audit\source-freeze-openexr-candidate-v2.json'
$argsDoc=Get-Content -Raw -LiteralPath $argsPath | ConvertFrom-Json
$freeze=Get-Content -Raw -LiteralPath $freezePath | ConvertFrom-Json
$vector=[string[]]$argsDoc.producer.argv
$output=[string]$argsDoc.producer.outputMustBeAbsent
$assigned=[IO.Path]::GetFullPath($output).TrimEnd([IO.Path]::DirectorySeparatorChar)
if($vector.Count -ne 6 -or $vector[4] -ne '-P' -or $vector[5] -ne 'F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/OpenEXR/BuildOpenEXRPackage.cmake'){throw 'Producer argv guard failed'}
$encodedOutput=($vector[3] -replace '^-DJE_OPENEXR_OUTPUT:PATH=','')
if(-not [string]::Equals([IO.Path]::GetFullPath($encodedOutput).TrimEnd([IO.Path]::DirectorySeparatorChar),$assigned,[StringComparison]::OrdinalIgnoreCase)){throw 'Output argv does not match assignment'}
if(Test-Path -LiteralPath $output){throw 'Producer output unexpectedly exists'}
if(!(Test-Path -LiteralPath $cmake)){throw 'CMake executable missing'}
$freezeActual=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash.ToUpperInvariant()
$argsActual=(Get-FileHash -LiteralPath $argsPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($freezeActual -ne '7745DC711AC2F20AE812108434D5F338B7FCD88E0A3793496B8DB8BC664B8163'){throw 'Source freeze hash mismatch'}
if($argsActual -ne '0D219066133D2A20EE8756ACEC2F1D06DF0EB1E2C34084F950D07D0943345880'){throw 'Arguments hash mismatch'}
if($freeze.inputCount -ne 1182 -or $freeze.entries.Count -ne 1182){throw 'Freeze entry count mismatch'}
$pre=@(foreach($item in $freeze.entries){$actual=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash.ToLowerInvariant();$size=(Get-Item -LiteralPath $item.path).Length;[pscustomobject]@{path=[string]$item.path;expectedBytes=[int64]$item.bytes;actualBytes=[int64]$size;expectedSha256=([string]$item.sha256).ToLowerInvariant();actualSha256=$actual;match=($size -eq [int64]$item.bytes -and $actual -eq ([string]$item.sha256).ToLowerInvariant())}})
if($pre.Count -ne 1182 -or @($pre|Where-Object{-not $_.match}).Count){throw 'Source freeze preflight mismatch'}
$zlib=@($pre|Where-Object path -Like '*zlib-1.2.11-je-rev1-windows.contents*')
if($zlib.Count -ne 4 -or @($zlib|Where-Object{-not $_.match}).Count){throw 'zlib package preflight mismatch'}
$archiveEntries=@($pre|Where-Object {$_.path -like '*openexr-697cc449*.tar.gz' -or $_.path -like '*Imath-6dc082*.tar.gz'})
$summary=[ordered]@{freezePath=$freezePath;freezeSha256=$freezeActual;argumentsPath=$argsPath;argumentsSha256=$argsActual;inputCount=$pre.Count;matched=@($pre|Where-Object match).Count;zlibBlobCount=$zlib.Count;zlibMatched=@($zlib|Where-Object match).Count;producerArgumentCount=$vector.Count;producerArguments=$vector;workingDirectory=$evidence;cmake=$cmake;cmakeSha256=(Get-FileHash -LiteralPath $cmake -Algorithm SHA256).Hash.ToUpperInvariant();outputPath=$output;outputAbsent=$true;archiveEntries=$archiveEntries}
$pre|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $evidence 'source-freeze-before.json') -Encoding utf8
$zlib|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $evidence 'zlib-inputs-before.json') -Encoding utf8
$vector|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $evidence 'producer-argv.json') -Encoding utf8
$summary|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $evidence 'producer-preflight.json') -Encoding utf8
$psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$evidence;$psi.UseShellExecute=$false;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true;foreach($arg in $vector){[void]$psi.ArgumentList.Add($arg)}
$proc=[Diagnostics.Process]::new();$proc.StartInfo=$psi;$timer=[Diagnostics.Stopwatch]::StartNew();if(!$proc.Start()){throw 'CMake failed to start'};$stdoutTask=$proc.StandardOutput.ReadToEndAsync();$stderrTask=$proc.StandardError.ReadToEndAsync();$proc.WaitForExit();$nativeExit=$proc.ExitCode;$timer.Stop();$stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult()
[IO.File]::WriteAllText((Join-Path $evidence 'producer.stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $evidence 'producer.stderr.log'),$stderr,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $evidence 'producer.exitcode'),[string]$nativeExit);[IO.File]::WriteAllText((Join-Path $evidence 'producer.elapsed.txt'),('{0:N3}s' -f $timer.Elapsed.TotalSeconds))
$post=@(foreach($item in $freeze.entries){$actual=(Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash.ToLowerInvariant();$size=(Get-Item -LiteralPath $item.path).Length;[pscustomobject]@{path=[string]$item.path;expectedBytes=[int64]$item.bytes;actualBytes=[int64]$size;expectedSha256=([string]$item.sha256).ToLowerInvariant();actualSha256=$actual;match=($size -eq [int64]$item.bytes -and $actual -eq ([string]$item.sha256).ToLowerInvariant())}})
$postZlib=@($post|Where-Object path -Like '*zlib-1.2.11-je-rev1-windows.contents*')
$post|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $evidence 'source-freeze-after-producer.json') -Encoding utf8
$postZlib|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $evidence 'zlib-inputs-after.json') -Encoding utf8
[pscustomobject]@{nativeExit=$nativeExit;elapsedSeconds=[math]::Round($timer.Elapsed.TotalSeconds,3);freezeEntries=$post.Count;freezeMatched=@($post|Where-Object match).Count;zlibEntries=$postZlib.Count;zlibMatched=@($postZlib|Where-Object match).Count;outputExists=(Test-Path -LiteralPath $output);stdoutPath=(Join-Path $evidence 'producer.stdout.log');stderrPath=(Join-Path $evidence 'producer.stderr.log')}|ConvertTo-Json -Depth 4