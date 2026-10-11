Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$auditRoot=$PSScriptRoot
$archivePath='F:/JericoEngine/Dependencies/archives/openssl-1.1.1o.tar.gz'
$snapshotPath='F:/JericoEngine/Dependencies/openssl-1.1.1o'
$sourceUrl='https://www.openssl.org/source/old/1.1.1/openssl-1.1.1o.tar.gz'
$expectedSha512='75b2f1499cb4640229eb6cd35d85cbff2e19db17b959ac4d04b60f1b395b73567f9003521452a0fcfeea9b31b26de0a7bccf476ecf9caae02298f3647cfb7e23'
foreach($newPath in @($archivePath,$snapshotPath,(Join-Path $auditRoot 'source-acquisition.json'))){if(Test-Path -LiteralPath $newPath){throw "Existing output; refusing overwrite: $newPath"}}
foreach($checkPath in @((Split-Path $archivePath),$auditRoot)){
    $item=Get-Item -LiteralPath $checkPath
    while($item){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Reparse ancestor rejected'};$item=$item.Parent}
}
function Invoke-AuditedRequest([string]$url,[string]$method,[string]$expectedHost,[int[]]$allowedStatus,[string]$receiptName){
    $uri=[Uri]$url
    if($uri.Scheme -ne 'https' -or $uri.Host -ne $expectedHost -or $uri.UserInfo){throw 'Unexpected destination'}
    $request=[Net.HttpWebRequest]::CreateHttp($url)
    $request.Method=$method
    $request.AllowAutoRedirect=$false
    $request.Credentials=$null
    $request.UseDefaultCredentials=$false
    $request.Timeout=60000
    $request.UserAgent='JE-exact-source-manual-audit'
    try{$response=$request.GetResponse()}catch [Net.WebException]{$response=$_.Exception.Response;if(-not $response){throw 'Request failed without response; URL/query omitted'}}
    $location=$response.Headers['Location']
    $receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');method=$method;url=$uri.GetLeftPart([UriPartial]::Path);queryRedacted=[bool]$uri.Query;status=[int]$response.StatusCode;autoRedirect=$false;credentials='none';contentLength=$response.ContentLength;contentType=$response.ContentType;location=if($location){([Uri]$location).GetLeftPart([UriPartial]::Path)}else{$null};locationQueryRedacted=if($location){[bool]([Uri]$location).Query}else{$false}}
    $receiptPath=Join-Path $auditRoot $receiptName
    if(Test-Path -LiteralPath $receiptPath){throw 'Receipt exists'}
    [IO.File]::WriteAllText($receiptPath,($receipt|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
    if([int]$response.StatusCode -notin $allowedStatus){$response.Close();throw 'Unexpected status; see redacted receipt'}
    return @{Response=$response;Location=$location}
}
# Explicit HEAD requests: each response is inspected before the next allowed host.
$hop0=Invoke-AuditedRequest $sourceUrl 'HEAD' 'www.openssl.org' @(301,302) 'source-head-hop0.json'
$hop0.Response.Close()
if($hop0.Location -ne 'https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1o/openssl-1.1.1o.tar.gz'){throw 'Official release destination changed'}
$hop1=Invoke-AuditedRequest $hop0.Location 'HEAD' 'github.com' @(302) 'source-head-hop1.json'
$hop1.Response.Close()
if(([Uri]$hop1.Location).Host -ne 'release-assets.githubusercontent.com'){throw 'Release asset host not allowed'}
$hop2=Invoke-AuditedRequest $hop1.Location 'HEAD' 'release-assets.githubusercontent.com' @(200) 'source-head-hop2.json'
$hop2.Response.Close()
if($hop2.Location){throw 'Further redirect not permitted'}
$download=Invoke-AuditedRequest $hop1.Location 'GET' 'release-assets.githubusercontent.com' @(200) 'source-get.json'
try{
    if($download.Location){throw 'GET redirect rejected'}
    $output=[IO.File]::Open($archivePath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try{$download.Response.GetResponseStream().CopyTo($output)}finally{$output.Dispose()}
}finally{$download.Response.Close()}
$actualSha512=(Get-FileHash -LiteralPath $archivePath -Algorithm SHA512).Hash.ToLowerInvariant()
$actualSha256=(Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
$summary=[ordered]@{officialUrl=$sourceUrl;archive=$archivePath;bytes=(Get-Item -LiteralPath $archivePath).Length;sha512=$actualSha512;expectedSha512=$expectedSha512;pinMatches=($actualSha512 -eq $expectedSha512);sha256=$actualSha256;sourceVersion='1.1.1o';sourceCommit='ca2e0784d2c38edcefd5d68028f4d954bd8faddb';autoRedirect=$false;credentials='none';signedQueriesPersisted=$false;extracted=$false}
[IO.File]::WriteAllText((Join-Path $auditRoot 'source-acquisition.json'),($summary|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
if(-not $summary.pinMatches){throw 'Source SHA512 mismatch; extraction forbidden'}
$summary|ConvertTo-Json -Depth 5
