#Requires -Version 7.0
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition-ne'Core'){throw 'PowerShell Core required'}
$root='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-negatives'
$attempt="$root/attempt1"
if(Test-Path -LiteralPath $attempt){throw 'Attempt exists; retain it'}
$positive=Get-Content -LiteralPath 'F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vvl-rev1-validation/attempt1/validation-result.json' -Raw | ConvertFrom-Json
if($positive.commands.Count-ne5-or@($positive.commands | Where-Object nativeExit -ne 0).Count-or$positive.changedPaths.Count){throw 'Positive consumer gate not passed'}
$assignment=Get-Content -LiteralPath "$root/negative-assignment-rev1.json" -Raw | ConvertFrom-Json
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
    if($changed.Count){$failed=$true}
    Write-Json "$attempt/negative-result.json" @{status=if($failed){'FAIL; RETAIN ACTUAL RESULTS'}else{'FOCAL REJECTIONS PASS; PARENT REVIEW PENDING'};results=@($results);fixtureChecks=$assignment.fixtures.Count;changedPaths=@($changed)}
}
if($failed){exit 1}
exit 0
