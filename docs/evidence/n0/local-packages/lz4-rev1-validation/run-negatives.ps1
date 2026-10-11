$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$producer='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\LZ4\BuildLz4Package.cmake'
$archive='F:/JericoEngine/Dependencies/archives/lz4-v1.9.4.tar.gz'
$hash='0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b'
$url='https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4'
$corrupt='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-corrupt-source-fixture.bin'
[IO.File]::WriteAllText($corrupt,'not an lz4 tar archive'+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
$corruptHash=(Get-FileHash -LiteralPath $corrupt -Algorithm SHA256).Hash.ToLowerInvariant()
$sentinel='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-guard-existing-sentinel'
[IO.Directory]::CreateDirectory($sentinel)|Out-Null
$sentinelPath=Join-Path $sentinel 'sentinel.txt'
[IO.File]::WriteAllText($sentinelPath,"keep this existing output`r`n",[Text.UTF8Encoding]::new($false))
$sentinelBefore=(Get-FileHash -LiteralPath $sentinelPath -Algorithm SHA256).Hash
$cases=@(
  [pscustomobject]@{name='required-missing';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-required-missing';expected='JE_LZ4_SOURCE_URL is required';kind='missing'},
  [pscustomobject]@{name='uri';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-uri';expected='JE_LZ4_SOURCE_ARCHIVE must not be a URI';kind='uri'},
  [pscustomobject]@{name='unc';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-unc';expected='JE_LZ4_SOURCE_ARCHIVE must be an absolute local path';kind='unc'},
  [pscustomobject]@{name='wrong-declared-hash';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-wrong-hash';expected='source URL/SHA256 must match';kind='wrong-hash'},
  [pscustomobject]@{name='wrong-url';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-wrong-url';expected='source URL/SHA256 must match';kind='wrong-url'},
  [pscustomobject]@{name='corrupt-archive-actual-hash';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-corrupt';expected='lz4 source archive SHA256 mismatch';kind='corrupt'},
  [pscustomobject]@{name='existing-output-sentinel';output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-guard-existing-sentinel';expected='output must be new; existing outputs are preserved';kind='existing'}
)
$results=@()
foreach($case in $cases){
  $source=$archive;$declaredHash=$hash;$sourceUrl=$url
  if($case.kind -eq 'uri'){$source='https://invalid.example/lz4.tar.gz'}
  if($case.kind -eq 'unc'){$source='//invalid.example/share/lz4.tar.gz'}
  if($case.kind -eq 'wrong-hash'){$declaredHash='0000000000000000000000000000000000000000000000000000000000000000'}
  if($case.kind -eq 'wrong-url'){$sourceUrl='https://invalid.example/lz4-v1.9.4.tar.gz'}
  if($case.kind -eq 'corrupt'){$source=$corrupt}
  if($case.kind -eq 'existing' -and -not (Test-Path -LiteralPath $sentinelPath -PathType Leaf)){throw 'Sentinel missing before existing-output guard.'}
  if($case.kind -ne 'existing' -and (Test-Path -LiteralPath $case.output)){throw "Negative output path already exists: $($case.output)"}
  if($case.kind -eq 'missing'){$argv=[string[]]@("-DJE_LZ4_SOURCE_ARCHIVE:FILEPATH=$source","-DJE_LZ4_SOURCE_SHA256:STRING=$declaredHash","-DJE_LZ4_OUTPUT:PATH=$($case.output)",'-P',$producer)}
  else{$argv=[string[]]@("-DJE_LZ4_SOURCE_ARCHIVE:FILEPATH=$source","-DJE_LZ4_SOURCE_SHA256:STRING=$declaredHash","-DJE_LZ4_SOURCE_URL:STRING=$sourceUrl","-DJE_LZ4_OUTPUT:PATH=$($case.output)",'-P',$producer)}
  $expectedCount=if($case.kind -eq 'missing'){5}else{6}
  if($argv.Count -ne $expectedCount -or $argv[$argv.Count-2] -ne '-P' -or $argv[$argv.Count-1] -ne $producer){throw "Bad argv guard for $($case.name)"}
  $prefix=Join-Path $ev ("negative-"+$case.name)
  $argv|ConvertTo-Json|Set-Content -LiteralPath ("$prefix.argv.json") -Encoding utf8
  $sw=[Diagnostics.Stopwatch]::StartNew()
  $p=Start-Process -FilePath $cmake -ArgumentList $argv -WorkingDirectory $cwd -NoNewWindow -PassThru -RedirectStandardOutput ("$prefix.stdout.log") -RedirectStandardError ("$prefix.stderr.log")
  $p.WaitForExit();$nativeExit=$p.ExitCode;$sw.Stop();$elapsed=[Math]::Round($sw.Elapsed.TotalSeconds,3)
  $stdout=Get-Content -LiteralPath ("$prefix.stdout.log") -Raw;$stderr=Get-Content -LiteralPath ("$prefix.stderr.log") -Raw;$combined=$stdout+"`r`n"+$stderr
  $combined|Set-Content -LiteralPath ("$prefix.log") -Encoding utf8
  $created=if($case.kind -eq 'existing'){$false}else{Test-Path -LiteralPath $case.output}
  $sentinelAfter=(Get-FileHash -LiteralPath $sentinelPath -Algorithm SHA256).Hash
  $sentinelUnchanged=($sentinelBefore -eq $sentinelAfter)
  $match=$combined.Contains($case.expected)
  $result=[ordered]@{name=$case.name;argv=$argv;argc=$argv.Count;cwd=$cwd;elapsed_seconds=$elapsed;native_exit_code=$nativeExit;expected_diagnostic=$case.expected;diagnostic_match=$match;output_path=$case.output;unexpected_output_created=$created;sentinel_sha256_before=$sentinelBefore;sentinel_sha256_after=$sentinelAfter;sentinel_unchanged=$sentinelUnchanged;corrupt_fixture_sha256=if($case.kind -eq 'corrupt'){$corruptHash}else{$null};pass=($nativeExit -ne 0 -and $match -and -not $created -and $sentinelUnchanged)}
  $result|ConvertTo-Json -Depth 5|Set-Content -LiteralPath ("$prefix.result.json") -Encoding utf8
  $results+=$result
}
$summary=[ordered]@{cases=$results;pass_count=@($results|Where-Object pass).Count;case_count=$results.Count;all_pass=(@($results|Where-Object{-not $_.pass}).Count -eq 0);sentinel_path=$sentinelPath;sentinel_sha256=$sentinelBefore;corrupt_fixture_path=$corrupt;corrupt_fixture_sha256=$corruptHash}
$summary|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $ev 'negative-summary.json') -Encoding utf8
[pscustomobject]@{case_count=$summary.case_count;pass_count=$summary.pass_count;all_pass=$summary.all_pass;summary=(Join-Path $ev 'negative-summary.json')}|Format-List
