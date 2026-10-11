$ErrorActionPreference='Stop'
$Audit=$PSScriptRoot
$recipe=Join-Path $Audit 'recipe-original';if(Test-Path -LiteralPath $recipe){throw 'Recipe outputs exist'};[IO.Directory]::CreateDirectory($recipe)|Out-Null
Add-Type -AssemblyName System.Net.Http
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-TIFF-source-audit/1.0')
$receipts=@()
try {
 $files=@('COPYRIGHT','FindTIFF.cmake','PackageInfo.json','build_config.json','build_tiff_windows.cmd','install_tiff_windows.cmd','test_tiff_windows.cmd','o3de_patch.patch')
 $metadata=Get-Content -Raw -LiteralPath (Join-Path $Audit 'recipe-directory-metadata.json')|ConvertFrom-Json
 foreach($name in $files){
  $entry=$metadata|Where-Object name -eq $name;if(-not $entry){throw "Missing recipe $name"}
  $u=[uri]$entry.download_url;if($u.Host -ne 'raw.githubusercontent.com' -or $u.Scheme -ne 'https'){throw 'Disallowed recipe URL'}
  $r=$client.GetAsync($u).GetAwaiter().GetResult();if([int]$r.StatusCode -ne 200){throw "Recipe status $([int]$r.StatusCode) $name"};$bytes=$r.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult();$out=Join-Path $recipe $name;[IO.File]::WriteAllBytes($out,$bytes)
  $receipts += [ordered]@{file=$name;url=$u.AbsoluteUri;status=[int]$r.StatusCode;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $out -Algorithm SHA256).Hash;gitBlob=$entry.sha;redirectsFollowed=$false;credentials='none'}
 }
 $receipts|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $Audit 'recipe-input-receipts.json') -Encoding utf8
 foreach($name in $files){"FILE: $name";Get-Content -LiteralPath (Join-Path $recipe $name)}
}finally{$client.Dispose();$handler.Dispose()}