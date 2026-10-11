#Requires -Version 7.0
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=$PSScriptRoot
$pin='f32d8d4dd9e06f8986cd5e10f12e199fc023f150'
$url="https://codeload.github.com/ARM-software/astc-encoder/tar.gz/$pin"
$archive="F:/JericoEngine/Dependencies/archives/astc-encoder-$pin.tar.gz"
$utf8=[Text.UTF8Encoding]::new($false)
function Write-NewJson([string]$path,$value){if(Test-Path -LiteralPath $path){throw 'Receipt exists'};[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 12)+"`n",$utf8)}
if(Test-Path -LiteralPath $archive){throw 'Retain existing source archive'}
$tag=Get-Content -LiteralPath (Join-Path $root 'upstream-tag-3.2.json') -Raw|ConvertFrom-Json
if($tag.object.type -ne 'commit' -or $tag.object.sha -ne $pin){throw 'Primary tag authority differs'}
$license=Join-Path $root 'upstream-LICENSE.txt'
if(-not ([IO.File]::ReadAllText($license).Contains('Apache License'))){throw 'Primary license missing'}
$uri=[Uri]$url
if($uri.Scheme -ne 'https' -or $uri.Host -ne 'codeload.github.com' -or $uri.AbsolutePath -cne "/ARM-software/astc-encoder/tar.gz/$pin" -or $uri.Query){throw 'Destination differs'}
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false
$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-source-audit');$client.Timeout=[TimeSpan]::FromSeconds(60)
try{
    $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$uri)
    $head=$client.SendAsync($request,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
    try{
        $headReceipt=@{url=$url;sourceCommit=$pin;upstreamTag='3.2';status=[int]$head.StatusCode;authorization=$false;automaticRedirects=$false;contentLength=$head.Content.Headers.ContentLength;contentType=[string]$head.Content.Headers.ContentType;etag=[string]$head.Headers.ETag;location=if($head.Headers.Location){[string]$head.Headers.Location}else{$null};license='Apache-2.0';licenseSHA=(Get-FileHash -LiteralPath $license).Hash}
        Write-NewJson (Join-Path $root 'source-head.json') $headReceipt
        if([int]$head.StatusCode -ne 200 -or $head.Headers.Location -or [string]$head.Content.Headers.ContentType -notmatch 'application/(x-gzip|gzip|octet-stream)'){throw 'HEAD audit did not authorize direct GET'}
    }finally{$head.Dispose();$request.Dispose()}
    $response=$client.GetAsync($uri,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
    try{
        if([int]$response.StatusCode -ne 200 -or $response.Headers.Location -or [string]$response.Headers.ETag -cne $headReceipt.etag){throw 'GET differs from audited direct HEAD'}
        $input=$response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
        $output=[IO.File]::Open($archive,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
        try{$input.CopyTo($output)}finally{$output.Dispose();$input.Dispose()}
        $bytes=([IO.FileInfo]::new($archive)).Length
        if($headReceipt.contentLength -and $bytes -ne $headReceipt.contentLength){throw 'Source byte count differs from HEAD'}
        $receipt=@{url=$url;archive=$archive;sourceCommit=$pin;upstreamTag='3.2';status=200;redirects=0;authorization=$false;bytes=$bytes;sha256=(Get-FileHash -LiteralPath $archive).Hash;etag=[string]$response.Headers.ETag;license='Apache-2.0';historicalPackageHash='17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295';historicalPackageHashIsSourceHash=$false;extracted=$false;nativeExecuted=$false}
        Write-NewJson (Join-Path $root 'source-acquisition.json') $receipt
        $receipt|ConvertTo-Json -Depth 8
    }finally{$response.Dispose()}
}finally{$client.Dispose();$handler.Dispose()}
