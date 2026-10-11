# Luna dispatch only; actual CMake guards, never a mock or positive producer.
#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$here=[IO.Path]::GetFullPath($PSScriptRoot)
if($here -ne 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\astc-rev1-guard-preparation'){throw 'Assigned output directory differs'}
$assignment=Get-Content "$here/negative-assignment.json" -Raw | ConvertFrom-Json
$freeze=Get-Content "$here/source-freeze-astc-guards.json" -Raw | ConvertFrom-Json
if($assignment.cases.Count -ne 10 -or @($assignment.cases | Where-Object scope -eq PRODUCER).Count -ne 9 -or @($assignment.cases | Where-Object scope -eq PATCH-HELPER).Count -ne 1){throw 'Guard scope/count differs'}
function Hash([string]$path){$stream=[IO.File]::OpenRead($path);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}}
function Json([string]$path,$data){$data | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath $path -Encoding utf8NoBOM}
if((Hash $assignment.sourceFreeze.path) -ne $assignment.sourceFreeze.sha256){throw 'Consumer source freeze changed'}
$old=Get-Content $assignment.sourceFreeze.path -Raw | ConvertFrom-Json
if($old.Count -ne 712){throw 'Consumer input count changed'}
function Frozen {
    $changes=@()
    foreach($file in $freeze){if(-not(Test-Path -LiteralPath $file.path -PathType Leaf)){$changes+=$file.path;continue};if((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){$changes+=$file.path}}
    foreach($tree in $assignment.snapshotMemberships){$prefix=[IO.Path]::GetFullPath($tree.path)+'\';$expected=@($old | Where-Object {[IO.Path]::GetFullPath($_.path).StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)} | ForEach-Object {[IO.Path]::GetFullPath($_.path)} | Sort-Object);$actual=@(Get-ChildItem -LiteralPath $tree.path -File -Recurse -Force | ForEach-Object FullName | Sort-Object);if($expected.Count -ne $tree.count -or $actual.Count -ne $tree.count -or ($expected -join "`n") -cne ($actual -join "`n")){$changes+="Snapshot membership: $($tree.path)"}}
    return ,$changes
}
if((Frozen).Count){throw 'Preflight input/snapshot delta'}
$attempt=$assignment.freshAttempt
if(Test-Path -LiteralPath $attempt){throw 'Fresh attempt required; retain previous evidence'}
foreach($path in $assignment.pathsMustRemainAbsent){if(Test-Path -LiteralPath $path){throw "Output sentinel exists: $path"}}
New-Item -ItemType Directory -Path "$attempt/fixtures" | Out-Null
Json "$attempt/preflight.json" @{sourceCount=712;guardFreezeCount=$freeze.Count;changes=@()}
$encoding=[Text.UTF8Encoding]::new($false)
$p=$assignment.fixturePaths
New-Item -ItemType Directory -Path $p.existing,$p.target,"$($p.patchWork)/Source" | Out-Null
[IO.File]::WriteAllText("$($p.existing)/retain.txt",'JE private existing-output sentinel. Preserve bytes.',$encoding)
[IO.File]::WriteAllBytes($p.tiny,[byte[]]@(0xA5))
[IO.File]::Copy($assignment.fixtureSources.cmake,"$($p.patchWork)/Source/CMakeLists.txt",$false)
[IO.File]::Copy($assignment.fixtureSources.core,"$($p.patchWork)/Source/cmake_core.cmake",$false)
$stream=[IO.File]::Open("$($p.patchWork)/Source/CMakeLists.txt",[IO.FileMode]::Append)
try{$bytes=$encoding.GetBytes("`n# JE negative private fixture only.`n");$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
# The junction exposes only this attempt's empty directory, never immutable source/payload.
New-Item -ItemType Junction -Path $p.junction -Value $p.target | Out-Null
$fixtureFiles=@($p.tiny,"$($p.existing)/retain.txt","$($p.patchWork)/Source/CMakeLists.txt","$($p.patchWork)/Source/cmake_core.cmake")
$fixtures=@(foreach($path in $fixtureFiles){[ordered]@{path=[IO.Path]::GetFullPath($path);bytes=(Get-Item -LiteralPath $path).Length;sha256=(Hash $path)}})
Json "$attempt/fixture-before.json" $fixtures
function Fixtures {
    $changes=@()
    foreach($file in $fixtures){if(-not(Test-Path -LiteralPath $file.path -PathType Leaf)){$changes+=$file.path;continue};if((Get-Item -LiteralPath $file.path).Length -ne $file.bytes -or (Hash $file.path) -ne $file.sha256){$changes+=$file.path}}
    $actual=@(Get-ChildItem -LiteralPath "$attempt/fixtures" -File -Recurse -Force | ForEach-Object FullName | Sort-Object)
    $expected=@($fixtures.path | Sort-Object)
    if(($actual -join "`n") -cne ($expected -join "`n")){$changes+='Fixture membership changed'}
    $link=Get-Item -LiteralPath $p.junction
    if(-not($link.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $link.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath([string]$link.Target) -ne [IO.Path]::GetFullPath($p.target)){$changes+='Private junction identity changed'}
    if(@(Get-ChildItem -LiteralPath $p.target -Force).Count){$changes+='Private junction target changed'}
    if(Test-Path -LiteralPath $p.missing){$changes+='Secondary missing archive unexpectedly created'}
    return ,$changes
}
$results=[Collections.Generic.List[object]]::new();$failed=$false
try{
    foreach($case in $assignment.cases){
        if((Fixtures).Count){throw 'Fixture delta before invocation'}
        $prefix="$attempt/$($case.label)"
        Json "$prefix-argv.json" @{executable=$case.executable;argv=@($case.argv);scope=$case.scope;secondaryMissingPath=$case.secondaryMissingPath}
        $info=[Diagnostics.ProcessStartInfo]::new();$info.FileName=$case.executable;$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
        foreach($arg in $case.argv){$info.ArgumentList.Add([string]$arg)}
        $process=[Diagnostics.Process]::new();$process.StartInfo=$info;$clock=[Diagnostics.Stopwatch]::StartNew()
        try{
            if(-not $process.Start()){throw 'Guard CMake process failed to start'}
            $stdoutTask=$process.StandardOutput.ReadToEndAsync();$stderrTask=$process.StandardError.ReadToEndAsync();$process.WaitForExit();$nativeExit=$process.ExitCode;$clock.Stop()
            $stdout=$stdoutTask.GetAwaiter().GetResult();$stderr=$stderrTask.GetAwaiter().GetResult()
            [IO.File]::WriteAllText("$prefix-stdout.log",$stdout,$encoding);[IO.File]::WriteAllText("$prefix-stderr.log",$stderr,$encoding)
            $diagnostic=($stdout+"`n"+$stderr) -replace '\s+',' ';$expected=$diagnostic -match [regex]::Escape($case.expectedError)
            $absent=$true;foreach($path in $case.mustRemainAbsent){if(Test-Path -LiteralPath $path){$absent=$false}}
            $fixtureChanges=Fixtures;$pass=$nativeExit -ne 0 -and $expected -and $absent -and $fixtureChanges.Count -eq 0
            $result=[ordered]@{label=$case.label;scope=$case.scope;nativeExit=$nativeExit;seconds=$clock.Elapsed.TotalSeconds;expectedError=$case.expectedError;observedExpectedError=$expected;mustRemainAbsent=@($case.mustRemainAbsent);outputAbsent=$absent;fixtureChanges=@($fixtureChanges);status=if($pass){'PASS REJECTION; PARENT ACCEPTANCE PENDING'}else{'FAIL'}}
            Json "$prefix-result.json" $result;$results.Add($result)
            if(-not $pass){$failed=$true;break}
        }finally{$process.Dispose()}
    }
}catch{$failed=$true;Json "$attempt/driver-error.json" @{message=$_.Exception.Message;stack=$_.ScriptStackTrace}}
finally{
    $sourceChanges=Frozen;$fixtureChanges=Fixtures
    $absent=$true;foreach($path in $assignment.pathsMustRemainAbsent){if(Test-Path -LiteralPath $path){$absent=$false}}
    if($sourceChanges.Count -or $fixtureChanges.Count -or $results.Count -ne 10 -or -not $absent){$failed=$true}
    Json "$attempt/fixture-after.json" @(foreach($file in $fixtures){if(Test-Path -LiteralPath $file.path -PathType Leaf){@{path=$file.path;bytes=(Get-Item -LiteralPath $file.path).Length;sha256=(Hash $file.path)}}})
    Json "$attempt/postflight.json" @{sourceInputCount=712;guardFreezeCount=$freeze.Count;sourceChanges=@($sourceChanges);fixtureChanges=@($fixtureChanges);snapshotMemberships=$assignment.snapshotMemberships;allOutputSentinelsAbsent=$absent}
    Json "$attempt/summary.json" @{exitCode=if($failed){1}else{0};results=@($results);expectedCases=10;producerCases=9;patchHelperCases=1;acceptance=$false;producerMustNotSucceed=$true}
}
if($failed){exit 1};exit 0
