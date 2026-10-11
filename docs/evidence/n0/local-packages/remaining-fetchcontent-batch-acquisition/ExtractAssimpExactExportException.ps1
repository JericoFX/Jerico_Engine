$ErrorActionPreference='Stop'
$base=$PSScriptRoot
$approvalPath=Join-Path $base 'assimp/parent-export-ignore-approval.json'
if((Get-FileHash -LiteralPath $approvalPath).Hash -ne '2035122802A0F76FC7B6CD9CBDEBD4E59B9F3AE41043C984643E577D6A371E29'){throw 'specific export approval changed'}
$approved=Get-Content -LiteralPath $approvalPath -Raw|ConvertFrom-Json
$archive='F:/JericoEngine/Dependencies/archives/assimp-6.0.4.tar.gz'
if((Get-FileHash -LiteralPath $archive).Hash -ne '00E4BF0B9D9D8E9B346FA3DD859C3CE5F3438451822D5933C07F65A76FC04745'){throw 'Assimp archive changed'}
$evidence=Join-Path $base 'assimp/extraction-rev2'
if(Test-Path -LiteralPath $evidence){throw 'extraction evidence exists'}
[IO.Directory]::CreateDirectory($evidence)|Out-Null
function AssertLocalDestination([string]$path){
    $full=[IO.Path]::GetFullPath($path)
    if(-not $full.StartsWith('F:\JericoEngine\Dependencies\',[StringComparison]::OrdinalIgnoreCase)){throw "destination outside approved Dependencies: $full"}
    $ancestor=$full
    while($ancestor){if(Test-Path -LiteralPath $ancestor){if((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "symlink/reparse ancestor: $ancestor"}};$ancestor=[IO.Path]::GetDirectoryName($ancestor)}
}
function Inspect([string]$archive,[string]$destination,[string]$treePath,[string]$evidence,[string]$commit){
    $tree=Get-Content -LiteralPath $treePath -Raw | ConvertFrom-Json
    if($tree.truncated){throw 'primary Git tree truncated'}
    $blobs=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
    foreach($row in $tree.tree){if($row.type -eq 'blob'){$blobs.Add($row.path,$row)}}
    $files=[Collections.Generic.Dictionary[string,byte[]]]::new([StringComparer]::OrdinalIgnoreCase)
    $paths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $records=[Collections.Generic.List[object]]::new();$violations=[Collections.Generic.List[object]]::new();$root=$null;$links=0;$matched=0
    $input=[IO.File]::OpenRead($archive);$gzip=[IO.Compression.GZipStream]::new($input,[IO.Compression.CompressionMode]::Decompress);$reader=[System.Formats.Tar.TarReader]::new($gzip)
    try{
        while($null -ne ($entry=$reader.GetNextEntry())){
            $kind=$entry.EntryType.ToString();$name=$entry.Name.TrimEnd('/')
            if($kind -eq 'GlobalExtendedAttributes'){
                if($name -ne 'pax_global_header' -or $records.Count){$violations.Add([pscustomobject]@{path=$name;reason='unexpected PAX record'})}
                $records.Add([pscustomobject]@{path=$name;type=$kind;bytes=0;sha256=$null});continue
            }
            $parts=$name.Split('/');if(-not $root){$root=$parts[0]}
            $safe=$parts[0] -ceq $root -and $name -notmatch '\\|:|(^|/)\.\.?(/|$)' -and -not $name.StartsWith('/')
            foreach($part in $parts){if(-not $part -or $part -match '[. ]$|^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)'){$safe=$false}}
            if(-not $safe){$violations.Add([pscustomobject]@{path=$name;reason='unsafe/traversal/Windows path'})}
            if(-not $paths.Add($name)){$violations.Add([pscustomobject]@{path=$name;reason='duplicate/case collision'})}
            $sha=$null;$blob=$null;$relative=if($name -ne $root){$name.Substring($root.Length+1)}else{''}
            if($kind -in @('RegularFile','V7RegularFile')){
                $memory=[IO.MemoryStream]::new();if($entry.Length -gt 0){$entry.DataStream.CopyTo($memory)};$bytes=$memory.ToArray();$memory.Dispose()
                if($bytes.Length -ne $entry.Length){throw "tar size mismatch: $name"}
                $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
                $prefix=[Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0")
                $blob=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([byte[]]($prefix+$bytes))).ToLowerInvariant()
                if(-not $blobs.ContainsKey($relative) -or $blob -ne $blobs[$relative].sha){$violations.Add([pscustomobject]@{path=$name;reason='primary Git blob absent/mismatched'})}else{$matched++}
                if($safe -and -not $files.ContainsKey($name)){$files.Add($name,$bytes)}
            }elseif($kind -in @('SymbolicLink','HardLink')){
                $links++;$violations.Add([pscustomobject]@{path=$name;type=$kind;target=$entry.LinkName;reason='link forbidden until explicit parent subtree review'})
            }elseif($kind -ne 'Directory'){$violations.Add([pscustomobject]@{path=$name;type=$kind;reason='unsupported tar type'})}
            $records.Add([pscustomobject]@{path=$name;type=$kind;bytes=$entry.Length;sha256=$sha;gitBlobSHA1=$blob})
        }
    }finally{$reader.Dispose();$gzip.Dispose();$input.Dispose()}
    foreach($name in $files.Keys){$parent=[IO.Path]::GetDirectoryName($name.Replace('/', '\'));while($parent){if($files.ContainsKey($parent.Replace('\','/'))){$violations.Add([pscustomobject]@{path=$name;reason='file/directory collision'})};$parent=[IO.Path]::GetDirectoryName($parent)}}
    $present=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach($name in $files.Keys){[void]$present.Add($name.Substring($root.Length+1))}
    $missing=@($blobs.Keys | Where-Object {-not $present.Contains($_)} | Sort-Object)
    $expected=@($approved.exactOmissionPaths | Sort-Object)
    if($matched -ne 3116 -or $blobs.Count -ne 3141 -or $missing.Count -ne 25 -or ([string]::Join("`n",$missing) -cne [string]::Join("`n",$expected))){$violations.Add([pscustomobject]@{reason='specific approved export omission differs';archiveBlobMatches=$matched;primaryBlobs=$blobs.Count;missing=$missing})}
    $receipt=[ordered]@{archive=$archive;commit=$commit;root=$root;members=$records.Count;regularFiles=$files.Count;fileBytes=(($records.bytes|Measure-Object -Sum).Sum);links=$links;gitBlobMatches=$matched;primaryBlobFiles=$blobs.Count;destination=$destination;violations=$violations.ToArray();extracted=$false;membersManifest=$records.ToArray()}
    $receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $evidence 'source-tar-audit.json') -Encoding utf8
    if($violations.Count){throw "tar audit has $($violations.Count) violations; immutable extraction blocked"}
    if(Test-Path -LiteralPath $destination){throw 'immutable snapshot destination exists'}
    AssertLocalDestination $destination
    [IO.Directory]::CreateDirectory($destination) | Out-Null
    $inputs=[Collections.Generic.List[object]]::new()
    foreach($name in $files.Keys){
        $relative=$name.Substring($root.Length+1);$path=Join-Path $destination $relative
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
        $stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None);try{$stream.Write($files[$name])}finally{$stream.Dispose()}
        $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        if($hash -ne [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($files[$name])).ToLowerInvariant()){throw "extraction SHA mismatch: $path"}
        $inputs.Add([pscustomobject]@{path=$path;bytes=$files[$name].Length;sha256=$hash})
    }
    [ordered]@{sourceRoot=$destination;commit=$commit;files=$files.Count;gitBlobMatches=$matched;extractionHashMatches=$inputs.Count;archive=$archive;archiveSHA256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant();inputs=$inputs.ToArray()} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $evidence 'source-tree-manifest.json') -Encoding utf8
    return [pscustomobject]@{files=$files.Count;fileBytes=$receipt.fileBytes;gitBlobMatches=$matched;links=0;extractionHashMatches=$inputs.Count}
}
$tree='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/remaining-fetchcontent-batch-audit/assimp-recursive-tree.json'
$result=Inspect $archive 'F:/JericoEngine/Dependencies/assimp-6.0.4' $tree $evidence 'e0b52347c6e52de2827ec957a9ebf00ce3c54f79'
[ordered]@{status='ORIGINAL ARCHIVE EXTRACTED; NATIVE UNVERIFIED';files=$result.files;fileBytes=$result.fileBytes;links=$result.links;gitBlobMatches=$result.gitBlobMatches;extractionHashMatches=$result.extractionHashMatches;exportOmissionCount=25;authoritySHA256=(Get-FileHash -LiteralPath $approvalPath).Hash;archiveSHA256=(Get-FileHash -LiteralPath $archive).Hash;immutableSnapshot='F:/JericoEngine/Dependencies/assimp-6.0.4';initialFailedAuditPreserved=$true;syntheticFiles=0}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $evidence 'source-extraction-summary.json') -Encoding utf8
$result|ConvertTo-Json -Compress