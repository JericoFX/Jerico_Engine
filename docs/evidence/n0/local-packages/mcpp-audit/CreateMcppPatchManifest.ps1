$ErrorActionPreference = 'Stop'
$source = 'F:/JericoEngine/Dependencies/mcpp-2.7.2'
$patchPath = Join-Path $PSScriptRoot 'mcpp_2.7.2_az.2-original.patch'
$patch = [IO.File]::ReadAllText($patchPath)
if ($patch.Contains("`r")) { throw 'Patch must retain original LF bytes' }
$encoding = [Text.UTF8Encoding]::new($false,$true)
$diffs = [regex]::Matches($patch,'(?ms)^diff --git a/(\S+) b/\1\n.*?(?=^diff --git |\z)')
$records = @()
foreach ($diff in $diffs) {
    $path = $diff.Groups[1].Value
    $bytes = [IO.File]::ReadAllBytes((Join-Path $source $path))
    $text = $encoding.GetString($bytes)
    if ($text.Contains("`r")) { throw "Non-LF source $path" }
    $preHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLower()
    $originalGitBlob = [Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($encoding.GetBytes("blob $($bytes.Length)`0") + $bytes)).ToLower()
    $indexes = [regex]::Match($diff.Value,'index ([0-9a-f]+)\.\.([0-9a-f]+) ')
    if (-not $originalGitBlob.StartsWith($indexes.Groups[1].Value)) { throw "Original Git blob mismatch $path" }
    $lines = $text.Split("`n")
    $hunks = [regex]::Matches($diff.Value,'(?ms)^@@ -(\d+),(\d+) \+(\d+),(\d+) @@[^\n]*\n(.*?)(?=^@@ |\z)')
    $manifestHunks = @()
    $working = $text
    for ($n=$hunks.Count-1; $n -ge 0; $n--) {
        $hunk=$hunks[$n]
        $old=''; $new=''
        $body=$hunk.Groups[5].Value
        foreach ($line in $body.TrimEnd("`n").Split("`n")) {
            if ($line.Length -lt 1 -or $line[0] -notin @(' ','-','+')) { throw "Invalid hunk line $path" }
            if ($line[0] -ne '+') { $old += $line.Substring(1)+"`n" }
            if ($line[0] -ne '-') { $new += $line.Substring(1)+"`n" }
        }
        $oldLine=[int]$hunk.Groups[1].Value-1
        $characterOffset=0
        for($l=0; $l -lt $oldLine; $l++) { $characterOffset += $lines[$l].Length+1 }
        if ($text.Substring($characterOffset,$old.Length) -cne $old) { throw "Exact source context mismatch $path hunk$n" }
        $working=$working.Substring(0,$characterOffset)+$new+$working.Substring($characterOffset+$old.Length)
        $manifestHunks=@([ordered]@{sourceOffset=$encoding.GetByteCount($text.Substring(0,$characterOffset));oldBytes=$encoding.GetByteCount($old);patchOffset=$encoding.GetByteCount($patch.Substring(0,$diff.Index+$hunk.Groups[5].Index));patchBytes=$encoding.GetByteCount($body)}) + $manifestHunks
    }
    $postBytes=$encoding.GetBytes($working)
    $patchedGitBlob=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($encoding.GetBytes("blob $($postBytes.Length)`0")+$postBytes)).ToLower()
    if (-not $patchedGitBlob.StartsWith($indexes.Groups[2].Value)) { throw "Patched Git blob mismatch $path" }
    $records += [ordered]@{path=$path;originalSHA256=$preHash;patchedSHA256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($postBytes)).ToLower();originalGitBlob=$originalGitBlob;patchedGitBlob=$patchedGitBlob;hunks=$manifestHunks}
}
$result=[ordered]@{recipeCommit='ce8be74060cf231c11d464edb0862234dea7782d';patchSHA256='8797760af297be90586f03ae69c3ffaeb89591fa69199c4242e05f4df6efdc6b';sourceArchiveSHA256='3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864';files=$records}
$output=Join-Path $PSScriptRoot 'McppPatchManifest.json'
$result|ConvertTo-Json -Depth 10|Set-Content -LiteralPath $output -Encoding utf8
[ordered]@{files=$records.Count;hunks=($records.hunks|Measure-Object).Count;originalAndPatchedGitBlobPrefixesVerified=$true;manifest=$output}|ConvertTo-Json
