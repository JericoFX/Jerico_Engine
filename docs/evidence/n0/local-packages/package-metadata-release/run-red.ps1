$ErrorActionPreference='Stop'
$ev='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\package-metadata-release'
$repo='F:\JericoEngine\JericoEngine'
$cmake='C:\Program Files\CMake\bin\cmake.exe'
$helper=Join-Path $repo 'cmake\Tools\LocalPackages\JECreateLocalPackage.cmake'
$witness=Join-Path $ev 'release-witness.cmake'
$out=Join-Path $ev 'red'
if (Test-Path -LiteralPath $out) { throw 'output path exists; aborting without launch' }
$helperHash=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash
$witnessHash=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash
if ($helperHash -ne 'B067C707919808DDA7697CE79EFEC94A31029FF03B6CC383E374209CC8B68E38' -or $witnessHash -ne 'C34909064FE9E852E5EC5A6B34A72B88433A6BEE57C7DE257AB1B23F9894F557') { throw 'frozen input hash mismatch; aborting without launch' }
$argv=[string[]]@("-DJE_TEST_OUTPUT:PATH=$out",'-P',$witness)
if ($argv.Count -ne 3 -or $argv[1] -ne '-P' -or $argv[2] -ne $witness -or $argv[0] -notlike '-DJE_TEST_OUTPUT:PATH=*') { throw 'argv guard failed; aborting without launch' }
$before=[ordered]@{helper_sha256=$helperHash;witness_sha256=$witnessHash;output_exists=(Test-Path -LiteralPath $out);argv=$argv;argc=$argv.Count;cwd=(Get-Location).Path;start_utc=[DateTime]::UtcNow.ToString('o')}
$before | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ev 'red-inputs-before.json') -Encoding utf8
$timer=[Diagnostics.Stopwatch]::StartNew()
& $cmake @argv 1> (Join-Path $ev 'red.stdout.log') 2> (Join-Path $ev 'red.stderr.log')
$nativeExit=$LASTEXITCODE
$timer.Stop()
$after=[ordered]@{exit_code=$nativeExit;elapsed_seconds=$timer.Elapsed.TotalSeconds;end_utc=[DateTime]::UtcNow.ToString('o');helper_sha256=(Get-FileHash -LiteralPath $helper -Algorithm SHA256).Hash;witness_sha256=(Get-FileHash -LiteralPath $witness -Algorithm SHA256).Hash;output_exists=(Test-Path -LiteralPath $out);stage_payload_exists=(Test-Path -LiteralPath (Join-Path $out 'stage\payload.txt'));packages_dir_exists=(Test-Path -LiteralPath (Join-Path $out 'packages'));archive_exists=(Test-Path -LiteralPath (Join-Path $out 'packages\JEReleaseMetadataWitness.tar.xz'));package_info_exists=(Test-Path -LiteralPath (Join-Path $out 'packages\JEReleaseMetadataWitness.contents\PackageInfo.json'))}
$after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ev 'red-result.json') -Encoding utf8
exit $nativeExit