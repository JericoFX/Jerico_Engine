$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\lz4-rev1-validation'
$cwd='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$producer='F:\JericoEngine\JericoEngine\cmake\Tools\LocalPackages\LZ4\BuildLz4Package.cmake'
$output='F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-uri-string'
if(Test-Path -LiteralPath $output){throw 'corrected URI output already exists'}
$argv=[string[]]@(
  '-DJE_LZ4_SOURCE_ARCHIVE:STRING=https://invalid.example/lz4.tar.gz',
  '-DJE_LZ4_SOURCE_SHA256:STRING=0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b',
  '-DJE_LZ4_SOURCE_URL:STRING=https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4',
  '-DJE_LZ4_OUTPUT:PATH=F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/lz4-rev1-validation/guard-output-uri-string',
  '-P',
  $producer
)
if($argv.Count -ne 6 -or $argv[4] -ne '-P' -or $argv[5] -ne $producer){throw 'corrected URI argv guard failed'}
$argv|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $ev 'negative-uri-string.argv.json') -Encoding utf8
$stdout=Join-Path $ev 'negative-uri-string.stdout.log';$stderr=Join-Path $ev 'negative-uri-string.stderr.log';$log=Join-Path $ev 'negative-uri-string.log'
$watch=[Diagnostics.Stopwatch]::StartNew()
$p=Start-Process -FilePath $cmake -ArgumentList $argv -WorkingDirectory $cwd -NoNewWindow -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$p.WaitForExit();$nativeExit=$p.ExitCode;$watch.Stop()
$out=Get-Content -LiteralPath $stdout -Raw;$err=Get-Content -LiteralPath $stderr -Raw;$combined=$out+"`r`n"+$err
$combined|Set-Content -LiteralPath $log -Encoding utf8
$result=[ordered]@{argv=$argv;argc=$argv.Count;cwd=$cwd;native_exit_code=$nativeExit;elapsed_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);expected_diagnostic='JE_LZ4_SOURCE_ARCHIVE must not be a URI';diagnostic_match=$combined.Contains('JE_LZ4_SOURCE_ARCHIVE must not be a URI');output_exists=(Test-Path -LiteralPath $output);log=$log}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $ev 'negative-uri-string.result.json') -Encoding utf8
[pscustomobject]@{native_exit=$nativeExit;elapsed=$result.elapsed_seconds;diagnostic_match=$result.diagnostic_match;output_exists=$result.output_exists;log=$log}|Format-List
exit $nativeExit
