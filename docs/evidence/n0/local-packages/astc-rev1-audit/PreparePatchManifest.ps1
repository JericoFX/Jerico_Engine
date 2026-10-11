#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=$PSScriptRoot
$source='F:/JericoEngine/Dependencies/astc-encoder-f32d8d4dd9e06f8986cd5e10f12e199fc023f150'
$provider='F:/JericoEngine/JericoEngine/cmake/Tools/LocalPackages/ASTCEncoder'
$patch=Join-Path $root 'recipe904-astc-encoder-3.2-rev1.patch'
if((Get-FileHash -LiteralPath $patch).Hash -ne '88E0389289A28DB8D937842D890A7EC4953D2295C263997236B05BE4B3B5BD00'){throw 'Historical original patch differs'}
$encoding=[Text.UTF8Encoding]::new($false)
$lines=[IO.File]::ReadAllLines($patch)
$files=[Collections.Generic.List[object]]::new()
for($i=0;$i -lt $lines.Count;){
    if($lines[$i] -notmatch '^diff --git a/(.+) b/(.+)$'){$i++;continue}
    $path=$Matches[1];if($path -cne $Matches[2] -or $path -notin @('Source/CMakeLists.txt','Source/cmake_core.cmake')){throw 'Unexpected patch path'}
    $i++;$hunks=[Collections.Generic.List[object]]::new()
    while($i -lt $lines.Count -and $lines[$i] -notmatch '^diff --git'){
        if($lines[$i] -notmatch '^@@ -(\d+),(\d+) \+(\d+),(\d+) @@'){$i++;continue}
        $oldCount=[int]$Matches[2];$newCount=[int]$Matches[4];$i++
        $old=[Collections.Generic.List[string]]::new();$new=[Collections.Generic.List[string]]::new()
        while($i -lt $lines.Count -and $lines[$i] -notmatch '^@@|^diff --git'){
            if($lines[$i].StartsWith(' ')){$old.Add($lines[$i].Substring(1));$new.Add($lines[$i].Substring(1))}
            elseif($lines[$i].StartsWith('-')){$old.Add($lines[$i].Substring(1))}
            elseif($lines[$i].StartsWith('+')){$new.Add($lines[$i].Substring(1))}
            else{throw 'Unexpected hunk marker'}
            $i++
        }
        if($old.Count -ne $oldCount -or $new.Count -ne $newCount){throw 'Exact hunk counts differ'}
        $hunks.Add(@{before=($old -join "`n")+"`n";after=($new -join "`n")+"`n"})
    }
    $original=[IO.File]::ReadAllText((Join-Path $source $path));$updated=$original
    foreach($hunk in $hunks){if([regex]::Matches($updated,[regex]::Escape($hunk.before)).Count -ne 1){throw 'Pristine hunk missing or ambiguous'};$updated=$updated.Replace($hunk.before,$hunk.after)}
    $inverse=$updated
    for($index=$hunks.Count-1;$index -ge 0;$index--){if([regex]::Matches($inverse,[regex]::Escape($hunks[$index].after)).Count -ne 1){throw 'Patched hunk ambiguous'};$inverse=$inverse.Replace($hunks[$index].after,$hunks[$index].before)}
    if($inverse -cne $original){throw 'Hunk inverse not exact'}
    $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($encoding.GetBytes($updated)))
    $files.Add(@{path=$path;beforeSHA=(Get-FileHash -LiteralPath (Join-Path $source $path)).Hash;afterSHA=$sha;hunks=@($hunks);exactInverse=$true})
}
if($files.Count -ne 2 -or @($files|ForEach-Object{$_.hunks.Count}|Measure-Object -Sum).Sum -ne 4){throw 'Historical patch scope differs'}
$manifest=@{sourceCommit='f32d8d4dd9e06f8986cd5e10f12e199fc023f150';recipeCommit='90480ca41d15e1b20a28f90e2f996fd0e5a3a68c';originalPackageVersion='3.2-rev2';patchSHA='88e0389289a28db8d937842d890a7ec4953d2295c263997236b05be4b3b5bd00';files=@($files)}
$manifestPath=Join-Path $provider 'PatchHunks.json';$patchPath=Join-Path $provider 'astc-encoder-3.2-rev1.patch'
if(Test-Path -LiteralPath $manifestPath){throw 'Provider manifest exists'}
if(Test-Path -LiteralPath $patchPath){throw 'Provider patch exists'}
[IO.File]::WriteAllText($manifestPath,($manifest|ConvertTo-Json -Depth 16)+"`n",$encoding)
[IO.File]::Copy($patch,$patchPath,$false)
$proof=@{status='STATIC ORIGINAL HUNKS / BYTE-EXACT INVERSE; NO NATIVE';files=@($files|ForEach-Object{@{path=$_.path;beforeSHA=$_.beforeSHA;afterSHA=$_.afterSHA;hunkCount=$_.hunks.Count;inverseExact=$true}});manifestSHA=(Get-FileHash -LiteralPath $manifestPath).Hash;patchSHA=(Get-FileHash -LiteralPath $patchPath).Hash;sourceModified=$false;nativeExecuted=$false}
$proofPath=Join-Path $root 'patch-preparation-proof.json';if(Test-Path -LiteralPath $proofPath){throw 'Patch proof exists'}
[IO.File]::WriteAllText($proofPath,($proof|ConvertTo-Json -Depth 12)+"`n",$encoding)
$proof|ConvertTo-Json -Depth 12
