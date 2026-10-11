$ErrorActionPreference='Stop'
$Audit=$PSScriptRoot
Add-Type -AssemblyName System.Net.Http
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-TIFF-source-audit/1.0')
try{
 $u='https://gitlab.com/api/v4/projects/libtiff%2Flibtiff/repository/tags/v4.2.0'
 foreach($verb in @('HEAD','GET')){
  $req=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::new($verb),$u);$r=$client.SendAsync($req).GetAwaiter().GetResult()
  $receipt=[ordered]@{method=$verb;url=$u;status=[int]$r.StatusCode;location=if($r.Headers.Location){$r.Headers.Location.AbsoluteUri}else{$null};redirectsFollowed=$false;credentials='none'}
  $receipt|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $Audit ('upstream-tag-'+$verb.ToLower()+'.json')) -Encoding utf8
  if($verb -eq 'GET'){$raw=$r.Content.ReadAsStringAsync().GetAwaiter().GetResult();[IO.File]::WriteAllText((Join-Path $Audit 'upstream-tag-response.json'),$raw,[Text.UTF8Encoding]::new($false));if([int]$r.StatusCode -eq 200){$raw|ConvertFrom-Json|Select-Object name,target,commit,release|ConvertTo-Json -Depth 5}else{$receipt|ConvertTo-Json}}
  $req.Dispose();$r.Dispose()
 }
}finally{$client.Dispose();$handler.Dispose()}