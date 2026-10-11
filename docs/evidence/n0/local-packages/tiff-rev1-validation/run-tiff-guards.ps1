$ErrorActionPreference='Stop'
$repo='F:/JericoEngine/JericoEngine';$base=Join-Path $repo 'build/n0-validation/local-packages/tiff-rev1-validation';$guardRoot=Join-Path $base 'guards'
$cmake='C:/Program Files/CMake/bin/cmake.exe';$producerScript="$repo/cmake/Tools/LocalPackages/TIFF/BuildTiffPackage.cmake";$archive=Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/libtiff-4862b0d7bcc786304ff4e8c31e8d5ccfb868fb99.tar.gz';$zlib='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/zlib-rev1/packages/zlib-1.2.11-je-rev1-windows.contents'
$archiveHash='cce672d2f5941759573cb3904cc8f00f065d54a3689a80719229e961d6a324c8';$outside='F:/JericoEngine/Dependencies/je-tiff-invalid-output'
if((Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $archiveHash){throw 'Frozen TIFF archive hash mismatch before guards'}
if(Test-Path -LiteralPath $outside){throw 'Outside-build guard path must be absent before test'}
$zlibInputs=@('FindZLIB.cmake','zlib/include/zlib.h','zlib/include/zconf.h','zlib/lib/zlibstatic.lib');$zlibBefore=@();foreach($r in $zlibInputs){$p=Join-Path $zlib $r;$zlibBefore += [ordered]@{path=$p;sha256=(Get-FileHash $p -Algorithm SHA256).Hash.ToLowerInvariant()}}
$manifest=Get-Content -Raw (Join-Path $repo 'build/n0-validation/local-packages/tiff-audit/validation-arguments.json')|ConvertFrom-Json
$baseProducer=[string[]]$manifest.producer.argv
function Invoke-Captured {
 param([string]$Label,[string[]]$Vector,[string]$Expected,[string]$Output,[bool]$ExpectExisting=$false)
 $dir=Join-Path $guardRoot $Label;[IO.Directory]::CreateDirectory($dir)|Out-Null
 if($Vector.Count -ne 5 -or $Vector[3] -ne '-P' -or $Vector[4] -ne $producerScript){throw "Producer guard vector mismatch: $Label"}
 [IO.File]::WriteAllText((Join-Path $dir 'argv.json'),(@{executable=$cmake;workingDirectory=$dir;arguments=$Vector}|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
 $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$dir;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true;foreach($a in $Vector){[void]$psi.ArgumentList.Add($a)}
 $pr=[Diagnostics.Process]::new();$pr.StartInfo=$psi;$sw=[Diagnostics.Stopwatch]::StartNew();if(-not $pr.Start()){throw "Guard process did not start: $Label"};$ot=$pr.StandardOutput.ReadToEndAsync();$et=$pr.StandardError.ReadToEndAsync();$pr.WaitForExit();$exit=$pr.ExitCode;$sw.Stop();$outText=$ot.GetAwaiter().GetResult();$errText=$et.GetAwaiter().GetResult()
 [IO.File]::WriteAllText((Join-Path $dir 'stdout.log'),$outText,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $dir 'stderr.log'),$errText,[Text.UTF8Encoding]::new($false));$diag=($outText+"`n"+$errText).Contains($Expected);$exists=Test-Path -LiteralPath $Output
 $result=[ordered]@{case=$Label;exitCode=$exit;elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3);expectedDiagnostic=$Expected;diagnosticMatched=$diag;output=$Output;outputExists=$exists;outputWasExpectedExisting=$ExpectExisting}
 [IO.File]::WriteAllText((Join-Path $dir 'result.json'),($result|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
 if($exit -eq 0 -or !$diag){throw "Guard failed expected rejection: $Label, exit=$exit"};if(!$ExpectExisting -and $exists){throw "Unexpected output created: $Label"};return [pscustomobject]$result
}
function New-ProducerVector {
 param([string]$Case,[string]$ChangeValue,[int]$ArgIndex,[string]$Type)
 $v=[string[]]$baseProducer.Clone();$parts=$v[$ArgIndex].Split('=',2);$v[$ArgIndex]="$($parts[0].Split(':')[0]):$Type=$ChangeValue";return $v
}
$results=@()
$uriOut=Join-Path $guardRoot 'uri/output';$results+=Invoke-Captured 'uri-output' (New-ProducerVector 'uri' 'https://example.invalid/tiff' 2 'STRING') 'must not be a URI' $uriOut
$uncOut=Join-Path $guardRoot 'unc-output/output';$results+=Invoke-Captured 'unc-output' (New-ProducerVector 'unc' '//server/share/tiff-invalid' 2 'STRING') 'must be an absolute local path' $uncOut
$outsideOut=$outside;$results+=Invoke-Captured 'outside-build' (New-ProducerVector 'outside' $outside 2 'PATH') 'output must be beneath this checkout/build' $outsideOut
$existingOut=Join-Path $repo 'build/n0-validation/local-packages/tiff-audit';$results+=Invoke-Captured 'existing-output' (New-ProducerVector 'existing' $existingOut 2 'PATH') 'output must be new; existing outputs are preserved' $existingOut $true
$missingArchive=Join-Path $guardRoot 'missing-archive/not-present.tar.gz';$missingOut=Join-Path $guardRoot 'missing-archive/output';$results+=Invoke-Captured 'missing-archive' (New-ProducerVector 'missing' $missingArchive 0 'FILEPATH') 'file is missing' $missingOut
$tamperedDir=Join-Path $guardRoot 'changed-archive';[IO.Directory]::CreateDirectory($tamperedDir)|Out-Null;$tamperedArchive=Join-Path $tamperedDir 'tiff-tampered.tar.gz';$bytes=[IO.File]::ReadAllBytes($archive);$bytes[0]=$bytes[0] -bxor 1;[IO.File]::WriteAllBytes($tamperedArchive,$bytes);$changedOut=Join-Path $tamperedDir 'output';$results+=Invoke-Captured 'changed-archive' (New-ProducerVector 'changed' $tamperedArchive 0 'FILEPATH') 'SHA256 mismatch' $changedOut
$badZlib=Join-Path $guardRoot 'changed-zlib-root';if(Test-Path $badZlib){throw 'Changed zlib fixture exists'};Copy-Item -LiteralPath $zlib -Destination $badZlib -Recurse
$zlibHeader=Join-Path $badZlib 'zlib/include/zlib.h';$zh=[IO.File]::ReadAllBytes($zlibHeader);$zh[0]=$zh[0] -bxor 1;[IO.File]::WriteAllBytes($zlibHeader,$zh);$badZlibOut=Join-Path $guardRoot 'changed-zlib-output'
$zvector=[string[]]$baseProducer.Clone();$zvector[1]="-DJE_ZLIB_PACKAGE_ROOT:PATH=$badZlib";$zvector[2]="-DJE_TIFF_OUTPUT:PATH=$badZlibOut";$results+=Invoke-Captured 'changed-zlib-input' $zvector 'zlib 1.2.11 header SHA256 mismatch' $badZlibOut
$wrongSource=Join-Path $guardRoot 'wrong-exact-source';$wrongBuild=Join-Path $guardRoot 'wrong-exact-build';[IO.Directory]::CreateDirectory($wrongSource)|Out-Null
$cmakelists=@'
cmake_minimum_required(VERSION 3.24)
project(JETiffExactVersionNegative LANGUAGES NONE)
list(PREPEND CMAKE_MODULE_PATH "${JE_TIFF_PACKAGE_ROOT}" "${JE_ZLIB_PACKAGE_ROOT}")
find_package(TIFF 4.3 EXACT REQUIRED MODULE)
'@
[IO.File]::WriteAllText((Join-Path $wrongSource 'CMakeLists.txt'),$cmakelists,[Text.UTF8Encoding]::new($false))
$wrongVector=[string[]]@('-S',$wrongSource,'-B',$wrongBuild,'-DJE_TIFF_PACKAGE_ROOT:PATH='+$base+'/extracted','-DJE_ZLIB_PACKAGE_ROOT:PATH='+$zlib)
[IO.File]::WriteAllText((Join-Path $guardRoot 'wrong-exact-identity-argv.json'),(@{executable=$cmake;workingDirectory=$guardRoot;arguments=$wrongVector}|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
$psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$cmake;$psi.WorkingDirectory=$guardRoot;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true;foreach($a in $wrongVector){[void]$psi.ArgumentList.Add($a)}
$pr=[Diagnostics.Process]::new();$pr.StartInfo=$psi;$sw=[Diagnostics.Stopwatch]::StartNew();if(-not $pr.Start()){throw 'Exact mismatch configure failed to start'};$ot=$pr.StandardOutput.ReadToEndAsync();$et=$pr.StandardError.ReadToEndAsync();$pr.WaitForExit();$identityExit=$pr.ExitCode;$sw.Stop();$identityOut=$ot.GetAwaiter().GetResult();$identityErr=$et.GetAwaiter().GetResult();[IO.File]::WriteAllText((Join-Path $guardRoot 'wrong-exact-identity.stdout.log'),$identityOut,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $guardRoot 'wrong-exact-identity.stderr.log'),$identityErr,[Text.UTF8Encoding]::new($false))
$combined=$identityOut+"`n"+$identityErr;$identityExpected=($combined -match 'unsuitable version|could not find.*version|version.*4\.3');$identityResult=[ordered]@{case='wrong-exact-tiff-4.3';exitCode=$identityExit;elapsedSeconds=[Math]::Round($sw.Elapsed.TotalSeconds,3);expectedVersion='4.3 EXACT';actualPackageVersion='4.2.0.15';diagnosticMatched=$identityExpected;binaryCompilationAttempted=$false;buildDirectoryExists=(Test-Path $wrongBuild)}
[IO.File]::WriteAllText((Join-Path $guardRoot 'wrong-exact-identity-result.json'),($identityResult|ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false));if($identityExit -eq 0 -or !$identityExpected){throw 'Wrong EXACT identity guard did not reject as expected'}
$archiveAfter=(Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant();$zlibAfter=@();foreach($x in $zlibInputs){$p=Join-Path $zlib $x;$zlibAfter += [ordered]@{path=$p;sha256=(Get-FileHash $p -Algorithm SHA256).Hash.ToLowerInvariant()}}
if($archiveAfter -ne $archiveHash){throw 'Frozen TIFF archive changed during guards'};foreach($i in 0..($zlibBefore.Count-1)){if($zlibBefore[$i].sha256 -ne $zlibAfter[$i].sha256){throw 'Frozen zlib package changed during guards'}}
$summary=[ordered]@{producerGuardCount=$results.Count;producerGuards=$results;wrongExactIdentity=$identityResult;archiveSHA256Before=$archiveHash;archiveSHA256After=$archiveAfter;zlibInputsBefore=$zlibBefore;zlibInputsAfter=$zlibAfter;outsideOutputPath=$outside;outsideOutputStillAbsent=(!(Test-Path -LiteralPath $outside));existingOutputPreserved=(Test-Path -LiteralPath $existingOut);tamperedArchiveCopy=$tamperedArchive;modifiedZlibFixture=$badZlib}
[IO.File]::WriteAllText((Join-Path $guardRoot 'guard-summary.json'),($summary|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false));Write-Output ($summary|ConvertTo-Json -Compress -Depth 5)
