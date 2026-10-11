$ErrorActionPreference='Stop'
$out=$PSScriptRoot
$audit=Join-Path (Split-Path -Parent $out) 'remaining-fetchcontent-batch-audit'
$planPath=Join-Path $audit 'batch-acquisition-plan.json'
$approvalPath=Join-Path $audit 'parent-acquisition-approval.json'
if((Get-FileHash -LiteralPath $planPath -Algorithm SHA256).Hash -ne '97722EC07C61B20CABE06119B0C4ABEB2F67C11A19B7B7E34031EE70009CD7DF' -or (Get-FileHash -LiteralPath $approvalPath -Algorithm SHA256).Hash -ne 'E79E6ECB3CF3F218E867740BD021724C6E00C7F3865CBDC68EC1284B2CB4867E'){throw 'parent source acquisition authority changed'}
$plan=Get-Content -LiteralPath $planPath -Raw | ConvertFrom-Json
$ids=@{xxhash='xxhash-0.8.3';D3D12MemoryAllocator='D3D12MemoryAllocator-3.2.0';meshoptimizer='meshoptimizer-1.2';'squish-ccr'='squish-ccr-deb557d2fa647b191b37a2d8682df54ec8a7cfba';sse2neon='sse2neon-1.9.1';mikktspace='mikktspace-3e895b49d05ea07e4c2133156cfa94369e19e409';ogg='ogg-1.3.6';vorbis='vorbis-1.3.7';miniaudio='miniaudio-0.11.22';'v-hacd'='v-hacd-4.1.0';RecastNavigation='RecastNavigation-1.6.0';assimp='assimp-6.0.4';tinyusdz_repo='tinyusdz-6050eef932f7d2788656d63297aa488fb0961ed1'}
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.Timeout=[TimeSpan]::FromMinutes(3);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-Approved-Source-Acquisition/1.0')
$results=[Collections.Generic.List[object]]::new()
function AssertLocalDestination([string]$path){
    $full=[IO.Path]::GetFullPath($path)
    if(-not $full.StartsWith('F:\JericoEngine\Dependencies\',[StringComparison]::OrdinalIgnoreCase)){throw "destination outside approved Dependencies: $full"}
    $ancestor=$full
    while($ancestor){if(Test-Path -LiteralPath $ancestor){if((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "symlink/reparse ancestor: $ancestor"}};$ancestor=[IO.Path]::GetDirectoryName($ancestor)}
}
function HTTP([string]$method,[string]$url){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -notin @('github.com','codeload.github.com','api.github.com') -or $uri.UserInfo -or $uri.Fragment -or ($uri.Query -and ($uri.Host -ne 'api.github.com' -or $uri.Query -ne '?recursive=1'))){throw "unapproved destination: $url"}
    $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::new($method),$uri)
    $response=$client.SendAsync($request,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
    $request.Dispose()
    return $response
}
function Head([string]$url){
    $response=HTTP 'HEAD' $url
    try{return [pscustomobject]@{url=$url;status=[int]$response.StatusCode;bytes=$response.Content.Headers.ContentLength;etag=[string]$response.Headers.ETag;location=[string]$response.Headers.Location;authorization=$false;automaticRedirects=$false;cookies=$false}}finally{$response.Dispose()}
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
    if($matched -ne $blobs.Count){$violations.Add([pscustomobject]@{reason='complete regular archive/Git tree count differs';archiveBlobMatches=$matched;primaryBlobs=$blobs.Count})}
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
try{
    foreach($item in $plan.items){
        $name=$item.name;$evidence=Join-Path $out $name
        if(Test-Path -LiteralPath $evidence){throw "source evidence directory exists: $name"}
        New-Item -ItemType Directory -Path $evidence | Out-Null
        $archive="F:/JericoEngine/Dependencies/archives/$($ids[$name]).tar.gz";$partial="$archive.download";$destination="F:/JericoEngine/Dependencies/$($ids[$name])"
        try{
            foreach($path in @($archive,$partial,$destination)){AssertLocalDestination $path;if(Test-Path -LiteralPath $path){throw "new output exists: $path"}}
            $metadata=if($name -eq 'assimp'){'assimp-destination-pin-audit.json'}elseif($name -eq 'tinyusdz_repo'){'tinyusdz-destination-pin-audit.json'}else{'destination-pin-audit.json'}
            $old=@(Get-Content -LiteralPath (Join-Path $audit $metadata) -Raw | ConvertFrom-Json | Where-Object {$_.name -eq $name})
            if($old.Count -ne 1 -or -not $old[0].commitCorroborated -or -not $old[0].tagPinMatched){throw 'primary commit identity absent'}
            $treePath=Join-Path $audit "$name-recursive-tree.json"
            if(-not(Test-Path -LiteralPath $treePath)){
                $treeURL="https://api.github.com/repos/$($old[0].repository)/git/trees/$($old[0].rootTree)?recursive=1"
                $treeResponse=HTTP 'GET' $treeURL
                try{
                    $treeReceipt=[pscustomobject]@{url=$treeURL;status=[int]$treeResponse.StatusCode;redirect=[string]$treeResponse.Headers.Location;authorization=$false;automaticRedirects=$false}
                    $treeReceipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidence 'primary-tree-http.json') -Encoding utf8
                    if($treeReceipt.status -ne 200 -or $treeReceipt.redirect){throw 'primary recursive tree unavailable'}
                    [IO.File]::WriteAllBytes((Join-Path $evidence 'source-primary-tree.json'),$treeResponse.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult())
                    $treePath=Join-Path $evidence 'source-primary-tree.json'
                }finally{$treeResponse.Dispose()}
            }
            $tree=Get-Content -LiteralPath $treePath -Raw | ConvertFrom-Json
            if($tree.truncated -or $tree.sha -ne $old[0].rootTree){throw 'primary recursive tree identity mismatch'}
            $heads=[Collections.Generic.List[object]]::new()
            if($name -eq 'tinyusdz_repo'){
                $renameURL='https://github.com/lighttransport/tinyusdz/archive/6050eef932f7d2788656d63297aa488fb0961ed1.tar.gz';$rename=Head $renameURL;$heads.Add($rename)
                if($rename.status -ne 301 -or $rename.location -cne $item.archiveCandidateURL){throw 'official repository rename changed'}
            }
            $first=Head $item.archiveCandidateURL;$heads.Add($first)
            if($first.status -notin @(301,302,303,307,308) -or $first.location -cne $item.directArchiveURL){throw 'original official source redirect changed'}
            $last=Head $item.directArchiveURL;$heads.Add($last)
            if($last.status -ne 200 -or $last.location){throw 'direct official archive HEAD not200'}
            $heads.ToArray() | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'source-head.json') -Encoding utf8
            $response=HTTP 'GET' $item.directArchiveURL
            try{
                if([int]$response.StatusCode -ne 200 -or $response.Headers.Location){throw 'direct archive GET not200/no-redirect'}
                $stream=[IO.File]::Open($partial,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
                try{$response.Content.CopyToAsync($stream).GetAwaiter().GetResult()}finally{$stream.Dispose()}
                $bytes=(Get-Item -LiteralPath $partial).Length;$hash=(Get-FileHash -LiteralPath $partial -Algorithm SHA256).Hash.ToLowerInvariant()
                $receipt=[ordered]@{name=$name;URL=$item.directArchiveURL;headStatus=200;getStatus=200;bytes=$bytes;sha256=$hash;expectedSHA256=$item.archiveSHA256;measuredSHAApproved=($name -eq 'tinyusdz_repo');automaticRedirects=$false;authentication=$false;archive=$archive;commit=$item.versionPin;matchesDeclaredPin=($hash -eq $item.archiveSHA256)}
                $receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'source-acquisition.json') -Encoding utf8
                if($item.archiveSHA256 -and $hash -ne $item.archiveSHA256){throw 'archive original SHA mismatch; partial retained'}
                if($last.bytes -and $bytes -ne $last.bytes){throw 'archive HEAD/content byte mismatch'}
                AssertLocalDestination $partial;AssertLocalDestination $archive
                Move-Item -LiteralPath $partial -Destination $archive
            }finally{$response.Dispose()}
            $inspected=Inspect $archive $destination $treePath $evidence $item.versionPin
            $result=[pscustomobject]@{name=$name;status='SOURCE SNAPSHOT READY; NATIVE UNVERIFIED';archive=$archive;archiveBytes=$bytes;archiveSHA256=$hash;snapshot=$destination;files=$inspected.files;fileBytes=$inspected.fileBytes;gitBlobMatches=$inspected.gitBlobMatches;extractionHashMatches=$inspected.extractionHashMatches;links=0}
        }catch{$result=[pscustomobject]@{name=$name;status='BLOCKED SOURCE AUDIT';archive=$archive;snapshot=$destination;reason=$_.Exception.Message};$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'source-blocker.json') -Encoding utf8}
        $results.Add($result)
        $results.ToArray() | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out 'acquisition-summary.json') -Encoding utf8
        $result | ConvertTo-Json -Compress
    }
}finally{$client.Dispose();$handler.Dispose()}
