$ErrorActionPreference='Stop'
$Audit=$PSScriptRoot
$commit='4862b0d7bcc786304ff4e8c31e8d5ccfb868fb99'
$URL="https://gitlab.com/libtiff/libtiff/-/archive/$commit/libtiff-$commit.tar.gz"
$archive=Join-Path $Audit "libtiff-$commit.tar.gz"
if(Test-Path -LiteralPath $archive){throw 'Archive exists; preserve it'}
Add-Type -AssemblyName System.Net.Http
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$client=[Net.Http.HttpClient]::new($handler);$client.Timeout=[TimeSpan]::FromMinutes(2);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-TIFF-source-audit/1.0')
try{
 $request=[Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$URL);$r=$client.SendAsync($request).GetAwaiter().GetResult()
 $head=[ordered]@{method='HEAD';url=$URL;status=[int]$r.StatusCode;location=if($r.Headers.Location){$r.Headers.Location.AbsoluteUri}else{$null};redirectsFollowed=$false;credentials='none';contentLength=$r.Content.Headers.ContentLength}
 $head|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $Audit 'source-head.json') -Encoding utf8
 if([int]$r.StatusCode -ne 200 -or $r.Headers.Location){throw 'Source HEAD not direct 200; inspect destination before GET'}
 $r.Dispose();$request.Dispose()
 $r=$client.GetAsync($URL).GetAwaiter().GetResult();if([int]$r.StatusCode -ne 200 -or $r.Headers.Location){throw 'Source GET not direct 200'}
 $bytes=$r.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult();[IO.File]::WriteAllBytes($archive,$bytes)
 $get=[ordered]@{method='GET';url=$URL;status=[int]$r.StatusCode;redirectsFollowed=$false;credentials='none';archive=$archive;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash;tag='v4.2.0';tagObject='846d7d97f3254adf388993b195a8079068c22beb';commit=$commit;sourcePinOrigin='Official GitLab tag API resolved commit; source archive SHA256 measured after manual direct acquisition; distinct from historical package SHA'}
 $get|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $Audit 'source-acquisition.json') -Encoding utf8
 $get|ConvertTo-Json
}finally{$client.Dispose();$handler.Dispose()}