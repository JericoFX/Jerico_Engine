Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$auditRoot=$PSScriptRoot
$archivePath='F:/JericoEngine/Dependencies/archives/strawberry-perl-5.42.3.1-64bit-portable.zip'
$snapshotPath='F:/JericoEngine/Dependencies/tools/strawberry-perl-5.42.3.1-64bit-portable'
$sourceUrl='https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_54231_64bit/strawberry-perl-5.42.3.1-64bit-portable.zip'
$expectedSha256='6a081a811781c30aca51dbc036afd93092af91e3297901f02c17043795a10690'
foreach($newPath in @($archivePath,$snapshotPath,(Join-Path $auditRoot 'tool-acquisition.json'))){if(Test-Path -LiteralPath $newPath){throw "Existing output; refusing overwrite: $newPath"}}
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
$hop0=Invoke-AuditedRequest $sourceUrl 'HEAD' 'github.com' @(302) 'tool-head-hop0.json'
$hop0.Response.Close()
if(([Uri]$hop0.Location).Host -ne 'release-assets.githubusercontent.com'){throw 'Unexpected asset destination'}
$hop1=Invoke-AuditedRequest $hop0.Location 'HEAD' 'release-assets.githubusercontent.com' @(200) 'tool-head-hop1.json'
$hop1Length=$hop1.Response.ContentLength
$hop1.Response.Close()
if($hop1.Location -or $hop1Length -ne 304765269){throw 'Unexpected endpoint/size'}
$download=Invoke-AuditedRequest $hop0.Location 'GET' 'release-assets.githubusercontent.com' @(200) 'tool-get.json'
try{
 if($download.Location){throw 'GET redirect rejected'}
 $output=[IO.File]::Open($archivePath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 try{$download.Response.GetResponseStream().CopyTo($output)}finally{$output.Dispose()}
}finally{$download.Response.Close()}
$hash=(Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
$receipt=[ordered]@{archive=$archivePath;officialUrl=$sourceUrl;bytes=(Get-Item $archivePath).Length;sha256=$hash;expectedSha256=$expectedSha256;pinMatches=($hash -eq $expectedSha256);snapshot=$snapshotPath;version='5.42.3.1';architecture='MSWin32-x64-multi-thread';autoRedirect=$false;credentials='none';signedQueriesPersisted=$false;toolExecuted=$false;extracted=$false}
[IO.File]::WriteAllText((Join-Path $auditRoot 'tool-acquisition.json'),($receipt|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
if(-not $receipt.pinMatches -or $receipt.bytes -ne 304765269){throw 'Tool pin mismatch; extraction forbidden'}
$receipt|ConvertTo-Json -Depth 5