$ErrorActionPreference='Stop'
$here=Join-Path $PSScriptRoot 'binary-audit'
if(Test-Path -LiteralPath $here){throw 'Audit output already exists'}
New-Item -ItemType Directory -Path $here | Out-Null
$bin='F:/JericoEngine/JericoEngine/build/n0-validation/original-engine/bin/profile'
$dumpbin='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe'
$baselinePath='F:/JericoEngine/JericoEngine/build/n0-validation/native-render-diagnostics-rev1/baseline/baseline.json'
$baseline=Get-Content -LiteralPath $baselinePath -Raw|ConvertFrom-Json
$hosts=@('Editor.exe','AssetProcessor.exe','AssetProcessorBatch.exe','AssetBuilder.exe','JEEditorFixture.GameLauncher.exe','MaterialEditor.exe','MaterialCanvas.exe','PassCanvas.exe','ShaderManagementConsole.exe')
$required=@('ProfilerCapture.dll','ProfilerView.dll','RenderDebug.dll')+$hosts
foreach($name in $required){if(-not(Test-Path -LiteralPath (Join-Path $bin $name))){throw "Missing required binary $name"}}
$targets=@(Get-ChildItem -LiteralPath $bin -File -Filter '*.dll')+@($hosts|ForEach-Object {Get-Item -LiteralPath (Join-Path $bin $_)})
$utf8=[Text.UTF8Encoding]::new($false)
function HashFile($path){$stream=[IO.File]::OpenRead($path);try{return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
$rows=@(foreach($file in $targets){
 $before=HashFile $file.FullName
 $start=[DateTime]::UtcNow
 $psi=[Diagnostics.ProcessStartInfo]::new();$psi.FileName=$dumpbin;$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true;$psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
 $psi.ArgumentList.Add('/DEPENDENTS');$psi.ArgumentList.Add($file.FullName)
 $proc=[Diagnostics.Process]::new();$proc.StartInfo=$psi
 [void]$proc.Start();$nativePid=$proc.Id
 $outTask=$proc.StandardOutput.ReadToEndAsync();$errTask=$proc.StandardError.ReadToEndAsync();$proc.WaitForExit();$exit=$proc.ExitCode
 $stdout=$outTask.GetAwaiter().GetResult();$stderr=$errTask.GetAwaiter().GetResult();$end=[DateTime]::UtcNow;$proc.Dispose()
 $stem=$file.Name.Replace('.','_');$outPath=Join-Path $here ($stem+'.dependents.stdout.txt');$errPath=Join-Path $here ($stem+'.dependents.stderr.txt')
 foreach($p in @($outPath,$errPath)){if(Test-Path -LiteralPath $p){throw 'Output exists'}}
 [IO.File]::WriteAllText($outPath,$stdout,$utf8);[IO.File]::WriteAllText($errPath,$stderr,$utf8)
 if($exit -ne 0){throw "dumpbin failed $($file.Name): $exit"}
 $after=HashFile $file.FullName;if($before -ne $after){throw "Binary drift $($file.Name)"}
 $deps=@([regex]::Matches($stdout,'(?im)^\s+([^\r\n]+\.dll)\s*$')|ForEach-Object {$_.Groups[1].Value.Trim()})
 [pscustomobject]@{name=$file.Name;path=$file.FullName;bytes=$file.Length;sha256=$before;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o');exitCode=$exit;pid=$nativePid;startedUtc=$start.ToString('o');finishedUtc=$end.ToString('o');dependents=$deps;retiredImports=@($deps|Where-Object {$_ -in @('Profiler.dll','ProfilerImGui.dll','ImguiAtom.dll','AtomImGuiTools.dll')});nativeImports=@($deps|Where-Object {$_ -in @('ProfilerCapture.dll','ProfilerView.dll','RenderDebug.dll')});stdout=$outPath;stderr=$errPath;inputUnchanged=$true}
})
[IO.File]::WriteAllText((Join-Path $here 'dependents-full.json'),($rows|ConvertTo-Json -Depth 7),$utf8)
$current=@(foreach($name in @('ProfilerCapture.dll','ProfilerCapture.pdb','ProfilerView.dll','ProfilerView.pdb','RenderDebug.dll','RenderDebug.pdb')){$p=Join-Path $bin $name;$f=Get-Item -LiteralPath $p;[pscustomobject]@{name=$name;bytes=$f.Length;sha256=(HashFile $p)}})
$comparison=[pscustomobject]@{baselineManifest=$baselinePath;baselineSHA256=(HashFile $baselinePath);baselineRetired=$baseline.artifacts;currentNative=$current;baselineDLLBytes=($baseline.artifacts|Where-Object name -like '*.dll'|Measure-Object bytes -Sum).Sum;currentDLLBytes=($current|Where-Object name -like '*.dll'|Measure-Object bytes -Sum).Sum;baselinePDBBytes=($baseline.artifacts|Where-Object name -like '*.pdb'|Measure-Object bytes -Sum).Sum;currentPDBBytes=($current|Where-Object name -like '*.pdb'|Measure-Object bytes -Sum).Sum;scope='Four retired implementation DLLs/PDBs versus three current native implementation DLLs/PDBs only. Retired files not deleted; no whole-product savings claim.'}
[IO.File]::WriteAllText((Join-Path $here 'size-comparison.json'),($comparison|ConvertTo-Json -Depth 6),$utf8)
$summary=[pscustomobject]@{status='DATA_AUDIT_COMPLETE_NO_PRODUCT_EXECUTION';scanCount=$rows.Count;dllCount=@($rows|Where-Object name -like '*.dll').Count;hostCount=$hosts.Count;requiredOutputs=@($rows|Where-Object name -in $required);directNativeConsumers=@($rows|Where-Object {$_.nativeImports.Count -gt 0});retiredImportFindings=@($rows|Where-Object {$_.retiredImports.Count -gt 0});tool=[pscustomobject]@{path=$dumpbin;sha256=(HashFile $dumpbin)};limits='All DLLs present are inspected to discover direct consumers, including stale inactive artifacts and tests. Import tables do not prove runtime module loading. No binary deletion or rebuild.'}
[IO.File]::WriteAllText((Join-Path $here 'audit-summary.json'),($summary|ConvertTo-Json -Depth 8),$utf8)
[pscustomobject]@{scanCount=$rows.Count;directNativeConsumers=$summary.directNativeConsumers.Count;retiredImportFindings=$summary.retiredImportFindings.Count;retiredImportNames=@($summary.retiredImportFindings|ForEach-Object name);baselineDLLBytes=$comparison.baselineDLLBytes;currentDLLBytes=$comparison.currentDLLBytes;baselinePDBBytes=$comparison.baselinePDBBytes;currentPDBBytes=$comparison.currentPDBBytes}|ConvertTo-Json -Depth 4