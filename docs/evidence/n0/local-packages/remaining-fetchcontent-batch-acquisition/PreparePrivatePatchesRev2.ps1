param([ValidateSet('Squish','Assimp','Tiny')][string]$Kind)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$base='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/remaining-fetchcontent-batch-acquisition'
function Hash([string]$p){ (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() }
function Manifest([string]$root){
    @(Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName | ForEach-Object {
        if($_.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'source link'}
        [pscustomobject]@{relative=[IO.Path]::GetRelativePath($root,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Hash $_.FullName)}
    })
}
function ApplyExact([string]$root,[string]$patch){
    $encoding=[Text.Encoding]::Latin1
    $lines=$encoding.GetString([IO.File]::ReadAllBytes($patch)).Replace("`r`n","`n").Split("`n")
    $changes=[Collections.Generic.List[object]]::new();$i=0
    while($i -lt $lines.Length){
        if($lines[$i] -notmatch '^diff --git a/(.+) b/(.+)$'){$i++;continue}
        $relative=$Matches[1];if($relative -cne $Matches[2] -or $relative -match '(^|/)\.\.?(/|$)|\\|:|^/'){throw 'unsupported patch path'}
        $path=Join-Path $root $relative
        $beforeHash=Hash $path
        $text=$encoding.GetString([IO.File]::ReadAllBytes($path))
        if($text.Contains("`r")){throw "unsupported source EOL: $relative"}
        $src=$text.Split("`n");$out=[Collections.Generic.List[string]]::new();$cursor=0;$hunks=0
        $i++
        while($i -lt $lines.Length -and -not $lines[$i].StartsWith('diff --git ')){
            if($lines[$i] -notmatch '^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@'){$i++;continue}
            $oldStart=[int]$Matches[1];$oldCount=if($Matches[2]){[int]$Matches[2]}else{1}
            $newStart=[int]$Matches[3];$newCount=if($Matches[4]){[int]$Matches[4]}else{1}
            $at=$oldStart-1;if($at -lt $cursor){throw 'overlapping hunk'}
            while($cursor -lt $at){$out.Add($src[$cursor]);$cursor++}
            if($out.Count -ne $newStart-1){throw "new hunk position mismatch: $relative"}
            $oldSeen=0;$newSeen=0;$i++;$hunks++
            while($i -lt $lines.Length -and $oldSeen -lt $oldCount -or $i -lt $lines.Length -and $newSeen -lt $newCount){
                $line=$lines[$i];if(-not $line.Length){throw "empty patch control line: $relative"}
                $op=$line[0];$body=$line.Substring(1)
                if($op -eq ' ' -or $op -eq '-'){
                    if($cursor -ge $src.Length -or $src[$cursor] -cne $body){throw "exact hunk differs: $relative source line $($cursor+1)"}
                    $cursor++;$oldSeen++
                }
                if($op -eq ' ' -or $op -eq '+'){$out.Add($body);$newSeen++}
                if($op -notin @(' ','-','+')){throw 'unsupported patch control'}
                $i++
            }
            if($oldSeen -ne $oldCount -or $newSeen -ne $newCount){throw 'hunk count differs'}
        }
        if(-not $hunks){throw 'patch file without hunks'}
        while($cursor -lt $src.Length){$out.Add($src[$cursor]);$cursor++}
        $result=$encoding.GetBytes([string]::Join("`n",$out))
        [IO.File]::WriteAllBytes($path,$result)
        $afterHash=Hash $path
        if($afterHash -ne [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($result)).ToLowerInvariant()){throw 'written bytes mismatch'}
        $changes.Add([pscustomobject]@{relative=$relative;hunks=$hunks;beforeSHA256=$beforeHash;afterSHA256=$afterHash;afterBytes=$result.Length})
    }
    if(-not $changes.Count){throw 'empty patch'}
    return $changes.ToArray()
}
$settings=switch($Kind){
 'Squish' { @('F:/JericoEngine/Dependencies/squish-ccr-deb557d2fa647b191b37a2d8682df54ec8a7cfba','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/squish-ccr-batch-root','F:/JericoEngine/JericoEngine/Code/3rdParty/squish-ccr/squish-ccr.patch','squish-ccr') }
 'Assimp' { @('F:/JericoEngine/Dependencies/assimp-6.0.4','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/assimp-6.0.4-batch-root-rev2','F:/JericoEngine/JericoEngine/Code/Tools/SceneAPI/SDKWrapper/3rdParty/tinyusd-include.patch','assimp') }
 'Tiny' { @('F:/JericoEngine/Dependencies/tinyusdz-6050eef932f7d2788656d63297aa488fb0961ed1','F:/JericoEngine/JericoEngine/build/n0-validation/original-engine/_deps/tinyusdz_repo-src','F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/source-work/assimp-6.0.4-batch-root-rev2/contrib/tinyusdz/patches/tinyusdz.patch','tinyusdz_repo') }
}
$source=$settings[0];$destination=$settings[1];$patch=$settings[2];$name=$settings[3]
foreach($p in @($source,$destination,$patch)){
    if(-not [IO.Path]::IsPathFullyQualified($p) -or $p -match '://|^//'){throw 'nonlocal path'}
    $parent=[IO.Path]::GetFullPath($p);while($parent){if(Test-Path -LiteralPath $parent){if((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'ancestor link'}};$parent=[IO.Path]::GetDirectoryName($parent)}
}
if(Test-Path -LiteralPath $destination){throw 'private output exists'}
if(-not [IO.Path]::GetFullPath($destination).StartsWith('F:\JericoEngine\JericoEngine\build\',[StringComparison]::OrdinalIgnoreCase)){throw 'private output outside build'}
if(-not (Test-Path -LiteralPath $source -PathType Container)){throw 'immutable snapshot missing'}
$sourceManifest=if($Kind -eq 'Assimp'){Join-Path $base 'assimp/extraction-rev2/source-tree-manifest.json'}elseif($Kind -eq 'Tiny'){Join-Path $base 'tinyusdz_repo/source-tree-manifest.json'}else{Join-Path $base 'squish-ccr/source-tree-manifest.json'}
if(-not (Test-Path -LiteralPath $sourceManifest -PathType Leaf)){throw 'immutable extraction not complete'}
$provisioned=Get-Content -LiteralPath $sourceManifest -Raw|ConvertFrom-Json
foreach($row in $provisioned.inputs){if((Hash $row.path) -ne $row.sha256 -or (Get-Item -LiteralPath $row.path).Length -ne $row.bytes){throw 'snapshot differs from completed extraction'}}
$before=Manifest $source
if($before.Count -ne $provisioned.files){throw 'snapshot file count differs from completed extraction'}
$receiptPath=Join-Path $base "$name/private-patch-receipt.json"
if(Test-Path -LiteralPath $receiptPath){throw 'patch receipt exists'}
[IO.Directory]::CreateDirectory($destination)|Out-Null
foreach($row in $before){$target=Join-Path $destination $row.relative;[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target))|Out-Null;[IO.File]::Copy((Join-Path $source $row.relative),$target,$false);if((Hash $target) -ne $row.sha256){throw 'private copy hash mismatch'}}
$changes=@(ApplyExact $destination $patch)
$after=Manifest $destination
$sourceAfter=Manifest $source
if(($before|ConvertTo-Json -Depth 5 -Compress) -cne ($sourceAfter|ConvertTo-Json -Depth 5 -Compress)){throw 'immutable source changed'}
$changedMap=@{};foreach($c in $changes){$changedMap[$c.relative]=$c}
for($j=0;$j -lt $before.Count;$j++){if($before[$j].relative -cne $after[$j].relative){throw 'private paths changed'};if(-not $changedMap.ContainsKey($before[$j].relative) -and $before[$j].sha256 -ne $after[$j].sha256){throw 'unpatched private file changed'}}
[ordered]@{status='EXACT PATCH PREPARED; NATIVE UNVERIFIED';source=$source;privateRoot=$destination;patch=$patch;patchSHA256=(Hash $patch);fileCount=$after.Count;changedFiles=$changes;immutableBefore=$before;immutableAfter=$sourceAfter;privateBaseline=$after;method='Latin1 byte-preserving unified hunks with exact source/new positions, context and count; no fuzz/Git/Python; original LF/EOL preserved'}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $receiptPath -Encoding utf8
[pscustomobject]@{name=$name;fileCount=$after.Count;changedFiles=$changes.Count;privateRoot=$destination;receiptSHA256=(Hash $receiptPath)}|ConvertTo-Json -Compress
