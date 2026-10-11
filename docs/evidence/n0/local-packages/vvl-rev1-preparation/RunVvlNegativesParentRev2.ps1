#Requires -Version 7.0
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition-ne'Core'){throw 'PowerShell Core required'}
$root='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-negatives'
$attempt="$root/attempt-rev2"
if(Test-Path -LiteralPath $attempt){throw 'Attempt exists; retain it'}
$positive=Get-Content -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-validation/attempt1/validation-result.json' -Raw | ConvertFrom-Json
$production=@($positive.commands | Where-Object {$_.label -in @('producer','extract-produced-archive')})
if($production.Count-ne2-or@($production | Where-Object nativeExit -ne 0).Count-or$positive.changedPaths.Count){throw 'Original production/extraction gate not passed'}
$consumer=Get-Content -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-validation/attempt1/validation-result.json' -Raw | ConvertFrom-Json
if($consumer.commands.Count-ne3-or@($consumer.commands | Where-Object nativeExit -ne 0).Count-or$consumer.sourceChanges.Count){throw 'Corrected consumer gate not passed'}
[xml]$junit=Get-Content -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev4-validation/attempt1/ctest.junit.xml' -Raw
$caseNames=@($junit.SelectNodes('//testcase') | ForEach-Object {[string]$_.name})
$expectedCases=@('JE.LocalPackages.VVL.release.exports','JE.LocalPackages.VVL.release.runtime','JE.LocalPackages.VVL.debug.exports','JE.LocalPackages.VVL.debug.runtime')
if($caseNames.Count-ne4-or@($junit.SelectNodes('//failure|//error|//skipped')).Count-or(($caseNames|Sort-Object)-join "`n")-ne(($expectedCases|Sort-Object)-join "`n")){throw 'Four corrected consumer cases were not passed'}
$freezePath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-preparation/consumer-rev4/source-freeze-vvl-provider-consumer-rev4.json'
if((Get-FileHash -LiteralPath $freezePath).Hash-ne'ABE7DE1FB29A4A1296337F756A7AFC4F0E65514948AE8A45A6E99F21F7DF34F2'){throw 'Current freeze changed'}
$freeze=Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
if($freeze.inputs.Count-ne6471){throw 'Unexpected source count'}
$packageRoot='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-consumer/package'
if((Get-FileHash -LiteralPath "$packageRoot/SHA256SUMS").Hash-ne'99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'){throw 'Payload manifest drift'}
$packageInputs=@(foreach($line in Get-Content -LiteralPath "$packageRoot/SHA256SUMS"){
 if($line-notmatch'^([a-f0-9]{64}) \*([^:]+)$'){throw 'Malformed manifest'}
 $digest=$Matches[1];$relative=$Matches[2];$full=[IO.Path]::GetFullPath((Join-Path $packageRoot $relative))
 if(-not$full.StartsWith([IO.Path]::GetFullPath("$packageRoot/"),[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest escape'}
 [pscustomobject]@{path=$full;sha256=$digest}
})
$packageInputs+=[pscustomobject]@{path="$packageRoot/SHA256SUMS";sha256='99C9654B300D1BBB88A04DBDEF4389D05BD4D91B0C3B2027595DFE37F8AA420A'}
if($packageInputs.Count-ne35){throw 'Unexpected package count'}
function Test-FrozenInputs {
 $bad=@()
 foreach($entry in @($freeze.inputs)+@($packageInputs)){
  if(-not(Test-Path -LiteralPath $entry.path -PathType Leaf)){ $bad+=$entry.path;continue }
  if((Get-FileHash -LiteralPath $entry.path).Hash-ne$entry.sha256){$bad+=$entry.path}
  elseif($null-ne$entry.bytes-and(Get-Item -LiteralPath $entry.path).Length-ne$entry.bytes){$bad+=$entry.path}
 }
 if(@(Get-ChildItem -LiteralPath $packageRoot -Recurse -File).Count-ne35){$bad+='Package inventory drift'}
 return ,$bad
}
if((Test-FrozenInputs).Count){throw 'Current frozen inputs changed'}
if((Get-FileHash -LiteralPath "$root/negative-assignment-rev1.json").Hash-ne'C1E21DF5AF11B36C835B2E78158EDB0C7190EEFBEE6ABE0987BF0FA570FC3DAD'){throw 'Negative assignment drift'}
$assignment=Get-Content -LiteralPath "$root/negative-assignment-rev1.json" -Raw | ConvertFrom-Json
if($assignment.commands.Count-ne11-or$assignment.fixtures.Count-ne5){throw 'Negative counts differ'}
function Test-Fixtures {
    $bad=@()
    foreach($entry in $assignment.fixtures){$item=Get-Item -LiteralPath $entry.path; if($item.Length-ne$entry.bytes-or(Get-FileHash -LiteralPath $entry.path).Hash-ne$entry.sha256){$bad+=$entry.path}}
    return ,$bad
}
if((Test-Fixtures).Count){throw 'Prepared fixture changed'}
[System.IO.Directory]::CreateDirectory($attempt) | Out-Null
$encoding=[System.Text.UTF8Encoding]::new($false)
function Write-Json($path,$value){[System.IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 10)+"`n",$encoding)}
$results=@()
$failed=$false
try{
    foreach($command in $assignment.commands){
        $prefix="$attempt/$($command.label)"
        Write-Json "$prefix.argv.json" @{executable=$command.executable;argv=@($command.argv)}
        $info=[System.Diagnostics.ProcessStartInfo]::new()
        $info.FileName=$command.executable
        $info.UseShellExecute=$false; $info.CreateNoWindow=$true
        $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
        foreach($arg in $command.argv){$info.ArgumentList.Add([string]$arg)}
        $process=[System.Diagnostics.Process]::new(); $process.StartInfo=$info
        $watch=[System.Diagnostics.Stopwatch]::StartNew()
        try{
            if(-not$process.Start()){throw 'Native start failed'}
            $stdoutTask=$process.StandardOutput.ReadToEndAsync()
            $stderrTask=$process.StandardError.ReadToEndAsync()
            $process.WaitForExit()
            $nativeExit=$process.ExitCode
            $watch.Stop()
            $stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult()
            [System.IO.File]::WriteAllText("$prefix.stdout.txt",$stdout,$encoding)
            [System.IO.File]::WriteAllText("$prefix.stderr.txt",$stderr,$encoding)
            # CMake wraps diagnostics; retain raw logs and match the same words.
            $diagnostic=($stdout+"`n"+$stderr) -replace '\s+',' '
            $observed=$diagnostic -match [regex]::Escape($command.expectedError)
            $absent= -not $command.outputMustRemainAbsent -or -not(Test-Path -LiteralPath $command.outputMustRemainAbsent)
            $pass=$nativeExit-ne0-and$observed-and$absent
            $result=[ordered]@{label=$command.label;nativeExit=$nativeExit;elapsedSeconds=$watch.Elapsed.TotalSeconds;expectedError=$command.expectedError;observedExpectedError=$observed;outputCondition=$absent;status=if($pass){'PASS REJECTION'}else{'FAIL'}}
            Write-Json "$prefix.result.json" $result
            $results+=$result
            Write-Host "$($command.label): exit=$nativeExit expected=$observed output=$absent"
            if(-not$pass){$failed=$true;break}
        }finally{$process.Dispose()}
    }
}catch{
    $failed=$true
    Write-Json "$attempt/driver-error.json" @{message=$_.Exception.Message;scriptStack=$_.ScriptStackTrace}
}finally{
    $changed=Test-Fixtures
    $sourceChanges=Test-FrozenInputs
    if($sourceChanges.Count-or$results.Count-ne11){$failed=$true}
    if($changed.Count){$failed=$true}
    Write-Json "$attempt/negative-result.json" @{status=if($failed){'FAIL; RETAIN ACTUAL RESULTS'}else{'FOCAL REJECTIONS PASS; PARENT REVIEW PENDING'};results=@($results);fixtureChecks=$assignment.fixtures.Count;changedPaths=@($changed);sourceInputs=6471;packageFiles=35;sourceChanges=@($sourceChanges)}
}
if($failed){exit 1}
exit 0
