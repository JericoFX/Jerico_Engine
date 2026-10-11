$ErrorActionPreference='Stop'
$out=$PSScriptRoot
$rows=@(Get-Content -LiteralPath (Join-Path $out 'destination-pin-audit.json') -Raw | ConvertFrom-Json)+@(Get-Content -LiteralPath (Join-Path $out 'assimp-destination-pin-audit.json') -Raw | ConvertFrom-Json)
$handler=[Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler)
$client.Timeout=[TimeSpan]::FromSeconds(30)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-Source-Text-Audit/1.0')
$receipts=[Collections.Generic.List[object]]::new()
function Fetch([string]$method,[string]$url){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -notin @('api.github.com','raw.githubusercontent.com') -or $uri.UserInfo -or $uri.Fragment -or ($uri.Query -and ($uri.Host -ne 'api.github.com' -or $uri.Query -ne '?recursive=1'))){throw "unapproved textual source: $url"}
    $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::new($method),$uri)
    try{
        $response=$client.SendAsync($request,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
        try{
            $receipt=[pscustomobject]@{method=$method;url=$url;status=[int]$response.StatusCode;length=$response.Content.Headers.ContentLength;contentType=[string]$response.Content.Headers.ContentType;redirect=[bool]$response.Headers.Location;authorization=$false;automaticRedirects=$false;cookies=$false}
            $receipts.Add($receipt)
            if($response.Headers.Location){throw "unexpected text redirect: $url"}
            $bytes=if($method -eq 'GET'){$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()}else{$null}
            return [pscustomobject]@{receipt=$receipt;bytes=$bytes}
        }finally{$response.Dispose()}
    }finally{$request.Dispose()}
}
$files=[Collections.Generic.List[object]]::new()
$gaps=[Collections.Generic.List[object]]::new()
$activeCMake=@('D3D12MemoryAllocator','ogg','vorbis','miniaudio','RecastNavigation','assimp')
try{
    foreach($row in $rows){
        $root=Get-Content -LiteralPath (Join-Path $out "$($row.name)-root-tree.json") -Raw | ConvertFrom-Json
        $tree=$root.tree
        if($row.name -in $activeCMake){
            $cachedTree=Join-Path $out "$($row.name)-recursive-tree.json"
            $result=if(Test-Path -LiteralPath $cachedTree){[pscustomobject]@{receipt=[pscustomobject]@{status=200};bytes=[IO.File]::ReadAllBytes($cachedTree)}}else{Fetch 'GET' "https://api.github.com/repos/$($row.repository)/git/trees/$($row.rootTree)?recursive=1"}
            if($result.receipt.status -ne 200){$gaps.Add([pscustomobject]@{name=$row.name;reason='Recursive CMake closure metadata unavailable';status=$result.receipt.status});continue}
            $treeJson=[Text.Encoding]::UTF8.GetString($result.bytes)
            [IO.File]::WriteAllBytes((Join-Path $out "$($row.name)-recursive-tree.json"),$result.bytes)
            $all=$treeJson | ConvertFrom-Json
            if($all.truncated){throw "truncated textual closure tree: $($row.name)"}
            $tree=$all.tree
        }
        $selected=@($tree | Where-Object {$_.type -eq 'blob' -and ($_.path -match '(^|/)(LICENSE[^/]*|License[^/]*|COPYING[^/]*|NOTICES[^/]*|CMakeLists\.txt)$' -or $_.path -match '\.cmake(\.in)?$' -or $_.path -eq '.gitmodules')})
        if($row.name -eq 'assimp'){$selected=@($selected | Where-Object {$_.path -match '(^|/)(LICENSE[^/]*|COPYING[^/]*|NOTICES[^/]*)$' -or $_.path -match '^(CMakeLists\.txt|code/CMakeLists\.txt|cmake-modules/[^/]+\.cmake(\.in)?|contrib/tinyusdz/.*\.cmake(\.in)?|contrib/tinyusdz/CMakeLists\.txt)$'})}
        if($row.name -eq 'mikktspace'){$selected += @($tree | Where-Object path -eq 'mikktspace.h')}
        if($row.name -eq 'squish-ccr'){$selected += @($tree | Where-Object {$_.path -in @('squish.h','README')})}
        foreach($blob in $selected){
            $url="https://raw.githubusercontent.com/$($row.repository)/$($row.expectedGitCommit)/$($blob.path)"
            $target=Join-Path (Join-Path (Join-Path $out 'texts') $row.name) $blob.path
            if(Test-Path -LiteralPath $target){$get=[pscustomobject]@{receipt=[pscustomobject]@{status=200;contentType='text/plain; reused pinned primary text'};bytes=[IO.File]::ReadAllBytes($target)}}else{
            $head=Fetch 'HEAD' $url
            if($head.receipt.status -ne 200){throw "text HEAD failed: $url"}
            $get=Fetch 'GET' $url
            }
            if($get.receipt.status -ne 200 -or ($get.receipt.contentType -notmatch '^text/plain' -and -not ($row.name -eq 'squish-ccr' -and $blob.path -eq 'squish.h' -and $get.receipt.contentType -eq 'application/octet-stream' -and $get.bytes -notcontains 0)) -or $get.bytes.Length -ne $blob.size){throw "text GET mismatch: $url"}
            $prefix=[Text.Encoding]::ASCII.GetBytes("blob $($get.bytes.Length)`0")
            $bytes=[byte[]]::new($prefix.Length+$get.bytes.Length)
            [Array]::Copy($prefix,0,$bytes,0,$prefix.Length);[Array]::Copy($get.bytes,0,$bytes,$prefix.Length,$get.bytes.Length)
            $sha1=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData($bytes)).ToLowerInvariant()
            if($sha1 -ne $blob.sha){throw "primary text Git blob mismatch: $url"}
            $target=Join-Path (Join-Path (Join-Path $out 'texts') $row.name) $blob.path
            if(Test-Path -LiteralPath $target){$write=$false}else{$write=$true}
            New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
            if($write){[IO.File]::WriteAllBytes($target,$get.bytes)}
            $files.Add([pscustomobject]@{name=$row.name;sourcePath=$blob.path;path=$target;url=$url;bytes=$get.bytes.Length;gitBlobSHA1=$sha1;gitBlobMatched=$true;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($get.bytes)).ToLowerInvariant()})
        }
    }
}finally{$client.Dispose();$handler.Dispose();$receipts.ToArray() | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'primary-text-http-receipts-rev2.json') -Encoding utf8}
[ordered]@{textFiles=$files.Count;files=$files.ToArray();gaps=$gaps.ToArray();archivesAcquired=0;nativeProcessesExecuted=0;content='Only primary CMake/license/notice/submodule text, data inspection; no executable or binary fetched'} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'primary-text-manifest-rev2.json') -Encoding utf8
[pscustomobject]@{textFiles=$files.Count;gaps=$gaps.Count;archivesAcquired=0;nativeProcessesExecuted=0} | ConvertTo-Json
