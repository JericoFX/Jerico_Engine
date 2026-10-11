$ErrorActionPreference = 'Stop'
$repo='F:/JericoEngine/JericoEngine'
$base=Join-Path $repo 'build/n0-validation/local-packages/mcpp-rev2-validation'
$caseRoot=Join-Path $base 'negative-cases'
$freezePath=Join-Path $repo 'build/n0-validation/local-packages/mcpp-audit/source-freeze-parent17-rev2.json'
$freeze=Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$cmake='C:/Program Files/CMake/bin/cmake.exe'
$script='F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/mcpp/BuildMcppPackage.cmake'
$archive='F:/JericoEngine/Dependencies/archives/mcpp-2.7.2.tar.gz'
$archiveSha='3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864'
$archiveUrl='https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download'
$outside=Join-Path $repo 'local-package-mcpp-outside-build'
if(Test-Path -LiteralPath $caseRoot){throw "guard case root already exists: $caseRoot"}
if(Test-Path -LiteralPath $outside){throw "outside-build sentinel path already exists: $outside"}
[IO.Directory]::CreateDirectory($caseRoot)|Out-Null
function Assert-Freeze {
  foreach($input in $freeze.inputs){$h=(Get-FileHash -LiteralPath $input.path -Algorithm SHA256).Hash.ToLowerInvariant();if($h -ne $input.sha256){throw "frozen input changed: $($input.path)"}}
}
function Invoke-Guard([string]$caseName,[string[]]$argumentVector,[string]$expectedText,[string]$outputPath,[string]$sentinelPath) {
  Assert-Freeze
  if($argumentVector.Count -ne 6 -or $argumentVector[4] -ne '-P' -or $argumentVector[5] -ne $script){throw "argv shape failed for $caseName"}
  if(!(Test-Path -LiteralPath $cmake -PathType Leaf)){throw "CMake executable missing: $cmake"}
  $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$caseRoot;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
  foreach($item in $argumentVector){[void]$psi.ArgumentList.Add($item)}
  $archiveBefore=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
  $sentinelBefore=$null
  if($sentinelPath){$sentinelBefore=(Get-FileHash -LiteralPath $sentinelPath -Algorithm SHA256).Hash.ToLowerInvariant()}
  $process=[Diagnostics.Process]::new();$process.StartInfo=$psi;$watch=[Diagnostics.Stopwatch]::StartNew();[void]$process.Start();$outTask=$process.StandardOutput.ReadToEndAsync();$errTask=$process.StandardError.ReadToEndAsync();$process.WaitForExit();$exit=$process.ExitCode;$stdout=$outTask.GetAwaiter().GetResult();$stderr=$errTask.GetAwaiter().GetResult();$watch.Stop()
  $caseDir=Join-Path $caseRoot $caseName;[IO.Directory]::CreateDirectory($caseDir)|Out-Null
  [IO.File]::WriteAllText((Join-Path $caseDir 'stdout.log'),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $caseDir 'stderr.log'),$stderr,[Text.UTF8Encoding]::new($false))
  $joined=$stdout+"`n"+$stderr
  $diagnosticMatched=$joined.Contains($expectedText)
  $outputAbsent=if($sentinelPath){$false}else{!(Test-Path -LiteralPath $outputPath)}
  $sentinelAfter=$null;$sentinelPreserved=$true
  if($sentinelPath){$sentinelAfter=(Get-FileHash -LiteralPath $sentinelPath -Algorithm SHA256).Hash.ToLowerInvariant();$sentinelPreserved=$sentinelBefore -eq $sentinelAfter}
  $archiveAfter=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
  $record=[ordered]@{case=$caseName;executable=$cmake;workingDirectory=$caseRoot;arguments=$argumentVector;nativeExitCode=$exit;elapsedSeconds=$watch.Elapsed.TotalSeconds;expectedDiagnostic=$expectedText;diagnosticMatched=$diagnosticMatched;outputPath=$outputPath;outputAbsent=$outputAbsent;sentinelPath=$sentinelPath;sentinelBeforeSha256=$sentinelBefore;sentinelAfterSha256=$sentinelAfter;sentinelPreserved=$sentinelPreserved;sourceArchiveBeforeSha256=$archiveBefore;sourceArchiveAfterSha256=$archiveAfter;sourceArchiveUnchanged=($archiveBefore -eq $archiveAfter);guardPass=($exit -ne 0 -and $diagnosticMatched -and ($outputAbsent -or $sentinelPreserved) -and ($archiveBefore -eq $archiveAfter))}
  [IO.File]::WriteAllText((Join-Path $caseDir 'result.json'),($record|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
  return $record
}
$records=@()
$cases=@(
  @{name='uri';archiveArg='-DJE_MCPP_SOURCE_ARCHIVE:STRING=https://invalid.example/mcpp.tar.gz';hash=$archiveSha;url=$archiveUrl;expected='JE_MCPP_SOURCE_ARCHIVE must not be a URI';leaf='out/package';output=$null},
  @{name='unc';archiveArg='-DJE_MCPP_SOURCE_ARCHIVE:STRING=//invalid-host/share/mcpp.tar.gz';hash=$archiveSha;url=$archiveUrl;expected='JE_MCPP_SOURCE_ARCHIVE must be an absolute local path';leaf='out/package';output=$null},
  @{name='missing-archive';archiveArg="-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$(Join-Path $caseRoot 'missing-source.tar.gz')";hash=$archiveSha;url=$archiveUrl;expected='mcpp source archive file is missing';leaf='out/package';output=$null},
  @{name='bad-hash';archiveArg="-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$archive";hash=('0'*64);url=$archiveUrl;expected='source URL/SHA256 must match';leaf='out/package';output=$null},
  @{name='bad-url';archiveArg="-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$archive";hash=$archiveSha;url='https://invalid.example/mcpp.tar.gz';expected='source URL/SHA256 must match';leaf='out/package';output=$null}
)
foreach($case in $cases){
  $out=Join-Path (Join-Path $caseRoot $case.name) $case.leaf
  [IO.Directory]::CreateDirectory((Split-Path -Parent $out))|Out-Null
  $vector=[string[]]@($case.archiveArg,"-DJE_MCPP_SOURCE_SHA256:STRING=$($case.hash)","-DJE_MCPP_SOURCE_URL:STRING=$($case.url)","-DJE_MCPP_OUTPUT:PATH=$out",'-P',$script)
  $records+=Invoke-Guard $case.name $vector $case.expected $out $null
}
$tamperDir=Join-Path $caseRoot 'tampered-archive';[IO.Directory]::CreateDirectory($tamperDir)|Out-Null
$tampered=Join-Path $tamperDir 'mcpp-tampered.tar.gz';[IO.File]::Copy($archive,$tampered,$false)
$bytes=[IO.File]::ReadAllBytes($tampered);$bytes[128]=$bytes[128] -bxor 1;[IO.File]::WriteAllBytes($tampered,$bytes)
$tamperOut=Join-Path $tamperDir 'out/package';[IO.Directory]::CreateDirectory((Split-Path -Parent $tamperOut))|Out-Null
$vector=[string[]]@("-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$tampered","-DJE_MCPP_SOURCE_SHA256:STRING=$archiveSha","-DJE_MCPP_SOURCE_URL:STRING=$archiveUrl","-DJE_MCPP_OUTPUT:PATH=$tamperOut",'-P',$script)
$records+=Invoke-Guard 'tampered-archive' $vector 'mcpp source archive SHA256 mismatch' $tamperOut $null
$existingDir=Join-Path $caseRoot 'existing-output';$existingOut=Join-Path $existingDir 'package';[IO.Directory]::CreateDirectory($existingOut)|Out-Null
$sentinel=Join-Path $existingOut 'sentinel.txt';[IO.File]::WriteAllText($sentinel,'preserve-existing-output',[Text.UTF8Encoding]::new($false))
$vector=[string[]]@("-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$archive","-DJE_MCPP_SOURCE_SHA256:STRING=$archiveSha","-DJE_MCPP_SOURCE_URL:STRING=$archiveUrl","-DJE_MCPP_OUTPUT:PATH=$existingOut",'-P',$script)
$records+=Invoke-Guard 'existing-output-sentinel' $vector 'output already exists' $existingOut $sentinel
$vector=[string[]]@("-DJE_MCPP_SOURCE_ARCHIVE:FILEPATH=$archive","-DJE_MCPP_SOURCE_SHA256:STRING=$archiveSha","-DJE_MCPP_SOURCE_URL:STRING=$archiveUrl","-DJE_MCPP_OUTPUT:PATH=$outside",'-P',$script)
$records+=Invoke-Guard 'outside-build' $vector 'output must be beneath this checkout/build' $outside $null
Assert-Freeze
$summary=[ordered]@{freezeSha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash.ToLowerInvariant();freezeInputCount=$freeze.inputs.Count;cases=$records;allPassed=(@($records|Where-Object{-not $_.guardPass}).Count -eq 0);symlinkCase='not attempted; no privilege changes permitted'}
[IO.File]::WriteAllText((Join-Path $base 'mcpp-guards-summary.json'),($summary|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
foreach($item in $records){"$($item.case): exit=$($item.nativeExitCode) expected=$($item.diagnosticMatched) outputSafe=$($item.outputAbsent -or $item.sentinelPreserved) pass=$($item.guardPass)"}
"AllGuardsPassed=$($summary.allPassed)"
