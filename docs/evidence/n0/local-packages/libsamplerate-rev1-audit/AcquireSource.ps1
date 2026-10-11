$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=$PSScriptRoot
function SaveNew([string]$path,[byte[]]$bytes){$s=[IO.File]::Open($path,[IO.FileMode]::CreateNew);try{$s.Write($bytes,0,$bytes.Length)}finally{$s.Dispose()}}
function JsonNew([string]$name,$obj){SaveNew (Join-Path $root $name) ([Text.UTF8Encoding]::new($false).GetBytes(($obj|ConvertTo-Json -Depth 20)+"`n"))}
$handler=[Net.Http.HttpClientHandler]::new();$handler.AllowAutoRedirect=$false;$handler.UseCookies=$false;$handler.UseDefaultCredentials=$false
$client=[Net.Http.HttpClient]::new($handler);$client.DefaultRequestHeaders.UserAgent.ParseAdd('JE-source-data-audit');$client.Timeout=[TimeSpan]::FromMinutes(5)
try{
 $url='https://codeload.github.com/libsndfile/libsamplerate/tar.gz/a147b6dc4046deda22b2aa772b348717de772e2b'
 $head=$client.SendAsync([Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head,$url)).GetAwaiter().GetResult()
 try{JsonNew 'source-head.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');method='HEAD';url=$url;status=[int]$head.StatusCode;location=if($head.Headers.Location){$head.Headers.Location.GetLeftPart([UriPartial]::Path)}else{$null};autoRedirect=$false;authentication=$false});if([int]$head.StatusCode -ne 200 -or $head.Headers.Location){throw 'HEAD requires manual redirect review before GET'}}finally{$head.Dispose()}
 $archive='F:/JericoEngine/Dependencies/archives/libsamplerate-a147b6dc4046deda22b2aa772b348717de772e2b.tar.gz'
 if(Test-Path -LiteralPath $archive){throw 'preserve archive'}
 $get=$client.GetAsync($url,[Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
 try{if([int]$get.StatusCode -ne 200 -or $get.Headers.Location){throw 'unexpected GET response'};$out=[IO.File]::Open($archive,[IO.FileMode]::CreateNew);try{$input=$get.Content.ReadAsStreamAsync().GetAwaiter().GetResult();try{$input.CopyTo($out)}finally{$input.Dispose()}}finally{$out.Dispose()}}finally{$get.Dispose()}
 $hash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
 JsonNew 'source-get.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');url=$url;hosts=@('codeload.github.com');status=200;redirects=0;autoRedirect=$false;authentication=$false;archive=$archive;bytes=([IO.FileInfo]::new($archive)).Length;sha256=$hash;verification='Measured SHA256; immutable commit/tag provenance and complete Git tree comparison precede extraction. No published archive digest assumed.'})
 $treeUrl='https://api.github.com/repos/libsndfile/libsamplerate/git/trees/a147b6dc4046deda22b2aa772b348717de772e2b?recursive=1'
 $tree=$client.GetAsync($treeUrl).GetAwaiter().GetResult();try{if([int]$tree.StatusCode -ne 200 -or $tree.Headers.Location){throw 'tree metadata unavailable'};SaveNew (Join-Path $root 'primary-tree.json') ($tree.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult())}finally{$tree.Dispose()}
}finally{$client.Dispose();$handler.Dispose()}