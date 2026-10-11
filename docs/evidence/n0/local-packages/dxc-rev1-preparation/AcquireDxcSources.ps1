$ErrorActionPreference='Stop'
$planPath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-current-audit/source-plan-dxc-closed.json'
$approvalPath='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/dxc-current-audit/parent-acquisition-approval-dxc-rev1.json'
if((Get-FileHash -LiteralPath $planPath).Hash -ne 'A07B2EF4514AD2043D3CDCED8953DC28C38603C4625CA5BFCF8859A09940CF02'){throw 'Approved plan changed'}
if((Get-FileHash -LiteralPath $approvalPath).Hash -ne 'A375FBAD58F76C95B5C21FE9FADBABBDCB436DAFF8E9B4742E318D2814ED6D39'){throw 'Approval receipt changed'}
$plan=Get-Content -LiteralPath $planPath -Raw | ConvertFrom-Json -AsHashtable -Depth 50
if($plan.sources.Count -ne 3){throw 'Only three sources approved'}
$root=Join-Path $PSScriptRoot 'acquisition'
if(Test-Path -LiteralPath $root){throw 'Acquisition output exists'}
[IO.Directory]::CreateDirectory($root)|Out-Null
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.Timeout=[TimeSpan]::FromMinutes(4)
$results=[Collections.Generic.List[object]]::new()
try {
 $pending=@()
 foreach($row in $plan.sources){
  $url=[Uri]$row.url
  if($url.Scheme -cne 'https' -or $url.Host -cne 'codeload.github.com' -or $url.Query -ne '' -or $url.AbsolutePath -cne "/$($row.repository)/tar.gz/$($row.commit)"){throw 'Unexpected archive destination'}
  $dir=Join-Path $root $row.id;[IO.Directory]::CreateDirectory($dir)|Out-Null
  $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$url)
  $pending+=[pscustomobject]@{row=$row;dir=$dir;url=$url;request=$request;task=$client.SendAsync($request)}
 }
 $gets=@()
 foreach($item in $pending){
  try{
   $response=$item.task.GetAwaiter().GetResult();$head=[ordered]@{url=$item.url.AbsoluteUri;method='HEAD';status=[int]$response.StatusCode;redirectFollowed=$false;authentication=$false;cookies=$false;contentLength=$response.Content.Headers.ContentLength;contentType=[string]$response.Content.Headers.ContentType;etag=[string]$response.Headers.ETag;utc=[DateTime]::UtcNow.ToString('o')}
   [IO.File]::WriteAllText((Join-Path $item.dir 'destination-head.json'),($head|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
   if($head.status -ne 200 -or $head.contentType -notlike 'application/x-gzip*'){throw "HEAD not audited gzip200: $($head.status)"}
   $gets+=[pscustomobject]@{item=$item;head=$head;task=$client.GetAsync($item.url)};$response.Dispose();$item.request.Dispose()
  }catch{$results.Add(@{id=$item.row.id;status='HEAD_FAILED';error=$_.Exception.Message})}
 }
 foreach($get in $gets){
  $item=$get.item
  try{
   $response=$get.task.GetAwaiter().GetResult();if([int]$response.StatusCode -ne 200){throw "GET not200: $([int]$response.StatusCode)"}
   $getEtag=[string]$response.Headers.ETag
   [byte[]]$bytes=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult();$response.Dispose()
   if($bytes.Length -lt 2 -or $bytes[0] -ne 31 -or $bytes[1] -ne 139){throw 'Not gzip'}
   if($null -ne $get.head.contentLength -and $get.head.contentLength -ne $bytes.Length){throw 'HEAD bytes mismatch'}
   if($getEtag -ne $get.head.etag){throw 'HEAD/GET ETag changed'}
   $archive=Join-Path $item.dir "$($item.row.id)-$($item.row.commit).tar.gz"
   if(Test-Path -LiteralPath $archive){throw 'Archive exists'};[IO.File]::WriteAllBytes($archive,$bytes)
   $rec=[ordered]@{id=$item.row.id;repository=$item.row.repository;commit=$item.row.commit;url=$item.url.AbsoluteUri;status=200;redirectFollowed=$false;authentication=$false;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant();archivePath=$archive;headGetEtagEqual=$true;etag=$getEtag;utc=[DateTime]::UtcNow.ToString('o');identity='Measured original source archive, not historical package hash; pending full tar/primary Gitblob verification'}
   [IO.File]::WriteAllText((Join-Path $item.dir 'source-acquisition.json'),($rec|ConvertTo-Json),[Text.UTF8Encoding]::new($false))
   $results.Add(@{id=$item.row.id;status='ACQUIRED_PENDING_TAR_AUDIT';archivePath=$archive;bytes=$bytes.Length;sha256=$rec.sha256})
  }catch{$results.Add(@{id=$item.row.id;status='GET_FAILED';error=$_.Exception.Message})}
 }
}finally{$client.Dispose();$handler.Dispose()}
[IO.File]::WriteAllText((Join-Path $root 'acquisition-summary.json'),($results.ToArray()|ConvertTo-Json -Depth 10),[Text.UTF8Encoding]::new($false))
$results.ToArray()|ConvertTo-Json
