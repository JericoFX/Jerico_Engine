$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$handler=[Net.Http.HttpClientHandler]::new()
$handler.AllowAutoRedirect=$false
$client=[Net.Http.HttpClient]::new($handler)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-source-audit')
$client.Timeout=[TimeSpan]::FromSeconds(45)
function Read-Primary([string]$url,[string]$filename){
 $uri=[Uri]$url
 if($uri.Scheme -ne 'https' -or $uri.Host -notin @('api.github.com','raw.githubusercontent.com','codeload.github.com','github.com')){throw 'Endpoint outside source audit'}
 if($uri.UserInfo -or $uri.Query -match '(?i)(sig|token|credential|key)='){throw 'Credentials/signed query forbidden'}
 $path=Join-Path $PSScriptRoot $filename
 if(Test-Path -LiteralPath $path){throw 'Preserve existing receipt'}
 $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$uri)
 $head=$client.Send($request)
 try{
  $headReceipt=[ordered]@{url=$url;status=[int]$head.StatusCode;location=[string]$head.Headers.Location;automaticRedirects=$false;authorization=$false}
  [IO.File]::WriteAllText($path+'.head.json',($headReceipt|ConvertTo-Json)+"`n",[Text.UTF8Encoding]::new($false))
  if(-not $head.IsSuccessStatusCode){throw 'HEAD failed/redirect; inspect manually before GET'}
 }finally{$head.Dispose();$request.Dispose()}
 $response=$client.GetAsync($uri).GetAwaiter().GetResult()
 try{
  if(-not $response.IsSuccessStatusCode){throw 'GET failed/redirect'}
  $body=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
  [IO.File]::WriteAllBytes($path,$body)
  $receipt=[ordered]@{url=$url;status=[int]$response.StatusCode;bytes=$body.Length;sha256=(Get-FileHash -LiteralPath $path).Hash;automaticRedirects=$false;authorization=$false}
  [IO.File]::WriteAllText($path+'.receipt.json',($receipt|ConvertTo-Json)+"`n",[Text.UTF8Encoding]::new($false))
  return [Text.UTF8Encoding]::new($false).GetString($body)
 }finally{$response.Dispose()}
}
