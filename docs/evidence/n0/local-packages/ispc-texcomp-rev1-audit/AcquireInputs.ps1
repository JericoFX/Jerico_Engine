$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$handler=[Net.Http.HttpClientHandler]::new(); $handler.AllowAutoRedirect=$false
$client=[Net.Http.HttpClient]::new($handler); $client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-N0-manual-input-audit'); $client.Timeout=[TimeSpan]::FromSeconds(120)
function Acquire([string]$initial,[string]$archive,[Nullable[long]]$expectedBytes,[string]$receiptName){
 if(Test-Path -LiteralPath $archive){throw 'Preserve existing archive'}
 $uri=[Uri]$initial; $heads=@(); $complete=$false
 for($hop=0;$hop -lt 5;$hop++){
  if($uri.Scheme -ne 'https' -or $uri.Host -notin @('github.com','release-assets.githubusercontent.com','codeload.github.com') -or $uri.UserInfo){throw 'Forbidden destination'}
  $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$uri); $head=$client.Send($request)
  try{
   $loc=$head.Headers.Location
   $heads+=[ordered]@{host=$uri.Host;path=$uri.AbsolutePath;query=$(if($uri.Query){'<redacted>'}else{''});status=[int]$head.StatusCode;bytes=$head.Content.Headers.ContentLength;authorization=$false;autoRedirect=$false;location=$(if($loc){$u=[Uri]::new($uri,$loc);$u.GetLeftPart([UriPartial]::Path)+$(if($u.Query){'?<redacted>'}else{''})}else{''})}
   if([int]$head.StatusCode -ge 300 -and [int]$head.StatusCode -lt 400){if(-not $loc){throw 'Missing redirect'};$uri=[Uri]::new($uri,$loc);continue}
   if(-not $head.IsSuccessStatusCode){throw 'HEAD denied'}
   if($expectedBytes -and $head.Content.Headers.ContentLength -ne $expectedBytes){throw 'HEAD size mismatch'}
   $complete=$true;break
  }finally{$head.Dispose();$request.Dispose()}
 }
 if(-not $complete){throw 'Redirect chain unfinished'}
 $response=$client.GetAsync($uri,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
 try{
  if(-not $response.IsSuccessStatusCode){throw 'GET denied; no automatic redirect'}
  $input=$response.Content.ReadAsStreamAsync().GetAwaiter().GetResult();$output=[IO.File]::Open($archive,[IO.FileMode]::CreateNew)
  try{$input.CopyTo($output)}finally{$output.Dispose();$input.Dispose()}
  $bytes=(Get-Item -LiteralPath $archive).Length
  if($expectedBytes -and $bytes -ne $expectedBytes){throw 'Archive size mismatch; retain partial input'}
  $receipt=[ordered]@{initialUrl=$initial;headChain=$heads;get=[ordered]@{host=$uri.Host;path=$uri.AbsolutePath;query=$(if($uri.Query){'<redacted>'}else{''});status=[int]$response.StatusCode;autoRedirect=$false;authorization=$false};archive=$archive;bytes=$bytes;sha256=(Get-FileHash -LiteralPath $archive).Hash;officialDigestAvailable=$false;verification='TLS official endpoint + exact pinned source commit or release asset/size; measured local SHA256, not claimed published upstream digest'}
  [IO.File]::WriteAllText((Join-Path $PSScriptRoot $receiptName),($receipt|ConvertTo-Json -Depth 8)+"`n",[Text.UTF8Encoding]::new($false))
  $receipt | Select-Object archive,bytes,sha256
 }finally{$response.Dispose()}
}
try{
 Acquire 'https://github.com/ispc/ispc/releases/download/v1.16.1/ispc-v1.16.1-windows.zip' 'F:/JericoEngine/Dependencies/archives/ispc-v1.16.1-windows.zip' 44708214 'tool-acquisition.json'
 Acquire 'https://codeload.github.com/GameTechDev/ISPCTextureCompressor/tar.gz/36b80aac50ea401a9adde44d86712c2241b2acfd' 'F:/JericoEngine/Dependencies/archives/ispc-texcomp-36b80aac50ea401a9adde44d86712c2241b2acfd.tar.gz' $null 'source-acquisition.json'
}finally{$client.Dispose();$handler.Dispose()}
