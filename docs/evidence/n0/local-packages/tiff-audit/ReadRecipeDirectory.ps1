$ErrorActionPreference='Stop'
$Audit=$PSScriptRoot
Add-Type -AssemblyName System.Net.Http
$handler=[System.Net.Http.HttpClientHandler]::new(); $handler.AllowAutoRedirect=$false
$client=[System.Net.Http.HttpClient]::new($handler); $client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-TIFF-source-audit/1.0')
try {
 $url='https://api.github.com/repos/o3de/3p-package-source/contents/package-system/tiff?ref=70f0b7b5257bf7cf935bf1cfd270e1b53ccead18'
 $r=$client.GetAsync($url).GetAwaiter().GetResult()
 [ordered]@{method='GET';url=$url;status=[int]$r.StatusCode;redirectsFollowed=$false;credentials='none';location=if($r.Headers.Location){$r.Headers.Location.AbsoluteUri}else{$null}} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Audit 'recipe-directory-request.json') -Encoding utf8
 if([int]$r.StatusCode -ne 200){throw 'Recipe directory not directly available'}
 $raw=$r.Content.ReadAsStringAsync().GetAwaiter().GetResult(); [IO.File]::WriteAllText((Join-Path $Audit 'recipe-directory-metadata.json'),$raw,[Text.UTF8Encoding]::new($false))
 $raw | ConvertFrom-Json | Select-Object name,type,path,download_url | ConvertTo-Json -Depth 4
} finally {$client.Dispose();$handler.Dispose()}