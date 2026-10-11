#Requires -Version 7.0
$ErrorActionPreference='Stop'
$taskRepo='F:/JericoEngine/JericoEngine'
$taskBase="$taskRepo/build/n0-validation/local-packages"
$taskOutput="$PSScriptRoot/parent-final-package-integration.json"
foreach($path in @($taskOutput,"$PSScriptRoot/BuiltInPackages_windows.before-dxc.cmake")){if(Test-Path -LiteralPath $path){throw 'Preserve existing integration output'}}
function Assert-TaskLocalPath([string]$path){
    if(-not [IO.Path]::IsPathFullyQualified($path) -or $path -match '^[\\/]{2}|://'){throw 'Absolute local path required'}
    $cursor=[IO.Path]::GetFullPath($path)
    while($cursor){
        if(([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and ([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint)){throw 'Reparse integration path rejected'}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
}
function Get-TaskHash([string]$path){
    $stream=[IO.File]::OpenRead($path)
    try{[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($stream))}finally{$stream.Dispose()}
}
$taskProducerPath="$taskBase/dxc-rev1-validation/attempt1/validation-result.json"
$taskConsumerPath="$taskBase/dxc-rev1-validation/consumer-attempt4/consumer-result.json"
$taskPostPath="$taskBase/dxc-rev1-validation/consumer-attempt4/postflight.json"
$taskNegativePath="$taskBase/dxc-rev1-negatives/attempt-rev3/negative-result.json"
$taskPePath="$PSScriptRoot/parent-pe-review-rev1.json"
$taskProducer=Get-Content -Raw -LiteralPath $taskProducerPath | ConvertFrom-Json
$taskConsumer=Get-Content -Raw -LiteralPath $taskConsumerPath | ConvertFrom-Json
$taskPost=Get-Content -Raw -LiteralPath $taskPostPath | ConvertFrom-Json
$taskNegative=Get-Content -Raw -LiteralPath $taskNegativePath | ConvertFrom-Json
$taskPe=Get-Content -Raw -LiteralPath $taskPePath | ConvertFrom-Json
if($taskProducer.producer.nativeExit -ne 0 -or $taskProducer.changedPaths.Count){throw 'Original producer evidence not clean'}
foreach($phase in @('configure','build','ctest')){if($taskConsumer.nativeResults.$phase.nativeExit -ne 0){throw 'Final consumer native exit differs'}}
if($taskConsumer.changedPaths.Count -or $taskConsumer.payloadStatus -ne 'PASS' -or $taskPost.status -ne 'PASS' -or $taskPost.sourceInputs -ne 23385 -or $taskPost.changes.Count){throw 'Final consumer preservation failed'}
[xml]$taskJUnit=[IO.File]::ReadAllText("$taskBase/dxc-rev1-validation/consumer-attempt4/ctest.junit.xml")
$taskCases=@($taskJUnit.SelectNodes('//testcase'))
$taskNames=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach($node in $taskCases){if(-not $taskNames.Add($node.GetAttribute('name')) -or $node.SelectNodes('./failure|./error|./skipped').Count){throw 'Actual native case failure/skip/duplicate'}}
if($taskCases.Count -ne 4 -or -not $taskNames.SetEquals([string[]]@('JE.LocalPackages.DXC.dxil','JE.LocalPackages.DXC.spirv','JE.LocalPackages.DXC.invalid','JE.LocalPackages.DXC.dxsc'))){throw 'Actual native case membership differs'}
if(Test-Path -LiteralPath "$taskBase/dxc-native-consumer-rev4/invalid"){throw 'Invalid shader output exists'}
if($taskNegative.expectedCases -ne 18 -or $taskNegative.results.Count -ne 18 -or $taskNegative.sourceInputs -ne 23385 -or $taskNegative.sourceChanges.Count -or $taskNegative.fixtureChanges.Count -or $taskNegative.fixtureFiles -ne 4){throw 'Negative evidence preservation/membership differs'}
foreach($case in $taskNegative.results){if($case.nativeExit -eq 0 -or -not $case.observedExpectedError -or -not $case.outputCondition -or $case.fixtureChanges.Count -or $case.status -ne 'PASS REJECTION'){throw 'Negative rejection failed'}}
$taskScopes=@($taskNegative.results | Group-Object scope)
foreach($pair in @(@('PRODUCER',16),@('PRIVATE-OVERLAY',1),@('FUNCTION-GUARD',1))){$group=@($taskScopes | Where-Object Name -EQ $pair[0]);if($group.Count -ne 1 -or $group[0].Count -ne $pair[1]){throw 'Negative scope membership differs'}}
if($taskPe.status -ne 'PARENT STATIC PE REVIEW PASS' -or $taskPe.packageFiles -ne 15 -or $taskPe.payloadManifestRecords -ne 14 -or $taskPe.binaries.Count -ne 4){throw 'Parent PE review differs'}
$taskFreeze=Get-Content -Raw -LiteralPath "$PSScriptRoot/consumer-candidate-rev4/source-freeze-dxc-consumer-rev4.json" | ConvertFrom-Json
$taskProviderRoot="$taskRepo/cmake/Tools/LocalPackages/DirectXShaderCompilerDxc"
$taskProviderFiles=@(Get-ChildItem -LiteralPath $taskProviderRoot -Recurse -File -Force)
if($taskProviderFiles.Count -ne 12){throw 'Reviewed provider membership differs'}
$taskProviders=@()
foreach($file in $taskProviderFiles){
    if($file.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Provider reparse path'}
    $entry=@($taskFreeze.files | Where-Object { [IO.Path]::GetFullPath($_.path) -eq $file.FullName })
    $hash=Get-TaskHash $file.FullName
    if($entry.Count -ne 1 -or $entry[0].bytes -ne $file.Length -or $entry[0].sha256 -ne $hash){throw 'Reviewed provider source changed'}
    $taskProviders+=@{path=[IO.Path]::GetRelativePath($taskRepo,$file.FullName).Replace([char]92,[char]47);bytes=$file.Length;sha256=$hash}
}
$taskArchiveName='DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows.tar.xz'
$taskArchive="$taskBase/dxc-local-rev1/packages/$taskArchiveName"
$taskArchiveSHA='62FC6978933215440718D8D810AB5ECF20A36368A8592D12822F902ED0654782'
if([IO.FileInfo]::new($taskArchive).Length -ne 7352940 -or (Get-TaskHash $taskArchive) -ne $taskArchiveSHA -or (Get-TaskHash "$taskArchive.SHA256SUMS") -ne '8A9C9F9F562F099559770A5770B1465928471416181CEBC38198B2FEFF2E627E'){throw 'Immutable actual package changed'}
$taskCache="$taskBase/root-package-cache"
foreach($path in @($taskArchive,$taskCache,$taskOutput)){Assert-TaskLocalPath $path}
$taskCacheBefore=@(Get-ChildItem -LiteralPath $taskCache -File -Force | ForEach-Object { @{path=$_.FullName;bytes=$_.Length;sha256=(Get-TaskHash $_.FullName)} })
if($taskCacheBefore.Count -ne 34){throw 'Approved cache count differs before integration'}
foreach($name in @($taskArchiveName,"$taskArchiveName.SHA256SUMS")){if(Test-Path -LiteralPath "$taskCache/$name"){throw 'Never overwrite existing cache output'}}
$taskCatalog="$taskRepo/cmake/3rdParty/Platform/Windows/BuiltInPackages_windows.cmake"
Assert-TaskLocalPath $taskCatalog
$taskCatalogBeforeSHA=Get-TaskHash $taskCatalog
if($taskCatalogBeforeSHA -ne '787C1CD917DE63F52FAE25B3825C9AE4E1DE18B65EA042CC0211482C594FA00A'){throw 'Reviewed catalog before bytes differ'}
$taskCatalogBefore=[IO.File]::ReadAllBytes($taskCatalog)
$taskText=[Text.Encoding]::UTF8.GetString($taskCatalogBefore)
$taskOldName='DirectXShaderCompilerDxc-1.8.2505.1-o3de-rev3-windows'
$taskNewName='DirectXShaderCompilerDxc-1.8.2505.1-o3de-je-rev1-windows'
$taskOldSHA='224a34691a17234c2294202368d9756635f11d66ac9bc528f929d84cf4acac8e'
foreach($fragment in @($taskOldName,$taskOldSHA)){if(([regex]::Matches($taskText,[regex]::Escape($fragment))).Count -ne 1){throw 'Catalog binding not unique'}}
$taskAfter=$taskText.Replace($taskOldName,$taskNewName).Replace($taskOldSHA,$taskArchiveSHA.ToLowerInvariant())
if($taskAfter.Replace($taskNewName,$taskOldName).Replace($taskArchiveSHA.ToLowerInvariant(),$taskOldSHA) -cne $taskText){throw 'Catalog inverse differs'}
[IO.File]::WriteAllBytes("$PSScriptRoot/BuiltInPackages_windows.before-dxc.cmake",$taskCatalogBefore)
[IO.File]::Copy($taskArchive,"$taskCache/$taskArchiveName",$false)
[IO.File]::Copy("$taskArchive.SHA256SUMS","$taskCache/$taskArchiveName.SHA256SUMS",$false)
[IO.File]::WriteAllBytes($taskCatalog,[Text.Encoding]::UTF8.GetBytes($taskAfter))
foreach($entry in $taskCacheBefore){if([IO.FileInfo]::new($entry.path).Length -ne $entry.bytes -or (Get-TaskHash $entry.path) -ne $entry.sha256){throw 'Existing cache changed'}}
if(@(Get-ChildItem -LiteralPath $taskCache -File -Force).Count -ne 36 -or (Get-TaskHash "$taskCache/$taskArchiveName") -ne $taskArchiveSHA){throw 'New cache membership/hash differs'}
$taskReceipt=[ordered]@{
    status='PARENT FOCAL PACKAGE ACCEPTED AND CATALOG/CACHE INTEGRATED';package=$taskNewName
    archiveBytes=7352940;archiveSHA256=$taskArchiveSHA;producer=(Get-TaskHash $taskProducerPath)
    finalConsumer=(Get-TaskHash $taskConsumerPath);consumerCases=4;negativeCases=18;negativeScopes=@{PRODUCER=16;'PRIVATE-OVERLAY'=1;'FUNCTION-GUARD'=1}
    negativeEvidenceSHA256=(Get-TaskHash $taskNegativePath);parentPeReviewSHA256=(Get-TaskHash $taskPePath)
    sourceInputs=23385;providerFiles=$taskProviders;cacheFilesBefore=34;cacheFilesAfter=36
    catalogBeforeSHA256=$taskCatalogBeforeSHA;catalogAfterSHA256=(Get-TaskHash $taskCatalog);catalogInverseExact=$true
    review='Original source pins, options, four binaries, codegen, legal notices and archive retained. Only consumer API/version and empty-object oracle corrected using upstream and actual RED/GREEN.'
    limits='Focal shader compiler package. Static PE/import presence and native API consumers do not establish portability, signing, Editor/AP/runtime, DX12/Vulkan frames or product dependency separation.'
    rootExecuted=$false;engineAccepted=$false
}
[IO.File]::WriteAllText($taskOutput,($taskReceipt | ConvertTo-Json -Depth 9)+"`n",[Text.UTF8Encoding]::new($false))
[ordered]@{status=$taskReceipt.status;sha256=(Get-TaskHash $taskOutput);catalogSHA256=$taskReceipt.catalogAfterSHA256;cacheFiles=36} | ConvertTo-Json
