$ErrorActionPreference='Stop'
$out=$PSScriptRoot
$repo='F:/JericoEngine/JericoEngine'
$inventoryPath=Join-Path $out 'batch-declared-inventory.json'
$inventory=Get-Content -LiteralPath $inventoryPath -Raw | ConvertFrom-Json
if($inventory.remainingCount -ne 12){throw 'expected twelve selected source wrappers'}
$handler=[Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect=$false
$handler.UseCookies=$false
$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler)
$client.Timeout=[TimeSpan]::FromSeconds(30)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-Source-Destination-Audit/1.0')
$rows=[Collections.Generic.List[object]]::new()
function Request([string]$method,[string]$url){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -notin @('github.com','codeload.github.com','api.github.com','raw.githubusercontent.com') -or $uri.UserInfo -or $uri.Query -or $uri.Fragment){throw "unapproved destination: $url"}
    $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::new($method),$uri)
    try{
        $response=$client.SendAsync($request,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
        try{
            $location=if($response.Headers.Location){$response.Headers.Location.AbsoluteUri}else{$null}
            $record=[pscustomobject]@{method=$method;url=$url;status=[int]$response.StatusCode;length=$response.Content.Headers.ContentLength;contentType=[string]$response.Content.Headers.ContentType;etag=[string]$response.Headers.ETag;location=$location;authorization=$false;automaticRedirects=$false;cookies=$false}
            $body=if($method -eq 'GET'){$response.Content.ReadAsStringAsync().GetAwaiter().GetResult()}else{$null}
            return [pscustomobject]@{receipt=$record;body=$body}
        }finally{$response.Dispose()}
    }finally{$request.Dispose()}
}
function Metadata([string]$url,[string]$file){
    $result=Request 'GET' $url
    $result.receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out "$file.receipt.json") -Encoding utf8
    if($result.receipt.status -ne 200){return $null}
    [IO.File]::WriteAllText((Join-Path $out "$file.json"),$result.body,[Text.UTF8Encoding]::new($false))
    return ($result.body | ConvertFrom-Json)
}
try{
    foreach($item in @($inventory.selected | Where-Object fetchName -eq 'assimp')){
        $name=$item.fetchName
        $wrapper=Join-Path $repo $item.wrapper
        if((Get-FileHash -LiteralPath $wrapper -Algorithm SHA256).Hash -ne $item.wrapperSHA256){throw "wrapper changed: $wrapper"}
        $uri=[Uri]$item.sourceURL
        $match=[regex]::Match($uri.AbsolutePath,'^/([^/]+/[^/]+)/archive/(.+)\.tar\.gz$')
        if($uri.Host -ne 'github.com' -or -not $match.Success){throw "unexpected declared source: $($item.sourceURL)"}
        $repository=$match.Groups[1].Value
        $ref=$match.Groups[2].Value
        $expected="https://codeload.github.com/$repository/tar.gz/$ref"
        $head1=Request 'HEAD' $item.sourceURL
        if($head1.receipt.status -notin @(301,302,303,307,308) -or $head1.receipt.location -cne $expected){throw "unexpected GitHub redirect for $name"}
        $head2=Request 'HEAD' $expected
        if($head2.receipt.status -ne 200 -or $head2.receipt.location){throw "source destination not direct200: $name"}
        @($head1.receipt,$head2.receipt) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out "$name-source-head.json") -Encoding utf8
        $commit=Metadata "https://api.github.com/repos/$repository/commits/$($item.gitCommit)" "$name-commit"
        $tagCommit=$null
        if($ref.StartsWith('refs/tags/')){
            $tag=$ref.Substring('refs/tags/'.Length)
            $tagRef=Metadata "https://api.github.com/repos/$repository/git/ref/tags/$tag" "$name-tag"
            if($tagRef){
                $tagCommit=$tagRef.object.sha
                if($tagRef.object.type -eq 'tag'){
                    $annotation=Metadata "https://api.github.com/repos/$repository/git/tags/$tagCommit" "$name-tag-object"
                    if($annotation){$tagCommit=$annotation.object.sha}else{$tagCommit=$null}
                }
            }
        }else{$tagCommit=$ref}
        if($tagCommit -and $tagCommit -ne $item.gitCommit){throw "declared tag does not match wrapper Git pin: $name"}
        $rootTree=$null
        if($commit){
            if($commit.sha -ne $item.gitCommit){throw "commit endpoint does not match pin: $name"}
            $rootTree=Metadata "https://api.github.com/repos/$repository/git/trees/$($commit.commit.tree.sha)" "$name-root-tree"
        }
        $rows.Add([pscustomobject]@{name=$name;repository=$repository;sourceURL=$item.sourceURL;directArchiveURL=$expected;expectedArchiveSHA256=$item.archiveSHA256;expectedGitCommit=$item.gitCommit;sourceHEADs=@($head1.receipt,$head2.receipt);commitCorroborated=[bool]$commit;tagCommit=$tagCommit;tagPinMatched=[bool]($tagCommit -eq $item.gitCommit);rootTreeAvailable=[bool]$rootTree;rootTree=$commit.commit.tree.sha;archiveAcquired=$false;archiveHashVerified=$false;licenseDeclared=$item.license;wrapper=$item.wrapper;wrapperSHA256=$item.wrapperSHA256;consumer=$item.consumer;noAddSubdir=$item.noAddSubdir;scope=$item.selection})
        $rows.ToArray() | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'assimp-destination-pin-audit.json') -Encoding utf8
    }
}finally{$client.Dispose();$handler.Dispose()}
[pscustomobject]@{entries=$rows.Count;commitCorroborated=@($rows | Where-Object commitCorroborated).Count;tagsMatched=@($rows | Where-Object tagPinMatched).Count;archivesDownloaded=0;nativeProcessesExecuted=0} | ConvertTo-Json
