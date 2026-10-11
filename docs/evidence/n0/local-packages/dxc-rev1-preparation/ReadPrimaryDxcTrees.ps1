$ErrorActionPreference='Stop'
$root=Join-Path $PSScriptRoot 'acquisition'
$sources=Get-Content -LiteralPath "$root/acquisition-summary.json" -Raw|ConvertFrom-Json
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.Add('User-Agent','JE-source-provenance-audit');$client.Timeout=[TimeSpan]::FromSeconds(60)
$records=@();$pending=@()
try{
 foreach($source in $sources){
  $receipt=Get-Content -LiteralPath "$root/$($source.id)/source-acquisition.json" -Raw|ConvertFrom-Json
  $url="https://api.github.com/repos/$($receipt.repository)/git/trees/$($receipt.commit)?recursive=1"
  $pending+=[pscustomobject]@{receipt=$receipt;url=$url;task=$client.GetAsync($url)}
 }
 foreach($item in $pending){
  $response=$item.task.GetAwaiter().GetResult()
  $record=[ordered]@{id=$item.receipt.id;url=$item.url;status=[int]$response.StatusCode;redirectFollowed=$false;authentication=$false}
  if($record.status -eq 200){
   $bytes=$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
   $path="$root/$($item.receipt.id)/primary-git-tree.json"
   if(Test-Path -LiteralPath $path){throw 'Primary metadata exists'}
   [IO.File]::WriteAllBytes($path,$bytes)
   $tree=[Text.Encoding]::UTF8.GetString($bytes)|ConvertFrom-Json -Depth 100
   $record.bytes=$bytes.Length;$record.path=$path;$record.sha256=(Get-FileHash -LiteralPath $path).Hash;$record.entries=$tree.tree.Count;$record.truncated=$tree.truncated
  }
  $records+=$record;$response.Dispose()
 }
}finally{$client.Dispose();$handler.Dispose()}
$path="$root/primary-git-tree-receipt.json"
if(Test-Path -LiteralPath $path){throw 'Receipt exists'}
[IO.File]::WriteAllText($path,($records|ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
$records|ConvertTo-Json
