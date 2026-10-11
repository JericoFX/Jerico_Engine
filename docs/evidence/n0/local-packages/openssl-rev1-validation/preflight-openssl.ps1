$ErrorActionPreference = 'Stop'
$freezePath = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\source-freeze-openssl-static-engines.json'
$freezeExpected = '8A1DA01093EF3F149C73D2D14F568ABC967FF63D5432A5C95820C15D0DCBAC14'
$freeze = Get-Content -LiteralPath $freezePath -Raw | ConvertFrom-Json
$checks = @()
foreach ($item in $freeze.files) {
  $f = Get-Item -LiteralPath $item.path
  $h = (Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash
  $checks += [pscustomobject]@{ path=$item.path; exists=$true; bytes=$f.Length; expected_bytes=$item.bytes; sha256=$h; expected_sha256=$item.sha256; pass=($f.Length -eq $item.bytes -and $h -ieq $item.sha256) }
}
$manifestPath = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\tool-manifest.json'
$toolRoot = 'F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$toolChecks = foreach ($item in $manifest) {
  $path = Join-Path $toolRoot ($item.path -replace '/', '\')
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { [pscustomobject]@{path=$item.path; pass=$false; issue='missing'}; continue }
  $f = Get-Item -LiteralPath $path
  $h = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
  [pscustomobject]@{path=$item.path; pass=($f.Length -eq $item.bytes -and $h -ieq $item.sha256); bytes=$f.Length; sha256=$h}
}
$freezeHash = (Get-FileHash -LiteralPath $freezePath -Algorithm SHA256).Hash
$candidate = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-perl-tool-audit\producer-candidate-inputs.cmake'
$output = 'F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev1'
$summary = [ordered]@{
  freeze_sha256=$freezeHash; freeze_expected_sha256=$freezeExpected; freeze_file_count=$freeze.files.Count
  freeze_entries_pass=(@($checks | Where-Object { -not $_.pass }).Count -eq 0)
  candidate_driver_sha256=(Get-FileHash -LiteralPath $candidate -Algorithm SHA256).Hash
  tool_manifest_sha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
  tool_manifest_entries=$manifest.Count; tool_manifest_pass=(@($toolChecks | Where-Object { -not $_.pass }).Count -eq 0)
  tool_payload_bytes=($toolChecks | Measure-Object -Property bytes -Sum).Sum
  output_exists=(Test-Path -LiteralPath $output)
  freeze_checks=$checks; tool_checks=$toolChecks
}
$out='F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-rev1-validation\preflight-openssl.json'
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $out -Encoding utf8
$summary | Select-Object freeze_sha256,freeze_file_count,freeze_entries_pass,candidate_driver_sha256,tool_manifest_sha256,tool_manifest_entries,tool_manifest_pass,tool_payload_bytes,output_exists | Format-List
