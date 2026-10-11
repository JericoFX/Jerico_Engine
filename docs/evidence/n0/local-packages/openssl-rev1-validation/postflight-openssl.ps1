$ErrorActionPreference = 'Stop'
$ev = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-rev1-validation'
$pre = Get-Content -LiteralPath (Join-Path $ev 'preflight-openssl.json') -Raw | ConvertFrom-Json
$freezePath = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\source-freeze-openssl-static-engines.json'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$sourceChecks = foreach ($item in $freeze.files) {
  $f = Get-Item -LiteralPath $item.path
  $h = (Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash
  [pscustomobject]@{path=$item.path;bytes=$f.Length;sha256=$h;pass=($f.Length -eq $item.bytes -and $h -ieq $item.sha256)}
}
$manifestPath = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\tool-manifest.json'
$toolRoot = 'F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$toolMismatch = @()
foreach ($item in $manifest) {
  $path = Join-Path $toolRoot ($item.path -replace '/', '\')
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $toolMismatch += [pscustomobject]@{path=$item.path;issue='missing'}; continue }
  $f = Get-Item -LiteralPath $path
  $h = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
  if ($f.Length -ne $item.bytes -or $h -ine $item.sha256) { $toolMismatch += [pscustomobject]@{path=$item.path;issue='hash_or_size_mismatch';bytes=$f.Length;sha256=$h} }
}
$toolFileCount = @(Get-ChildItem -LiteralPath $toolRoot -Recurse -File).Count
$summary = [ordered]@{
  freeze_sha256=(Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash
  freeze_file_count=$freeze.files.Count
  freeze_entries_pass=(@($sourceChecks | Where-Object { -not $_.pass }).Count -eq 0)
  freeze_failures=@($sourceChecks | Where-Object { -not $_.pass })
  tool_manifest_sha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
  tool_manifest_entries=$manifest.Count
  tool_tree_files=$toolFileCount
  tool_manifest_mismatches=$toolMismatch.Count
  tool_manifest_pass=($toolMismatch.Count -eq 0 -and $toolFileCount -eq $manifest.Count)
  tool_manifest_failure_sample=@($toolMismatch | Select-Object -First 10)
  candidate_driver_sha256=(Get-FileHash -LiteralPath 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\producer-candidate-inputs.cmake' -Algorithm SHA256).Hash
  output_exists=(Test-Path -LiteralPath 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev1')
  producer_native_exit=1
  producer_elapsed_seconds=2.439
  input_freeze_unchanged=(@($sourceChecks | Where-Object { -not $_.pass }).Count -eq 0 -and (Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash -eq $pre.freeze_sha256)
  tool_manifest_unchanged=((Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash -eq $pre.tool_manifest_sha256)
}
$path = Join-Path $ev 'postflight-openssl.json'
$summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $path -Encoding utf8
$summary | Select-Object freeze_sha256,freeze_file_count,freeze_entries_pass,tool_manifest_sha256,tool_manifest_entries,tool_tree_files,tool_manifest_mismatches,tool_manifest_pass,candidate_driver_sha256,output_exists,input_freeze_unchanged,tool_manifest_unchanged | Format-List
