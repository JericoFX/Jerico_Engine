$ErrorActionPreference='Stop'
$commit='1cc0ab76dafe08a4144e419e619a8951a3d219db'
$prefix='https://api.github.com/repos/o3de/DirectXShaderCompiler/commits'
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.Add('User-Agent','JE-source-provenance-audit');$client.Timeout=[TimeSpan]::FromSeconds(60)
$records=@();$count=$null
function ReadPage([string]$Url,[string]$Label){
 $uri=[Uri]$Url
 $query=@{}
 foreach($part in $uri.Query.TrimStart('?').Split('&')){ $pair=$part.Split('=',2); if($pair.Count -ne 2 -or $query.ContainsKey($pair[0])){throw 'Ambiguous public history query'}; $query[$pair[0]]=[Uri]::UnescapeDataString($pair[1]) }
 if($uri.Scheme -cne 'https' -or $uri.Host -cne 'api.github.com' -or $uri.AbsolutePath -cne '/repos/o3de/DirectXShaderCompiler/commits' -or $query.sha -ne '1cc0ab76dafe08a4144e419e619a8951a3d219db' -or $query.per_page -ne '1' -or @($query.Keys|Where-Object{$_ -notin @('sha','per_page','page')}).Count -or ($query.ContainsKey('page') -and $query.page -notmatch '^\d+$')){throw 'Unexpected public history endpoint'}
 $response=$client.GetAsync($Url).GetAwaiter().GetResult()
 $text=$response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
 $links=if($response.Headers.Contains('Link')){($response.Headers.GetValues('Link') -join ', ')}else{''}
 $row=[ordered]@{url=$Url;label=$Label;status=[int]$response.StatusCode;redirectFollowed=$false;authentication=$false;link=$links}
 if($row.status -eq 200){
  $page=ConvertFrom-Json -InputObject $text -Depth 100
  $row.returnedCount=@($page).Count;$row.commitSHAs=@($page|ForEach-Object{$_.sha})
  $path=Join-Path $PSScriptRoot "history-$Label-rev2.json"
  if(Test-Path -LiteralPath $path){throw 'History evidence exists'}
  [IO.File]::WriteAllText($path,$text,[Text.UTF8Encoding]::new($false));$row.path=$path;$row.sha256=(Get-FileHash -LiteralPath $path).Hash
 }
 $response.Dispose();return $row
}
try{
 $first=ReadPage "$prefix`?sha=$commit&per_page=1" 'first';$records+=$first
 if($first.status -eq 200 -and $first.returnedCount -eq 1 -and $first.commitSHAs[0] -eq $commit){
  $match=[regex]::Match($first.link,'<([^>]+)>; rel="last"')
  if($match.Success){
   $lastUrl=$match.Groups[1].Value
   if($lastUrl -notmatch '[?&]page=(\d+)(?:&|$)'){throw 'Unexpected last-page metadata'}
   $count=[int64]$Matches[1]
   $last=ReadPage $lastUrl 'last';$records+=$last
   $next=ReadPage "$prefix`?sha=$commit&per_page=1&page=$($count+1)" 'after-last';$records+=$next
   if($last.status -ne 200 -or $last.returnedCount -ne 1 -or $next.status -ne 200 -or $next.returnedCount -ne 0){$count=$null}
  }
 }
}finally{$client.Dispose();$handler.Dispose()}
$out=Join-Path $PSScriptRoot 'commit-history-metadata-receipt-rev2.json';if(Test-Path -LiteralPath $out){throw 'Receipt exists'}
$result=[ordered]@{sourceCommit=$commit;apiListCount=$count;meaning='Pinned GitHub REST commit-list pagination, per_page1, first exact source pin, last-page plus next empty boundary; not a local Git rev-list execution';records=$records;nativeExecuted=$false;sourceModified=$false}
[IO.File]::WriteAllText($out,($result|ConvertTo-Json -Depth 15),[Text.UTF8Encoding]::new($false))
$result|ConvertTo-Json -Depth 15
