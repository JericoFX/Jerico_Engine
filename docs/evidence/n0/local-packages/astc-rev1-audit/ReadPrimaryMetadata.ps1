#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=$PSScriptRoot
$handler=[Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect=$false
$client=[Net.Http.HttpClient]::new($handler)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-source-audit')
$client.Timeout=[TimeSpan]::FromSeconds(45)
$utf8=[Text.UTF8Encoding]::new($false)
function Read-Primary([string]$url,[string]$filename){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -notin @('api.github.com','raw.githubusercontent.com')){throw 'Endpoint outside primary metadata scope'}
    $path=Join-Path $root $filename
    if(Test-Path -LiteralPath $path){throw 'Preserve prior response'}
    $response=$client.GetAsync($uri).GetAwaiter().GetResult()
    try{
        $body=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
        $receipt=@{url=$url;status=[int]$response.StatusCode;authorization=$false;automaticRedirects=$false;bytes=$body.Length;contentType=[string]$response.Content.Headers.ContentType}
        if([int]$response.StatusCode -ge 300 -and [int]$response.StatusCode -lt 400){throw 'Metadata redirect requires separate audit'}
        [IO.File]::WriteAllBytes($path,$body)
        $receipt.sha256=(Get-FileHash -LiteralPath $path).Hash
        [IO.File]::WriteAllText("$path.receipt.json",($receipt|ConvertTo-Json -Depth 8)+"`n",$utf8)
        if(-not $response.IsSuccessStatusCode){throw "Primary metadata failure $($receipt.status) retained"}
        return $utf8.GetString($body)
    }finally{$response.Dispose()}
}
try{
    $tag=Read-Primary 'https://api.github.com/repos/ARM-software/astc-encoder/git/ref/tags/3.2' 'upstream-tag-3.2.json'|ConvertFrom-Json
    $commit=$tag.object.sha
    if($tag.object.type -eq 'tag'){$tagObject=Read-Primary "https://api.github.com/repos/ARM-software/astc-encoder/git/tags/$commit" 'upstream-tag-object-3.2.json'|ConvertFrom-Json;$commit=$tagObject.object.sha;if($tagObject.object.type -ne 'commit'){throw 'Tag does not resolve to commit'}}
    elseif($tag.object.type -ne 'commit'){throw 'Unexpected source ref type'}
    $tree=Read-Primary "https://api.github.com/repos/ARM-software/astc-encoder/git/trees/$commit`?recursive=1" 'upstream-tree.json'|ConvertFrom-Json
    if($tree.truncated){throw 'Incomplete upstream primary tree'}
    $history=Read-Primary 'https://api.github.com/repos/o3de/3p-package-source/commits?path=package-system%2Fastc-encoder%2Fbuild_config.json&per_page=20&sha=70f0b7b5257bf7cf935bf1cfd270e1b53ccead18' 'recipe-history.json'|ConvertFrom-Json
    @{commit=$commit;treeFiles=@($tree.tree|Where-Object type -eq blob).Count;gitlinks=@($tree.tree|Where-Object mode -eq '160000').Count;recipeHistory=@($history|ForEach-Object{@{commit=$_.sha;subject=$_.commit.message.Split("`n")[0]}})}|ConvertTo-Json -Depth 5
}finally{$client.Dispose();$handler.Dispose()}
