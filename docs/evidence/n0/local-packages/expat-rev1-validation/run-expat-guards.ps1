$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages/expat-rev1-validation'
$guardRoot=Join-Path $base 'guards'
$cmake='C:/Program Files/CMake/bin/cmake.exe'
$script="$repo/cmake/Tools/LocalPackages/Expat/BuildExpatPackage.cmake"
$source='F:/JericoEngine/Dependencies/archives/expat-e26a263aa9182d1e4552cd615857f0f61df3ba42.tar.gz'
$sourceHash='a6130cab18d9df124dbfe4045f57f8c2180811c35d37bc357a42cff61afa4057'
$sourceUrl='https://codeload.github.com/libexpat/libexpat/tar.gz/e26a263aa9182d1e4552cd615857f0f61df3ba42'
$sourceHashBefore=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
if($sourceHashBefore -ne $sourceHash){throw 'Pinned source archive hash mismatch before guards'}
function Invoke-Guard {
  param([string]$Name,[string]$Archive,[string]$Hash,[string]$Url,[string]$Output,[string]$Expected)
  $dir=Join-Path $guardRoot $Name
  [IO.Directory]::CreateDirectory($dir)|Out-Null
  $vector=[string[]]@("-DJE_EXPAT_SOURCE_ARCHIVE:STRING=$Archive","-DJE_EXPAT_SOURCE_SHA256:STRING=$Hash","-DJE_EXPAT_SOURCE_URL:STRING=$Url","-DJE_EXPAT_OUTPUT:PATH=$Output",'-P',$script)
  if($vector.Count -ne 6 -or $vector[4] -ne '-P' -or $vector[5] -ne $script){throw "Guard argv failed mode/count: $Name"}
  $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$dir;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
  foreach($arg in $vector){[void]$psi.ArgumentList.Add($arg)}
  [IO.File]::WriteAllText((Join-Path $dir 'argv.json'),(@{executable=$cmake;workingDirectory=$dir;arguments=$vector}|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
  $proc=[Diagnostics.Process]::new();$proc.StartInfo=$psi;$sw=[Diagnostics.Stopwatch]::StartNew();if(-not $proc.Start()){throw "Could not start $Name"}
  $ot=$proc.StandardOutput.ReadToEndAsync();$et=$proc.StandardError.ReadToEndAsync();$proc.WaitForExit();$nativeExit=$proc.ExitCode;$sw.Stop();$out=$ot.GetAwaiter().GetResult();$err=$et.GetAwaiter().GetResult()
  [IO.File]::WriteAllText((Join-Path $dir 'stdout.log'),$out,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $dir 'stderr.log'),$err,[Text.UTF8Encoding]::new($false))
  $combined=$out+"`n"+$err;$diagMatch=$combined.Contains($Expected);$outputExists=Test-Path -LiteralPath $Output
  $result=[ordered]@{case=$Name;exitCode=$nativeExit;elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3);expectedDiagnostic=$Expected;diagnosticMatched=$diagMatch;outputExists=$outputExists}
  [IO.File]::WriteAllText((Join-Path $dir 'result.json'),($result|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
  if($nativeExit -eq 0 -or -not $diagMatch){throw "Guard did not reject as expected: $Name (exit=$nativeExit)"}
  if($Name -ne 'existing-sentinel' -and $outputExists){throw "Guard created output unexpectedly: $Name"}
  return [pscustomobject]$result
}
$cases=@()
$uriOut=Join-Path $guardRoot 'uri/output'
$cases+=Invoke-Guard 'uri' 'https://invalid.example/expat.tar.gz' $sourceHash $sourceUrl $uriOut 'must not be a URI'
$uncOut=Join-Path $guardRoot 'unc/output'
$cases+=Invoke-Guard 'unc' '\\invalid-host\share\expat.tar.gz' $sourceHash $sourceUrl $uncOut 'must be an absolute local path'
$missing=Join-Path $guardRoot 'missing-archive/not-present.tar.gz'
$missingOut=Join-Path $guardRoot 'missing-archive/output'
$cases+=Invoke-Guard 'missing-archive' $missing $sourceHash $sourceUrl $missingOut 'file is missing'
$badHashOut=Join-Path $guardRoot 'bad-hash/output'
$cases+=Invoke-Guard 'bad-hash' $source ('0000000000000000000000000000000000000000000000000000000000000000') $sourceUrl $badHashOut 'source URL/SHA256 must match'
$badUrlOut=Join-Path $guardRoot 'bad-url/output'
$cases+=Invoke-Guard 'bad-url' $source $sourceHash 'https://invalid.example/expat.tar.gz' $badUrlOut 'source URL/SHA256 must match'
$tampered=Join-Path $guardRoot 'tampered-archive/source.tar.gz'
$tamperedDir=Split-Path -Parent $tampered;[IO.Directory]::CreateDirectory($tamperedDir)|Out-Null
$bytes=[IO.File]::ReadAllBytes($source);$bytes[0]=$bytes[0] -bxor 1;[IO.File]::WriteAllBytes($tampered,$bytes)
$tamperedOut=Join-Path $guardRoot 'tampered-archive/output'
$cases+=Invoke-Guard 'tampered-archive' $tampered $sourceHash $sourceUrl $tamperedOut 'SHA256 mismatch'
$sentinelOut=Join-Path $guardRoot 'existing-sentinel/output';[IO.Directory]::CreateDirectory($sentinelOut)|Out-Null
$sentinel=Join-Path $sentinelOut 'keep.txt';[IO.File]::WriteAllText($sentinel,'preserve-expat-sentinel',[Text.UTF8Encoding]::new($false));$sentinelHash=(Get-FileHash $sentinel -Algorithm SHA256).Hash
$cases+=Invoke-Guard 'existing-sentinel' $source $sourceHash $sourceUrl $sentinelOut 'output already exists'
if((Get-FileHash $sentinel -Algorithm SHA256).Hash -ne $sentinelHash){throw 'Existing output sentinel changed'}
$outside=Join-Path $repo 'cmake'
$cases+=Invoke-Guard 'outside-build' $source $sourceHash $sourceUrl $outside 'output must be beneath this checkout/build'
$sourceHashAfter=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
if($sourceHashAfter -ne $sourceHashBefore){throw 'Pinned source archive changed during guards'}
$summary=[ordered]@{caseCount=$cases.Count;cases=$cases;sourceArchiveSHA256Before=$sourceHashBefore;sourceArchiveSHA256After=$sourceHashAfter;sourceUnchanged=($sourceHashAfter -eq $sourceHash);symlinkGuard='not attempted; no unprivileged safe symlink fixture assigned'}
[IO.File]::WriteAllText((Join-Path $guardRoot 'guard-summary.json'),($summary|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
Write-Output ($summary|ConvertTo-Json -Compress)
