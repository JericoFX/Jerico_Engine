$ErrorActionPreference='Stop'
$planPath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/vulkan-validationlayers-current-audit/source-plan-vvl.json'
if((Get-FileHash $planPath).Hash -ne 'D795467E0E7DDDB85D8B12293883B4FFA5B1B1CE7A8EC86445A98D372DBA6E9D'){throw 'Approved plan changed'}
$plan=Get-Content $planPath -Raw|ConvertFrom-Json -AsHashtable -Depth 40
$root=Join-Path $PSScriptRoot 'acquisition'
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.Timeout=[TimeSpan]::FromMinutes(3)
$results=@()
try {
 foreach($start in @(0,4)) {
  $pending=@()
  foreach($row in $plan.sources[$start..([Math]::Min($start+3,$plan.sources.Count-1))]){
   $url=[Uri]$row.proposedArchiveUrl
   if($url.Scheme -ne 'https' -or $url.Host -ne 'codeload.github.com' -or $url.Query -ne '' -or $url.AbsolutePath -ne "/$($row.repository)/tar.gz/$($row.commit)"){throw 'Unexpected archive destination'}
   $dir=Join-Path $root $row.id;if(Test-Path $dir){throw 'Acquisition output exists'};[IO.Directory]::CreateDirectory($dir)|Out-Null
   $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$url)
   $pending+=[pscustomobject]@{row=$row;dir=$dir;url=$url;request=$request;task=$client.SendAsync($request)}
  }
  $gets=@()
  foreach($item in $pending){
   try{
    $response=$item.task.GetAwaiter().GetResult();$head=[ordered]@{url=$item.url.AbsoluteUri;method='HEAD';status=[int]$response.StatusCode;redirectFollowed=$false;authentication=$false;cookies=$false;contentLength=$response.Content.Headers.ContentLength;contentType=[string]$response.Content.Headers.ContentType;etag=[string]$response.Headers.ETag;utc=[DateTime]::UtcNow.ToString('o')}
    [IO.File]::WriteAllText((Join-Path $item.dir 'destination-head.json'),($head|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
    if($head.status -ne 200){throw "HEAD did not return200: $($head.status)"}
    $gets+=[pscustomobject]@{item=$item;head=$head;task=$client.GetAsync($item.url)};$response.Dispose();$item.request.Dispose()
   }catch{$results+=@{id=$item.row.id;status='HEAD_FAILED';error=$_.Exception.Message}}
  }
  foreach($get in $gets){
   $item=$get.item
   try{
    $response=$get.task.GetAwaiter().GetResult();if([int]$response.StatusCode -ne 200){throw "GET did not return200: $([int]$response.StatusCode)"}
    [byte[]]$bytes=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult();$response.Dispose()
    if($bytes.Length -lt 2 -or $bytes[0] -ne 31 -or $bytes[1] -ne 139){throw 'Not gzip'}
    if($null -ne $get.head.contentLength -and $get.head.contentLength -ne $bytes.Length){throw 'HEAD bytes mismatch'}
    $archive=Join-Path $item.dir "$($item.row.id)-$($item.row.commit).tar.gz"
    if(Test-Path $archive){throw 'Archive exists'};[IO.File]::WriteAllBytes($archive,$bytes)
    $rec=[ordered]@{id=$item.row.id;repository=$item.row.repository;commit=$item.row.commit;url=$item.url.AbsoluteUri;status=200;redirectFollowed=$false;authentication=$false;bytes=$bytes.Length;sha256=(Get-FileHash $archive).Hash.ToLowerInvariant();archivePath=$archive;utc=[DateTime]::UtcNow.ToString('o')}
    [IO.File]::WriteAllText((Join-Path $item.dir 'source-acquisition.json'),($rec|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
    $results+=@{id=$item.row.id;status='ACQUIRED_PENDING_TAR_AUDIT';archivePath=$archive;bytes=$bytes.Length;sha256=$rec.sha256}
   }catch{$results+=@{id=$item.row.id;status='GET_FAILED';error=$_.Exception.Message}}
  }
 }
}finally{$client.Dispose();$handler.Dispose()}
[IO.File]::WriteAllText((Join-Path $root 'acquisition-summary.json'),($results|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
$results|ConvertTo-Json