$ErrorActionPreference='Stop'
$audit=$PSScriptRoot
$archive='F:/JericoEngine/Dependencies/archives/jsoncpp-1.6.2.tar.gz'
$pin='c40e5a2583df633b904525305bda40b622d9971fee1ece7ce48ca723a4907d36'
if(Test-Path -LiteralPath $archive){throw 'archive destination exists'}
$handler=[Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect=$false
$handler.UseCookies=$false
$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-source-audit')
function Head([string]$url,[int]$expectedStatus){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -notin @('github.com','codeload.github.com','api.github.com') -or $uri.Query -or $uri.UserInfo){throw 'non-primary destination'}
    $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$url)
    $response=$client.SendAsync($request).GetAwaiter().GetResult()
    $row=[pscustomobject]@{url=$url;status=[int]$response.StatusCode;contentLength=$response.Content.Headers.ContentLength;etag=[string]$response.Headers.ETag;location=[string]$response.Headers.Location;authentication=$false;autoRedirect=$false}
    if($row.status -ne $expectedStatus){throw "unexpected HEAD status: $url $($row.status)"}
    $response.Dispose();$request.Dispose()
    return $row
}
function GetPrimaryJson([string]$url,[string]$name){
    $head=Head $url 200
    $response=$client.GetAsync($url).GetAwaiter().GetResult()
    if([int]$response.StatusCode -ne 200){throw "primary GET failed: $url"}
    $bytes=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
    [IO.File]::WriteAllBytes((Join-Path $audit $name),$bytes)
    [ordered]@{head=$head;getStatus=200;bytes=$bytes.Length;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $audit "$name.receipt.json") -Encoding utf8
    $response.Dispose()
}
$original='https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.6.2.tar.gz'
$final='https://codeload.github.com/open-source-parsers/jsoncpp/tar.gz/refs/tags/1.6.2'
$first=Head $original 302
if($first.location -cne $final){throw 'unexpected official redirect destination'}
$last=Head $final 200
@($first,$last) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $audit 'source-destination-head.json') -Encoding utf8
GetPrimaryJson 'https://api.github.com/repos/open-source-parsers/jsoncpp/git/ref/tags/1.6.2' 'source-tag.json'
GetPrimaryJson 'https://api.github.com/repos/open-source-parsers/jsoncpp/git/commits/24682e37bf1425455b45e7db69dbdbcd30aec7dd' 'source-commit.json'
# The recursive tree query is a non-secret fixed primary API parameter.
$treeURL='https://api.github.com/repos/open-source-parsers/jsoncpp/git/trees/24682e37bf1425455b45e7db69dbdbcd30aec7dd?recursive=1'
$request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$treeURL)
$treeHead=$client.SendAsync($request).GetAwaiter().GetResult()
if([int]$treeHead.StatusCode -ne 200){throw 'tree HEAD failed'}
$treeHead.Dispose();$request.Dispose()
$treeResponse=$client.GetAsync($treeURL).GetAwaiter().GetResult()
if([int]$treeResponse.StatusCode -ne 200){throw 'tree GET failed'}
$treeBytes=$treeResponse.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
[IO.File]::WriteAllBytes((Join-Path $audit 'source-tree.json'),$treeBytes)
[ordered]@{url=$treeURL;headStatus=200;getStatus=200;autoRedirect=$false;authentication=$false;bytes=$treeBytes.Length;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($treeBytes)).ToLowerInvariant()} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $audit 'source-tree.json.receipt.json') -Encoding utf8
$tag=Get-Content -Raw -LiteralPath (Join-Path $audit 'source-tag.json') | ConvertFrom-Json
$commit=Get-Content -Raw -LiteralPath (Join-Path $audit 'source-commit.json') | ConvertFrom-Json
if($tag.object.type -ne 'commit' -or $tag.object.sha -ne '24682e37bf1425455b45e7db69dbdbcd30aec7dd' -or $commit.sha -ne $tag.object.sha){throw 'tag/consumer commit mismatch'}
$response=$client.GetAsync($final).GetAwaiter().GetResult()
if([int]$response.StatusCode -ne 200){throw 'archive GET failed'}
$bytes=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
$hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
if($hash -ne $pin){throw 'original consumer archive SHA mismatch; no archive published'}
$stream=[IO.File]::Open($archive,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
try{$stream.Write($bytes)}finally{$stream.Dispose()}
[ordered]@{originalURL=$original;effectiveURL=$final;headStatus=200;getStatus=200;autoRedirect=$false;authentication=$false;bytes=$bytes.Length;sha256=$hash;matchesOriginalConsumer=$true;commit=$commit.sha;tag='1.6.2';license='Public Domain OR MIT';archive=$archive} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $audit 'source-acquisition.json') -Encoding utf8
$response.Dispose();$treeResponse.Dispose();$client.Dispose();$handler.Dispose()
Get-Content -Raw -LiteralPath (Join-Path $audit 'source-acquisition.json')
