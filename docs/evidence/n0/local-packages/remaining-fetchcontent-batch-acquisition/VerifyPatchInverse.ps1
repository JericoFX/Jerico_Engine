$ErrorActionPreference='Stop'
$base=$PSScriptRoot
$encoding=[Text.Encoding]::Latin1
$results=[Collections.Generic.List[object]]::new()
foreach($name in @('squish-ccr','assimp','tinyusdz_repo')){
    $receipt=Get-Content -LiteralPath (Join-Path $base "$name/private-patch-receipt.json") -Raw|ConvertFrom-Json
    $lines=$encoding.GetString([IO.File]::ReadAllBytes($receipt.patch)).Replace("`r`n","`n").Split("`n")
    $i=0
    while($i -lt $lines.Length){
        if($lines[$i] -notmatch '^diff --git a/(.+) b/(.+)$'){$i++;continue}
        $relative=$Matches[1];if($relative -cne $Matches[2]){throw 'rename not allowed'}
        $private=Join-Path $receipt.privateRoot $relative
        $original=Join-Path $receipt.source $relative
        $src=$encoding.GetString([IO.File]::ReadAllBytes($private)).Split("`n")
        $out=[Collections.Generic.List[string]]::new();$cursor=0;$hunks=0;$i++
        while($i -lt $lines.Length -and -not $lines[$i].StartsWith('diff --git ')){
            if($lines[$i] -notmatch '^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@'){$i++;continue}
            $oldStart=[int]$Matches[1];$oldCount=if($Matches[2]){[int]$Matches[2]}else{1}
            $newStart=[int]$Matches[3];$newCount=if($Matches[4]){[int]$Matches[4]}else{1}
            while($cursor -lt $newStart-1){$out.Add($src[$cursor]);$cursor++}
            if($out.Count -ne $oldStart-1){throw "inverse old position: $relative"}
            $oldSeen=0;$newSeen=0;$i++;$hunks++
            while(($oldSeen -lt $oldCount) -or ($newSeen -lt $newCount)){
                $line=$lines[$i];$op=$line[0];$body=$line.Substring(1)
                if($op -eq ' ' -or $op -eq '+'){
                    if($src[$cursor] -cne $body){throw "inverse context differs: $relative"}
                    $cursor++;$newSeen++
                }
                if($op -eq ' ' -or $op -eq '-'){$out.Add($body);$oldSeen++}
                if($op -notin @(' ','-','+')){throw 'inverse unsupported control'}
                $i++
            }
            if($oldSeen -ne $oldCount -or $newSeen -ne $newCount){throw 'inverse counts differ'}
        }
        while($cursor -lt $src.Length){$out.Add($src[$cursor]);$cursor++}
        [byte[]]$restored=$encoding.GetBytes([string]::Join("`n",$out))
        [byte[]]$pristine=[IO.File]::ReadAllBytes($original)
        if(-not [MemoryExtensions]::SequenceEqual[byte]($restored,$pristine)){throw "inverse raw bytes differ: $relative"}
        $results.Add([pscustomobject]@{name=$name;relative=$relative;hunks=$hunks;rawByteInverseExact=$true;restoredSHA256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($restored)).ToLowerInvariant();originalSHA256=(Get-FileHash -LiteralPath $original).Hash.ToLowerInvariant();originalBytes=$pristine.Length})
    }
}
$output=Join-Path $base 'patch-inverse-byte-check.json'
if(Test-Path -LiteralPath $output){throw 'inverse output exists'}
[ordered]@{status='14/14 PATCH INVERSIONS RAW BYTE EXACT; NATIVE UNVERIFIED';method='Original existing unified diffs reversed in memory only; all source positions/counts/context exact; no source writes or patch tools executed';files=$results.ToArray();fileCount=$results.Count}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $output -Encoding utf8
[pscustomobject]@{fileCount=$results.Count;allRawByteInverseExact=$true;outputSHA256=(Get-FileHash -LiteralPath $output).Hash}|ConvertTo-Json
