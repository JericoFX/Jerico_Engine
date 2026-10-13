param([Parameter(Mandatory=$true)][string]$EvidenceRoot)
$ErrorActionPreference='Stop'
$baseline=Get-Content -LiteralPath (Join-Path (Split-Path $EvidenceRoot) 'baseline/baseline.json') -Raw | ConvertFrom-Json
$before=Get-Content -LiteralPath (Join-Path $EvidenceRoot 'before.json') -Raw | ConvertFrom-Json
$after=Get-Content -LiteralPath (Join-Path $EvidenceRoot 'after.json') -Raw | ConvertFrom-Json
$sourceChanges=@()
$afterSources=@{}
foreach($s in $after.sourceIdentities){$afterSources[[string]$s.SourceID]=$s}
foreach($s in $before.sourceIdentities){
    $n=$afterSources[[string]$s.SourceID]
    if(!$n -or $n.SourceGuid -cne $s.SourceGuid -or $n.SourceName -cne $s.SourceName -or $n.PortableKey -cne $s.PortableKey){
        $sourceChanges += [ordered]@{before=$s;after=$n}
    }
}
$oldFiles=@{}
foreach($f in $baseline.sourceFiles){$oldFiles[[string]$f.sourceId]=$f}
$fileChanges=@()
foreach($f in $after.sourceFiles){
    $old=$oldFiles[[string]$f.sourceId]
    if(!$old -or !$f.exists -or $old.bytes -ne $f.bytes -or $old.sha256 -cne $f.sha256){$fileChanges += [ordered]@{before=$old;after=$f}}
}
function ProductKey($p){return "$($p.SourceGuid)|$($p.SubID)|$($p.ProductName)|$($p.AssetType)"}
$products=@{}
foreach($p in $after.products){$products[(ProductKey $p)]=$p}
$oldProducts=@{}
foreach($p in $before.products){$oldProducts[(ProductKey $p)]=$p}
$missingProducts=@($before.products | Where-Object {!$products.ContainsKey((ProductKey $_))})
$newProducts=@($after.products | Where-Object {!$oldProducts.ContainsKey((ProductKey $_))})
$preview=@($after.sourceIdentities | Where-Object {$_.PortableKey -eq 'gemassets-{0CF3D5DF-4FD1-5108-B937-DC20B6A53B48}'})
$canvas=@($after.sourceIdentities | Where-Object {$_.PortableKey -eq 'gemassets-{BA3076F4-E9F4-5243-9754-E19A40F6B90C}'})
$wrongRoots=@($preview | Where-Object {$_.ScanFolder.Replace('\','/').TrimEnd('/') -cne 'F:/JericoEngine/JericoEngine/Editor/Materials/Preview/Assets'})
$wrongRoots+=@($canvas | Where-Object {$_.ScanFolder.Replace('\','/').TrimEnd('/') -cne 'F:/JericoEngine/JericoEngine/Editor/Materials/Assets'})
$passed=($before.sourceIdentities.Count -eq 44 -and $after.sourceIdentities.Count -eq 44 -and $preview.Count -eq 35 -and $canvas.Count -eq 9 -and $sourceChanges.Count -eq 0 -and $fileChanges.Count -eq 0 -and $missingProducts.Count -eq 0 -and $wrongRoots.Count -eq 0)
$report=[ordered]@{capturedUtc=[DateTime]::UtcNow.ToString('o');scope='44 existing preview/material graph sources; GUID/SourceID/name/portable key; source bytes/SHA256; existing product GUID/SubID/name/type. Does not certify global cook, product bytes, rendering or UI.';passed=$passed;beforeSources=$before.sourceIdentities.Count;afterSources=$after.sourceIdentities.Count;previewSources=$preview.Count;canvasSources=$canvas.Count;sourceIdentityChangeCount=$sourceChanges.Count;sourceBytesOrHashChangeCount=$fileChanges.Count;wrongRootCount=$wrongRoots.Count;beforeProducts=$before.products.Count;afterProducts=$after.products.Count;missingProductIdentityCount=$missingProducts.Count;newProductIdentityCount=$newProducts.Count;sourceIdentityChanges=$sourceChanges;sourceFileChanges=$fileChanges;wrongRoots=$wrongRoots;missingProducts=$missingProducts;newProducts=$newProducts}
$report | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'migration-comparison.json') -Encoding UTF8
[pscustomobject]$report | Select-Object passed,beforeSources,afterSources,previewSources,canvasSources,sourceIdentityChangeCount,sourceBytesOrHashChangeCount,wrongRootCount,beforeProducts,afterProducts,missingProductIdentityCount,newProductIdentityCount | ConvertTo-Json
if(!$passed){exit 1}
