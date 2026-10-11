#Requires -Version 7.0
param([Parameter(Mandatory)][ValidatePattern('^[A-F0-9]{64}$')][string]$ISPCAcceptanceSHA256)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskOutput="$PSScriptRoot/parent-package-integration.json"
$taskBeforeCopy="$PSScriptRoot/BuiltInPackages_windows.before-texture-codecs.cmake"
foreach($p in @($taskOutput,$taskBeforeCopy)){if(Test-Path -LiteralPath $p){throw 'Preserve existing integration evidence'}}
function Get-TaskHash([string]$p){$s=[IO.File]::OpenRead($p);try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($s))}finally{$s.Dispose()}}
function Assert-TaskLocal([string]$p){
    if(-not [IO.Path]::IsPathFullyQualified($p) -or $p -match '^[\\/]{2}|://'){throw 'Absolute local path required'}
    $cursor=[IO.Path]::GetFullPath($p)
    while($cursor){
        if(([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse integration path rejected'}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
}
$taskAstcReview="$taskBase/astc-rev1-guard-preparation/parent-guard-result-review-rev1.json"
$taskIspcReview="$taskBase/ispc-texcomp-rev1-validation-preparation/parent-focal-acceptance-rev1.json"
if((Get-TaskHash $taskAstcReview) -ne 'E5963FEE082AC01F37BDF442B8BFB0F52B8E28111C5752CC9732DB7A969360AA' -or (Get-TaskHash $taskIspcReview) -ne $ISPCAcceptanceSHA256){throw 'Parent acceptance receipt changed'}
$taskAstc=Get-Content -LiteralPath $taskAstcReview -Raw | ConvertFrom-Json
$taskIspc=Get-Content -LiteralPath $taskIspcReview -Raw | ConvertFrom-Json
if($taskAstc.cases.Count -ne 10 -or $taskAstc.inputDeltas -ne 0 -or $taskAstc.fixtureDeltas -ne 0 -or $taskAstc.catalogIntegrated -or $taskAstc.engineAccepted){throw 'ASTC accepted focal scope differs'}
if(-not $taskIspc.acceptedForCatalogIntegration -or $taskIspc.consumerCases -ne 16 -or $taskIspc.guardCases -ne 9 -or $taskIspc.inputDeltas -ne 0 -or $taskIspc.engineAccepted){throw 'ISPC accepted focal scope differs'}
$taskAstcFreeze="$taskBase/astc-rev1-consumer-preparation/source-freeze-astc-consumer.json"
$taskIspcFreeze="$taskBase/ispc-texcomp-rev1-validation-preparation/validation-inputs.json"
if((Get-TaskHash $taskAstcFreeze) -ne 'AE4E7EB656FEB7D6711EBDE3695C52884C1ADC156AFB6B139B40B94337E2E1EA' -or (Get-TaskHash $taskIspcFreeze) -ne 'DF45F46828446FE0113777546CFDE2983FC099588404548821C9D24A7B6E8710'){throw 'Accepted source freezes changed'}
$taskAstcEntries=@(Get-Content -LiteralPath $taskAstcFreeze -Raw | ConvertFrom-Json)
$taskIspcEntries=(Get-Content -LiteralPath $taskIspcFreeze -Raw | ConvertFrom-Json).entries
$taskProviderFiles=@()
foreach($pair in @(@('ASTCEncoder',9,$taskAstcEntries),@('ISPCTexComp',8,$taskIspcEntries))){
    $taskProviderRoot="$taskRepo/cmake/Tools/LocalPackages/$($pair[0])"
    $taskFiles=@(Get-ChildItem -LiteralPath $taskProviderRoot -Recurse -File -Force)
    if($taskFiles.Count -ne $pair[1]){throw 'Accepted provider membership differs'}
    foreach($f in $taskFiles){
        Assert-TaskLocal $f.FullName
        $e=@($pair[2] | Where-Object {[IO.Path]::GetFullPath($_.path) -eq $f.FullName})
        $hash=Get-TaskHash $f.FullName
        if($e.Count -ne 1 -or $e[0].bytes -ne $f.Length -or $e[0].sha256 -ne $hash){throw 'Accepted provider input changed'}
        $taskProviderFiles+=@{path=[IO.Path]::GetRelativePath($taskRepo,$f.FullName).Replace([char]92,[char]47);bytes=$f.Length;sha256=$hash}
    }
}
$taskPackages=@(
    @{name='astc-encoder-3.2-je-rev1-windows';oldName='astc-encoder-3.2-rev2-windows';oldHash='17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295';bytes=1179220;sha256='4EED3F722E8F4CCA60BF14684F22FA351D0D59960ED518BE04CCE30C707949CA';archive="$taskBase/astc-local-rev1/packages/astc-encoder-3.2-je-rev1-windows.tar.xz"},
    @{name='ISPCTexComp-36b80aa-je-rev1-windows';oldName='ISPCTexComp-36b80aa-rev1-windows';oldHash='b6fa6ea28a2808a9a5524c72c37789c525925e435770f2d94eb2d387360fa2d0';bytes=491564;sha256='8308677642330694AB1C63C74A7AAABF4A0A9633234F998B12E838CDA64008DF';archive="$taskBase/ispc-texcomp-rev1-build/packages/ISPCTexComp-36b80aa-je-rev1-windows.tar.xz"}
)
$taskCache="$taskBase/root-package-cache"
$taskCatalog="$taskRepo/cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake"
foreach($p in @($taskCache,$taskCatalog,$taskOutput,$taskBeforeCopy)){Assert-TaskLocal $p}
$taskCacheBefore=@(Get-ChildItem -LiteralPath $taskCache -File -Force | ForEach-Object {@{path=$_.FullName;bytes=$_.Length;sha256=(Get-TaskHash $_.FullName)}})
if($taskCacheBefore.Count -ne 36){throw 'Approved cache count differs'}
$taskCatalogBeforeSHA=Get-TaskHash $taskCatalog
if($taskCatalogBeforeSHA -ne 'EF3CD0C05C25764DE725A0D197846BE6F0F5CA043A812812E9DB5B757733F326'){throw 'Reviewed catalog before bytes differ'}
$taskCatalogBytes=[IO.File]::ReadAllBytes($taskCatalog)
$taskBefore=[Text.Encoding]::UTF8.GetString($taskCatalogBytes)
$taskAfter=$taskBefore
foreach($p in $taskPackages){
    Assert-TaskLocal $p.archive
    if([IO.FileInfo]::new($p.archive).Length -ne $p.bytes -or (Get-TaskHash $p.archive) -ne $p.sha256){throw 'Produced archive changed'}
    $taskSidecar=[IO.File]::ReadAllText($p.archive+'.SHA256SUMS').Trim()
    if($taskSidecar -cne ($p.sha256.ToLowerInvariant()+' *'+$p.name+'.tar.xz')){throw 'Archive sidecar identity differs'}
    foreach($fragment in @($p.oldName,$p.oldHash)){if([regex]::Matches($taskAfter,[regex]::Escape($fragment)).Count -ne 1){throw 'Original catalog binding not unique'}}
    foreach($suffix in @('.tar.xz','.tar.xz.SHA256SUMS')){if(Test-Path -LiteralPath "$taskCache/$($p.name)$suffix"){throw 'Preserve existing cache output'}}
    $taskAfter=$taskAfter.Replace($p.oldName,$p.name).Replace($p.oldHash,$p.sha256.ToLowerInvariant())
}
$taskInverse=$taskAfter
foreach($p in $taskPackages){$taskInverse=$taskInverse.Replace($p.name,$p.oldName).Replace($p.sha256.ToLowerInvariant(),$p.oldHash)}
if($taskInverse -cne $taskBefore){throw 'Catalog inverse differs'}
[IO.File]::WriteAllBytes($taskBeforeCopy,$taskCatalogBytes)
foreach($p in $taskPackages){
    [IO.File]::Copy($p.archive,"$taskCache/$($p.name).tar.xz",$false)
    [IO.File]::Copy($p.archive+'.SHA256SUMS',"$taskCache/$($p.name).tar.xz.SHA256SUMS",$false)
}
[IO.File]::WriteAllBytes($taskCatalog,[Text.Encoding]::UTF8.GetBytes($taskAfter))
foreach($e in $taskCacheBefore){if([IO.FileInfo]::new($e.path).Length -ne $e.bytes -or (Get-TaskHash $e.path) -ne $e.sha256){throw 'Existing cache changed'}}
if(@(Get-ChildItem -LiteralPath $taskCache -File -Force).Count -ne 40){throw 'Integrated cache membership differs'}
foreach($p in $taskPackages){if((Get-TaskHash "$taskCache/$($p.name).tar.xz") -ne $p.sha256){throw 'Copied package hash differs'}}
$taskReceipt=[ordered]@{
    status='PARENT FOCAL ASTC/ISPC PACKAGES ACCEPTED AND CATALOG/CACHE INTEGRATED'
    packages=$taskPackages;providerFiles=$taskProviderFiles;providerCount=17
    astcAcceptanceSHA256=(Get-TaskHash $taskAstcReview);ispcAcceptanceSHA256=$ISPCAcceptanceSHA256
    cacheFilesBefore=36;cacheFilesAfter=40;existingCacheFilesBytePreserved=36
    catalogBeforeSHA256=$taskCatalogBeforeSHA;catalogAfterSHA256=(Get-TaskHash $taskCatalog);catalogInverseExact=$true
    astcConsumerCases=18;astcGuards=10;ispcConsumerCases=16;ispcGuards=9
    limitations='Focal packages only. Original engine generation, Editor/AP/cook/runtime, Python/AWS/multiplayer absence and effective Tools/Runtime separation remain pending. No product size or performance improvement measured.'
    engineAccepted=$false;rootExecuted=$false
}
$s=[IO.File]::Open($taskOutput,[IO.FileMode]::CreateNew)
try{$b=[Text.UTF8Encoding]::new($false).GetBytes(($taskReceipt | ConvertTo-Json -Depth 10)+"`n");$s.Write($b)}finally{$s.Dispose()}
[ordered]@{status=$taskReceipt.status;sha256=(Get-TaskHash $taskOutput);catalogSHA256=$taskReceipt.catalogAfterSHA256;cacheFiles=40} | ConvertTo-Json
