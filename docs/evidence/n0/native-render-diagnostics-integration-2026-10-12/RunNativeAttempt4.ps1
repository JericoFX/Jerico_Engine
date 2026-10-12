[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Repository,
    [Parameter(Mandatory=$true)][string]$IntegratedFreezePath,
    [Parameter(Mandatory=$true)][string]$IntegratedFreezeSHA256,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory
)
$ErrorActionPreference='Stop'
$script:fatal=$null
$script:phases=@()
$repo=[IO.Path]::GetFullPath($Repository)
$freezePath=[IO.Path]::GetFullPath($IntegratedFreezePath)
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
$build=Join-Path $repo 'build\n0-validation\original-engine'
$cache=Join-Path $build 'CMakeCache.txt'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$ctest='C:\Program Files\CMake\bin\ctest.exe'
if(Test-Path -LiteralPath $evidence){if(@(Get-ChildItem -LiteralPath $evidence -Force).Count -gt 0){throw "Evidence directory is not fresh: $evidence"}}else{New-Item -ItemType Directory -Path $evidence|Out-Null}
if((Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash -ne $IntegratedFreezeSHA256){throw 'Integrated freeze hash mismatch'}
$freeze=Get-Content -LiteralPath $freezePath -Raw|ConvertFrom-Json
if($freeze.status -ne 'FROZEN_FOR_NATIVE_VALIDATION' -or $freeze.inputs.Count -ne 78){throw 'Unexpected integrated freeze status/count'}
$freezeRoot=Split-Path (Split-Path $freezePath)
foreach($writerPath in $freeze.writerFreezes){$writer=Join-Path $freezeRoot $writerPath;$w=Get-Content -LiteralPath $writer -Raw|ConvertFrom-Json;if($w.status -notin @('HOLD_SOURCE_ONLY_NATIVE_NOT_RUN','IMPLEMENTED_NOT_COMPILED_HOLD')){throw "Writer freeze not held: $writerPath status=$($w.status)"}}
function Get-InputMap {
  $map=[ordered]@{}
  foreach($e in $freeze.inputs){$p=[IO.Path]::GetFullPath([string]$e.path);if(!(Test-Path -LiteralPath $p -PathType Leaf)){throw "Frozen input missing: $p"};$i=Get-Item -LiteralPath $p;$h=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash;if($i.Length -ne [long]$e.bytes -or $h -ne [string]$e.sha256){throw "Frozen input mismatch: $p"};$map[$p]=[pscustomobject]@{bytes=$i.Length;sha256=$h}}
  return $map
}
$before=Get-InputMap
$required=@('CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022','CMAKE_GENERATOR_PLATFORM:INTERNAL=x64','FETCHCONTENT_FULLY_DISCONNECTED:BOOL=ON','LY_PACKAGE_SERVER_URLS:STRING=','O3DE_EXTERNAL_SUBDIRS:STRING=','LY_PROJECTS:STRING=F:/JericoEngine/JericoEngine/Projects/JEEditorFixture')
$cacheLines=Get-Content -LiteralPath $cache
foreach($line in $required){if($line -notin $cacheLines){throw "Offline/config cache guard failed: $line"}}
$blocked=@('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','JEEditorFixture.GameLauncher.exe','MaterialEditor.exe','MaterialCanvas.exe','PassCanvas.exe','ShaderManagementConsole.exe','Profiler.Tests.exe','RenderDebug.Tests.exe','DebugImGui.Tests.exe','EditorLib.Tests.exe','cmake.exe','MSBuild.exe','cl.exe','link.exe','ctest.exe','AzTestRunner.exe')
$allProcesses=@(Get-CimInstance Win32_Process)
$live=@($allProcesses|Where-Object{$blocked -contains $_.Name})
$residualNodes=@($allProcesses|Where-Object{$_.Name -eq 'MSBuild.exe' -and $_.CommandLine -match '/nodemode:1' -and $allProcesses.ProcessId -notcontains $_.ParentProcessId})
# Treat detached MSBuild node-reuse workers from the prior owned attempt as residual only; any active parent/non-node MSBuild still blocks.
$blocking=@($live|Where-Object{if($_.Name -ne 'MSBuild.exe'){$true}else{($_.CommandLine -notmatch '/nodemode:1') -or ($allProcesses.ProcessId -contains $_.ParentProcessId)}})
if($blocking.Count){throw "Active product/build processes: $(($blocking|ForEach-Object{"$($_.Name):$($_.ProcessId)"}) -join ', ')"}
$env:MSBUILDDISABLENODEREUSE='1'
$pre=[ordered]@{capturedUtc=[DateTime]::UtcNow.ToString('o');freezePath=$freezePath;freezeSha256=$IntegratedFreezeSHA256;inputCount=$before.Count;inputs=$before;cachePath=$cache;requiredCacheLines=$required;activeProcesses=@();residualMSBuildNodeReuse=@($residualNodes|ForEach-Object{[pscustomobject]@{processId=$_.ProcessId;parentProcessId=$_.ParentProcessId;creationDate=$_.CreationDate;commandLine=$_.CommandLine}});msbuildDisableNodeReuse=$env:MSBUILDDISABLENODEREUSE;configureReusedFrom='native-attempt3/configure exit 0'}
[IO.File]::WriteAllText((Join-Path $evidence 'preflight.json'),($pre|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
function Invoke-Phase([string]$name,[string]$exe,[string[]]$phaseArguments){
  $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$exe;$psi.WorkingDirectory=$repo;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true;foreach($a in $phaseArguments){[void]$psi.ArgumentList.Add($a)}
  $p=[Diagnostics.Process]::new();$p.StartInfo=$psi;$start=[DateTime]::UtcNow;$sw=[Diagnostics.Stopwatch]::StartNew();if(!$p.Start()){throw "Could not start $name"};$outTask=$p.StandardOutput.ReadToEndAsync();$errTask=$p.StandardError.ReadToEndAsync();$startRecord=[ordered]@{phase=$name;executable=$exe;arguments=$phaseArguments;workingDirectory=$repo;startedUtc=$start.ToString('o');nativePid=$p.Id};[IO.File]::WriteAllText((Join-Path $evidence "$name-start.json"),($startRecord|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false));Write-Output "$name PID=$($p.Id) START_UTC=$($start.ToString('o'))";[Console]::Out.Flush();$p.WaitForExit();$sw.Stop();$code=$p.ExitCode;$stdout=$outTask.GetAwaiter().GetResult();$stderr=$errTask.GetAwaiter().GetResult();[IO.File]::WriteAllText((Join-Path $evidence "$name.stdout.txt"),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $evidence "$name.stderr.txt"),$stderr,[Text.UTF8Encoding]::new($false));$row=[ordered]@{phase=$name;exitCode=$code;startedUtc=$start.ToString('o');finishedUtc=[DateTime]::UtcNow.ToString('o');elapsedSeconds=$sw.Elapsed.TotalSeconds;stdoutBytes=[Text.Encoding]::UTF8.GetByteCount($stdout);stderrBytes=[Text.Encoding]::UTF8.GetByteCount($stderr)};[IO.File]::WriteAllText((Join-Path $evidence "$name-result.json"),($row|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false));$script:phases+= $row;if($code -ne 0){throw "$name exited $code"}
}
function Assert-CTestInventory([string[]]$expected,[string]$phaseName){
  $regex='^('+ (($expected|ForEach-Object{[regex]::Escape($_)}) -join '|') +')$'
  $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$ctest;$psi.WorkingDirectory=$repo;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true;foreach($a in @('--test-dir',$build,'-C','Profile','--show-only=json-v1','-R',$regex)){[void]$psi.ArgumentList.Add($a)}
  $p=[Diagnostics.Process]::new();$p.StartInfo=$psi;$start=[DateTime]::UtcNow;$sw=[Diagnostics.Stopwatch]::StartNew();if(!$p.Start()){throw "Could not start $phaseName"};$outTask=$p.StandardOutput.ReadToEndAsync();$errTask=$p.StandardError.ReadToEndAsync();$startRecord=[ordered]@{phase=$phaseName;executable=$ctest;arguments=@('--test-dir',$build,'-C','Profile','--show-only=json-v1','-R',$regex);startedUtc=$start.ToString('o');nativePid=$p.Id};[IO.File]::WriteAllText((Join-Path $evidence "$phaseName-start.json"),($startRecord|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false));$p.WaitForExit();$sw.Stop();$code=$p.ExitCode;$stdout=$outTask.GetAwaiter().GetResult();$stderr=$errTask.GetAwaiter().GetResult();[IO.File]::WriteAllText((Join-Path $evidence "$phaseName.stdout.txt"),$stdout,[Text.UTF8Encoding]::new($false));[IO.File]::WriteAllText((Join-Path $evidence "$phaseName.stderr.txt"),$stderr,[Text.UTF8Encoding]::new($false));if($code -ne 0){throw "$phaseName inventory exited $code"};$json=$stdout|ConvertFrom-Json;$names=@($json.tests|ForEach-Object{$_.name});$delta=@($names|Where-Object{$_ -notin $expected});$missing=@($expected|Where-Object{$_ -notin $names});if($names.Count -ne $expected.Count -or $delta.Count -or $missing.Count){throw "Unexpected $phaseName CTest inventory. Actual=$($names -join ', ') Missing=$($missing -join ', ') Extra=$($delta -join ', ')"};[IO.File]::WriteAllText((Join-Path $evidence "$phaseName.json"),($json|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false));$script:phases+= [ordered]@{phase=$phaseName;exitCode=$code;startedUtc=$start.ToString('o');finishedUtc=[DateTime]::UtcNow.ToString('o');elapsedSeconds=$sw.Elapsed.TotalSeconds;testNames=$names}
}
try {
  Invoke-Phase 'build-focal-tests' $cmake @('--build',$build,'--config','Profile','--target','Profiler.Tests','RenderDebug.Tests','--parallel','6')
  $focal=@('JE::Profiler.Tests.main::TEST_RUN','JE::RenderDebug.Tests.main::TEST_RUN')
  Assert-CTestInventory $focal 'ctest-focal-inventory'
  foreach($test in $focal){$safe=$test -replace '[^A-Za-z0-9.-]','_';Invoke-Phase "ctest-focal-$safe" $ctest @('--test-dir',$build,'-C','Profile','--output-on-failure','--tests-regex',("^"+[regex]::Escape($test)+"$"))}
  Invoke-Phase 'build-hosts' $cmake @('--build',$build,'--config','Profile','--target','DebugImGui.Tests','EditorLib.Tests','Editor','AssetProcessor','AssetProcessorBatch','AssetBuilder','JEEditorFixture.GameLauncher','MaterialEditor','MaterialCanvas','PassCanvas','ShaderManagementConsole','--parallel','6')
  $hosts=@('JE::DebugImGui.Tests.main::TEST_RUN','Legacy::EditorLib.Tests.main::TEST_RUN')
  Assert-CTestInventory $hosts 'ctest-host-inventory'
  foreach($test in $hosts){$safe=$test -replace '[^A-Za-z0-9.-]','_';Invoke-Phase "ctest-host-$safe" $ctest @('--test-dir',$build,'-C','Profile','--output-on-failure','--tests-regex',("^"+[regex]::Escape($test)+"$"))}
} catch { $script:fatal=$_.Exception.Message }
$post=[ordered]@{capturedUtc=[DateTime]::UtcNow.ToString('o');freezePath=$freezePath;freezeSha256=$IntegratedFreezeSHA256;inputCount=$freeze.inputs.Count;inputs=[ordered]@{};mismatches=@();activeProcesses=@();phases=$script:phases;fatal=$script:fatal}
foreach($e in $freeze.inputs){$p=[IO.Path]::GetFullPath([string]$e.path);if(!(Test-Path -LiteralPath $p -PathType Leaf)){$post.mismatches+=@([pscustomobject]@{path=$p;status='MISSING'});continue};$i=Get-Item -LiteralPath $p;$h=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash;$post.inputs[$p]=[pscustomobject]@{bytes=$i.Length;sha256=$h};if($i.Length -ne [long]$e.bytes -or $h -ne [string]$e.sha256){$post.mismatches+=@([pscustomobject]@{path=$p;status='MISMATCH';bytes=$i.Length;sha256=$h})}}
[IO.File]::WriteAllText((Join-Path $evidence 'postflight.json'),($post|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
if($script:fatal){Write-Error $script:fatal;exit 1};if($post.mismatches.Count){Write-Error "Postflight mismatches: $($post.mismatches.Count)";exit 1};exit 0
